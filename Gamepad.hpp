#pragma once

#include "Config.hpp"

#include <SDL_gamecontroller.h>

#include <vector>

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
  Gamepad(Config const & config, unsigned int index = 0)
    : config_(config)
  {
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) != 1) {
      SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    SDL_GameControllerEventState(SDL_IGNORE);

    gamecontroller_ = SDL_GameControllerOpen(index);
  }

  virtual std::string_view name() const { return "Gamepad"; }

  auto const & state() const { return state_; }

  void poll_input() {
    SDL_GameControllerUpdate();

    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_A] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_B);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_B] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_A);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_X] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_Y);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_Y] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_X);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_L] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_R] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_SELECT] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_GUIDE);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_START] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_START);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_UP] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_DPAD_UP);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_DOWN] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_LEFT] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    state_.pressed()[RETRO_DEVICE_ID_JOYPAD_RIGHT] = SDL_GameControllerGetButton(gamecontroller_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
  }

  bool pressed(unsigned int id) {
    return state_.pressed()[id];
  }

private:
  Config const & config_;
  SDL_GameController * gamecontroller_ = nullptr;
  InputState state_;
};

}
