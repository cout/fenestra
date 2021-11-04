#pragma once

#include "Plugin.hpp"

#include <GLFW/glfw3.h>

namespace fenestra {

class GLFWGamepad
  : public Plugin
{
public:
  GLFWGamepad(Config const & config)
    : joystick_(config.fetch<unsigned int>("glfw-gamepad.joystick", 0))
    , port_(config.fetch<unsigned int>("glfw-gamepad.port", 0))
    , enabled_(config.fetch<bool>("glfw-gamepad.enabled", true))
  {
  }

  virtual void poll_input(State & state) override {
    if (state.input_state.size() <= port_) {
      state.input_state.resize(port_ + 1);
    }

    GLFWgamepadstate gamepad_state;

    if (glfwGetGamepadState(joystick_, &gamepad_state)) {
      for (auto const & binding : button_bindings_) {
        auto pressed = gamepad_state.buttons[binding.button];
        state.input_state[port_].pressed[binding.retro_button] = pressed;
      }

      for (auto const & binding : axis_bindings_) {
        auto pos = gamepad_state.axes[binding.axis];
        auto thresh = binding.threshold;
        auto pressed = thresh > 0 ? pos >= thresh : pos <= thresh;
        state.input_state[port_].pressed[binding.retro_button] = pressed;
      }
    }
  }

private:
  struct ButtonBinding {
    unsigned int button;
    unsigned int retro_button;
  };

  // Button bindings for 8bitdo SN30+
  static constexpr inline std::array button_bindings_ = {
    ButtonBinding { GLFW_GAMEPAD_BUTTON_B, RETRO_DEVICE_ID_JOYPAD_B },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_A, RETRO_DEVICE_ID_JOYPAD_A },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_Y, RETRO_DEVICE_ID_JOYPAD_Y },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_X, RETRO_DEVICE_ID_JOYPAD_X },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, RETRO_DEVICE_ID_JOYPAD_L },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, RETRO_DEVICE_ID_JOYPAD_R },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_GUIDE, RETRO_DEVICE_ID_JOYPAD_SELECT },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_START, RETRO_DEVICE_ID_JOYPAD_START },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_DPAD_UP, RETRO_DEVICE_ID_JOYPAD_UP },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_DPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN },
    ButtonBinding { GLFW_GAMEPAD_BUTTON_DPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT },
  };

  struct AxisBinding {
    unsigned int axis;
    float threshold;
    unsigned int retro_button;
  };

  // Axis bindings for 8bitdo SN30+
  static constexpr inline std::array axis_bindings_ = {
    AxisBinding { GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, +1, RETRO_DEVICE_ID_JOYPAD_L2 },
    AxisBinding { GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, +1, RETRO_DEVICE_ID_JOYPAD_R2 },
  };

  unsigned int const & joystick_;
  unsigned int const & port_;
  bool const & enabled_;
};

}
