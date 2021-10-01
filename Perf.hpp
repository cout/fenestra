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
  }

  auto const & name() const { return name_; }

  void record(Nanoseconds obs) {
    last_ = obs;
    total_ += obs;
    best_ = std::min(best_, obs);
    worst_ = std::max(worst_, obs);
    ++count_;
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
    Milliseconds avg_ms = total_ / count_;
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

  Nanoseconds last_ = Nanoseconds::zero();
  Nanoseconds total_ = Nanoseconds::zero();
  Nanoseconds best_ = Nanoseconds(999999999); // TODO
  Nanoseconds worst_ = Nanoseconds::zero();
  std::size_t count_ = 0;
};

std::ostream & operator<<(std::ostream & out, Perfcounter const & pc) {
  out << "avg=" << pc.avg_ms() << ", best=" << pc.best_ms() << ", worst=" << pc.worst_ms();
  return out;
}

class Perf {
public:
  Perf(Timestamp now = Clock::gettime(CLOCK_MONOTONIC))
    : start_(Clock::gettime(CLOCK_MONOTONIC))
    , sync_savefile_("Sync savefile")
    , poll_window_events_("Poll window events")
    , frame_delay_("Frame delay")
    , core_run_("Core run")
    , video_render_("Video render")
    , window_refresh_("Window refresh")
    , glfinish_("Glfinish")
    , last_frame_(start_)
  {
  }

  ~Perf() {
  }

  auto mark() {
    auto now = Clock::gettime(CLOCK_MONOTONIC);

    if (current_ && last_.nanos_ != Nanoseconds::zero()) {
      auto delta = now - last_;
      current_->record(delta);
    }

    last_ = now;

    return now;
  }

  auto mark_sync_savefile() {
    auto now = mark();
    current_ = &sync_savefile_;
    return now;
  }

  auto mark_frame_delay() {
    auto now = mark();
    current_ = &frame_delay_;
    return now;
  }

  auto mark_poll_window_events() {
    auto now = mark();
    current_ = &poll_window_events_;
    return now;
  }

  auto mark_core_run() {
    auto now = mark();
    current_ = &core_run_;
    return now;
  }

  auto mark_video_render() {
    auto now = mark();
    current_ = &video_render_;
    return now;
  }

  auto mark_window_refresh() {
    auto now = mark();
    current_ = &window_refresh_;
    return now;
  }

  auto mark_glfinish() {
    auto now = mark();
    current_ = &glfinish_;
    return now;
  }

  auto mark_loop_done() {
    auto now = mark();
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

    current_ = 0;
    last_frame_ = now;

    return now;
  }

  void dump_last() {
    std::cout << sync_savefile_.name() << ": " << sync_savefile_.last_ms() << std::endl;
    std::cout << poll_window_events_.name() << ": " << poll_window_events_.last_ms() << std::endl;
    std::cout << frame_delay_.name() << ": " << frame_delay_.last_ms() << std::endl;
    std::cout << core_run_.name() << ": " << core_run_.last_ms() << std::endl;
    std::cout << video_render_.name() << ": " << video_render_.last_ms() << std::endl;
    std::cout << window_refresh_.name() << ": " << window_refresh_.last_ms() << std::endl;
    std::cout << glfinish_.name() << ": " << glfinish_.last_ms() << std::endl;
  }

  void dump(Timestamp now) {
    auto delta = now - start_;
    auto fps = frames_ / Seconds(delta).count();

    std::cout << "FPS: " << fps << std::endl;
    std::cout << sync_savefile_.name() << ": " << sync_savefile_ << std::endl;
    std::cout << poll_window_events_.name() << ": " << poll_window_events_ << std::endl;
    std::cout << frame_delay_.name() << ": " << frame_delay_ << std::endl;
    std::cout << core_run_.name() << ": " << core_run_ << std::endl;
    std::cout << video_render_.name() << ": " << video_render_ << std::endl;
    std::cout << window_refresh_.name() << ": " << window_refresh_ << std::endl;
    std::cout << glfinish_.name() << ": " << glfinish_ << std::endl;
  }

  void reset(Timestamp now) {
    auto frame = frame_;
    *this = Perf(now);
    frame_ = frame;
  }

private:
  Timestamp start_;

  Perfcounter sync_savefile_;
  Perfcounter poll_window_events_;
  Perfcounter frame_delay_;
  Perfcounter core_run_;
  Perfcounter video_render_;
  Perfcounter window_refresh_;
  Perfcounter glfinish_;

  Timestamp last_ = Nanoseconds::zero();
  Timestamp last_frame_ = Nanoseconds::zero();
  Perfcounter * current_ = 0;

  std::uint64_t frames_ = 0;
  std::uint64_t frame_ = 0;
};

}
