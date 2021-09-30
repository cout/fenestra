#pragma once

#include "Config.hpp"

#include <vector>

#include <GLFW/glfw3.h>

namespace fenestra {

class InputState {
public:
  auto const & pressed() const { return pressed_; }
  auto & pressed() { return pressed_; }

private:
  unsigned int pressed_[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
};

class Gamepad {
public:
  Gamepad(Config const & config, unsigned int joystick = 0)
    : config_(config)
    , joystick_(joystick)
  {
  }

  virtual std::string_view name() const { return "Gamepad"; }

  auto const & state() const { return state_; }

  void poll_input() {
    GLFWgamepadstate state;

    if (glfwGetGamepadState(joystick_, &state)) {
      for (auto const & binding : config_.button_bindings()) {
        auto pressed = state.buttons[binding.button];
        state_.pressed()[binding.retro_button] = pressed;
      }

      for (auto const & binding : config_.axis_bindings()) {
        auto pos = state.axes[binding.axis];
        auto thresh = binding.threshold;
        auto pressed = thresh > 0 ? pos >= thresh : pos <= thresh;
        state_.pressed()[binding.retro_button] = pressed;
      }
    }
  }

  bool pressed(unsigned int id) {
    return state_.pressed()[id];
  }

private:
  Config const & config_;
  unsigned int joystick_;
  InputState state_;
};

}
