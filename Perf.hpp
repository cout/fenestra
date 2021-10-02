#pragma once

#include "Clock.hpp"
#include "Probe.hpp"

#include <iostream>
#include <algorithm>
#include <string_view>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

namespace fenestra {

class Perfcounter {
public:
  Perfcounter(std::string_view name, Probe::Depth depth)
    : name_(name)
    , depth_(depth)
  {
    reset();
  }

  auto const & name() const { return name_; }
  auto depth() const { return depth_; }

  void record(std::uint64_t frame, Nanoseconds obs) {
    last_frame_ = frame;
    last_ = obs;
    total_ += obs;
    best_ = std::min(best_, obs);
    worst_ = std::max(worst_, obs);
    ++count_;
  }

  void reset() {
    last_ = Nanoseconds::zero();
    total_ = Nanoseconds::zero();
    best_ = Nanoseconds(99999999999999); // TODO
    worst_ = Nanoseconds::zero();
    count_ = 0;
  }

  auto last_frame() const { return last_frame_; }
  auto last() const { return last_; }
  auto total() const { return total_; }
  auto best() const { return best_; }
  auto worst() const { return worst_; }
  auto count() const { return count_; }

  auto last_ms() const {
    Milliseconds last_ms = last_;
    return last_ms.count();
  }

  auto avg_ms() const {
    Milliseconds avg_ms = count_ > 0 ? total_ / count_ : Milliseconds::zero();
    return avg_ms.count();
  }

  auto best_ms() const {
    Milliseconds best_ms = best_;
    return best_ms.count();
  }

  auto worst_ms() const {
    Milliseconds worst_ms = worst_;
    return worst_ms.count();
  }

private:
  std::string name_;
  Probe::Depth depth_;

  std::uint64_t last_frame_;
  Nanoseconds last_;
  Nanoseconds total_;
  Nanoseconds best_;
  Nanoseconds worst_;
  std::size_t count_;
};

std::ostream & operator<<(std::ostream & out, Perfcounter const & pc) {
  out << "avg=" << pc.avg_ms() << ", best=" << pc.best_ms() << ", worst=" << pc.worst_ms();
  return out;
}

class Perf {
public:
  using PerfID = std::size_t;

  auto loop_done(Timestamp now) {
    auto last_frame_time = now - last_frame_;

    ++frames_;

    if (last_frame_time > Nanoseconds(20'000'000)) {
      std::cout << std::endl;
      std::cout << "!!!!!!!!!!!!!! LAST FRAME TOOK TOO LONG !!!!!!!!!!!!!!!!!" << std::endl;
      Milliseconds last_frame_time_ms = last_frame_time;
      std::cout << "Frame #: " << frame_ << std::endl;
      std::cout << "Last frame time: " << last_frame_time_ms.count() << std::endl;
      dump_last();
    }

    if (frames_ >= 60) {
      std::cout << std::endl;
      dump(now);
      reset(now);
    }

    ++frame_;
    last_frame_ = now;

    return now;
  }

  void dump_last() {
    for (auto const & pc : perf_counters_) {
      if (pc.last_frame() == frame_) {
        std::cout << std::string(2*pc.depth(), ' ') << pc.name() << ": " << pc.last_ms() << std::endl;
      }
    }
  }

  void dump(Timestamp now) {
    auto delta = now - last_dump_;
    auto fps = delta > Seconds::zero() ? frames_ / Seconds(delta).count() : 0;

    std::cout << "FPS: " << fps << std::endl;
    for (auto const & pc : perf_counters_) {
      std::cout << std::string(2*pc.depth(), ' ') << pc.name() << ": " << pc << std::endl;
    }
  }

  void reset(Timestamp now) {
    for (auto & pc : perf_counters_) {
      pc.reset();
    }
    last_dump_ = now;
    frames_ = 0;
  }

  Probe::Key probe_key(std::string const & name) {
    auto it = probe_keys_.find(name);
    if (it == probe_keys_.end()) {
      auto key = probe_keys_.size();
      bool inserted;
      std::tie(it, inserted) = probe_keys_.emplace(name, key);
      probe_names_.emplace(key, name);
    }
    return it->second;
  }

  std::string const & probe_name(Probe::Key key) {
    return probe_names_.at(key);
  }

  auto & get_counter(std::string name, Probe::Depth depth) {
    auto it = perf_counter_name_to_idx_.find(name);
    if (it != perf_counter_name_to_idx_.end()) {
      auto idx = it->second;
      return perf_counters_[idx];
    } else {
      auto idx = perf_counters_.size();
      perf_counters_.emplace_back(name, depth);
      perf_counter_name_to_idx_.emplace(name, idx);
      return perf_counters_[idx];
    }
  }

  void record_probe(Probe const & probe) {
    Probe::Stamp last_stamp;

    for (auto const & stamp : probe) {
      if (last_stamp.time != Timestamp()) {
        auto delta = stamp.time - last_stamp.time;
        auto & counter = get_counter(probe_name(last_stamp.key), last_stamp.depth);
        counter.record(frame_, delta);
      }

      last_stamp = stamp;
    }
  }

private:
  std::map<std::string, Probe::Key> probe_keys_;
  std::map<Probe::Key, std::string> probe_names_;
  std::vector<Perfcounter> perf_counters_;
  std::unordered_map<std::string, std::size_t> perf_counter_name_to_idx_;

  Timestamp last_dump_ = Nanoseconds::zero();
  Timestamp last_frame_ = Nanoseconds::zero();

  std::uint64_t frames_ = 0;
  std::uint64_t frame_ = 0;
};

}
