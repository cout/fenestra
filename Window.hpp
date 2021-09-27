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
    glfwSwapInterval(config_.adaptive_vsync() ? -1 : config_.vsync() ? 1 : 0);
  }

  void poll_events() {
    current_ = this;

    if (config_.oml_sync()) {
      glXGetSyncValuesOML(glfwGetX11Display(), glfwGetGLXWindow(win_), &ust_, &msc_, &sbc_);
    }

    glfwPollEvents();

    // if (glfwGetKey(win_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      // glfwSetWindowShouldClose(win_, true);
    // }
  }

  bool done() const {
    return glfwWindowShouldClose(win_);
  }

  void frame_delay() const {
    if (config_.nv_delay_before_swap()) {
      Seconds delay_before_swap = Milliseconds(16.7) - config_.frame_delay();
      // glFinish(); // TODO: see comment below in glfinish()
      glXDelayBeforeSwapNV(glfwGetX11Display(), glfwGetGLXWindow(win_), delay_before_swap.count());
    } else {
      Clock::nanosleep_until(last_refresh_ + config_.frame_delay(), CLOCK_MONOTONIC);
    }
  }

  void refresh() {
    if (config_.oml_sync()) {
      // TODO: Obviously this is not ideal.  One problem is we don't
      // know how long it takes to swap buffers, so we can't push the
      // frame delay too for or we might miss deadline for swapping even
      // if we attempt to swap before the vertical retrace.
      //
      // This also doesn't achieve the goal of preferring visual
      // artifacts over jitter.
      glXSwapBuffersMscOML(glfwGetX11Display(), glfwGetGLXWindow(win_), msc_ + 1, 0, 0);
    } else {
      // glfwSwapBuffers(win_);
      glXSwapBuffers(glfwGetX11Display(), glfwGetGLXWindow(win_));
    }

    last_refresh_ = Clock::gettime(CLOCK_MONOTONIC);
  }

  void glfinish() {
    // TODO: The glFinish is required to make glXDelayBeforeSwapNV work
    // correctly, but it has other downsides, like occasionally making
    // us wait until the next frame if we miss the swap window.
    //
    // The documentation says we can do glFinish just before calling
    // glXDelayBeforeSwapNV, but that doesn't work (it always sleeps for
    // 15ms when we do that, regardles of what delay we pass in).
    glClear(GL_COLOR_BUFFER_BIT);
    if (config_.glfinish() || config_.nv_delay_before_swap()) {
      glFinish();
    }
  }

  auto & win() { return *win_; }

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
  GLFWwindow * win_ = 0;
  std::int64_t ust_ = 0;
  std::int64_t msc_ = 0;
  std::int64_t sbc_ = 0;
  Timestamp last_refresh_;

  bool close_requested_ = false;
  Timestamp close_requested_time_;

  bool reset_requested_ = false;
  Timestamp reset_requested_time_;

  bool paused_ = false;
};

}
