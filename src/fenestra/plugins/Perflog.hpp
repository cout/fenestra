#pragma once

#include "fenestra/Plugin.hpp"

#include <sstream>
#include <fstream>
#include <thread>
#include <atomic>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace fenestra {

class Perflog
  : public Plugin
{
private:
  class Perfcounter;

  template <typename T>
  class Queue;

public:
  Perflog(Config const & config);

  ~Perflog();

  void open(std::string const & filename);

  virtual void record_probe(Probe const & probe, Probe::Dictionary const & dictionary) override;

private:
  Perfcounter & get_counter(Probe::Key key, Probe::Depth depth, Probe::Dictionary const & dictionary) {
    auto it = key_to_idx_.find(key);
    if (it != key_to_idx_.end()) {
      auto idx = it->second;
      return perf_counters_[idx];
    } else {
      auto idx = perf_counters_.size();
      auto scale = dictionary.scale(key);
      perf_counters_.emplace_back(dictionary[key], key, depth, scale);
      key_to_idx_.emplace(key, idx);
      ++version_;
      return perf_counters_[idx];
    }
  }

  void write_header();

private:
  std::string const & filename_;
  Probe::Dictionary probe_dict_;
  Probe last_;
  std::ofstream file_;
  bool file_open_ = false;
  bool header_written_ = false;
  std::vector<char> buf_;

  std::vector<Probe::Stamp> stamps_;

  std::vector<Perfcounter> perf_counters_;
  std::unordered_map<Probe::Key, std::size_t> key_to_idx_;

  std::uint64_t version_ = 0;
  std::uint64_t header_version_ = 0;
  std::int64_t frame_ = 0;

  std::unique_ptr<Queue<char>> queue_;
  std::thread th_;
  std::atomic<bool> done_ = false;
};

class Perflog::Perfcounter {
public:
  Perfcounter(std::string_view name, Probe::Key key, Probe::Depth depth, Probe::Scale scale)
    : name_(name)
    , key_(key)
    , depth_(depth)
    , scale_(scale)
  {
  }

  auto const & name() const { return name_; }
  auto const & key() const { return key_; }
  auto depth() const { return depth_; }
  auto scale() const { return scale_; }

  void record(std::uint64_t frame, Nanoseconds obs) {
    total_ += obs.count();
    total_is_ns_ = true;
  }

  void record(std::uint64_t frame, Probe::Value obs) {
    total_ += obs;
    total_is_ns_ = false;
  }

  void reset() {
    total_ = 0;
    total_is_ns_ = true;
  }

  std::uint32_t total() const {
    return total_is_ns_ ? total_ / 1000 : total_ * 1000;
  }

private:
  std::string name_;
  Probe::Key key_;
  Probe::Depth depth_ = 0;
  Probe::Scale scale_ = 1;
  std::uint64_t total_ = 0;
  bool total_is_ns_ = true;
};


template <typename T>
class Perflog::Queue {
public:
  Queue(std::size_t size)
    : size_(size)
  {
    queue_ = new T[size];
  }

  ~Queue() {
    delete[] queue_;
  }

  template<typename It>
  void write(It s, It e) {
    auto rd_idx = rd_idx_.load();
    auto wr_idx = wr_idx_.load();

    // Only do full record writes
    auto len = e - s;
    if (wr_idx + len - rd_idx > size_) {
      return;
    }

    while (s != e) {
      // TODO: inefficient
      queue_[wr_idx % size_] = *s;
      ++wr_idx;
      ++s;
    }
    wr_idx_.store(wr_idx);
  }

  std::size_t read(T * buf, std::size_t max) {
    std::size_t n = 0;
    auto rd_idx = rd_idx_.load();
    auto wr_idx = wr_idx_.load();
    while (n < max && rd_idx < wr_idx) {
      // TODO: inefficient
      *buf = queue_[rd_idx % size_];
      ++buf;
      ++rd_idx;
      ++n;
    }
    rd_idx_.store(rd_idx);
    return n;
  }

private:
  std::atomic<std::uint64_t> rd_idx_ = 0;
  std::atomic<std::uint64_t> wr_idx_ = 0;
  std::size_t size_;
  T * queue_ = nullptr;
};

inline
Perflog::
Perflog(Config const & config)
  : filename_(config.fetch<std::string>("perflog.filename", ""))
{
  if (filename_ != "") {
    open(filename_);
  }
}

inline
Perflog::
~Perflog() {
  done_.store(true);
  if (th_.joinable()) {
    th_.join();
  }
}

inline
void
Perflog::
open(std::string const & filename) {
  queue_ = std::make_unique<Queue<char>>(262144);

  file_.open(filename, std::ios::out | std::ios::trunc);
  std::cout << "Starting thread" << std::endl;
  th_ = std::thread([&]() {
    bool done = false;
    while(!done) {
      char buf[16384];
      auto n = queue_->read(buf, sizeof(buf));
      if (n > 0) {
        file_.write(buf, n);
      }
      sleep(1);
      done = done_.load();
    }
  });
  file_open_ = true;
}

inline
void
Perflog::
record_probe(Probe const & probe, Probe::Dictionary const & dictionary) {
  if (!file_open_) {
    return;
  }

  std::uint64_t timestamp;

  for (auto const & stamp : probe) {
    switch(stamp.type) {
      case Probe::FINAL:
        timestamp = stamp.value;
        break;

      case Probe::INVALID_:
        break;

      default:
        // Fetch the counter now so they will be printed in the right
        // order
        get_counter(stamp.key, stamp.depth, dictionary);
        break;
    }
  }

  probe.for_each_perf_metric([&](auto key, auto depth, auto value) {
    auto & counter = get_counter(key, depth, dictionary);
    counter.record(frame_, value);
  });

  if (!header_written_) {
    write_header();
  }

  if (header_version_ != version_) {
    throw std::runtime_error("More counters were added since header was written");
  }

  buf_.clear();

  buf_.insert(buf_.end(), reinterpret_cast<char const *>(&timestamp), reinterpret_cast<char const *>(&timestamp) + sizeof(timestamp));

  for (auto const & pc : perf_counters_) {
    std::uint32_t total = pc.total() / pc.scale();
    buf_.insert(buf_.end(), reinterpret_cast<char const *>(&total), reinterpret_cast<char const *>(&total) + sizeof(total));
  }

  queue_->write(buf_.data(), buf_.data() + buf_.size());

  for (auto & pc : perf_counters_) {
    pc.reset();
  }

  ++frame_;
}

inline
void
Perflog::
write_header() {
  buf_.clear();
  std::string time = "Time";
  buf_.insert(buf_.end(), time.c_str(), time.c_str() + time.length() + 1);
  for (auto const & pc : perf_counters_) {
    auto name = std::string(4*pc.depth(), ' ') + pc.name();
    buf_.insert(buf_.end(), name.c_str(), name.c_str() + name.length() + 1);
  }

  buf_.push_back('\n');

  queue_->write(buf_.data(), buf_.data() + buf_.size());

  header_version_ = version_;
  header_written_ = true;
}
}
