#pragma once

#include "../libretro.h"

#include "../Probe.hpp"
#include "../Geometry.hpp"
#include "../Core.hpp"
#include "../Config.hpp"
#include "../State.hpp"

#include <cstddef>
#include <cstdarg>
#include <string_view>

namespace fenestra {

class Plugin {
public:
  virtual ~Plugin() { }

  virtual void start_metrics(Probe & probe, Probe::Dictionary & dictionary) { }
  virtual void collect_metrics(Probe & probe, Probe::Dictionary & dictionary) { }
  virtual void record_probe(Probe const & probe, Probe::Dictionary const & dictionary) { }

  virtual void set_sample_rate(double sample_rate, double adjusted_rate) { }
  virtual void write_audio_sample(void const * buf, std::size_t frames) { }

  virtual void set_pixel_format(retro_pixel_format format) { }
  virtual void set_geometry(Geometry const & geom) { }
  virtual void video_refresh(void const * data, unsigned int width, unsigned int height, std::size_t pitch) { }
  virtual void video_render() { }
  virtual void video_rendered() { }

  virtual void window_created() { }

  virtual void window_update() { }
  virtual void window_updated() { }

  virtual void window_sync(State & state) { }
  virtual void window_synced(State const & state) { }

  virtual void pre_frame_delay() { }
  virtual void frame_delay() { }

  virtual void log_libretro(retro_log_level level, char const * fmt, va_list ap) { }

  virtual void game_loaded(Core const & core, std::string const & filename) { }
  virtual void unloading_game(Core const & core) { }
  virtual void game_unloaded(Core const & core) { }
};

}
