#pragma once

#include "PerflogReader.hpp"

#include <epoxy/glx.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SOIL/SOIL.h>

#include <ftgl.h>

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <limits>
#include <cassert>

class PerflogViewer {
public:
  PerflogViewer(PerflogReader & reader, std::uint32_t width, std::uint32_t height)
    : reader_(reader)
    , width_(width)
    , height_(height)
    , title_("Perflog Viewer")
    , font_("/usr/share/fonts/truetype/msttcorefonts/Arial.ttf")
  {
    if (!glfwInit()) {
      throw std::runtime_error("glfwInit failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    if (!(win_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr))) {
      throw std::runtime_error("Failed to create window.");
    }

    GLFWimage icon;
    icon.pixels = SOIL_load_image("icons/perflog-viewer.png", &icon.width, &icon.height, 0, SOIL_LOAD_RGBA);
    glfwSetWindowIcon(win_, 1, &icon);
    SOIL_free_image_data(icon.pixels);

    glfwSetKeyCallback(win_, key_callback_);
    glfwSetWindowRefreshCallback(win_, refresh_callback_);

    glfwMakeContextCurrent(win_);
    glfwSwapInterval(1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width_, 0, height_);
    glMatrixMode(GL_MODELVIEW);
  }

  virtual ~PerflogViewer() {
    glfwTerminate();
  }

  void poll() {
    current_ = this;

    glfwPollEvents();
    reader_.poll();

    if (reader_.time() > last_time_) {
      need_redraw_ = true;
    }
  }

  virtual void redraw() {
    if (!need_redraw_) {
      need_refresh_ = false;
      return;
    }

    if (!render()) return;

    last_time_ = reader_.time();
    need_redraw_ = false;
    need_refresh_ = true;
  }

  virtual bool render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    auto num_queues = reader_.queues().size();

    if (num_queues == 0) {
      return false;
    }

    // TODO: This math is all wrong...
    // (obviously it's wrong since the top margin shouldn't be negative)
    double top_margin = -15;
    double bottom_margin = 10;
    double left_margin = 5;
    double right_margin = 10;
    double column_margin = 20;
    double row_height = (height_ - top_margin - bottom_margin) / num_queues;

    auto next_x = draw_metric_names(
        left_margin,
        height_ - row_height - top_margin,
        row_height);

    next_x = draw_averages(
        next_x + column_margin,
        height_ - row_height - top_margin,
        row_height);

    std::vector<std::uint32_t> mins;
    std::vector<std::uint32_t> maxes;

    mins.resize(reader_.queues().size());
    maxes.resize(reader_.queues().size());

    // Calculate min/max values
    for (std::size_t i = 0; i < reader_.queues().size(); ++i) {
      std::uint32_t max = 0;
      std::uint32_t min = std::numeric_limits<std::uint32_t>::max();
      for (auto val : reader_.queues()[i]) {
        min = std::min(val, min);
        max = std::max(val, max);
      }
      mins[i] = min;
      maxes[i] = max;
    }

    next_x = draw_min_max(
        mins,
        maxes,
        next_x + column_margin,
        height_ - row_height - top_margin,
        row_height);

    auto graph_width = width_ - (next_x + column_margin) - right_margin;

    draw_plots(
        mins,
        maxes,
        next_x + column_margin,
        height_ - row_height - top_margin,
        row_height,
        graph_width);

    draw_markers(
        next_x + column_margin,
        height_ - row_height - top_margin,
        graph_width);

    return true;
  }

  double draw_metric_names(double x, double y, double row_height) {
    font_.FaceSize(row_height * 0.5875);
    FTGL_DOUBLE next_x = 0;
    for (auto const & queue : reader_.queues()) {
      std::string s(queue.name());
      auto pos = font_.Render(s.c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    return next_x;
  }

  double draw_averages(double x, double y, double row_height) {
    font_.FaceSize(row_height * 0.5875);
    FTGL_DOUBLE next_x = 0;
    for (auto const & queue : reader_.queues()) {
      std::stringstream strm;
      strm << std::fixed << std::setprecision(2);
      strm << queue.avg() / 1000.0;
      auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    return next_x;
  }

  double draw_min_max(std::vector<std::uint32_t> const & mins, std::vector<std::uint32_t> const & maxes, double x, double y, double row_height) {
    font_.FaceSize(row_height * 0.75 / 2);
    y += row_height / 4 + row_height / 8;
    FTGL_DOUBLE next_x = 0;
    for (std::size_t i = 0; i < reader_.queues().size(); ++i) {
      y -= row_height / 8;
      auto min = mins[i];
      auto max = maxes[i];
      {
        std::stringstream strm;
        strm << std::fixed << std::setprecision(2);
        strm << max / 1000.0;
        auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
        next_x = std::max(next_x, pos.X());
        y -= 0.75 * (row_height / 2);
      }
      {
        std::stringstream strm;
        strm << std::fixed << std::setprecision(2);
        strm << min / 1000.0;
        auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
        next_x = std::max(next_x, pos.X());
        y -= 0.75 * (row_height / 2);
      }
      y -= row_height / 8;
    }

    return next_x;
  }

  void draw_plots(std::vector<std::uint32_t> const & mins, std::vector<std::uint32_t> const & maxes, double x, double y, double row_height, double graph_width) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    std::vector<GLfloat> coords;
    std::size_t qidx = 0;
    for (auto const & queue : reader_.queues()) {
      auto min = mins[qidx];
      auto max = maxes[qidx];

      if (max > 0) {
        draw_plot(x, y, queue, min, max, row_height, graph_width, coords);
      }

      y -= row_height;
      ++qidx;
    }
  }

  template <typename Queue>
  void draw_plot(double x, double y, Queue const & queue, std::uint32_t min, std::uint32_t max, double row_height, double graph_width, std::vector<GLfloat> & coords) {
    std::size_t i = 0;
    auto num_points = queue.size();
    coords.resize(num_points * 2);
    for (auto val : queue) {
      assert(i * 2 + 1 < coords.size());
      auto pct = float(val - min) / max;
      assert(pct >= 0.0);
      // assert(pct <= 1.0);
      coords[i*2] = x + float(i) / num_points * graph_width;
      coords[i*2 + 1] = pct * row_height * 0.8 + y + 0.1 * row_height;
      ++i;
    }

    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glDrawArrays(GL_LINE_STRIP, 0, coords.size() / 2);
  }

  void draw_markers(double x, double y, double graph_width) {
    glEnable(GL_BLEND);
    glEnableClientState(GL_COLOR_ARRAY);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<GLfloat> coords;
    std::vector<GLfloat> colors;

    // first queue is frame time
    auto & queue = reader_.queues()[0];
    auto num_points = queue.size();
    std::uint32_t prev_val = 16667;
    for (std::size_t i = 0; i < queue.size(); ++i) {
      auto val = queue[i];
      auto next_val = i < queue.size() - 1 ? queue[i+1] : 16667;
      if (val > 18000 && next_val < 16667) {
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(0);
        colors.push_back(0.2);
        colors.push_back(0.2);
        colors.push_back(1.0);
        colors.push_back(0.4);
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(height_);
        colors.push_back(0.2);
        colors.push_back(0.2);
        colors.push_back(1.0);
        colors.push_back(0.4);
      } else if (val > 19000 && prev_val > 16667) {
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(0);
        colors.push_back(1.0);
        colors.push_back(0.0);
        colors.push_back(0.0);
        colors.push_back(0.4);
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(height_);
        colors.push_back(1.0);
        colors.push_back(0.0);
        colors.push_back(0.0);
        colors.push_back(0.4);
      } if (val > 18000 && prev_val > 16667 && val <= 19000) {
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(0);
        colors.push_back(1.0);
        colors.push_back(1.0);
        colors.push_back(0.0);
        colors.push_back(0.2);
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(height_);
        colors.push_back(1.0);
        colors.push_back(1.0);
        colors.push_back(0.0);
        colors.push_back(0.2);
      }
      prev_val = val;
    }
    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glColorPointer(4, GL_FLOAT, 0, colors.data());
    glDrawArrays(GL_LINES, 0, coords.size() / 2);

  }

  void refresh() {
    if (need_refresh_) {
      glfwSwapBuffers(win_);
      need_refresh_ = false;
    } else {
      fenestra::Clock::nanosleep(fenestra::Milliseconds(16), CLOCK_REALTIME);
    }
  }

  bool done() const {
    return glfwWindowShouldClose(win_);
  }

private:
  static void key_callback_(GLFWwindow * window, int key, int scancode, int action, int mods) {
    current_->key_callback(key, scancode, action, mods);
  }

  static void refresh_callback_(GLFWwindow * window) {
    current_->refresh_callback();
  }

  void key_callback(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      key_pressed(key, scancode, mods);
    }
  }

  void key_pressed(int key, int scancode, int mods) {
    switch(key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win_, true);
        break;
    }
  }

  void refresh_callback() {
    need_redraw_ = true;
  }

protected:
  static inline PerflogViewer * current_ = nullptr;

  PerflogReader & reader_;
  std::uint32_t width_;
  std::uint32_t height_;

  std::string title_;
  // FTGLPixmapFont font_;
  FTGLTextureFont font_;
  GLFWwindow * win_ = nullptr;

  std::uint64_t last_time_ = 0;
  bool need_redraw_ = false;
  bool need_refresh_ = false;
};
