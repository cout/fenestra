#include "PerflogViewer.hpp"
#include "PerflogReader.hpp"

#include <iostream>
#include <string>

bool is_main_loop(std::string const & name) {
  return (
         name == "Pre frame delay"
      || name == "Poll window events"
      || name == "Frame delay"
      || name == "Core run"
      || name == "    Render"
      || name == "    Update delay"
      || name == "    Update"
      || name == "    Sync");
}

bool is_drawn_main_loop(std::string const & name) {
  return is_main_loop(name)
    && name != "Pre frame delay";
}

std::string main_loop_label(std::string const & name) {
  if (name == "Pre frame delay") return "YOU SHOULD NOT SEE THIS";
  if (name == "Poll window events") return "UI";
  if (name == "Frame delay") return "Core delay";
  if (name == "Core run") return "Core";
  if (name == "    Render") return "Render";
  if (name == "    Update delay") return "Update delay";
  if (name == "    Update") return "Update";
  if (name == "    Sync") return "Sync";
  return std::string("NOT MAIN LOOP: `") + name + "'";
}

template<typename Queues, typename Fn>
void for_each_main_loop_queue(Queues const & queues, Fn f) {
  for (auto const & queue : queues) {
    if (is_main_loop(queue.name())) {
      f(queue);
    }
  }
}

void set_stacked_color(std::size_t idx) {
  switch(idx) {
    case 0: glColor4f(1.0, 0.5, 0.5, 1.0); break;
    case 1: glColor4f(0.5, 1.0, 0.5, 1.0); break;
    case 2: glColor4f(1.0, 1.0, 0.5, 1.0); break;
    case 3: glColor4f(0.5, 0.5, 1.0, 1.0); break;
    case 4: glColor4f(1.0, 0.5, 1.0, 1.0); break;
    case 5: glColor4f(0.5, 1.0, 1.0, 1.0); break;
    default: glColor4f(1.0, 1.0, 1.0, 1.0); break;
  }
}

class PerflogViewerMini
  : public PerflogViewer
{
public:
  using PerflogViewer::PerflogViewer;

  virtual bool render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    // TODO: This math is all wrong...
    // (obviously it's wrong since the top margin shouldn't be negative)
    double top_margin = -15;
    double bottom_margin = 10;
    double left_margin = 5;
    double right_margin = 10;
    double column_margin = 20;
    double row_margin = 20;
    // double row_height = (height_ - top_margin - bottom_margin) / num_queues;

    auto num_queues = reader_.queues().size();

    if (num_queues == 0) {
      return false;
    }

    std::uint32_t min = 0;
    std::uint32_t max = 0;

    std::size_t max_main_loop_queue_size = 0;
    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      max_main_loop_queue_size = std::max(max_main_loop_queue_size, queue.size());
    });
    
    for (std::size_t i = 0; i < max_main_loop_queue_size; ++i) {
      std::uint32_t sum_at_point = 0;
      for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
        sum_at_point += queue[i];
      });
      max = std::max(max, sum_at_point);
    }

    max = std::max(max, std::uint32_t(16667));

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    double row_height = 400;
    double graph_width = width_ - left_margin - right_margin;
    double x = left_margin;
    double y = height_ - row_height - top_margin;

    std::vector<std::uint32_t> vals;

    std::vector<GLfloat> coords;

    vals.resize(reader_.queues()[0].size());
    glColor4f(0.5, 0.5, 0.5, 0.5);
    draw_plot(x, y, vals, min, max, row_height, graph_width, coords);

    auto top_line = vals;
    for (auto & v : top_line) v = max;
    glColor4f(0.5, 0.5, 0.5, 0.5);
    draw_plot(x, y, top_line, min, max, row_height, graph_width, coords);

    std::size_t qidx = 0;
    std::size_t cidx = 0;
    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      if (vals.size() < queue.size()) {
        vals.resize(queue.size());
      }

      std::size_t idx = 0;
      for (auto val : queue) {
        vals[idx] += val;
        ++idx;
      }

      if (is_drawn_main_loop(queue.name())) {
        set_stacked_color(cidx);
        draw_plot(x, y, vals, min, max, row_height, graph_width, coords);
        ++cidx;
      }

      ++qidx;
    });

    auto lmt_line = vals;
    for (auto & v : lmt_line) v = 16667;
    glColor4f(0.4, 0.4, 0.6, 0.5);
    draw_plot(x, y, lmt_line, min, max, row_height, graph_width, coords);

    cidx = 0;
    y = height_ - row_height - top_margin - row_margin;
    row_height = 25;
    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      if (is_drawn_main_loop(queue.name())) {
        set_stacked_color(cidx);
        font_.FaceSize(row_height * 0.8);
        std::string s(main_loop_label(queue.name()));
        auto pos = font_.Render(s.c_str(), -1, FTPoint(x, y, 0));
        // y -= row_height;
        x = pos.X() + column_margin;
        ++cidx;
      }
    });

    return true;
  }
};

int main(int argc, char * argv[]) {
  if (argc < 1) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  PerflogReader reader(argv[1]);
  PerflogViewerMini app(reader, 752, 450);

  while (!app.done()) {
    app.poll();
    app.redraw();
    app.refresh();
  }
}
