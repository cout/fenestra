#pragma once

#include "Clock.hpp"

#include <vector>
#include <string>
#include <map>
#include <tuple>

namespace fenestra {

class Probe {
public:
  using Key = std::uint16_t;
  using Depth = std::uint8_t;
  enum Type : std::uint8_t { DELTA, START, END, FINAL, INVALID_ };

  class Dictionary {
  public:
    Probe::Key operator[](std::string const & name) {
      auto it = keys_.find(name);
      if (it == keys_.end()) {
        auto key = keys_.size();
        bool inserted;
        std::tie(it, inserted) = keys_.emplace(name, key);
        names_.emplace(key, name);
        ++version_;
      }
      return it->second;
    }

    std::string const & operator[](Probe::Key key) const {
      return names_.at(key);
    }

    std::uint64_t version() const { return version_; }

    auto begin() const { return names_.begin(); }
    auto end() const { return names_.end(); }

  private:
    std::map<std::string, Probe::Key> keys_;
    std::map<Probe::Key, std::string> names_;
    std::uint64_t version_ = 0;
  };

  struct __attribute__((packed)) Stamp {
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

  template<typename Fn>
  void for_each_delta(Fn fn) const {
    Probe::Stamp last_stamp;

    tmp_stack_.clear();

    for (auto const & stamp : *this) {
      if (stamp.time == Timestamp()) continue;

      if (stamp.depth > last_stamp.depth) {
        tmp_stack_.push_back(last_stamp);
        last_stamp = Probe::Stamp();

      } else if (stamp.depth < last_stamp.depth) {
        // TODO: Check for empty before popping
        last_stamp = tmp_stack_.back();
        tmp_stack_.pop_back();
      }

      switch (last_stamp.type) {
        case Probe::DELTA:
        case Probe::START: {
          auto delta = stamp.time - last_stamp.time;
          fn(last_stamp.key, last_stamp.depth, delta);
          break;
        }

        case Probe::END:
        case Probe::FINAL:
        case Probe::INVALID_:
          break;
      }

      last_stamp = stamp;
    }
  }

private:
  std::vector<Stamp> stamps_;
  mutable std::vector<Stamp> tmp_stack_;
};

}
