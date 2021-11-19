#pragma once

#include "Clock.hpp"
#include "JSON.hpp"

#include "libretro.h"

#include <string_view>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <functional>
#include <memory>
#include <iostream>

namespace fenestra {

struct Abstract_Setting {
  virtual ~Abstract_Setting() { }
};

template <typename T>
struct Setting : Abstract_Setting {
  virtual ~Setting() { }
  Setting() { }
  Setting(T const & value) : value(value) { }
  T value;
};

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
    Setter_T(std::string const  & name, Setting<T> * setting)
      : name_(name)
      , setting_(setting)
    {
    }

    virtual void operator()(Json::Value const & cfg) override {
      auto cfg_value = find(&cfg, name_);
      if (cfg_value) {
        json::assign(setting_->value, *cfg_value);
        log_value(name_, setting_->value);
      }
    }

    virtual ~Setter_T() { }

  private:
    std::string name_;
    Setting<T> * setting_;
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
    Json::Value v;
    std::ifstream file(filename);
    file >> v;

    merge(v, cfg_);
    refresh();
  }

  template <typename T, typename Default>
  T & fetch(std::string const & name, Default dflt) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto [ inserted_it, inserted ] = values_.emplace(name, new Setting<T>(dflt));
      auto * setting = static_cast<Setting<T> *>(inserted_it->second.get());
      setters_.emplace_back(new Setter_T<T>(name, setting));
      (*setters_.back())(cfg_);
      return setting->value;
    } else {
      return dynamic_cast<Setting<T> *>(it->second.get())->value;
    }
  }

private:
  static void merge(Json::Value v, Json::Value & target) {
    for (auto it = v.begin(); it != v.end(); ++it) {
      auto & t = target[it.name()];
      if (it->type() == Json::objectValue && t.type() == Json::objectValue) {
        merge(*it, t);
      } else {
        t = *it;
      }
    }
  }

  void refresh() {
    for (auto const & setter : setters_) {
      (*setter)(cfg_);
    }
  }

  static Json::Value const * find(Json::Value const * v, std::string const & name) {
    auto it = name.begin();
    auto begin_word = it;
    auto end = name.end();

    while (it != end) {
      if (*it == '.') {
        v = v->find(&*begin_word, &*it);
        if (!v) return nullptr;
        ++it;
        begin_word = it;
      } else {
        ++it;
      }
    }

    return v->find(&*begin_word, &*it);
  }

private:
  Json::Value cfg_;
  mutable std::map<std::string, std::unique_ptr<Abstract_Setting>> values_;
  mutable std::vector<std::unique_ptr<Setter>> setters_;
};

}
