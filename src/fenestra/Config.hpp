#pragma once

#include "Clock.hpp"
#include "JSON.hpp"

#include "libretro.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <iostream>
#include <any>

namespace fenestra {

class Config {
public:
  Config()
  {
  }

  explicit Config(std::string const & filename)
    : Config()
  {
    load(filename);
  }

  void load(std::string const & filename) {
    auto v = json::read_file(filename);
    json::merge(cfg_, v);
    refresh();
  }

  template <typename T>
  T & fetch(std::string const & name, T const & dflt) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto [ inserted_it, inserted ] = values_.emplace(name, std::make_any<T>(dflt));
      T * value = std::any_cast<T>(&inserted_it->second);
      auto & setter = setters_.emplace_back([name, value](Json::Value const & cfg) {
        if (auto cfg_value = json::deep_find(&cfg, name)) {
          json::assign(*value, *cfg_value);
          log_value(name, *value);
        }
      });
      setter(cfg_);
      return *value;
    } else {
      return *std::any_cast<T>(&it->second);
    }
  }

  class Subtree;

  Subtree subtree(std::string const & name) const;

private:
  void refresh() {
    for (auto const & setter : setters_) {
      setter(cfg_);
    }
  }

  template <typename T>
  static void log_value(std::string const & name, std::vector<T> const & value) {
  }

  template <typename T>
  static void log_value(std::string const & name, std::set<T> const & value) {
  }

  template <typename T>
  static void log_value(std::string const & name, std::map<std::string, T> const & value) {
  }

  static void log_value(std::string const & name, Milliseconds const & value) {
    std::cout << "[INFO config] " << name << ": " << value.count() << std::endl;
  }

  template <typename T>
  static void log_value(std::string const & name, T const & value) {
    std::cout << "[INFO config] " << name << ": " << value << std::endl;
  }

private:
  Json::Value cfg_;
  mutable std::map<std::string, std::any> values_;
  mutable std::vector<std::function<void (Json::Value const &)>> setters_;
};

class Config::Subtree {
public:
  Subtree(Config const & config, std::string const & prefix)
    : config_(config)
    , prefix_(prefix)
  {
  }

  template <typename T>
  T & fetch(std::string const & name, T const & dflt) const {
    return config_.fetch<T>(prefix_ + name, dflt);
  }

  Config const & root() const { return config_; }

private:
  Config const & config_;
  std::string prefix_;
};

inline
Config::Subtree
Config::subtree(std::string const & name) const {
  return Subtree(*this, name + ".");
}

}
