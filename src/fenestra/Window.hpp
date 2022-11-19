#pragma once

#include "Config.hpp"
#include "Geometry.hpp"
#include "Core.hpp"
#include "State.hpp"

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
  Window(std::string const & title, Config const & config)
    : title_(title)
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

    if (done()) state.done = true;
    state.key_events.insert(state.key_events.begin(), key_events_.begin(), key_events_.end());
    key_events_.clear();
  }

  bool done() const {
    return glfwWindowShouldClose(win_);
  }

private:
  static void key_callback_(GLFWwindow * window, int key, int scancode, int action, int mods) {
    current_->key_callback(key, scancode, action, mods);
  }

  void key_callback(int key, int scancode, int action, int mods) {
    KeyEvent event = { KeyAction::UNKNOWN, key::NUL };

    if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_GRAVE_ACCENT) {
      event.key = Key(key);
    } else {
      switch(key) {
        case GLFW_KEY_ESCAPE: event.key = key::ESC; break;
        case GLFW_KEY_ENTER: event.key = key::ENTER; break;
        case GLFW_KEY_TAB: event.key = key::TAB; break;
        case GLFW_KEY_BACKSPACE: event.key = key::BACKSPACE; break;
        case GLFW_KEY_LEFT: event.key = key::LEFT; break;
        case GLFW_KEY_UP: event.key = key::UP; break;
        case GLFW_KEY_RIGHT: event.key = key::RIGHT; break;
        case GLFW_KEY_DOWN: event.key = key::DOWN; break;
        default: break;
      }
    }

    switch (action) {
      case GLFW_PRESS: event.action = KeyAction::PRESS; break;
      case GLFW_RELEASE: event.action = KeyAction::RELEASE; break;
      case GLFW_REPEAT: event.action = KeyAction::REPEAT; break;
      default: break;
    }

    if (event.action != KeyAction::UNKNOWN && key != '\0') {
      key_events_.push_back(event);
    }
  }

private:
  static inline Window * current_ = nullptr;

  std::string title_;

  GLFWwindow * win_ = nullptr;

  std::vector<KeyEvent> key_events_;
};

}
