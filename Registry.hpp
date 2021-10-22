#pragma once

#include <map>
#include <string>
#include <typeinfo>
#include <memory>
#include <tuple>

namespace fenestra {

class Registry {
private:
  template <typename T>
  using Factory_Function = std::function<std::shared_ptr<T>()>;

public:
  Registry()
  {
  }

  template <typename T, typename Factory>
  void register_factory(Factory factory) {
    auto type_name = this->name<T>();
    auto it = values_.find(type_name);
    if (it != values_.end()) {
      throw std::runtime_error("Factory already exists: " + type_name);
    }
    bool inserted;
    std::tie(it, inserted) = factories_.emplace(
        type_name,
        std::make_shared<Factory_Function<T>>([=]() { return factory(); }));
  }

  template <typename T, typename ... Names>
  T & fetch(Names const & ... names) {
    auto name = this->name<T>(names...);
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto type_name = this->name<T>();
      auto factory_it = factories_.find(type_name);
      if (factory_it == values_.end()) {
        throw std::runtime_error("Could not find factory for: " + type_name);
      }
      auto & factory = *std::static_pointer_cast<Factory_Function<T>>(factory_it->second);
      auto plugin = factory();
      bool inserted;
      std::tie(it, inserted) = values_.emplace(name, plugin);
    }
    return *std::static_pointer_cast<T>(it->second);
  }

private:
  template <typename T, typename ... Names>
  static std::string name(Names const & ... names) {
    auto name = std::string(typeid(T).name());
    std::array<std::string, sizeof...(Names)> a { names... };
    for (auto const & n : a) {
      name += "|" + n;
    }
    return name;
  }

private:
  std::map<std::string, std::shared_ptr<void>> factories_;
  std::map<std::string, std::shared_ptr<void>> values_;
};

}
