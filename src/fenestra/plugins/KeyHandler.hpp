#pragma once

#include "fenestra/Plugin.hpp"
#include "fenestra/Core.hpp"
#include "fenestra/Config.hpp"
#include "fenestra/State.hpp"
#include "fenestra/CoreState.hpp"
#include "fenestra/Clock.hpp"

#include <string>
#include <sstream>
#include <vector>

namespace fenestra {

class KeyHandler
  : public Plugin
{
public:
  KeyHandler(Config::Subtree const & config, std::string const & instance)
    : state_directory_(config.root().fetch<std::string>("paths.state_directory", "."))
  {
  }

  virtual void handle_key_events(std::vector<KeyEvent> const & key_events, State & state) override {
    for (auto const & event : key_events) {
      if (event.action == KeyAction::PRESS) {
        handle_key_pressed(event.key, state);
      }
    }
  }

  void game_loaded(Core const & core, std::string const & filename);

  void unloading_game(Core const & core);

private:
  void handle_key_pressed(int key, State & state) {
    // TODO: Create a timer queue, instead of manually doing timing like
    // this?  That will be necessary if I want to show status text in
    // the main window.

    // TODO: Delay showing Close/reset requested message, so it's not
    // possible to accidentally press esc-esc (there has to be a delay
    // between them)

    auto now = Clock::gettime(CLOCK_REALTIME);

    switch(key) {
      case '\033':
        if (close_requested_ && now - close_requested_time_ < Seconds(1)) {
          state.done = true;
          close_requested_ = false;
        } else {
          std::cout << "Close requested, press ESC within 1s to confirm" << std::endl;
          close_requested_ = true;
          close_requested_time_ = now;
        }
        break;

      case 'R':
        if (reset_requested_ && now - reset_requested_time_ < Seconds(1)) {
          core_->reset();
          reset_requested_ = false;
        } else {
          std::cout << "Reset requested, press R within 1s to confirm" << std::endl;
          reset_requested_ = true;
          reset_requested_time_ = now;
        }
        break;

      case 'P':
        state.paused = !state.paused;
        std::cout << (state.paused ? "Paused." : "GLHF!") << std::endl;
        if (state.paused) state.fast_forward = false;
        break;

      case ' ':
        state.fast_forward = !state.fast_forward;
        std::cout << (state.fast_forward ? "Fast-foward." : "Back to normal speed") << std::endl;
        break;

      case 'S':
        {
          std::string filename = state_filename();
          auto state = CoreState::serialize(*core_);
          state.save(filename);
          std::cout << "Saved state to " << filename << std::endl;
        }
        break;

      case 'L':
        {
          last_state_ = CoreState::serialize(*core_);

          std::string filename = state_filename();
          auto state = CoreState::load(filename);
          state.unserialize(*core_);
          std::cout << "Loaded state from " << filename << std::endl;
        }
        break;

      case 'U':
        if (last_state_.valid()) {
          last_state_.unserialize(*core_);
        }
        std::cout << "Loaded state from undo buffer" << std::endl;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        {
          auto digit = key - '0';

          if (now - last_digit_time_ > Seconds(1)) {
            state_number_ = 0;
          }

          state_number_ *= 10;
          state_number_ += digit;

          std::cout << "Selected state " << state_number_ << std::endl;

          last_digit_time_ = now;
        }
        break;

      case '\010':
        if (now - last_digit_time_ <= Seconds(1)) {
          state_number_ /= 10;

          std::cout << "Selected state " << state_number_ << std::endl;

          last_digit_time_ = now;
        }
    }
  }

  std::string state_filename() {
    std::stringstream strm;
    strm << state_basename_;
    strm << state_number_;
    return strm.str();
  }

private:
  Core const * core_ = nullptr;

  std::string const & state_directory_;

  bool close_requested_ = false;
  Timestamp close_requested_time_;

  bool reset_requested_ = false;
  Timestamp reset_requested_time_;

  Timestamp last_digit_time_;

  std::string state_basename_;

  unsigned int state_number_ = 0;

  CoreState last_state_;
};

}
