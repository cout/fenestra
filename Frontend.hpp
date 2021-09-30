#pragma once

#include "libretro.h"

#include "Config.hpp"
#include "Window.hpp"
#include "Gamepad.hpp"
#include "Logger.hpp"
#include "Geometry.hpp"
#include "Netcmds.hpp"
#include "Plugin.hpp"

#include <string>
#include <vector>
#include <memory>

namespace fenestra {

class Frontend {
public:
  Frontend(std::string const & title, Core & core, Config const & config)
    : config_(config)
    , window_(title, core, config_)
    , gamepad_()
    , logger_()
  {
  }

  template<typename T>
  void add_plugin() {
    plugins_.emplace_back(std::make_shared<T>(config_));
  }

  auto & window() { return window_; }
  auto & gamepad() { return gamepad_; }
  auto & logger() { return logger_; }

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
        plugin->video_refresh(data, width, height, pitch);
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
  Window window_;
  Gamepad gamepad_;
  Logger logger_;

  std::vector<std::shared_ptr<Plugin>> plugins_;
};

}
