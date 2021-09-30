#pragma once

#include "Geometry.hpp"

#include "libretro.h"

#include <cstddef>

namespace fenestra {

class Plugin {
public:
  virtual ~Plugin() { }

  virtual void set_sample_rate(int sample_rate) { }
  virtual void write_audio_sample(void const * buf, std::size_t frames) { }

  virtual void set_pixel_format(retro_pixel_format format) { }
  virtual void set_geometry(Geometry const & geom) { }
  virtual void video_refresh(void const * data, unsigned int width, unsigned int height, std::size_t pitch) { }
  virtual void video_render() { }

  virtual void pre_frame_delay() { }

  virtual void log_libretro(retro_log_level level, char const * fmt, va_list ap) { }
};

}
