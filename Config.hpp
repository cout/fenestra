#pragma once

#include "Clock.hpp"

#include "libretro.h"

#include <json/json.h>

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
public:
  Config()
    : plugins_(fetch<std::map<std::string, bool>>("plugins", std::map<std::string, bool>({
        { "logger", true }, { "perf", true }, { "savefile", true }, { "glfw-gamepad", true },
        { "portaudio", true }, { "gl", true }, { "sync", true }, { "framedelay", true },
        { "v4l2stream", true }, { "gstreamer", true }, { "ssr", true }, { "netcmds", true },
        { "rusage", true }
      })))
    , scale_factor_(fetch<float>("scale_factor", 6.0f))
    , system_directory_(fetch<std::string>("system_directory", "."))
    , save_directory_(fetch<std::string>("save_directory", "."))
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

    for (auto const & setter : setters_) {
      setter(cfg_);
    }
  }

  auto const & plugins() const { return plugins_; }

  float const & scale_factor() const { return scale_factor_; }

  char const * system_directory() const { return system_directory_.c_str(); }
  char const * save_directory() const { return save_directory_.c_str(); }

  template <typename T, typename Default>
  T & fetch(std::string const & name, Default dflt) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto [ inserted_it, inserted ] = values_.emplace(name, std::make_shared<Setting<T>>(dflt));
      auto setting = std::static_pointer_cast<Setting<T>>(inserted_it->second);
      setters_.emplace_back([name, setting](Json::Value const & cfg) {
        auto cfg_value = find(&cfg, name);
        if (cfg_value) {
          assign(setting->value, *cfg_value);
          log_value(name, setting->value);
        }
      });
      setters_.back()(cfg_);
      return setting->value;
    } else {
      return std::dynamic_pointer_cast<Setting<T>>(it->second)->value;
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
  template <typename T>
  static void assign(std::vector<T> & dest, Json::Value const & src) {
    std::vector<T> result;
    for (auto v : src) {
      T tmp;
      assign(tmp, v);
      result.push_back(tmp);
    }
    dest = result;
  }

  template <typename T>
  static void assign(std::set<T> & dest, Json::Value const & src) {
    std::set<T> result;
    for (auto v : src) {
      T tmp;
      assign(tmp, v);
      result.insert(tmp);
    }
    dest = result;
  }

  template <typename T>
  static void assign(std::map<std::string, T> & dest, Json::Value const & src) {
    std::map<std::string, T> result;
    auto it = src.begin();
    auto end = src.end();
    while (it != end) {
      T tmp;
      assign(tmp, *it);
      result[it.name()] = tmp;
      ++it;
    }
    dest = result;
  }

  static void assign(bool & dest, Json::Value const & src) {
    dest = src.asBool();
  }

  static void assign(int & dest, Json::Value const & src) {
    dest = src.asInt();
  }

  static void assign(unsigned int & dest, Json::Value const & src) {
    dest = src.asUInt();
  }

  static void assign(float & dest, Json::Value const & src) {
    dest = src.asFloat();
  }

  static void assign(double & dest, Json::Value const & src) {
    dest = src.asDouble();
  }

  static void assign(std::string & dest, Json::Value const & src) {
    dest = src.asString();
  }

  static void assign(Milliseconds & dest, Json::Value const & src) {
    dest = Milliseconds(src.asDouble());
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
  mutable std::map<std::string, std::shared_ptr<Abstract_Setting>> values_;
  mutable std::vector<std::function<void(Json::Value const &)>> setters_;

  std::map<std::string, bool> & plugins_;

  float & scale_factor_;

  std::string & system_directory_;
  std::string & save_directory_;
};

}
