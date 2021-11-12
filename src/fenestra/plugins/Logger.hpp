#pragma once

#include "Plugin.hpp"

#include <iostream>
#include <cstdarg>

namespace fenestra {

class Logger
  : public Plugin
{
public:
  static inline const char * levelstr[] = { "DEBUG", "INFO", "WARN", "ERROR" };

  Logger(Config const & config) {
  }

  virtual void log_libretro(retro_log_level level, char const * fmt, va_list ap) override {
    char buffer[4096] = {0};

    std::vsnprintf(buffer, sizeof(buffer), fmt, ap);

    std::cerr << "[" << levelstr[level] << " libretro] " << buffer;
  }
};

}
