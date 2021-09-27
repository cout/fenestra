#pragma once

#include <string>
#include <sstream>

#include <dlfcn.h>

namespace fenestra {

class DL {
public:
  DL(std::string const & filename, int flag)
    : handle_(dlopen(filename.c_str(), flag))
  {
    if (!handle_)
    {   
      std::stringstream strm;
      strm << "dlopen failed: " << dlerror();
      throw std::runtime_error(strm.str());
    }   
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
  P sym(std::string const & symbol) {
    void * p = dlsym(handle_, symbol.c_str());
    if (!p)
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
