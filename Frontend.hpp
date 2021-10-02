#pragma once

#include "libretro.h"

#include "Config.hpp"
#include "Window.hpp"
#include "Gamepad.hpp"
#include "Geometry.hpp"
#include "Plugin.hpp"
#include "Perf.hpp"
#include "Probe.hpp"
#include "Clock.hpp"

#include <string>
#include <vector>
#include <memory>

namespace fenestra {

class PluginSlot {
public:
  PluginSlot(Perf & perf, std::shared_ptr<Plugin> plugin)
    : plugin_(plugin)
    , probe_key_(perf.probe_key(std::string(plugin->name())))
  {
  }

  Plugin * operator->() { return plugin_.get(); }
  Plugin & operator*() { return *plugin_; }

  Plugin * operator->() const { return plugin_.get(); }
  Plugin & operator*() const { return *plugin_; }

  Probe::Key probe_key() const { return probe_key_; }

private:
  std::shared_ptr<Plugin> plugin_;
  Probe::Key probe_key_;
};

class Frontend {
public:
  Frontend(std::string const & title, Core & core, Config const & config, Perf & perf)
    : config_(config)
    , core_(core)
    , window_(title, core, config_)
    , gamepad_(config_)
    , perf_(perf)
  {
  }

  Probe const & probe() const { return probe_; }

  template<typename T>
  void add_plugin() {
    plugins_.emplace_back(perf_, std::make_shared<T>(config_));
  }

  auto & window() { return window_; }
  auto & gamepad() { return gamepad_; }

  void init(retro_system_av_info const & av) {
    Geometry geom(av.geometry, config_.scale_factor());
    window_.init(geom);

    for (auto const & plugin : plugins_) {
      plugin->set_geometry(geom);
    }

    for (auto const & plugin : plugins_) {
      plugin->set_sample_rate(av.timing.sample_rate);
    }
  }

  void log_libretro(enum retro_log_level level, char const * fmt, va_list ap) {
    for (auto const & plugin : plugins_) {
      plugin->log_libretro(level, fmt, ap);
    }
  }

  void game_loaded() {
    for (auto const & plugin : plugins_) {
      plugin->game_loaded(core_);
    }
  }

  void game_unloaded() {
    for (auto const & plugin : plugins_) {
      plugin->game_unloaded(core_);
    }
  }

  bool video_set_pixel_format(retro_pixel_format format) {
    for (auto const & plugin : plugins_) {
      plugin->set_pixel_format(format);
    }

    return true;
  }

  void pre_frame_delay() {
    for (auto const & plugin : plugins_) {
      plugin->pre_frame_delay();
    }
  }

  void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) {
    if (data) {
      for (auto const & plugin : plugins_) {
        probe_.mark(plugin.probe_key(), Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
        plugin->video_refresh(data, width, height, pitch);
        probe_.mark(plugin.probe_key(), Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
      }
    }
  }

  void video_render() {
    for (auto const & plugin : plugins_) {
      plugin->video_render();
    }
  }

  void poll_input() {
    gamepad().poll_input();
  }

  std::int16_t input_state(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
    if (port > 0 || index > 0 || device != RETRO_DEVICE_JOYPAD) {
      return 0;
    }

    return gamepad().pressed(id);
  }

  void audio_sample(std::int16_t left, std::int16_t right) {
    std::int16_t buf[2] = { left, right };
    for (auto const & plugin : plugins_) {
      plugin->write_audio_sample(buf, 1);
    }
  }

  std::size_t audio_sample_batch(const std::int16_t * data, std::size_t frames) {
    for (auto const & plugin : plugins_) {
      plugin->write_audio_sample(data, frames);
    }
    return frames;
  }

private:
  Config const & config_;
  Core & core_;
  Window window_;
  Gamepad gamepad_;
  Probe probe_;
  Perf & perf_;

  std::vector<PluginSlot> plugins_;
};

}
