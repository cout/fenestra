#pragma once

#include "Plugin.hpp"

#include <vector>

#include <GLFW/glfw3.h>

namespace fenestra {

class GLFWGamepad
  : public Plugin
{
public:
  GLFWGamepad(Config const & config)
    : config_(config)
    , joystick_(config.fetch<unsigned int>("glfw-gamepad.joystick", 0))
    , port_(config.fetch<unsigned int>("glfw-gamepad.port", 0))
  {
  }

  virtual void poll_input(State & state) override {
    if (state.input_state.size() <= port_) {
      state.input_state.resize(port_ + 1);
    }

    GLFWgamepadstate gamepad_state;

    if (glfwGetGamepadState(joystick_, &gamepad_state)) {
      for (auto const & binding : config_.button_bindings()) {
        auto pressed = gamepad_state.buttons[binding.button];
        state.input_state[port_].pressed[binding.retro_button] = pressed;
      }

      for (auto const & binding : config_.axis_bindings()) {
        auto pos = gamepad_state.axes[binding.axis];
        auto thresh = binding.threshold;
        auto pressed = thresh > 0 ? pos >= thresh : pos <= thresh;
        state.input_state[port_].pressed[binding.retro_button] = pressed;
      }
    }
  }

private:
  Config const & config_;
  unsigned int & joystick_;
  unsigned int & port_;
};

}
