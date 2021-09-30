#pragma once

#include "libretro.h"

#include "Config.hpp"
#include "Window.hpp"
#include "Gamepad.hpp"
#include "Video.hpp"
#include "Capture.hpp"
#include "Audio.hpp"
#include "Logger.hpp"
#include "Geometry.hpp"
#include "Netcmds.hpp"

#include <string>

namespace fenestra {

class Frontend {
public:
  Frontend(std::string const & title, Core & core, Config const & config)
    : config_(config)
    , window_(title, core, config_)
    , gamepad_()
    , video_()
    , capture_()
    , audio_(config)
    , logger_()
  {
    if (config.v4l2_device() != "") {
      capture_.open(config.v4l2_device());
    }
  }

  auto & window() { return window_; }
  auto & gamepad() { return gamepad_; }
  auto & video() { return video_; }
  auto & capture() { return capture_; }
  auto & audio() { return audio_; }
  auto & logger() { return logger_; }

  void init(retro_system_av_info const & av) {
    Geometry geom(av.geometry, scale_);
    window_.init(geom);
    video_.configure(geom);
    capture_.configure(geom);
    audio_.init(av.timing.sample_rate);
  }

  bool video_set_pixel_format(retro_pixel_format format) {
    return video().set_pixel_format(format) && capture().set_pixel_format(format);
  }

  void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) {
    if (data) {
      video().refresh(data, width, height, pitch);
      capture().refresh(data, width, height, pitch);
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
    audio().write(buf, 1);
  }

  std::size_t audio_sample_batch(const std::int16_t * data, std::size_t frames) {
    return audio().write(data, frames);
  }

private:
  Config const & config_;
  Window window_;
  Gamepad gamepad_;
  Video video_;
  Capture capture_;
  Audio audio_;
  Logger logger_;

  // Config
  float scale_ = 6;
};

}
