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
private:
  class Setter {
  public:
    virtual ~Setter() { }

    virtual void operator()(Json::Value const & cfg) = 0;

  protected:
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
        assign(setting_->value, *cfg_value);
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
    : scale_factor_(fetch<float>("scale_factor", 6.0f))
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
      (*setter)(cfg_);
    }
  }

  float const & scale_factor() const { return scale_factor_; }

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

  float & scale_factor_;
};

}
