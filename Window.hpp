#pragma once

#include "Config.hpp"
#include "Geometry.hpp"
#include "Clock.hpp"
#include "Core.hpp"
#include "State.hpp"

#include <epoxy/glx.h>

#include <SDL.h>

#include <memory>
#include <string>
#include <iostream>

namespace fenestra {

class Window {
public:
  Window(std::string const & title, Core & core, Config const & config)
    : title_(title)
    , core_(core)
    , config_(config)
  {
    SDL_JoystickEventState(SDL_IGNORE);
    SDL_GameControllerEventState(SDL_IGNORE);
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  }

  ~Window() {
  }

  void init(Geometry const & geom) {
    win_ = SDL_CreateWindow("sdlarch", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, geom.scaled_width(), geom.scaled_height(), SDL_WINDOW_OPENGL);
    if (!win_) {
      throw std::runtime_error("SDL_CreateWindow failed");
    }

    ctx_ = SDL_GL_CreateContext(win_);
    if (!ctx_) {
      throw std::runtime_error("SDL_GL_CreateContext failed");
    }

    SDL_GL_MakeCurrent(win_, ctx_);

    std::cout << "GL: Vendor: " << glGetString(GL_VENDOR)
              << " Renderer: "  << glGetString(GL_RENDERER)
              << " Version: "   << glGetString(GL_VERSION) << std::endl;
  }

  void poll_events(State & state) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch(ev.type) {
        case SDL_QUIT:
          done_ = true;
          break;

        case SDL_WINDOWEVENT:
          switch(ev.window.event) {
            case SDL_WINDOWEVENT_CLOSE:
              done_ = true;
              break;
          }

        case SDL_KEYDOWN:
          key_pressed(ev.key.keysym.sym, ev.key.keysym.scancode, ev.key.keysym.mod);
      }
    }

    state.paused = paused();
    state.done = done();
  }

  bool paused() const { return paused_; }
  bool done() const { return done_; }

private:
  void key_pressed(int key, int scancode, int mods) {
    // TODO: This is quick&dirty, but I think there should be some sort
    // of event system, rather than having the Window know about the
    // Core.
    //
    // Perhaps simpler but just as effective would be to have a Keyboard
    // class, and allow it to know about the core.

    // TODO: Create a timer queue, instead of manually doing timing like
    // this?  That will be necessary if I want to show status text in
    // the main window.

    // TODO: Delay showing Close/reset requested message, so it's not
    // possible to accidentally press esc-esc (there has to be a delay
    // between them)

    auto now = Clock::gettime(CLOCK_REALTIME);

    switch(key) {
      case SDLK_ESCAPE:
        if (close_requested_ && now - close_requested_time_ < Seconds(1)) {
          done_ = true;
          close_requested_ = false;
        } else {
          std::cout << "Close requested, press ESC within 1s to confirm" << std::endl;
          close_requested_ = true;
          close_requested_time_ = now;
        }
        break;

      case SDLK_r:
        if (reset_requested_ && now - reset_requested_time_ < Seconds(1)) {
          core_.reset();
          reset_requested_ = false;
        } else {
          std::cout << "Reset requested, press R within 1s to confirm" << std::endl;
          reset_requested_ = true;
          reset_requested_time_ = now;
        }
        break;

      case SDLK_p:
        // TODO: Stop audio when emulator is paused, otherwise we get
        // underflow errors
        paused_ = !paused_;
        std::cout << (paused_ ? "Paused." : "GLHF!") << std::endl;
        break;
    }
  }

private:
  static inline Window * current_ = nullptr;

  std::string title_;
  Core & core_;
  Config const & config_;
  SDL_Window * win_ = NULL;
  SDL_GLContext ctx_ = NULL;

  bool close_requested_ = false;
  Timestamp close_requested_time_;

  bool reset_requested_ = false;
  Timestamp reset_requested_time_;

  bool paused_ = false;
  bool done_ = false;
};

}
