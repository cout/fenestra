#pragma once

#include <map>
#include <string>
#include <typeinfo>
#include <memory>
#include <functional>
#include <vector>

namespace fenestra {

class Registry {
private:
  class Name {
  public:
    template <typename ... Names>
    Name(std::string type_name, Names const & ... args)
      : type_name_(type_name)
      , args_({ args... })
    {
      char const * sep = "";
      for (auto const & n : args_) {
        stringified_args_ += sep;
        stringified_args_ += n;
        sep = ".";
      }
    }

    std::string const & args() const { return stringified_args_; }

    bool operator<(Name const & rhs) const {
      if (type_name_ == rhs.type_name_) return args_ < rhs.args_;
      return type_name_ < rhs.type_name_;
    }

  private:
    std::string type_name_;
    std::vector<std::string> args_;
    std::string stringified_args_;
  };

  using Factory_Function = std::function<std::shared_ptr<void>(Name const &)>;

public:
  Registry()
  {
  }

  template <typename T, typename Factory>
  void register_factory(Factory factory) {
    std::string type_name = typeid(T).name();
    auto [ it, inserted ] = factories_.emplace(
        type_name,
        [=](Name const & name) -> std::shared_ptr<void> { return factory(name.args()); });
    if (!inserted) {
      throw std::runtime_error("Factory already exists: " + type_name);
    }
  }

  template <typename T, typename ... Names>
  T & fetch(Names const & ... names) {
    Name name(typeid(T).name(), names...);
    auto it = values_.find(name);
    if (it == values_.end()) {
      std::string type_name = typeid(T).name();
      auto factory_it = factories_.find(type_name);
      if (factory_it == factories_.end()) {
        throw std::runtime_error("Could not find factory for: " + type_name);
      }
      auto & factory = factory_it->second;
      auto plugin = factory(name);
      bool inserted;
      std::tie(it, inserted) = values_.emplace(name, plugin);
    }
    return *std::static_pointer_cast<T>(it->second);
  }

private:
  std::map<std::string, Factory_Function> factories_;
  std::map<Name, std::shared_ptr<void>> values_;
};

}
