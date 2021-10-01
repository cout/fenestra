#pragma once

#include "Clock.hpp"

#include <iostream>
#include <algorithm>
#include <string_view>
#include <string>

namespace fenestra {

class Perfcounter {
public:
  Perfcounter(std::string_view name)
    : name_(name)
  {
    reset();
  }

  auto const & name() const { return name_; }

  void record(Nanoseconds obs) {
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
    ++frame_;

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

    last_frame_ = now;

    return now;
  }

  void dump_last() {
    for (auto const & pc : perf_counters_) {
      std::cout << pc.name() << ": " << pc.last_ms() << std::endl;
    }
  }

  void dump(Timestamp now) {
    auto delta = now - start_;
    auto fps = delta > Seconds::zero() ? frames_ / Seconds(delta).count() : 0;

    std::cout << "FPS: " << fps << std::endl;
    for (auto const & pc : perf_counters_) {
      std::cout << pc.name() << ": " << pc << std::endl;
    }
  }

  void reset(Timestamp now) {
    for (auto & pc : perf_counters_) {
      pc.reset();
    }
    start_ = now;
    last_frame_ = Nanoseconds::zero();
    frames_ = 0;
  }

  PerfID add_counter(std::string name) {
    auto id = perf_counters_.size();
    perf_counters_.emplace_back(name);
    return id;
  }

  auto & get_counter(PerfID id) {
    return perf_counters_[id];
  }

private:
  std::vector<Perfcounter> perf_counters_;

  Timestamp start_ = Nanoseconds::zero();
  Timestamp last_frame_ = Nanoseconds::zero();

  std::uint64_t frames_ = 0;
  std::uint64_t frame_ = 0;
};

}
