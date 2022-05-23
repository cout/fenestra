#include "PerflogViewer.hpp"
#include "PerflogReader.hpp"

#include <iostream>
#include <string>
#include <sstream>

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

bool is_latency(std::string const & name) {
  return (
         name == "Input latency"
      || name == "Render latency"
      || name == "Sync latency");
}

std::string latency_label(std::string const & name) {
  return name;
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

    auto num_queues = reader_.queues().size();

    if (num_queues == 0) {
      return false;
    }

    std::vector<PerflogReader::PerfQueue const *> main_loop_queues;
    std::size_t num_main_loop_queues = 0;

    std::size_t max_main_loop_queue_size = 0;
    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      max_main_loop_queue_size = std::max(max_main_loop_queue_size, queue.size());
      ++num_main_loop_queues;
      main_loop_queues.push_back(&queue);
    });
    
    auto main_loop_names = this->main_loop_names();
    auto main_loop_vals = this->main_loop_vals();
    auto main_loop_stacked_vals = this->main_loop_stacked_vals(main_loop_vals);

    double graph_height = 250; // 166.67 * 1.5

    double next_x;

    auto row_height = graph_height / (main_loop_names.size() + 1); // TODO TODO TODO: Why do I have to add 1 here???

    next_x = draw_main_loop_labels(
        left_margin,
        height_ - top_margin,
        row_height,
        main_loop_names,
        main_loop_vals);

    draw_latency_labels(
        left_margin,
        height_ - top_margin - graph_height, // TODO: row margin?
        row_height); // TODO: should use a different row height for this graph (but if I do it might look inconsistent)

    next_x = draw_main_loop_averages(
        next_x + column_margin,
        height_ - top_margin,
        row_height,
        main_loop_vals);


    auto graph_width = width_ - (next_x + column_margin) - right_margin;

    draw_main_loop_plot(
        next_x + column_margin,
        height_ - top_margin,
        graph_width,
        graph_height,
        0,
        16667,
        main_loop_stacked_vals);

    draw_latency_plot(
        next_x + column_margin,
        height_ - top_margin - graph_height, // TODO: shouldn't this include row margin to get spacing between the two graphs?
        graph_width,
        100,
        0, // TODO
        16667); // TODO

    return true;
  }

  double draw_main_loop_labels(double x, double y, double row_height, std::vector<std::string> const & main_loop_names, std::vector<std::vector<std::uint32_t>> const & main_loop_vals) {
    auto next_x = x;
    auto cidx = main_loop_names.size() - 1;
    auto text_height = 25 * 0.8;
    y -= text_height * 2;

    font_.FaceSize(text_height);

    std::size_t idx = main_loop_vals.size() - 1;
    for (auto it = main_loop_names.rbegin(); it != main_loop_names.rend(); ++it) {
      auto const & name = *it;

      std::uint32_t max = 0;
      std::uint32_t min = std::numeric_limits<std::uint32_t>::max();
      std::uint32_t total = 0;
      for (auto val : main_loop_vals[idx]) {
        min = std::min(val, min);
        max = std::max(val, max);
        total += val;
      }
      auto avg = total / main_loop_vals[idx].size();

      std::stringstream strm;
      strm << main_loop_label(name)
        // << " (" << avg / 1000.0
        // << ", " << min / 1000.0
        // << "-" << max / 1000.0
        // << ")"
        ;
      auto const & label = strm.str();

      set_stacked_color(cidx);
      // auto pos = font_.Render("█ ", -1, FTPoint(x, y, 0));
      // auto pos = font_.Render("■ ", -1, FTPoint(x, y, 0));
      // auto pos = font_.Render("▐▌ ", -1, FTPoint(x, y, 0));
      auto pos = font_.Render("▐█ ", -1, FTPoint(x, y, 0));
      glColor4f(1.0, 1.0, 1.0, 1.0);
      pos = font_.Render(label.c_str(), -1, pos);
      y -= row_height;
      next_x = std::max(pos.X(), next_x);
      --cidx;
      --idx;
    }

    return next_x;
  }

  double draw_main_loop_averages(double x, double y, double row_height, std::vector<std::vector<std::uint32_t>> const & main_loop_vals) {
    auto text_height = 25 * 0.8;
    y -= text_height * 2;

    font_.FaceSize(text_height);

    double next_x = 0;
    for (auto it = main_loop_vals.rbegin(); it != main_loop_vals.rend(); ++it) {
      auto const & vals = *it;

      std::uint64_t total = 0;
      for (auto val : vals) total += val;
      auto avg = total / vals.size();

      std::stringstream strm;
      strm << std::fixed << std::setprecision(2);
      strm << avg / 1000.0;
      auto pos = font_.Render(strm.str().c_str(), -1, FTPoint(x, y, 0));
      next_x = std::max(next_x, pos.X());
      y -= row_height;
    }

    return next_x;
  }

  std::vector<std::string>
  main_loop_names() {
    std::vector<std::string> main_loop_names;

    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      if (is_drawn_main_loop(queue.name())) {
        main_loop_names.push_back(queue.name());
      }
    });

    return main_loop_names;
  }

  std::vector<std::vector<std::uint32_t>>
  main_loop_vals() {
    std::vector<std::vector<std::uint32_t>> main_loop_vals;

    std::vector<std::uint32_t> vals;
    for_each_main_loop_queue(reader_.queues(), [&](auto const & queue) {
      if (vals.size() < queue.size()) {
        vals.resize(queue.size());
      }

      std::size_t i = 0;
      for (auto val : queue) {
        vals[i] += val;
        ++i;
      }

      if (is_drawn_main_loop(queue.name())) {
        main_loop_vals.push_back(vals);
        for (auto & val : vals) val = 0;
      }
    });

    return main_loop_vals;
  }

  std::vector<std::vector<std::uint32_t>>
  main_loop_stacked_vals(std::vector<std::vector<std::uint32_t>> const & main_loop_vals) {
    std::vector<std::vector<std::uint32_t>> main_loop_stacked_vals;

    std::size_t idx = 0;
    std::vector<std::uint32_t> vals;
    for (auto const & queue_vals : main_loop_vals) {
      if (vals.size() < queue_vals.size()) {
        vals.resize(queue_vals.size());
      }

      std::size_t i = 0;
      for (auto val : queue_vals) {
        vals[i] += val;
        ++i;
      }

      main_loop_stacked_vals.push_back(vals);
      ++idx;
    }

    return main_loop_stacked_vals;
  }

  void draw_main_loop_plot(double x, double y, double graph_width, double graph_height, std::uint32_t min, std::uint32_t max, std::vector<std::vector<std::uint32_t>> const & main_loop_stacked_vals) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    y -= graph_height;

    std::vector<std::uint32_t> vals;

    std::vector<GLfloat> coords;

    vals.resize(reader_.queues()[0].size());
    glColor4f(0.5, 0.5, 0.5, 0.5);
    draw_plot(x, y, vals, min, max, graph_height, graph_width, coords);

    auto top_line = vals;
    for (auto & v : top_line) v = max;
    glColor4f(0.5, 0.5, 0.5, 0.5);
    draw_plot(x, y, top_line, min, max, graph_height, graph_width, coords);

    glEnable(GL_BLEND);
    glBlendFunc(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
    glBlendColor(1.0f, 1.0f, 1.0f, 0.4f);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    auto cidx = main_loop_stacked_vals.size() - 1;
    for (auto it = main_loop_stacked_vals.rbegin(); it != main_loop_stacked_vals.rend(); ++it) {
      auto const & vals = *it;
      set_stacked_color(cidx);
      draw_plot_filled(x, y, vals, min, max, graph_height, graph_width, coords);
      --cidx;
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    cidx = main_loop_stacked_vals.size() - 1;
    for (auto it = main_loop_stacked_vals.rbegin(); it != main_loop_stacked_vals.rend(); ++it) {
      auto const & vals = *it;
      set_stacked_color(cidx);
      draw_plot(x, y, vals, min, max, graph_height, graph_width, coords);
      --cidx;
    }

    auto lmt_line = vals;
    for (auto & v : lmt_line) v = 16667;
    glColor4f(0.8, 0.2, 0.2, 0.5);
    draw_plot(x, y, lmt_line, min, max, graph_height, graph_width, coords);
  }

  template <typename Queue>
  void draw_plot_filled(double x, double y, Queue const & queue, std::uint32_t min, std::uint32_t max, double graph_height, double graph_width, std::vector<GLfloat> & coords) {
    std::size_t i = 0;
    auto num_points = queue.size();
    coords.resize(num_points * 4);
    for (auto val : queue) {
      assert(i * 2 + 1 < coords.size());
      auto pct = float(val - min) / max;
      assert(pct >= 0.0);
      // assert(pct <= 1.0);
      coords[i*4 + 0] = x + float(i) / num_points * graph_width;
      coords[i*4 + 1] = 0 * graph_height * 0.8 + y + 0.1 * graph_height;
      coords[i*4 + 2] = x + float(i) / num_points * graph_width;
      coords[i*4 + 3] = pct * graph_height * 0.8 + y + 0.1 * graph_height;
      ++i;
    }

    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glDrawArrays(GL_QUAD_STRIP, 0, coords.size() / 2);
  }

  double draw_latency_labels(double x, double y, double row_height) {
    std::vector<std::string> latency_names;
    std::vector<std::vector<std::uint32_t>> latency_vals;

    for (auto const & queue : reader_.queues()) {
      if (is_latency(queue.name())) {
        latency_names.push_back(queue.name());
        latency_vals.emplace_back(queue.begin(), queue.end());
      }
    }

    // TODO: This is mostly the same as draw_main_loop_labels from here
    // on

    auto next_x = x;
    auto cidx = latency_names.size() - 1;
    auto text_height = 25 * 0.8;
    y -= text_height;

    font_.FaceSize(text_height);

    for (auto it = latency_names.rbegin(); it != latency_names.rend(); ++it) {
      auto const & name = *it;

      std::stringstream strm;
      strm << latency_label(name);
      auto const & label = strm.str();

      set_stacked_color(cidx);
      // auto pos = font_.Render("█ ", -1, FTPoint(x, y, 0));
      // auto pos = font_.Render("■ ", -1, FTPoint(x, y, 0));
      // auto pos = font_.Render("▐▌ ", -1, FTPoint(x, y, 0));
      auto pos = font_.Render("▐█ ", -1, FTPoint(x, y, 0));
      glColor4f(1.0, 1.0, 1.0, 1.0);
      pos = font_.Render(label.c_str(), -1, pos);
      y -= row_height;
      next_x = std::max(pos.X(), next_x);
      --cidx;
    }

    return next_x;
  }

  void draw_latency_plot(double x, double y, double graph_width, double graph_height, double min, double max) {
    y -= graph_height;

    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    std::vector<GLfloat> coords;

    coords.resize(8);
    coords[0] = x;
    coords[1] = y;
    coords[2] = x + graph_width;
    coords[3] = y;
    coords[4] = x;
    coords[5] = y + graph_height;
    coords[6] = x + graph_width;
    coords[7] = y + graph_height;

    glEnable(GL_BLEND);
    glBlendFunc(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
    glBlendColor(1.0f, 1.0f, 1.0f, 0.4f);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glColor4ub(68, 1, 84, 100);
    glVertexPointer(2, GL_FLOAT, 0, coords.data());
    glDrawArrays(GL_QUAD_STRIP, 0, coords.size() / 2);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    auto cidx = 0;
    for (auto const & queue : reader_.queues()) {
      if (is_latency(queue.name())) {
        set_stacked_color(cidx);
        draw_plot(x, y, queue, min, max, graph_height, graph_width, coords);
        ++cidx;
      }
    }
  }

};

int main(int argc, char * argv[]) {
  if (argc < 1) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  PerflogReader reader(argv[1]);
  PerflogViewerMini app(reader, 752, 336);

  while (!app.done()) {
    app.poll();
    app.redraw();
    app.refresh();
  }
}
