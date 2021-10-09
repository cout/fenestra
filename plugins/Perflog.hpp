#pragma once

#include "Plugin.hpp"

#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fenestra {

class Perflog
  : public Plugin
{
private:
  class Perfcounter;

public:
  Perflog(Config const & config) {
    if (config.perflog_filename() != "") {
      open(config.perflog_filename());
    }
  }

  void open(std::string const & filename) {
    if ((fd_ = ::open(filename.c_str(), O_CREAT|O_TRUNC|O_RDWR|O_NOATIME, 0664)) < 0) {
      throw std::runtime_error("open failed");
    }
  }

  virtual void record_probe(Probe const & probe, Probe::Dictionary const & dictionary) override;

private:
  Perfcounter & get_counter(std::string name, Probe::Depth depth) {
    auto it = perf_counter_name_to_idx_.find(name);
    if (it != perf_counter_name_to_idx_.end()) {
      auto idx = it->second;
      return perf_counters_[idx];
    } else {
      auto idx = perf_counters_.size();
      perf_counters_.emplace_back(name, depth);
      perf_counter_name_to_idx_.emplace(name, idx);
      ++version_;
      return perf_counters_[idx];
    }
  }

private:
  Probe::Dictionary probe_dict_;
  Probe last_;
  int fd_ = -1;
  bool header_written_ = false;
  std::vector<char> buf_;

  std::vector<Probe::Stamp> stamps_;

  std::vector<Perfcounter> perf_counters_;
  std::unordered_map<std::string, std::size_t> perf_counter_name_to_idx_;

  std::uint64_t version_ = 0;
  std::uint64_t header_version_ = 0;
  std::int64_t frame_ = 0;
};

class Perflog::Perfcounter {
public:
  Perfcounter(std::string_view name, Probe::Depth depth)
    : name_(name)
    , depth_(depth)
  {
  }

  auto const & name() const { return name_; }
  auto depth() const { return depth_; }

  void record(std::uint64_t frame, Nanoseconds obs) {
    last_frame_ = frame;
    last_ = obs;
  }

  auto last_frame() const { return last_frame_; }
  auto last() const { return last_; }

  auto last_ms() const {
    Milliseconds last_ms = last_;
    return last_ms.count();
  }

private:
  std::string name_;
  Probe::Depth depth_ = 0;
  std::uint64_t last_frame_ = 0;
  Nanoseconds last_ = Nanoseconds::zero();
};

inline
void
Perflog::
record_probe(Probe const & probe, Probe::Dictionary const & dictionary) {
  if (fd_ < 0) {
    return;
  }

  auto now = probe.back().time;

  Probe::Stamp last_stamp;

  stamps_.clear();

  for (auto const & stamp : probe) {
    // Fetch the counter now so they will be printed in the right
    // order
    get_counter(dictionary[stamp.key], stamp.depth);

    if (stamp.time == Timestamp()) continue;

    if (stamp.depth > last_stamp.depth) {
      stamps_.push_back(last_stamp);
      last_stamp = Probe::Stamp();

    } else if (stamp.depth < last_stamp.depth) {
      // TODO: Check for empty before popping
      last_stamp = stamps_.back();
      stamps_.pop_back();
    }

    switch (last_stamp.type) {
      case Probe::DELTA:
      case Probe::START: {
        auto delta = stamp.time - last_stamp.time;
        auto & counter = get_counter(dictionary[last_stamp.key], last_stamp.depth);
        counter.record(frame_, delta);
        break;
      }

      case Probe::END:
      case Probe::FINAL:
      case Probe::INVALID_:
        break;
    }

    last_stamp = stamp;
  }

  if (!header_written_) {
    buf_.clear();
    std::string time = "Time";
    buf_.insert(buf_.end(), time.c_str(), time.c_str() + time.length() + 1);
    for (auto const & pc : perf_counters_) {
      auto last = pc.last();
      buf_.insert(buf_.end(), pc.name().c_str(), pc.name().c_str() + pc.name().length() + 1);
    }

    buf_.push_back('\n');

    if (::write(fd_, buf_.data(), buf_.size()) < 0) {
      throw std::runtime_error("write failed");
    }

    header_version_ = version_;
    header_written_ = true;
  }

  if (header_version_ != version_) {
    throw std::runtime_error("More counters were added since header was written");
  }

  buf_.clear();

  buf_.insert(buf_.end(), reinterpret_cast<char const *>(&now), reinterpret_cast<char const *>(&now) + sizeof(now));

  for (auto const & pc : perf_counters_) {
    auto last = pc.last();
    buf_.insert(buf_.end(), reinterpret_cast<char const *>(&last), reinterpret_cast<char const *>(&last) + sizeof(last));
  }

  if (::write(fd_, buf_.data(), buf_.size()) < 0) {
    throw std::runtime_error("write failed");
  }

  ++frame_;
}

}
