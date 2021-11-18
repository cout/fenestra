#pragma once

#include "libretro.h"

#include "Config.hpp"
#include "Window.hpp"
#include "Geometry.hpp"
#include "Plugin.hpp"
#include "Probe.hpp"
#include "Clock.hpp"

#include <string>
#include <vector>
#include <memory>

namespace fenestra {

class PluginSlot {
public:
  PluginSlot(Probe::Dictionary & probe_dict, Plugin & plugin, std::string const & name)
    : plugin_(plugin)
    , probe_key_(probe_dict[name])
    , name_(name)
  {
  }

  Plugin * operator->() { return &plugin_; }
  Plugin & operator*() { return plugin_; }

  Plugin * operator->() const { return &plugin_; }
  Plugin & operator*() const { return plugin_; }

  Probe::Key probe_key() const { return probe_key_; }

  auto const & name() const { return name_; }

private:
  Plugin & plugin_;
  Probe::Key probe_key_;
  std::string name_;
};

class Frontend {
public:
  Frontend(std::string const & title, Core & core, Config const & config, std::map<std::string, bool> const & plugins)
    : core_(core)
    , config_(config)
    , configured_plugins_(plugins)
    , window_(title, core, config_)
    , probe_dict_()
  {
  }

  Probe const & probe() const { return probe_; }
  Probe & probe() { return probe_; }

  auto & probe_dict() { return probe_dict_; }

  template<typename T>
  void add_plugin(std::string const & name) {
    auto it = configured_plugins_.find(name);
    if (it == configured_plugins_.end() || !it->second) {
      return;
    }

    auto & plugin = *plugins_.emplace_back(new T(config_));

    if (!std::is_same_v<decltype(&T::video_refresh), decltype(&Plugin::video_refresh)>) {
      video_refresh_plugins_.emplace_back(probe_dict_, plugin, "Video: " + name);
    }

    if (!std::is_same_v<decltype(&T::write_audio_sample), decltype(&Plugin::write_audio_sample)>) {
      audio_sample_plugins_.emplace_back(probe_dict_, plugin, "Audio: " + name);
    }

    if (!std::is_same_v<decltype(&T::poll_input), decltype(&Plugin::poll_input)>) {
      poll_input_plugins_.emplace_back(probe_dict_, plugin, "Input: " + name);
    }
  }

  bool paused() const { return state_.paused; }
  bool done() const { return state_.done; }

  void init(retro_system_av_info const & av) {
    Geometry geom(av.geometry, config_.scale_factor());
    window_.init(geom);

    for (auto const & plugin : plugins_) {
      plugin->window_created();
    }

    for (auto const & plugin : plugins_) {
      plugin->set_geometry(geom);
    }

    for (auto const & plugin : plugins_) {
      auto sample_rate = av.timing.sample_rate;
      auto refresh_rate = 60.0; // TODO: use better initial estimate of refresh rate
      auto adjusted_rate = sample_rate / (av.timing.fps / refresh_rate);
      plugin->set_sample_rate(av.timing.sample_rate, adjusted_rate);
    }
  }

  void start_metrics(Probe & probe) {
    for (auto const & plugin : plugins_) {
      plugin->start_metrics(probe, probe_dict_);
    }
  }

  void collect_metrics(Probe & probe) {
    for (auto const & plugin : plugins_) {
      plugin->collect_metrics(probe, probe_dict_);
    }
  }

  void record_probe(Probe const & probe) {
    for (auto const & plugin : plugins_) {
      plugin->record_probe(probe, probe_dict_);
    }
  }

  void log_libretro(enum retro_log_level level, char const * fmt, va_list ap) {
    for (auto const & plugin : plugins_) {
      plugin->log_libretro(level, fmt, ap);
    }
  }

  void game_loaded(std::string const & filename) {
    for (auto const & plugin : plugins_) {
      plugin->game_loaded(core_, filename);
    }
  }

  void unloading_game() {
    for (auto const & plugin : plugins_) {
      plugin->unloading_game(core_);
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
      plugin->pre_frame_delay(state_);
    }
  }

  void poll_window_events() {
    window_.poll_events(state_);
  }

  void frame_delay() {
    for (auto const & plugin : plugins_) {
      plugin->frame_delay();
    }
  }

  void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) {
    if (data) {
      for (auto const & plugin : video_refresh_plugins_) {
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

    for (auto const & plugin : plugins_) {
      plugin->video_rendered();
    }
  }

  void window_update_delay() {
    for (auto const & plugin : plugins_) {
      plugin->window_update_delay();
    }
  }

  void window_update() {
    for (auto const & plugin : plugins_) {
      plugin->window_update();
    }

    for (auto const & plugin : plugins_) {
      plugin->window_updated();
    }
  }

  void window_sync() {
    for (auto const & plugin : plugins_) {
      plugin->window_sync(state_);
    }

    for (auto const & plugin : plugins_) {
      plugin->window_synced(state_);
    }
  }

  void poll_input() {
    for (auto const & plugin : poll_input_plugins_) {
      plugin->poll_input(state_);
    }
  }

  std::int16_t input_state(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
    if (port >= state_.input_state.size() || index > 0 || device != RETRO_DEVICE_JOYPAD) {
      return 0;
    }

    return state_.input_state[port].pressed[id];
  }

  void audio_sample(std::int16_t left, std::int16_t right) {
    std::int16_t buf[2] = { left, right };
    audio_sample_batch(buf, 1);
  }

  std::size_t audio_sample_batch(const std::int16_t * data, std::size_t frames) {
    for (auto const & plugin : audio_sample_plugins_) {
      probe_.mark(plugin.probe_key(), Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      plugin->write_audio_sample(data, frames);
      probe_.mark(plugin.probe_key(), Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    }
    return frames;
  }

private:
  Core & core_;
  Config const & config_;
  std::map<std::string, bool> const & configured_plugins_;

  State state_;

  Window window_;
  Probe probe_;

  Probe::Dictionary probe_dict_;
  std::vector<std::unique_ptr<Plugin>> plugins_;
  std::vector<PluginSlot> video_refresh_plugins_;
  std::vector<PluginSlot> audio_sample_plugins_;
  std::vector<PluginSlot> poll_input_plugins_;
};

}
