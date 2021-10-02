#pragma once

#include "Frontend.hpp"
#include "Clock.hpp"
#include "Context.hpp"
#include "Perf.hpp"
#include "Probe.hpp"

#include <utility>
#include <map>

namespace fenestra {

class Loop {
public:
  Loop(Frontend & frontend, Context & ctx, Perf & perf)
    : frontend_(frontend)
    , ctx_(ctx)
    , perf_(perf)
  {
  }

  template <typename Fn>
  void step(Probe & probe, Probe::Key key, Fn && fn) {
    try {
      auto now = Clock::gettime(CLOCK_MONOTONIC);
      probe.mark(key, 0, now);
      std::forward<Fn>(fn)();
    } catch(...) {
      std::cout << "error during " << perf_.probe_name(key) << std::endl;
    }
  }

  void run_core(Probe & probe) {
    if (!frontend_.window().paused()) {
      frontend_.probe().clear();
      ctx_.run_core();
      probe.append(frontend_.probe());
    }
  }

  void run() {
    auto perf_metrics_key = perf_.probe_key("Perf metrics");
    auto sync_savefile_key = perf_.probe_key("Sync savefile");
    auto pre_frame_delay_key = perf_.probe_key("Pre frame delay");
    auto poll_window_events_key = perf_.probe_key("Poll window events");
    auto frame_delay_key = perf_.probe_key("Frame delay");
    auto core_run_key = perf_.probe_key("Core run");
    auto video_render_key = perf_.probe_key("Video render");
    auto window_refresh_key = perf_.probe_key("Window refresh");
    auto glfinish_key = perf_.probe_key("Glfinish");

    Probe probe;

    while (!frontend_.window().done()) {
      step(probe, sync_savefile_key,      [&] { ctx_.sync_savefile();             });
      step(probe, pre_frame_delay_key,    [&] { frontend_.pre_frame_delay();      });
      step(probe, poll_window_events_key, [&] { frontend_.window().poll_events(); });
      step(probe, frame_delay_key,        [&] { frontend_.window().frame_delay(); });
      step(probe, core_run_key,           [&] { run_core(probe);                  });
      step(probe, video_render_key,       [&] { frontend_.video_render();         });
      step(probe, window_refresh_key,     [&] { frontend_.window().refresh();     });
      step(probe, glfinish_key,           [&] { frontend_.window().glfinish();    });

      auto perf_metrics_time = Clock::gettime(CLOCK_MONOTONIC);

      perf_.record_probe(perf_metrics_time, probe);

      perf_.loop_done(perf_metrics_time);

      probe.clear();
      probe.mark(perf_metrics_key, 0, perf_metrics_time);
    }
  }

private:
  Frontend & frontend_;
  Context & ctx_;
  Perf & perf_;
};

}
