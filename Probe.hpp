#pragma once

#include "Clock.hpp"

#include <vector>

namespace fenestra {

class Probe {
public:
  using Key = std::uint32_t;
  using Depth = std::uint32_t;
  enum Type { DELTA, START, END, FINAL, INVALID_ };

  struct Stamp {
    Stamp() {
    }

    Stamp(Key key, Type type, Depth depth, Timestamp time)
      : key(key)
      , type(type)
      , depth(depth)
      , time(time)
    {
    }

    Key key = 0;
    Type type = INVALID_;
    Depth depth = 0;
    Timestamp time = Nanoseconds::zero();
  };

  void mark(Key key, Depth depth, Timestamp time) {
    stamps_.emplace_back(key, DELTA, depth, time);
  }

  void mark(Key key, Type type, Depth depth, Timestamp time) {
    stamps_.emplace_back(key, type, depth, time);
  }

  void clear() {
    stamps_.clear();
  }

  auto begin() const { return stamps_.begin(); }
  auto end() const { return stamps_.end(); }

  void append(Probe & probe) {
    for (auto const & stamp : probe) {
      stamps_.emplace_back(stamp);
    }
  }

private:
  std::vector<Stamp> stamps_;
};

}
