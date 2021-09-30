#pragma once

#include "Clock.hpp"

#include "libretro.h"

#include <string_view>
#include <string>
#include <vector>

namespace fenestra {

struct Button_Binding {
  unsigned int button;
  unsigned int retro_button;
};

struct Axis_Binding {
  unsigned int axis;
  float threshold;
  unsigned int retro_button;
};

class Config {
public:
  bool vsync() const { return true; }
  bool adaptive_vsync() const { return false; }
  bool glfinish() const { return false; }
  bool oml_sync() const { return false; }
  bool nv_delay_before_swap() const { return false; }
  float scale_factor() const { return 6.0; }

  Milliseconds frame_delay() const { return Milliseconds(8); }

  char const * system_directory() const { return "."; }
  char const * save_directory() const { return "."; }

  std::string_view audio_api() const { return "ALSA"; }
  std::string_view audio_device() const { return "pipewire"; }
  int audio_suggested_latency() const { return 128; }

  int network_command_port() const { return 55355; }

  std::string v4l2_device() const { return "/dev/video3"; }

  auto const & button_bindings() const { return button_bindings_; }
  auto const & axis_bindings() const { return axis_bindings_; }

private:
  // Button bindings for 8bitdo SN30+
  std::vector<Button_Binding> button_bindings_ = {
    { 0, RETRO_DEVICE_ID_JOYPAD_B },
    { 1, RETRO_DEVICE_ID_JOYPAD_A },
    { 2, RETRO_DEVICE_ID_JOYPAD_Y },
    { 3, RETRO_DEVICE_ID_JOYPAD_X },
    { 4, RETRO_DEVICE_ID_JOYPAD_L },
    { 5, RETRO_DEVICE_ID_JOYPAD_R },
    { 6, RETRO_DEVICE_ID_JOYPAD_SELECT },
    { 7, RETRO_DEVICE_ID_JOYPAD_START },
    { 11, RETRO_DEVICE_ID_JOYPAD_UP },
    { 12, RETRO_DEVICE_ID_JOYPAD_RIGHT },
    { 13, RETRO_DEVICE_ID_JOYPAD_DOWN },
    { 14, RETRO_DEVICE_ID_JOYPAD_LEFT },
  };

  // Axis bindings for 8bitdo SN30+
  std::vector<Axis_Binding> axis_bindings_ = {
    { 4, +1, RETRO_DEVICE_ID_JOYPAD_L2 },
    { 5, +1, RETRO_DEVICE_ID_JOYPAD_R2 },
};

};

}
