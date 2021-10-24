#pragma once

#include "Config.hpp"
#include "Geometry.hpp"
#include "Clock.hpp"
#include "Core.hpp"

#include <epoxy/glx.h>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>

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
    if (!glfwInit()) {
      throw std::runtime_error("glfwInit failed");
    }
  }

  ~Window() {
    glfwTerminate();
  }

  void init(Geometry const & geom) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    if (!(win_ = glfwCreateWindow(geom.scaled_width(), geom.scaled_height(), title_.c_str(), nullptr, nullptr))) {
      throw std::runtime_error("Failed to create window.");
    }

    glfwSetKeyCallback(win_, key_callback_);

    glfwMakeContextCurrent(win_);
  }

  void poll_events() {
    current_ = this;

    glfwPollEvents();
  }

  bool done() const {
    return glfwWindowShouldClose(win_);
  }

  void frame_delay() const {
    if (config_.nv_delay_before_swap()) {
      Seconds delay_before_swap = Milliseconds(16.7) - config_.frame_delay();
      glXDelayBeforeSwapNV(glXGetCurrentDisplay(), glXGetCurrentDrawable(), delay_before_swap.count());
    } else {
      Clock::nanosleep_until(last_refresh_ + config_.frame_delay(), CLOCK_MONOTONIC);
    }
  }

  void window_refreshed() {
    last_refresh_ = Clock::gettime(CLOCK_MONOTONIC);
  }

  bool paused() const { return paused_; }

private:
  static void key_callback_(GLFWwindow * window, int key, int scancode, int action, int mods) {
    current_->key_callback(key, scancode, action, mods);
  }

  void key_callback(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      key_pressed(key, scancode, mods);
    }
  }

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
      case GLFW_KEY_ESCAPE:
        if (close_requested_ && now - close_requested_time_ < Seconds(1)) {
          glfwSetWindowShouldClose(win_, true);
          close_requested_ = false;
        } else {
          std::cout << "Close requested, press ESC within 1s to confirm" << std::endl;
          close_requested_ = true;
          close_requested_time_ = now;
        }
        break;

      case GLFW_KEY_R:
        if (reset_requested_ && now - reset_requested_time_ < Seconds(1)) {
          core_.reset();
          reset_requested_ = false;
        } else {
          std::cout << "Reset requested, press R within 1s to confirm" << std::endl;
          reset_requested_ = true;
          reset_requested_time_ = now;
        }
        break;

      case GLFW_KEY_P:
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
  GLFWwindow * win_ = nullptr;

  bool close_requested_ = false;
  Timestamp close_requested_time_;

  bool reset_requested_ = false;
  Timestamp reset_requested_time_;

  bool paused_ = false;
  Timestamp last_refresh_;
};

}
