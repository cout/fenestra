#pragma once

#include "Config.hpp"

#include <map>
#include <string>
#include <typeinfo>
#include <memory>

namespace fenestra {

class Registry {
public:
  Registry(Config const & config)
    : config_(config)
  {
  }

  template<typename T, typename ... Names>
  T & fetch(Names const & ... names) {
    auto name = this->name<T>(names...);
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto plugin = std::make_shared<T>(config_);
      bool inserted;
      std::tie(it, inserted) = values_.emplace(name, plugin);
    }
    return *std::static_pointer_cast<T>(it->second);
  }

private:
  template<typename T, typename ... Names>
  static std::string name(Names const & ... names) {
    auto name = std::string(typeid(T).name());
    std::array<std::string, sizeof...(Names)> a { names... };
    for (auto const & n : a) {
      name += "|" + n;
    }
    return name;
  }

private:
  Config const & config_;
  std::map<std::string, std::shared_ptr<void>> values_;
};

}