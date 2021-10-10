#include "../Clock.hpp"

#include <epoxy/glx.h>

#include <GLFW/glfw3.h>

#include <ftgl.h>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

class PerflogReader {
public:
  PerflogReader(std::ifstream & file)
    : file_(file)
  {
    std::string line;
    std::getline(file_, line);

    std::cout << line.length();

    std::string name;
    for (auto c : line) {
      if (c == '\0') {
        std::cout << name << std::endl;
        names_.push_back(name);
        name = "";
      } else {
        name += c;
      }
    }

    auto n_deltas = names_.size() - 1;
    record_size_ = 8 + n_deltas * 4; // 64-bit time, 32-bit deltas
  }

  void poll() {
    while (file_) {
      buf_.clear();
      buf_.resize(record_size_);
      file_.read(buf_.data(), record_size_);

      if (file_) {
        auto const * p = buf_.data();
        auto const * end = p + buf_.size();
        std::int64_t time = reinterpret_cast<std::uint64_t const &>(*p);
        p += sizeof(std::uint64_t);

        std::vector<std::uint32_t> deltas;
        for (; p < end; p += sizeof(std::uint32_t)) {
          deltas.push_back(reinterpret_cast<std::uint32_t const &>(*p));
        }

        std::cout << time;
        for (auto delta : deltas) {
          std::cout << " " << delta;
        }
        std::cout << std::endl;
      }
    }
  }

private:
  std::ifstream & file_;
  std::vector<std::string> names_;
  std::size_t record_size_ = 0;
  std::vector<char> buf_;
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

    if (!(win_ = glfwCreateWindow(400, 800, title_.c_str(), nullptr, nullptr))) {
      throw std::runtime_error("Failed to create window.");
    }

    glfwSetKeyCallback(win_, key_callback_);

    glfwMakeContextCurrent(win_);
    glfwSwapInterval(1);

    font_.FaceSize(72);
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
    glClear(GL_COLOR_BUFFER_BIT);
    font_.Render("Hello", -1, FTPoint(0, 0, 0));
  }

  void refresh() {
    glfwSwapBuffers(win_);
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
