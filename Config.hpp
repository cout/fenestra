#pragma once

#include "Clock.hpp"

#include "libretro.h"

#include <json/json.h>

#include <string_view>
#include <string>
#include <vector>
#include <set>
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

    set(plugins_, v, "plugins");

    set(perflog_filename_, v, "perflog_filename");

    set(vsync_, v, "vsync");
    set(adaptive_vsync_, v, "adaptive_vsync");
    set(glfinish_, v, "glfinish");
    set(oml_sync_, v, "oml_sync");
    set(sgi_sync_, v, "sgi_sync");
    set(nv_delay_before_swap_, v, "nv_delay_before_swap");
    set(scale_factor_, v, "scale_factor");

    set(frame_delay_, v, "frame_delay");
    set(system_directory_, v, "system_directory");
    set(save_directory_, v, "save_directory");

    set(audio_api_, v, "audio_api");
    set(audio_device_, v, "audio_device");
    set(audio_suggested_latency_, v, "audio_suggested_latency");
    set(audio_maximum_latency_, v, "audio_maximum_latency");
    set(audio_nonblock_, v, "audio_nonblock");

    set(network_command_port_, v, "network_command_port");

    set(v4l2_device_, v, "v4l2_device");
    set(gstreamer_sink_pipeline_, v, "gstreamer_sink_pipeline");
    set(ssr_channel_, v, "ssr_channel");
  }

  auto const & plugins() const { return plugins_; }

  std::string const & perflog_filename() const { return perflog_filename_; }

  bool vsync() const { return vsync_; }
  bool adaptive_vsync() const { return adaptive_vsync_; }
  bool glfinish() const { return glfinish_; }
  bool oml_sync() const { return oml_sync_; }
  bool sgi_sync() const { return sgi_sync_; }
  bool nv_delay_before_swap() const { return nv_delay_before_swap_; }
  float scale_factor() const { return scale_factor_; }

  Milliseconds frame_delay() const { return frame_delay_; }

  char const * system_directory() const { return system_directory_.c_str(); }
  char const * save_directory() const { return save_directory_.c_str(); }

  std::string_view audio_api() const { return audio_api_; }
  std::string_view audio_device() const { return audio_device_; }
  int audio_suggested_latency() const { return audio_suggested_latency_; }
  int audio_maximum_latency() const { return audio_maximum_latency_; }
  bool audio_nonblock() const { return audio_nonblock_; }

  int network_command_port() const { return network_command_port_; }

  std::string const & v4l2_device() const { return v4l2_device_; }

  std::string const & gstreamer_sink_pipeline() const { return gstreamer_sink_pipeline_; }

  std::string const & ssr_channel() const { return ssr_channel_; }

  auto const & button_bindings() const { return button_bindings_; }
  auto const & axis_bindings() const { return axis_bindings_; }

private:
  template <typename T>
  void set(T & dest, Json::Value const & v, std::string_view name) {
    auto src = v.find(name.data(), name.data() + name.size());
    if (src) {
      assign(dest, *src);
    }
  }

  template <typename T>
  static void assign(std::vector<T> & dest, Json::Value const & src) {
    std::vector<T> result;
    for (auto v : src) {
      T tmp;
      assign(tmp, v);
      result.push_back(tmp);
    }
    dest = result;
  }

  template <typename T>
  static void assign(std::set<T> & dest, Json::Value const & src) {
    std::set<T> result;
    for (auto v : src) {
      T tmp;
      assign(tmp, v);
      result.insert(tmp);
    }
    dest = result;
  }

  static void assign(bool & dest, Json::Value const & src) {
    dest = src.asBool();
  }

  static void assign(int & dest, Json::Value const & src) {
    dest = src.asInt();
  }

  static void assign(float & dest, Json::Value const & src) {
    dest = src.asFloat();
  }

  static void assign(double & dest, Json::Value const & src) {
    dest = src.asDouble();
  }

  static void assign(std::string & dest, Json::Value const & src) {
    dest = src.asString();
  }

  static void assign(Milliseconds & dest, Json::Value const & src) {
    dest = Milliseconds(src.asDouble());
  }

private:
  std::set<std::string> plugins_ { "logger", "perf", "savefile", "portaudio", "gl", "v4l2stream", "gstreamer", "ssr", "netcmds", "rusage" };

  std::string perflog_filename_ = "";

  bool vsync_ = true;
  bool adaptive_vsync_ = true;
  bool glfinish_ = false;
  bool oml_sync_ = false;
  bool sgi_sync_ = false;
  bool nv_delay_before_swap_ = false;
  float scale_factor_ = 6.0f;

  Milliseconds frame_delay_ = Milliseconds(4);
  std::string system_directory_ = ".";
  std::string save_directory_ = ".";

  std::string audio_api_ = "ALSA";
  std::string audio_device_ = "pipewire";
  int audio_suggested_latency_ = 64;
  int audio_maximum_latency_ = 64;
  bool audio_nonblock_ = true;

  int network_command_port_ = 55355;

  std::string v4l2_device_ = "";

  // E.g.: v4l2sink device=/dev/video3
  std::string gstreamer_sink_pipeline_ = "";

  std::string ssr_channel_ = "";

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
