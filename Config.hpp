#pragma once

#include "Clock.hpp"

#include <string_view>

namespace fenestra {

class Config {
public:
  bool vsync() const { return true; }
  bool adaptive_vsync() const { return false; }
  bool glfinish() const { return false; }
  bool oml_sync() const { return false; }
  bool nv_delay_before_swap() const { return false; }

  Milliseconds frame_delay() const { return Milliseconds(8); }

  char const * system_directory() const { return "."; }
  char const * save_directory() const { return "."; }

  std::string_view audio_api() const { return "ALSA"; }
  std::string_view audio_device() const { return "pipewire"; }
  int audio_suggested_latency() const { return 128; }

  int network_command_port() const { return 55355; }
};

}
