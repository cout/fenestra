#pragma once

#include "Config.hpp"

#include <vector>

#include <GLFW/glfw3.h>

namespace fenestra {

class Gamepad {
public:
  Gamepad(Config const & config, unsigned int joystick = 0)
    : config_(config)
    , joystick_(joystick)
  {
  }

  virtual std::string_view name() const { return "Gamepad"; }

  void poll_input(State & state) {
    GLFWgamepadstate gamepad_state;

    if (glfwGetGamepadState(joystick_, &gamepad_state)) {
      for (auto const & binding : config_.button_bindings()) {
        auto pressed = gamepad_state.buttons[binding.button];
        state.input_state.pressed[binding.retro_button] = pressed;
      }

      for (auto const & binding : config_.axis_bindings()) {
        auto pos = gamepad_state.axes[binding.axis];
        auto thresh = binding.threshold;
        auto pressed = thresh > 0 ? pos >= thresh : pos <= thresh;
        state.input_state.pressed[binding.retro_button] = pressed;
      }
    }
  }

private:
  Config const & config_;
  unsigned int joystick_;
};

}
