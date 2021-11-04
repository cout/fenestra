#pragma once

#include "Plugin.hpp"

#include <unordered_map>

#include <linux/input.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

namespace fenestra {

class EvdevGamepad
  : public Plugin
{
public:
  EvdevGamepad(Config const & config)
    : device_(config.fetch<std::string>("evdev-gamepad.device", ""))
    , port_(config.fetch<unsigned int>("evdev-gamepad.port", 0))
  {
    if (device_ != "") {
      open(device_);
    }
  }

  virtual void pre_frame_delay(State const & state) override {
    if (fd_ < 0) {
      if (device_ != "") {
        open(device_);
      }
    }
  }

  virtual void poll_input(State & state) override {
    if (fd_ < 0) return;

    if (state.input_state.size() <= port_) {
      state.input_state.resize(port_ + 1);
    }

    input_event ev;

    for (;;) {
      auto n = ::read(fd_, &ev, sizeof(ev));

      // TODO: set fd_ to 0 if device is disconnected

      if (n < 0) break;

      switch (ev.type) {
        case EV_KEY: {
          auto it = button_bindings_.find(ev.code);
          if (it != button_bindings_.end()) {
            auto button = it->second.retro_button;
            auto pressed = ev.value;
            state.input_state[port_].pressed[button] = pressed;
          }
          break;
        }

        case EV_ABS: {
          auto range = axis_bindings_.equal_range(ev.code);
          for (auto it = range.first; it != range.second; ++it) {
            auto button = it->second.retro_button;
            auto pos = ev.value;
            auto thresh = it->second.threshold;
            auto pressed = thresh > 0 ? pos >= thresh : pos <= thresh;
            state.input_state[port_].pressed[button] = pressed;
          }
          break;
        }
      }
    }
  }

private:
  void open(std::string const & path) {
    fd_ = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd_ < 0) {
      throw std::runtime_error("open failed");
    }

    char name[128];
    ioctl(fd_, EVIOCGNAME(sizeof(name)), name);

    // TODO: Verify # of buttons/axes
  }

  static char const * type_str(int type) {
    switch (type) {
      case EV_SYN: return "EV_SYN";
      case EV_KEY: return "EV_KEY";
      case EV_REL: return "EV_REL";
      case EV_ABS: return "EV_ABS";
      case EV_MSC: return "EV_MSC";
      case EV_SW: return "EV_SW";
      case EV_LED: return "EV_LED";
      case EV_SND: return "EV_SND";
      case EV_REP: return "EV_REP";
      case EV_FF: return "EV_FF";
      case EV_PWR: return "EV_PWR";
      case EV_FF_STATUS: return "EV_FF_STATUS";
      case EV_MAX: return "EV_MAX";
      case EV_CNT: return "EV_CNT";
      default: return "(unknown)";
    }
  }

private:
  struct ButtonBinding {
    unsigned int retro_button;
  };

  // Button bindings for 8bitdo SN30+
  static inline std::unordered_map<std::uint16_t, ButtonBinding> button_bindings_ {
    { BTN_A, { RETRO_DEVICE_ID_JOYPAD_B } },
    { BTN_B, { RETRO_DEVICE_ID_JOYPAD_A } },
    { BTN_Y, { RETRO_DEVICE_ID_JOYPAD_Y } },
    { BTN_X, { RETRO_DEVICE_ID_JOYPAD_X } },
    { BTN_TL, { RETRO_DEVICE_ID_JOYPAD_L } },
    { BTN_TR, { RETRO_DEVICE_ID_JOYPAD_R } },
    { BTN_SELECT, { RETRO_DEVICE_ID_JOYPAD_SELECT } },
    { BTN_START, { RETRO_DEVICE_ID_JOYPAD_START } },
  };

  struct AxisBinding {
    std::int32_t threshold;
    unsigned int retro_button;
  };

  // Axis bindings for 8bitdo SN30+
  static inline std::unordered_multimap<std::uint16_t, AxisBinding> axis_bindings_ {
    { ABS_HAT0X, { -1, RETRO_DEVICE_ID_JOYPAD_LEFT } },
    { ABS_HAT0X, { +1, RETRO_DEVICE_ID_JOYPAD_RIGHT } },
    { ABS_HAT0Y, { -1, RETRO_DEVICE_ID_JOYPAD_UP } },
    { ABS_HAT0Y, { +1, RETRO_DEVICE_ID_JOYPAD_DOWN } },
    { ABS_Z, { +1, RETRO_DEVICE_ID_JOYPAD_L2 } },
    { ABS_RZ, { +1, RETRO_DEVICE_ID_JOYPAD_R2 } },
  };

private:
  std::string const & device_;
  unsigned int const & port_;

  int fd_ = -1;
};

}
