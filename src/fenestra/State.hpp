#pragma once

#include <cstdint>

namespace fenestra {

struct InputState {
  std::int16_t pressed[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
};

enum class KeyAction { UNKNOWN, PRESS, RELEASE, REPEAT };

using Key = char32_t;

namespace key {

constexpr Key NUL = '\0';
constexpr Key ESC = '\033';
constexpr Key ENTER = '\n';
constexpr Key TAB = '\t';
constexpr Key BACKSPACE = '\b';
constexpr Key LEFT = U'\u2190';
constexpr Key UP = U'\u2191';
constexpr Key RIGHT = U'\u2192';
constexpr Key DOWN = U'\u2193';

}

struct KeyEvent {
  KeyAction action;
  Key key;
  // TODO: modifiers
};

struct State {
  bool paused = false;
  bool done = false;
  bool synchronized = true;
  bool fast_forward = false;

  std::vector<InputState> input_state;
  std::vector<KeyEvent> key_events;
};

}
