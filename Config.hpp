#pragma once

#include "Clock.hpp"

#include "libretro.h"

#include <json/json.h>

#include <string_view>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <functional>
#include <memory>

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

struct Abstract_Setting {
  virtual ~Abstract_Setting() { }
};

template <typename T>
struct Setting : Abstract_Setting {
  virtual ~Setting() { }
  Setting() { }
  Setting(T const & value) : value(value) { }
  T value;
};

class Config {
public:
  Config()
    : plugins_(fetch<std::set<std::string>>("plugins", std::set<std::string>({ "logger", "perf", "savefile", "portaudio", "gl", "v4l2stream", "gstreamer", "ssr", "netcmds", "rusage" })))
    , perflog_filename_(fetch<std::string>("perflog_filename", ""))
    , vsync_(fetch<bool>("vsync", true))
    , adaptive_vsync_(fetch<bool>("adaptive_vsync", true))
    , glfinish_(fetch<bool>("glfinish", false))
    , oml_sync_(fetch<bool>("oml_sync", false))
    , sgi_sync_(fetch<bool>("sgi_sync", false))
    , nv_delay_before_swap_(fetch<bool>("nv_delay_before_swap", false))
    , scale_factor_(fetch<float>("scale_factor", 6.0f))
    , frame_delay_(fetch<Milliseconds>("frame_deplay", Milliseconds(4)))
    , system_directory_(fetch<std::string>("system_directory", "."))
    , save_directory_(fetch<std::string>("save_directory", "."))
    , audio_api_(fetch<std::string>("audio_api", "ALSA"))
    , audio_device_(fetch<std::string>("audio_device", "pipewire"))
    , audio_suggested_latency_(fetch<int>("audio_suggested_latency", 64))
    , audio_maximum_latency_(fetch<int>("audio_maximum_latency", 64))
    , audio_nonblock_(fetch<bool>("audio_nonblock", true))
    , network_command_port_(fetch<int>("network_command_port", 55355))
    , v4l2_device_(fetch<std::string>("v4l2_device", ""))
    , gstreamer_sink_pipeline_(fetch<std::string>("gstreamer_sink_pipeline", ""))
    , ssr_channel_(fetch<std::string>("ssr_channel", ""))
  {
  }

  explicit Config(std::string const & filename)
    : Config()
  {
    load(filename);
  }

  void load(std::string const & filename) {
    Json::Value v;
    std::ifstream file(filename);
    file >> v;

    cfg_ = v;

    for (auto const & setter : setters_) {
      setter(cfg_);
    }
  }

  auto const & plugins() const { return plugins_; }

  std::string const & perflog_filename() const { return perflog_filename_; }

  bool const & vsync() const { return vsync_; }
  bool const & adaptive_vsync() const { return adaptive_vsync_; }
  bool const & glfinish() const { return glfinish_; }
  bool const & oml_sync() const { return oml_sync_; }
  bool const & sgi_sync() const { return sgi_sync_; }
  bool const & nv_delay_before_swap() const { return nv_delay_before_swap_; }
  float const & scale_factor() const { return scale_factor_; }

  Milliseconds const & frame_delay() const { return frame_delay_; }

  char const * system_directory() const { return system_directory_.c_str(); }
  char const * save_directory() const { return save_directory_.c_str(); }

  std::string_view audio_api() const { return audio_api_; }
  std::string_view audio_device() const { return audio_device_; }
  int const & audio_suggested_latency() const { return audio_suggested_latency_; }
  int const & audio_maximum_latency() const { return audio_maximum_latency_; }
  bool const & audio_nonblock() const { return audio_nonblock_; }

  int const & network_command_port() const { return network_command_port_; }

  std::string const & v4l2_device() const { return v4l2_device_; }

  std::string const & gstreamer_sink_pipeline() const { return gstreamer_sink_pipeline_; }

  std::string const & ssr_channel() const { return ssr_channel_; }

  auto const & button_bindings() const { return button_bindings_; }
  auto const & axis_bindings() const { return axis_bindings_; }

  template <typename T, typename Default>
  T & fetch(std::string const & name, Default dflt) {
    auto it = values_.find(name);
    if (it == values_.end()) {
      auto [ inserted_it, inserted ] = values_.emplace(name, std::make_shared<Setting<T>>(dflt));
      auto setting = std::static_pointer_cast<Setting<T>>(inserted_it->second);
      setters_.emplace_back([name, setting](Json::Value const & cfg) {
        auto cfg_value = cfg.find(name.data(), name.data() + name.size());
        if (cfg_value) {
          assign(setting->value, *cfg_value);
        }
      });
      setters_.back()(cfg_);
      return setting->value;
    } else {
      return std::dynamic_pointer_cast<Setting<T>>(it->second)->value;
    }
  }

private:
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
  Json::Value cfg_;
  std::map<std::string, std::shared_ptr<Abstract_Setting>> values_;
  std::vector<std::function<void(Json::Value const &)>> setters_;

  std::set<std::string> & plugins_;

  std::string & perflog_filename_;

  bool & vsync_;
  bool & adaptive_vsync_;
  bool & glfinish_;
  bool & oml_sync_;
  bool & sgi_sync_;
  bool & nv_delay_before_swap_;
  float & scale_factor_;

  Milliseconds & frame_delay_;
  std::string & system_directory_;
  std::string & save_directory_;

  std::string & audio_api_;
  std::string & audio_device_;
  int & audio_suggested_latency_;
  int & audio_maximum_latency_;
  bool & audio_nonblock_;

  int & network_command_port_;

  std::string & v4l2_device_;

  // E.g.: v4l2sink device=/dev/video3
  std::string & gstreamer_sink_pipeline_;

  std::string & ssr_channel_;

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
