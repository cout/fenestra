#pragma once

#include "Frontend.hpp"
#include "Clock.hpp"
#include "Context.hpp"
#include "Perf.hpp"
#include "Netcmds.hpp"

#include <GL/gl.h>

namespace fenestra {

class Loop {
public:
  Loop(Config const & config, Frontend & frontend, Context & ctx, Netcmds & netcmds)
    : config_(config)
    , frontend_(frontend)
    , ctx_(ctx)
    , netcmds_(netcmds)
    , perf_()
  {
  }

  void run() {
    while (!frontend_.window().done()) {
      try {
        perf_.mark_sync_savefile();
        ctx_.sync_savefile();
      } catch(...) {
        std::cout << "error syncing savefile" << std::endl;
      }

      // TODO: mark capture
      frontend_.capture().render();

      try {
        perf_.mark_poll_window_events();
        frontend_.window().poll_events();
      } catch(...) {
        std::cout << "error polling window events" << std::endl;
      }

      // TODO: mark read netcmds
      try {
        netcmds_.poll();
      } catch(...) {
        std::cout << "error polling network" << std::endl;
      }

      try {
        perf_.mark_frame_delay();
        frontend_.window().frame_delay();
      } catch(...) {
        std::cout << "error in frame delay" << std::endl;
      }

      try {
        perf_.mark_core_run();
        if (!frontend_.window().paused()) {
          ctx_.run_core();
        }
      } catch(...) {
        std::cout << "error running core" << std::endl;
      }

      try {
        perf_.mark_video_render();
        frontend_.video_render();
      } catch(...) {
        std::cout << "error rendering video" << std::endl;
      }

      try {
        perf_.mark_window_refresh();
        frontend_.window().refresh();

        perf_.mark_glfinish();
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
  Netcmds & netcmds_;
  Perf perf_;
};

}
