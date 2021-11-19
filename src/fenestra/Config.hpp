#pragma once

#include "Clock.hpp"
#include "JSON.hpp"

#include "libretro.h"

#include <string_view>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include <any>

namespace fenestra {

class Config {
private:
  class Setter {
  public:
    virtual ~Setter() { }

    virtual void operator()(Json::Value const & cfg) = 0;

  protected:
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
  };

  template<typename T>
  class Setter_T
    : public Setter
  {
  public:
    Setter_T(std::string const  & name, T * value)
      : name_(name)
      , value_(value)
    {
    }

    virtual void operator()(Json::Value const & cfg) override {
      auto cfg_value = json::deep_find(&cfg, name_);
      if (cfg_value) {
        json::assign(*value_, *cfg_value);
        log_value(name_, *value_);
      }
    }

    virtual ~Setter_T() { }

  private:
    std::string name_;
    T * value_;
  };

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

  template <typename T, typename Default>
  T & fetch(std::string const & name, Default dflt) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto [ inserted_it, inserted ] = values_.emplace(name, std::make_any<T>(dflt));
      T * value = std::any_cast<T>(&inserted_it->second);
      setters_.emplace_back(new Setter_T<T>(name, value));
      (*setters_.back())(cfg_);
      return *value;
    } else {
      return *std::any_cast<T>(&it->second);
    }
  }

private:
  void refresh() {
    for (auto const & setter : setters_) {
      (*setter)(cfg_);
    }
  }

private:
  Json::Value cfg_;
  mutable std::map<std::string, std::any> values_;
  mutable std::vector<std::unique_ptr<Setter>> setters_;
};

}
