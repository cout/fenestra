#include "../Clock.hpp"

#include <epoxy/glx.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <ftgl.h>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cassert>

class PerfQueue {
public:
  PerfQueue(std::string const & name)
    : name_(name)
  {
  }

  auto const & name() const { return name_; }

  static constexpr inline std::size_t max_secs = 60;
  static constexpr inline std::size_t max_frames = 60 * max_secs;

  void record(std::uint32_t delta) {
    total_ += delta;
    deltas_.push_back(delta);
    if (deltas_.size() > max_frames) {
      total_ -= deltas_.front();
      deltas_.pop_front();
    }
  }

  auto total() const { return total_; }
  auto avg() const { return total_ / deltas_.size(); }
  auto size() const { return deltas_.size(); }
  auto begin() const { return deltas_.begin(); }
  auto end() const { return deltas_.end(); }

private:
  std::string name_;
  std::deque<std::uint32_t> deltas_;
  std::uint64_t total_ = 0;
};

class PerflogReader {
public:
  PerflogReader(std::ifstream & file)
    : file_(file)
  {
    std::string line;
    std::getline(file_, line);
    last_pos_ = file_.tellg();

    std::string name;
    bool first = true;
    for (auto c : line) {
      if (c == '\0') {
        if (first) {
          first = false;
        } else {
          queues_.emplace_back(name);
        }
        name = "";
      } else {
        name += c;
      }
    }

    auto n_deltas = queues_.size();
    record_size_ = 8 + n_deltas * 4; // 64-bit time, 32-bit deltas

    std::int64_t time;
    std::vector<std::uint32_t> deltas;
    while (read_record(&time, &deltas)) {
      handle(time, deltas);
    }
  }

  auto const & queues() const { return queues_; }
  auto time() const { return time_; }

  void poll() {
    file_.clear();
    file_.seekg(last_pos_);

    std::int64_t time;
    std::vector<std::uint32_t> deltas;
    while (read_record(&time, &deltas)) {
      handle(time, deltas);
    }
  }

private:
  bool read_record(std::int64_t * time, std::vector<std::uint32_t> * deltas) {
    buf_.clear();
    buf_.resize(record_size_);
    file_.read(buf_.data(), record_size_);

    if (file_) {
      auto const * p = buf_.data();
      auto const * end = p + buf_.size();
      *time = reinterpret_cast<std::uint64_t const &>(*p);
      p += sizeof(std::uint64_t);

      for (; p < end; p += sizeof(std::uint32_t)) {
        deltas->push_back(reinterpret_cast<std::uint32_t const &>(*p));
      }

      last_pos_ = file_.tellg();
      return true;
    }

    return false;
  }

  void handle(std::int64_t time, std::vector<std::uint32_t> const & deltas) {
    for (std::size_t i = 0; i < std::min(deltas.size(), queues_.size()); ++i) {
      queues_[i].record(deltas[i]);
    }

    time_ = time;
  }

private:
  std::ifstream & file_;
  std::ifstream::pos_type last_pos_;
  std::vector<PerfQueue> queues_;
  std::size_t record_size_ = 0;
  std::vector<char> buf_;
  std::uint64_t time_ = 0;
};

class PerflogViewer {
public:
  PerflogViewer(PerflogReader & reader)
    : reader_(reader)
    , title_("Perlog Viewer")
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

    glfwSetKeyCallback(win_, key_callback_);

    glfwMakeContextCurrent(win_);
    glfwSwapInterval(1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width_, 0, height_);
    glMatrixMode(GL_MODELVIEW);
  }

  ~PerflogViewer() {
    glfwTerminate();
  }

  void poll() {
    current_ = this;

    glfwPollEvents();
    reader_.poll();
  }

  void redraw() {
    if (reader_.time() == last_time_) {
      return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    auto num_queues = reader_.queues().size();
    auto row_height = height_ / num_queues;
    font_.FaceSize(row_height * 0.8);

    FTGL_DOUBLE y = height_ - row_height;
    FTGL_DOUBLE x = 0;
    FTGL_DOUBLE next_x = 0;
    for (auto const & queue : reader_.queues()) {
      std::string s(queue.name());
      auto pos = font_.Render(s.c_str(), -1, FTPoint(0, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    y = height_ - row_height;
    x = next_x + 20;
    for (auto const & queue : reader_.queues()) {
      std::stringstream strm;
      strm << std::fixed << std::setprecision(2);
      strm << queue.avg() / 1000.0;
      auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    std::vector<std::uint32_t> mins;
    std::vector<std::uint32_t> maxes;

    mins.resize(reader_.queues().size());
    maxes.resize(reader_.queues().size());

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

    y = height_ - row_height;
    x = next_x + 20;
    for (auto min : mins) {
      std::stringstream strm;
      strm << std::fixed << std::setprecision(2);
      strm << min / 1000.0;
      auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    y = height_ - row_height;
    x = next_x + 20;
    for (auto max : maxes) {
      std::stringstream strm;
      strm << std::fixed << std::setprecision(2);
      strm << max / 1000.0;
      auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    y = height_ - row_height;
    x = next_x + 20;
    auto graph_width = width_ - x - 20;
    std::vector<GLfloat> coords;
    std::size_t qidx = 0;
    for (auto const & queue : reader_.queues()) {
      auto num_points = queue.size();
      coords.clear();
      coords.resize(num_points * 2);
      auto min = mins[qidx];
      auto max = maxes[qidx];

      if (max > 0) {
        std::size_t i = 0;
        for (auto val : queue) {
          assert(i * 2 + 1 < coords.size());
          auto pct = float(val - min) / max;
          assert(pct >= 0.0);
          assert(pct <= 1.0);
          coords[i*2] = x + float(i) / num_points * graph_width;
          coords[i*2 + 1] = pct * row_height * 0.8 + y + row_height + 0.1 * row_height;
          ++i;
        }

        glVertexPointer(2, GL_FLOAT, 0, coords.data());
        glDrawArrays(GL_LINE_STRIP, 0, coords.size() / 2);
      }

      y -= row_height;
      ++qidx;
    }

    last_time_ = reader_.time();
    need_refresh_ = true;
  }

  void refresh() {
    if (need_refresh_) {
      glfwSwapBuffers(win_);
      need_refresh_ = false;
    }
  }

  bool done() const {
    return glfwWindowShouldClose(win_);
  }

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
    switch(key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win_, true);
        break;
    }
  }

private:
  static inline PerflogViewer * current_ = nullptr;

  PerflogReader & reader_;
  std::string title_;
  FTGLPixmapFont font_;
  GLFWwindow * win_ = nullptr;

  std::uint32_t width_ = 1000;
  std::uint32_t height_ = 600;
  std::uint64_t last_time_ = 0;
  bool need_refresh_ = false;
};

int main(int argc, char * argv[]) {
  if (argc < 1) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  std::ifstream file(argv[1]);
  PerflogReader reader(file);
  PerflogViewer app(reader);

  while (!app.done()) {
    app.poll();
    app.redraw();
    app.refresh();
  }
}
