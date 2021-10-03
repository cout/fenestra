#pragma once

#include "Clock.hpp"

#include "libretro.h"

#include <json/json.h>

#include <string_view>
#include <string>
#include <vector>
#include <fstream>

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
  Config() {
  }

  Config(std::string const & filename) {
    load(filename);
  }

  void load(std::string const & filename) {
    Json::Value v;
    std::ifstream file(filename);
    file >> v;
  }

  bool vsync() const { return vsync_; }
  bool adaptive_vsync() const { return adaptive_vsync_; }
  bool glfinish() const { return glfinish_; }
  bool oml_sync() const { return oml_sync_; }
  bool nv_delay_before_swap() const { return nv_delay_before_swap_; }
  float scale_factor() const { return scale_factor_; }

  Milliseconds frame_delay() const { return frame_delay_; }

  char const * system_directory() const { return system_directory_.c_str(); }
  char const * save_directory() const { return save_directory_.c_str(); }

  std::string_view audio_api() const { return audio_api_; }
  std::string_view audio_device() const { return audio_device_; }
  int audio_suggested_latency() const { return audio_suggested_latency_; }

  int network_command_port() const { return network_command_port_; }

  std::string v4l2_device() const { return v4l2_device_; }

  auto const & button_bindings() const { return button_bindings_; }
  auto const & axis_bindings() const { return axis_bindings_; }

private:
  bool vsync_ = true;
  bool adaptive_vsync_ = false;
  bool glfinish_ = false;
  bool oml_sync_ = false;
  bool nv_delay_before_swap_ = false;
  float scale_factor_ = 6.0f;

  Milliseconds frame_delay_ = Milliseconds(4);
  std::string system_directory_ = ".";
  std::string save_directory_ = ".";

  std::string audio_api_ = "ALSA";
  std::string_view audio_device_ = "pipewire";
  int audio_suggested_latency_ = 128;

  int network_command_port_ = 55355;

  std::string v4l2_device_ = "/dev/video3";

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
