#pragma once

namespace fenestra {

struct InputState {
  std::int16_t pressed[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
};

struct State {
  bool paused = false;
  bool done = false;
  bool synchronized = true;

  std::vector<InputState> input_state;
};

}
