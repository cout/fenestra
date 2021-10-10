#pragma once

#include <cstdint>
#include <chrono>

#include <time.h>

namespace fenestra {

using Nanoseconds = std::chrono::duration<std::int64_t, std::nano>;
using Milliseconds = std::chrono::duration<double, std::milli>;
using Seconds = std::chrono::duration<double>;

// TODO: Use std::chrono::time_point (but this requires chosing a clock)
struct __attribute__((packed)) Timestamp {
  Timestamp(Nanoseconds nanos = Nanoseconds::zero()) : nanos_(nanos.count()) { }
  std::int64_t nanos_;
};

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.nanos_ == rhs.nanos_;
}

inline bool operator!=(Timestamp lhs, Timestamp rhs) {
  return lhs.nanos_ != rhs.nanos_;
}

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.nanos_ < rhs.nanos_;
}

inline bool operator>(Timestamp lhs, Timestamp rhs) {
  return lhs.nanos_ > rhs.nanos_;
}

inline Nanoseconds operator-(Timestamp lhs, Timestamp rhs) {
  return Nanoseconds(lhs.nanos_ - rhs.nanos_);
}

inline Nanoseconds nanoseconds(timespec const & ts) {
  return Nanoseconds(ts.tv_sec * 1'000'000'000 + ts.tv_nsec);
}

inline Nanoseconds nanoseconds_since_epoch(Timestamp ts) {
  return Nanoseconds(ts.nanos_);
}

template<typename Rep, typename Period>
inline Timestamp operator+(Timestamp ts, std::chrono::duration<Rep, Period> delta) {
  return Timestamp(Nanoseconds(ts.nanos_) + std::chrono::duration_cast<Nanoseconds>(delta));
}

template<typename Rep, typename Period>
inline Timestamp operator-(Timestamp ts, std::chrono::duration<Rep, Period> delta) {
  return Timestamp(ts.nanos_ - std::chrono::duration_cast<Nanoseconds>(delta));
}

inline timespec to_timespec(Nanoseconds nanos) {
  timespec ts;
  ts.tv_sec = nanos.count() / 1'000'000'000;
  ts.tv_nsec = nanos.count() % 1'000'000'000;
  return ts;
}

class Clock {
public:
  static Timestamp gettime(clockid_t clk_id) {
    timespec ts;
    clock_gettime(clk_id, &ts);
    return Timestamp(nanoseconds(ts));
  }

  static auto nanosleep(Nanoseconds delay, clockid_t clk_id, int flags = 0) {
    if (delay > Nanoseconds::zero()) {
      timespec req(to_timespec(delay));
      timespec rem;
      clock_nanosleep(clk_id, flags, &req, &rem);
      return nanoseconds(rem);
    } else {
      return Nanoseconds::zero();
    }
  }

  static auto nanosleep_until(Timestamp timestamp, clockid_t clk_id, int flags = 0) {
    return nanosleep(nanoseconds_since_epoch(timestamp), clk_id, flags | TIMER_ABSTIME);
  }

  static auto nanosleep(Milliseconds delay, clockid_t clk_id) {
    Nanoseconds nanos(std::uint64_t(delay.count() * 1'000'000));
    return nanosleep(nanos, clk_id);
  }
};

}
