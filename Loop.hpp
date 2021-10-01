#pragma once

#include "Frontend.hpp"
#include "Clock.hpp"
#include "Context.hpp"
#include "Perf.hpp"

#include <GL/gl.h>

namespace fenestra {

class Loop {
public:
  Loop(Config const & config, Frontend & frontend, Context & ctx)
    : config_(config)
    , frontend_(frontend)
    , ctx_(ctx)
    , perf_()
  {
  }

  void run() {
    auto sync_savefile_id = perf_.add_counter("Sync savefile");
    auto poll_window_events_id = perf_.add_counter("Poll window events");
    auto frame_delay_id = perf_.add_counter("Frame delay");
    auto core_run_id = perf_.add_counter("Core run");
    auto video_render_id = perf_.add_counter("Video render");
    auto window_refresh_id = perf_.add_counter("Window refresh");
    auto glfinish_id = perf_.add_counter("Glfinish");

    while (!frontend_.window().done()) {
      try {
        perf_.mark_start(sync_savefile_id);
        ctx_.sync_savefile();
      } catch(...) {
        std::cout << "error syncing savefile" << std::endl;
      }

      // TODO: mark
      frontend_.pre_frame_delay();

      try {
        perf_.mark_start(poll_window_events_id);
        frontend_.window().poll_events();
      } catch(...) {
        std::cout << "error polling window events" << std::endl;
      }

      try {
        perf_.mark_start(frame_delay_id);
        frontend_.window().frame_delay();
      } catch(...) {
        std::cout << "error in frame delay" << std::endl;
      }

      try {
        perf_.mark_start(core_run_id);
        if (!frontend_.window().paused()) {
          ctx_.run_core();
        }
      } catch(...) {
        std::cout << "error running core" << std::endl;
      }

      try {
        perf_.mark_start(video_render_id);
        frontend_.video_render();
      } catch(...) {
        std::cout << "error rendering video" << std::endl;
      }

      try {
        perf_.mark_start(window_refresh_id);
        frontend_.window().refresh();

        perf_.mark_start(glfinish_id);
        frontend_.window().glfinish();

      } catch(...) {
        std::cout << "error refreshing window" << std::endl;
      }

      perf_.mark_loop_done();
    }
  }

private:
  Config const & config_;
  Frontend & frontend_;
  Context & ctx_;
  Perf perf_;
};

}
