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

#include <sys/stat.h>
#include <unistd.h>

using namespace fenestra;

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
  auto avg() const { return deltas_.size() == 0 ? 0 : total_ / deltas_.size(); }
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
  PerflogReader(std::string const & filename)
    : filename_(filename)
  {
    this->stat();
    last_stat_time_ = Clock::gettime(CLOCK_REALTIME);
  }

  auto const & queues() const { return queues_; }
  auto time() const { return time_; }

  void poll() {
    auto now = Clock::gettime(CLOCK_REALTIME);
    if (now - last_stat_time_ > Seconds(1.0)) {
      stat();
      last_stat_time_ = now;
    }

    file_.clear();
    file_.seekg(last_pos_);

    std::int64_t time;
    std::vector<std::uint32_t> deltas;
    while (read_record(&time, &deltas)) {
      handle(time, deltas);
    }
  }

private:
  void stat() {
    struct stat statbuf;
    if (lstat(filename_.c_str(), &statbuf) < 0) {
      // TODO: this might be a transient error, so there should be away
      // to ignore this and continue
      throw std::runtime_error("lstat failed");
    }

    if (statbuf.st_ino != statbuf_.st_ino || statbuf.st_size < statbuf_.st_size || queues_.size() == 0) {
      if (last_stat_time_ > Timestamp()) {
        std::cout << "File was truncated; re-opening file" << std::endl;
      }
      open(filename_);
    }

    statbuf_ = statbuf;
  }

  void open(std::string const & filename) {
    queues_.clear();

    file_.close();
    file_.open(filename);
    filename_ = filename;

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
    time_ = 0;
    record_size_ = 8 + n_deltas * 4; // 64-bit time, 32-bit deltas

    if (n_deltas > 0) {
      queues_.emplace(queues_.begin(), "FPS");
      queues_.emplace(queues_.begin(), "Frame Time");
    }

    std::int64_t time;
    std::vector<std::uint32_t> deltas;
    while (read_record(&time, &deltas)) {
      handle(time, deltas);
    }
  }

  bool read_record(std::int64_t * time, std::vector<std::uint32_t> * deltas) {
    buf_.clear();
    buf_.resize(record_size_);
    file_.read(buf_.data(), record_size_);

    if (file_) {
      auto const * p = buf_.data();
      auto const * end = p + buf_.size();
      *time = reinterpret_cast<std::uint64_t const &>(*p);
      p += sizeof(std::uint64_t);

      deltas->clear();
      for (; p < end; p += sizeof(std::uint32_t)) {
        deltas->push_back(reinterpret_cast<std::uint32_t const &>(*p));
      }

      last_pos_ = file_.tellg();
      return true;
    }

    return false;
  }

  void handle(std::uint64_t time, std::vector<std::uint32_t> const & deltas) {
    std::uint32_t frame_time_us = 16'667;
    if (time_ > 0 && time != time_) {
      auto last_time = time_;
      auto time_delta_ns = time - last_time;
      frame_time_us = time_delta_ns / 1'000;
    }
    queues_[0].record(frame_time_us);

    float fps = 60.0;
    if (queues_[0].size() >= 8) {
      // Average fps over 8 frames
      auto it = std::prev(queues_[0].end());
      std::uint64_t eight_frame_time = 0;
      for (std::size_t i = 0; i < 8; ++i, --it) {
        eight_frame_time += *it;
      }
      auto avg_us = eight_frame_time / 8.0;
      auto avg_s = avg_us / 1'000'000;
      fps = 1.0 / avg_s;
    }
    queues_[1].record(fps * 1000);

    for (std::size_t i = 0; i < std::min(deltas.size(), queues_.size()); ++i) {
      queues_[i+2].record(deltas[i]); // +2 for frame time, fps
    }

    time_ = time;
  }

private:
  std::string filename_;
  std::ifstream file_;
  std::ifstream::pos_type last_pos_;
  std::vector<PerfQueue> queues_;
  std::size_t record_size_ = 0;
  std::vector<char> buf_;
  std::uint64_t time_ = 0;
  Timestamp last_stat_time_ = Nanoseconds::zero();
  struct stat statbuf_ { 0, 0 }; // dev, inode
};

class PerflogViewer {
public:
  PerflogViewer(PerflogReader & reader)
    : reader_(reader)
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

    glfwSetKeyCallback(win_, key_callback_);
    glfwSetWindowRefreshCallback(win_, refresh_callback_);

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

    if (reader_.time() > last_time_) {
      need_redraw_ = true;
    }
  }

  void redraw() {
    if (!need_redraw_) {
      need_refresh_ = false;
      return;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    auto num_queues = reader_.queues().size();

    if (num_queues == 0) {
      return;
    }

    auto top_margin = 0;
    auto bottom_margin = 5;
    auto left_margin = 5;
    auto right_margin = 10;
    auto column_margin = 20;
    auto row_height = (height_ - top_margin - bottom_margin) / num_queues;
    font_.FaceSize(row_height * 0.5);

    FTGL_DOUBLE y = height_ - row_height - top_margin;
    FTGL_DOUBLE x = left_margin;
    FTGL_DOUBLE next_x = 0;
    for (auto const & queue : reader_.queues()) {
      std::string s(queue.name());
      auto pos = font_.Render(s.c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    y = height_ - row_height - top_margin;
    x = next_x + column_margin;
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

    font_.FaceSize(row_height * 0.75 / 2);

    y = height_ - row_height / 2 - top_margin;
    x = next_x + column_margin;
    for (std::size_t i = 0; i < reader_.queues().size(); ++i) {
      auto min = mins[i];
      auto max = maxes[i];
      {
        std::stringstream strm;
        strm << std::fixed << std::setprecision(2);
        strm << max / 1000.0;
        auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
        next_x = std::max(next_x, pos.X());
        y -= row_height / 2;
      }
      {
        std::stringstream strm;
        strm << std::fixed << std::setprecision(2);
        strm << min / 1000.0;
        auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
        next_x = std::max(next_x, pos.X());
        y -= row_height / 2;
      }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    y = height_ - row_height - top_margin;
    x = next_x + column_margin;
    auto graph_width = width_ - x - right_margin;
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
          coords[i*2 + 1] = pct * row_height * 0.8 + y + 0.1 * row_height;
          ++i;
        }

        glVertexPointer(2, GL_FLOAT, 0, coords.data());
        glDrawArrays(GL_LINE_STRIP, 0, coords.size() / 2);
      }

      y -= row_height;
      ++qidx;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0, 1.0, 0.0, 0.2);

    // first queue is frame time
    coords.clear();
    std::size_t i = 0;
    auto num_points = reader_.queues()[0].size();
    std::uint32_t prev_val = 16667;
    for (auto val : reader_.queues()[0]) {
      if (val > 18000 && prev_val > 16667 && val <= 19000) {
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(0);
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(height_);
      }
      prev_val = val;
      ++i;
    }
    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glDrawArrays(GL_LINES, 0, coords.size() / 2);

    glColor4f(1.0, 0.0, 0.0, 0.4);

    // first queue is frame time
    coords.clear();
    i = 0;
    num_points = reader_.queues()[0].size();
    prev_val = 16667;
    for (auto val : reader_.queues()[0]) {
      if (val > 19000 && prev_val > 16667) {
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(0);
        coords.push_back(x + float(i) / num_points * graph_width);
        coords.push_back(height_);
      }
      prev_val = val;
      ++i;
    }
    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glDrawArrays(GL_LINES, 0, coords.size() / 2);

    last_time_ = reader_.time();
    need_redraw_ = false;
    need_refresh_ = true;
  }

  void refresh() {
    if (need_refresh_) {
      glfwSwapBuffers(win_);
      need_refresh_ = false;
    } else {
      Clock::nanosleep(Milliseconds(16), CLOCK_REALTIME);
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

private:
  static inline PerflogViewer * current_ = nullptr;

  PerflogReader & reader_;
  std::string title_;
  FTGLPixmapFont font_;
  GLFWwindow * win_ = nullptr;

  std::uint32_t width_ = 768;
  std::uint32_t height_ = 900;
  std::uint64_t last_time_ = 0;
  bool need_redraw_ = false;
  bool need_refresh_ = false;
};

int main(int argc, char * argv[]) {
  if (argc < 1) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  PerflogReader reader(argv[1]);
  PerflogViewer app(reader);

  while (!app.done()) {
    app.poll();
    app.redraw();
    app.refresh();
  }
}
