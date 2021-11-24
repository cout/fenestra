#pragma once

#include "Config.hpp"
#include "Geometry.hpp"
#include "Clock.hpp"
#include "Core.hpp"
#include "State.hpp"
#include "CoreState.hpp"

#include <epoxy/glx.h>

#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

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
    , state_directory_(config.fetch<std::string>("paths.state_directory", "."))
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

    GLFWimage icon;
    icon.pixels = SOIL_load_image("icons/fenestra.png", &icon.width, &icon.height, 0, SOIL_LOAD_RGBA);
    glfwSetWindowIcon(win_, 1, &icon);
    SOIL_free_image_data(icon.pixels);

    glfwSetKeyCallback(win_, key_callback_);

    glfwMakeContextCurrent(win_);

    std::cout << "GL: Vendor: " << glGetString(GL_VENDOR)
              << " Renderer: "  << glGetString(GL_RENDERER)
              << " Version: "   << glGetString(GL_VERSION) << std::endl;
  }

  void poll_events(State & state) {
    current_ = this;

    glfwPollEvents();

    state.paused = paused();
    state.done = done();
  }

  bool done() const {
    return glfwWindowShouldClose(win_);
  }

  bool paused() const { return paused_; }

  void game_loaded(Core const & core, std::string const & filename);

  void unloading_game(Core const & core);

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

      case GLFW_KEY_S:
        {
          std::string filename = state_filename();
          auto state = CoreState::serialize(core_);
          state.save(filename);
          std::cout << "Saved state to " << filename << std::endl;
        }
        break;

      case GLFW_KEY_L:
        {
          last_state_ = CoreState::serialize(core_);

          std::string filename = state_filename();
          auto state = CoreState::load(filename);
          state.unserialize(core_);
          std::cout << "Loaded state from " << filename << std::endl;
        }
        break;

      case GLFW_KEY_U:
        if (last_state_.valid()) {
          last_state_.unserialize(core_);
        }
        std::cout << "Loaded state from undo buffer" << std::endl;
        break;

      case GLFW_KEY_0:
      case GLFW_KEY_1:
      case GLFW_KEY_2:
      case GLFW_KEY_3:
      case GLFW_KEY_4:
      case GLFW_KEY_5:
      case GLFW_KEY_6:
      case GLFW_KEY_7:
      case GLFW_KEY_8:
      case GLFW_KEY_9:
        auto digit = key - GLFW_KEY_0;

        if (now - last_digit_time_ > Seconds(1)) {
          state_number_ = 0;
        }

        state_number_ *= 10;
        state_number_ += digit;

        std::cout << "Selected state " << state_number_ << std::endl;

        last_digit_time_ = now;
        break;
    }
  }

  std::string state_filename() {
    std::stringstream strm;
    strm << state_basename_;
    strm << state_number_;
    return strm.str();
  }

private:
  static inline Window * current_ = nullptr;

  std::string title_;
  Core & core_;

  std::string const & state_directory_;

  GLFWwindow * win_ = nullptr;

  bool close_requested_ = false;
  Timestamp close_requested_time_;

  bool reset_requested_ = false;
  Timestamp reset_requested_time_;

  Timestamp last_digit_time_;

  bool paused_ = false;

  std::string state_basename_;

  unsigned int state_number_ = 0;

  CoreState last_state_;
};

}
