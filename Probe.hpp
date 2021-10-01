#pragma once

#include "Clock.hpp"

#include <vector>

namespace fenestra {

class Probe {
public:
  using Key = std::uint32_t;
  using Depth = std::uint32_t;

  struct Stamp {
    Stamp(Key key, Depth depth, Timestamp time)
      : key(key)
      , depth(depth)
      , time(time)
    {
    }

    Key key;
    Depth depth;
    Timestamp time;
  };

  void mark(Key key, Depth depth, Timestamp time) {
    stamps_.emplace_back(key, depth, time);
  }

  void clear() {
    stamps_.clear();
  }

  auto begin() const { return stamps_.begin(); }
  auto end() const { return stamps_.end(); }

private:
  std::vector<Stamp> stamps_;
};

}
