#pragma once

#include "Plugin.hpp"
#include "../Clock.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string_view>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <iterator>

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

  auto total_ms() const {
    Milliseconds total_ms = total_;
    return total_ms.count();
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

class Perf
  : public Plugin
{
public:
  Perf(Config const & config) {
  }

  using PerfID = std::size_t;

  auto loop_done(Timestamp now) {
    auto last_frame_time = now - last_frame_;

    ++frames_;

    // We consider this frame slow if it is more than 3.3ms longer than
    // expected (TODO and we assume a hard-coded 60fps).  But if the
    // previous frame finished ahead of schedule, then it might just be
    // that this frame was longer because we have to re-sync.
    bool slow_frame = last_frame_time > Nanoseconds(20'000'000)
      && prev_last_frame_time_ > Nanoseconds(16'700'000);

    if (slow_frame) {
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
    prev_last_frame_time_ = last_frame_time;

    return now;
  }

  void dump_last() {
    std::stringstream strm;
    strm << std::fixed << std::setprecision(3);
    for (auto const & pc : perf_counters_) {
      if (pc.last_frame() == frame_) {
        strm << std::string(2*pc.depth(), ' ') << pc.name() << ": " << pc.last_ms() << std::endl;
      }
    }
    std::cout << strm.str();
  }

  void dump(Timestamp now) {
    auto delta = now - last_dump_;
    auto fps = delta > Seconds::zero() ? frames_ / Seconds(delta).count() : 0;

    std::stringstream strm;
    strm << std::fixed << std::setprecision(2);
    strm << "FPS: " << fps << std::endl;
    strm << std::setprecision(3);
    for (auto const & pc : perf_counters_) {
      if (pc.count() > 0) {
        strm << std::string(2*pc.depth(), ' ') << pc.name() << ": "
             << "avg=" << pc.total_ms() / frames_
             << ", best=" << pc.best_ms()
             << ", worst=" << pc.worst_ms()
             << std::endl;
      }
    }
    std::cout << strm.str();
  }

  void reset(Timestamp now) {
    for (auto & pc : perf_counters_) {
      pc.reset();
    }
    last_dump_ = now;
    frames_ = 0;
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

  virtual void record_probe(Probe const & probe, Probe::Dictionary const & dictionary) override {
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
          // TODO: Include parent counters in the counter name,
          // otherwise we will count all probe samples with the same
          // key/depth as the same (e.g. audio sample and video refresh)
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

    if (probe.size() > 0) {
      auto now = probe.back().time;
      loop_done(now);
    }
  }

private:
  Probe::Dictionary dictionary_;
  std::vector<Perfcounter> perf_counters_;
  std::unordered_map<std::string, std::size_t> perf_counter_name_to_idx_;

  std::vector<Probe::Stamp> stamps_;

  Timestamp last_dump_ = Nanoseconds::zero();
  Timestamp last_frame_ = Nanoseconds::zero();

  std::uint64_t frames_ = 0;
  std::uint64_t frame_ = 0;

  Nanoseconds prev_last_frame_time_ = Nanoseconds::zero();
};

}
