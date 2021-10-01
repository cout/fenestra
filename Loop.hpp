#pragma once

#include "Frontend.hpp"
#include "Clock.hpp"
#include "Context.hpp"
#include "Perf.hpp"

namespace fenestra {

class Loop {
public:
  Loop(Frontend & frontend, Context & ctx)
    : frontend_(frontend)
    , ctx_(ctx)
    , perf_()
  {
  }

  template <typename Fn>
  auto step(Perf::PerfID id, Fn && fn) {
    return std::make_pair(id, std::forward<Fn>(fn));
  }

  template <typename Step>
  void run_step(Step && step) {
    try {
      perf_.mark_start(step.first);
      std::forward<typename Step::second_type>(step.second)();
    } catch(...) {
      std::cout << "error during " << perf_.get_counter(step.first).name() << std::endl;
    }
  }

  template <typename ... Steps>
  void run_steps(Steps && ... steps) {
    (run_step(std::forward<Steps>(steps)), ...);
  }

  void run_core() {
    if (!frontend_.window().paused()) {
      ctx_.run_core();
    }
  }

  void run() {
    auto sync_savefile_id = perf_.add_counter("Sync savefile");
    auto pre_frame_delay_id = perf_.add_counter("Pre-frame-delay");
    auto poll_window_events_id = perf_.add_counter("Poll window events");
    auto frame_delay_id = perf_.add_counter("Frame delay");
    auto core_run_id = perf_.add_counter("Core run");
    auto video_render_id = perf_.add_counter("Video render");
    auto window_refresh_id = perf_.add_counter("Window refresh");
    auto glfinish_id = perf_.add_counter("Glfinish");

    while (!frontend_.window().done()) {
      run_steps(
          step( sync_savefile_id,      [&] { ctx_.sync_savefile();             } ),
          step( pre_frame_delay_id,    [&] { frontend_.pre_frame_delay();      } ),
          step( poll_window_events_id, [&] { frontend_.window().poll_events(); } ),
          step( frame_delay_id,        [&] { frontend_.window().frame_delay(); } ),
          step( core_run_id,           [&] { run_core();                       } ),
          step( video_render_id,       [&] { frontend_.video_render();         } ),
          step( window_refresh_id,     [&] { frontend_.window().refresh();     } ),
          step( glfinish_id,           [&] { frontend_.window().glfinish();    } )
          );

      perf_.mark_loop_done();
    }
  }

private:
  Frontend & frontend_;
  Context & ctx_;
  Perf perf_;
};

}
