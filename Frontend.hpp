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
    , video_refresh_key_(perf_.probe_key("Video refresh"))
    , audio_sample_key_(perf_.probe_key("Audio sample"))
  {
  }

  Probe const & probe() const { return probe_; }
  Probe & probe() { return probe_; }

  template<typename T>
  void add_plugin() {
    auto plugin = std::make_shared<T>(config_);

    std::string name(plugin->name());
    if (config_.plugins().find(name) == config_.plugins().end()) {
      return;
    }

    plugins_.emplace_back(perf_, plugin);

    if (!std::is_same_v<decltype(&T::video_refresh), decltype(&Plugin::video_refresh)>) {
      video_refresh_plugins_.emplace_back(perf_, plugin);
    }

    if (!std::is_same_v<decltype(&T::write_audio_sample), decltype(&Plugin::write_audio_sample)>) {
      audio_sample_plugins_.emplace_back(perf_, plugin);
    }
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
    probe_.mark(video_refresh_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
    if (data) {
      for (auto const & plugin : video_refresh_plugins_) {
        probe_.mark(plugin.probe_key(), Probe::START, 2, Clock::gettime(CLOCK_MONOTONIC));
        plugin->video_refresh(data, width, height, pitch);
        probe_.mark(plugin.probe_key(), Probe::END, 2, Clock::gettime(CLOCK_MONOTONIC));
      }
    }
    probe_.mark(video_refresh_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
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
    audio_sample_batch(buf, 1);
  }

  std::size_t audio_sample_batch(const std::int16_t * data, std::size_t frames) {
    probe_.mark(audio_sample_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
    for (auto const & plugin : audio_sample_plugins_) {
      probe_.mark(plugin.probe_key(), Probe::START, 2, Clock::gettime(CLOCK_MONOTONIC));
      plugin->write_audio_sample(data, frames);
      probe_.mark(plugin.probe_key(), Probe::END, 2, Clock::gettime(CLOCK_MONOTONIC));
    }
    probe_.mark(audio_sample_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    return frames;
  }

private:
  Config const & config_;
  Core & core_;
  Window window_;
  Gamepad gamepad_;
  Probe probe_;
  Perf & perf_;

  Probe::Key video_refresh_key_;
  Probe::Key audio_sample_key_;
  std::vector<PluginSlot> plugins_;
  std::vector<PluginSlot> video_refresh_plugins_;
  std::vector<PluginSlot> audio_sample_plugins_;
};

}
