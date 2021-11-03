#pragma once

#include <string>
#include <sstream>

#include <dlfcn.h>

namespace fenestra {

class DL {
public:
  DL(char const * filename, int flag)
    : handle_(dlopen(filename, flag))
  {
    if (!handle_)
    {   
      std::stringstream strm;
      strm << "dlopen failed: " << dlerror();
      throw std::runtime_error(strm.str());
    }   
  }

  DL(std::string const & filename, int flag)
    : DL(filename.c_str(), flag)
  {
  }

  DL(int flag)
    : DL(nullptr, flag)
  {
  }

  ~DL() {
    dlclose(handle_);
  }

  DL(DL const &) = delete;
  DL(DL &&)=delete;
  DL operator=(DL const &) = delete;
  DL operator=(DL &&) = delete;

  auto handle() { return handle_; }

  template<typename P = void*>
  P sym(std::string const & symbol, bool throw_exception_on_failure = true) {
    void * p = dlsym(handle_, symbol.c_str());
    if (!p && throw_exception_on_failure)
    {
      std::stringstream strm;
      strm << "dlsym failed: " << dlerror();
      throw std::runtime_error(strm.str());
    }

    return reinterpret_cast<P>(p);
  }

private:
  void * handle_;
};

}
