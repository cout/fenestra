#pragma once

#include "Clock.hpp"

#include <vector>
#include <string>
#include <map>
#include <tuple>

namespace fenestra {

class Probe {
public:
  using Key = std::uint32_t;
  using Depth = std::uint32_t;
  enum Type { DELTA, START, END, FINAL, INVALID_ };

  class Dictionary {
  public:
    Probe::Key operator[](std::string const & name) {
      auto it = keys_.find(name);
      if (it == keys_.end()) {
        auto key = keys_.size();
        bool inserted;
        std::tie(it, inserted) = keys_.emplace(name, key);
        names_.emplace(key, name);
      }
      return it->second;
    }

    std::string const & operator[](Probe::Key key) {
      return names_.at(key);
    }

  private:
    std::map<std::string, Probe::Key> keys_;
    std::map<Probe::Key, std::string> names_;
  };

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

  auto size() const { return stamps_.size(); }
  auto const & back() const { return stamps_.back(); }

  void append(Probe & probe) {
    for (auto const & stamp : probe) {
      stamps_.emplace_back(stamp);
    }
  }

private:
  std::vector<Stamp> stamps_;
};

}
