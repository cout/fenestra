#pragma once

#include "libretro.h"

#include <iostream>
#include <cstdarg>

namespace fenestra {

class Logger {
public:
  static inline const char * levelstr[] = { "DEBUG", "INFO", "WARN", "ERROR" };

  void log_libretro(enum retro_log_level level, char const * fmt, va_list ap) {
    char buffer[4096] = {0};

    std::vsnprintf(buffer, sizeof(buffer), fmt, ap);

    std::cerr << "[" << levelstr[level] << " libretro] " << buffer;
  }
};

}
