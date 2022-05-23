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
  // Viridis
  switch(idx) {
    case 0: glColor4f(253/255.0, 231/255.0, 37/255.0, 1.0); break;
    case 1: glColor4f(144/255.0, 215/255.0, 67/255.0, 1.0); break;
    case 2: glColor4f(53/255.0, 183/255.0, 121/255.0, 1.0); break;
    case 3: glColor4f(33/255.0, 145/255.0, 140/255.0, 1.0); break;
    case 4: glColor4f(49/255.0, 104/255.0, 142/255.0, 1.0); break;
    case 5: glColor4f(68/255.0, 57/255.0, 131/255.0, 1.0); break;
    default: glColor4f(68/255.0, 1/255.0, 84/255.0, 1.0); break;
  }

  // learnui
  /*
  switch(idx) {
    case 0: glColor4ub(0x00, 0x3f, 0x5c, 0xff); break;
    case 1: glColor4ub(0x37, 0x4c, 0x80, 0xff); break;
    case 2: glColor4ub(0x7a, 0x51, 0x95, 0xff); break;
    case 3: glColor4ub(0xbc, 0x50, 0x90, 0xff); break;
    case 4: glColor4ub(0xef, 0x56, 0x75, 0xff); break;
    case 5: glColor4ub(0xff, 0x76, 0x4a, 0xff); break;
    default: glColor4ub(0xff, 0xa6, 0x00, 0xff); break;
  }
  */

  // Retro Metro
  /*
  switch(idx) {
    case 0: glColor4ub(0xea, 0x55, 0x45, 0xff); break;
    case 1: glColor4ub(0xf4, 0x6a, 0x9b, 0xff); break;
    case 2: glColor4ub(0xef, 0x9b, 0x20, 0xff); break;
    case 3: glColor4ub(0xed, 0xbf, 0x33, 0xff); break;
    case 4: glColor4ub(0xed, 0xe1, 0x5b, 0xff); break;
    case 5: glColor4ub(0xbd, 0xcf, 0x32, 0xff); break;
    case 6: glColor4ub(0x87, 0xbc, 0x45, 0xff); break;
    case 7: glColor4ub(0x27, 0xae, 0xef, 0xff); break;
    default: glColor4ub(0xb3, 0x3d, 0xc6, 0xff); break;
  }
  */

  // Dutch Field
  /*
  switch(idx) {
    case 0: glColor4ub(0xe6, 0x00, 0x49, 0xff); break;
    case 1: glColor4ub(0x0b, 0xb4, 0xff, 0xff); break;
    case 2: glColor4ub(0x50, 0xe9, 0x91, 0xff); break;
    case 3: glColor4ub(0xe6, 0xd8, 0x00, 0xff); break;
    case 4: glColor4ub(0x9b, 0x19, 0xf5, 0xff); break;
    case 5: glColor4ub(0xff, 0xa3, 0x00, 0xff); break;
    case 6: glColor4ub(0xdc, 0x0a, 0xb4, 0xff); break;
    case 7: glColor4ub(0xb3, 0xd4, 0xff, 0xff); break;
    default: glColor4ub(0x00, 0xbf, 0xa0, 0xff); break;
  }
  */
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

    std::vector<PerflogReader::PerfQueue const *> main_loop_queues;
    std::size_t num_main_loop_queues = 0;

    std::size_t max_main_loop_queue_size = 0;
    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      max_main_loop_queue_size = std::max(max_main_loop_queue_size, queue.size());
      ++num_main_loop_queues;
      main_loop_queues.push_back(&queue);
    });
    
    for (std::size_t i = 0; i < max_main_loop_queue_size; ++i) {
      std::uint32_t sum_at_point = 0;
      for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
        sum_at_point += queue[i];
      });
      max = std::max(max, sum_at_point);
    }

    max = std::max(max, std::uint32_t(16667));
    max = std::min(max, std::uint32_t(20000));

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    double graph_height = 400;
    double x = left_margin;
    double row_height = graph_height / num_main_loop_queues;
    double y = height_ - row_height - top_margin - row_margin + row_margin/2;

    auto next_x = x;
    auto cidx = num_main_loop_queues - 2;
    auto text_height = 25 * 0.8;
    for (auto it = main_loop_queues.rbegin(); it != main_loop_queues.rend(); ++it) {
      auto const & queue = **it;
      if (is_drawn_main_loop(queue.name())) {
        set_stacked_color(cidx);
        font_.FaceSize(text_height);
        std::string s(main_loop_label(queue.name()));
        auto pos = font_.Render(s.c_str(), -1, FTPoint(x, y, 0));
        y -= row_height;
        next_x = std::max(pos.X(), next_x);
        --cidx;
      }
    }

    row_height = graph_height;
    auto graph_width = width_ - (next_x + column_margin) - right_margin;
    x = next_x + column_margin;
    y = height_ - row_height - top_margin;

    std::vector<std::uint32_t> vals;

    std::vector<GLfloat> coords;

    vals.resize(reader_.queues()[0].size());
    glColor4f(0.5, 0.5, 0.5, 0.5);
    draw_plot(x, y, vals, min, max, row_height, graph_width, coords);

    auto top_line = vals;
    for (auto & v : top_line) v = max;
    glColor4f(0.5, 0.5, 0.5, 0.5);
    draw_plot(x, y, top_line, min, max, row_height, graph_width, coords);

    std::vector<std::vector<std::uint32_t>> main_loop_vals;

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
        main_loop_vals.push_back(vals);
      }
    });

    glEnable(GL_BLEND);
    glBlendFunc(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
    glBlendColor(1.0f, 1.0f, 1.0f, 0.4f);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    cidx = main_loop_vals.size() - 1;
    for (auto it = main_loop_vals.rbegin(); it != main_loop_vals.rend(); ++it) {
      auto const & vals = *it;
      set_stacked_color(cidx);
      draw_plot_filled(x, y, vals, min, max, row_height, graph_width, coords);
      --cidx;
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    cidx = main_loop_vals.size() - 1;
    for (auto it = main_loop_vals.rbegin(); it != main_loop_vals.rend(); ++it) {
      auto const & vals = *it;
      set_stacked_color(cidx);
      draw_plot(x, y, vals, min, max, row_height, graph_width, coords);
      --cidx;
    }

    auto lmt_line = vals;
    for (auto & v : lmt_line) v = 16667;
    glColor4f(0.8, 0.2, 0.2, 0.5);
    draw_plot(x, y, lmt_line, min, max, row_height, graph_width, coords);

    return true;
  }

  template <typename Queue>
  void draw_plot_filled(double x, double y, Queue const & queue, std::uint32_t min, std::uint32_t max, double row_height, double graph_width, std::vector<GLfloat> & coords) {
    std::size_t i = 0;
    auto num_points = queue.size();
    coords.resize(num_points * 4);
    for (auto val : queue) {
      assert(i * 2 + 1 < coords.size());
      auto pct = float(val - min) / max;
      assert(pct >= 0.0);
      // assert(pct <= 1.0);
      coords[i*4 + 0] = x + float(i) / num_points * graph_width;
      coords[i*4 + 1] = 0 * row_height * 0.8 + y + 0.1 * row_height;
      coords[i*4 + 2] = x + float(i) / num_points * graph_width;
      coords[i*4 + 3] = pct * row_height * 0.8 + y + 0.1 * row_height;
      ++i;
    }

    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glDrawArrays(GL_QUAD_STRIP, 0, coords.size() / 2);
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
