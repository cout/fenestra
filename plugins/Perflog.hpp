#pragma once

#include "Plugin.hpp"

#include <sstream>
#include <fstream>

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
    file_.open(filename, std::ios::out | std::ios::trunc);
  }

  virtual void record_probe(Probe const & probe, Probe::Dictionary const & dictionary) override;

private:
  Perfcounter & get_counter(Probe::Key key, Probe::Depth depth, Probe::Dictionary const & dictionary) {
    auto it = key_to_idx_.find(key);
    if (it != key_to_idx_.end()) {
      auto idx = it->second;
      return perf_counters_[idx];
    } else {
      auto idx = perf_counters_.size();
      perf_counters_.emplace_back(dictionary[key], depth);
      key_to_idx_.emplace(key, idx);
      ++version_;
      return perf_counters_[idx];
    }
  }

  void write_header();

private:
  Probe::Dictionary probe_dict_;
  Probe last_;
  std::ofstream file_;
  bool header_written_ = false;
  std::vector<char> buf_;

  std::vector<Probe::Stamp> stamps_;

  std::vector<Perfcounter> perf_counters_;
  std::unordered_map<Probe::Key, std::size_t> key_to_idx_;

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
    total_ += obs;
  }

  void reset() {
    total_ = Nanoseconds::zero();
  }

  auto total() const { return total_; }

  std::uint32_t total_us() const {
    return total_.count() / 1000;
  }


private:
  std::string name_;
  Probe::Depth depth_ = 0;
  Nanoseconds total_ = Nanoseconds::zero();
};

inline
void
Perflog::
record_probe(Probe const & probe, Probe::Dictionary const & dictionary) {
  if (!file_.good()) {
    return;
  }

  auto now = probe.back().time;

  for (auto const & stamp : probe) {
    // Fetch the counter now so they will be printed in the right
    // order
    get_counter(stamp.key, stamp.depth, dictionary);
  }

  probe.for_each_delta([&](auto key, auto depth, auto delta) {
    auto & counter = get_counter(key, depth, dictionary);
    counter.record(frame_, delta);
  });

  if (!header_written_) {
    write_header();
  }

  if (header_version_ != version_) {
    throw std::runtime_error("More counters were added since header was written");
  }

  buf_.clear();

  buf_.insert(buf_.end(), reinterpret_cast<char const *>(&now), reinterpret_cast<char const *>(&now) + sizeof(now));

  for (auto const & pc : perf_counters_) {
    auto total_us = pc.total_us();
    buf_.insert(buf_.end(), reinterpret_cast<char const *>(&total_us), reinterpret_cast<char const *>(&total_us) + sizeof(total_us));
  }

  file_.write(buf_.data(), buf_.size());

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
    buf_.insert(buf_.end(), pc.name().c_str(), pc.name().c_str() + pc.name().length() + 1);
  }

  buf_.push_back('\n');

  file_.write(buf_.data(), buf_.size());

  header_version_ = version_;
  header_written_ = true;
}

}
