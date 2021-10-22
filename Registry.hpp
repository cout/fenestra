#pragma once

#include <map>
#include <string>
#include <typeinfo>
#include <memory>
#include <tuple>

namespace fenestra {

class Registry {
public:
  Registry()
  {
  }

  template<typename T, typename Factory, typename ... Names>
  T & create(Factory factory, Names const & ... names) {
    auto name = this->name<T>(names...);
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto plugin = factory();
      bool inserted;
      std::tie(it, inserted) = values_.emplace(name, plugin);
    }
    return *std::static_pointer_cast<T>(it->second);
  }

  template<typename T, typename ... Names>
  T & fetch(Names const & ... names) {
    auto name = this->name<T>(names...);
    auto it = values_.find(name);
    if (it == values_.end()) {
      throw std::runtime_error("Not found: " + name);
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
  std::map<std::string, std::shared_ptr<void>> values_;
};

}
