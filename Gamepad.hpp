#pragma once

#include <vector>

#include <GLFW/glfw3.h>

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

// Button bindings for 8bitdo SN30+
inline std::vector<Button_Binding> button_bindings = {
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
inline std::vector<Axis_Binding> axis_bindings = {
  { 4, +1, RETRO_DEVICE_ID_JOYPAD_L2 },
  { 5, +1, RETRO_DEVICE_ID_JOYPAD_R2 },
};

class InputState {
public:
  auto const & pressed() const { return pressed_; }
  auto & pressed() { return pressed_; }

private:
  unsigned int pressed_[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
};

class Gamepad {
public:
  Gamepad(unsigned int joystick = 0)
    : joystick_(joystick)
  {
  }

  auto const & state() const { return state_; }

  void poll_input() {
    GLFWgamepadstate state;

    if (glfwGetGamepadState(joystick_, &state)) {
      for (auto const & binding : button_bindings) {
        auto pressed = state.buttons[binding.button];
        state_.pressed()[binding.retro_button] = pressed;
      }

      for (auto const & binding : axis_bindings) {
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
unsigned int joystick_;
  InputState state_;
};

}
