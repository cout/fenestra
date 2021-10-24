#pragma once

#include "Frontend.hpp"
#include "Clock.hpp"
#include "Context.hpp"
#include "Probe.hpp"

#include <utility>
#include <map>

namespace fenestra {

class Loop {
public:
  Loop(Frontend & frontend, Context & ctx)
    : frontend_(frontend)
    , ctx_(ctx)
  {
  }

  template <typename Fn>
  void step(Probe & probe, Probe::Key key, Fn && fn) {
    try {
      auto now = Clock::gettime(CLOCK_MONOTONIC);
      probe.mark(key, 0, now);
      std::forward<Fn>(fn)();
    } catch(...) {
      std::cout << "error during " << frontend_.probe_dict()[key] << std::endl;
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
    auto final_key = frontend_.probe_dict()["---"];
    auto perf_metrics_key = frontend_.probe_dict()["Perf metrics"];
    auto pre_frame_delay_key = frontend_.probe_dict()["Pre frame delay"];
    auto poll_window_events_key = frontend_.probe_dict()["Poll window events"];
    auto frame_delay_key = frontend_.probe_dict()["Frame delay"];
    auto core_run_key = frontend_.probe_dict()["Core run"];
    auto video_render_key = frontend_.probe_dict()["Video render"];
    auto window_refresh_key = frontend_.probe_dict()["Window refresh"];

    Probe probe;

    // Make sure this is always the first probe, otherwise we will show
    // the timing for the previous perf_record as if it were the one
    // that happened in this loop iteration.
    probe.mark(perf_metrics_key, 0, Clock::gettime(CLOCK_MONOTONIC));

    while (!frontend_.window().done()) {
      step(probe, pre_frame_delay_key,    [&] { frontend_.pre_frame_delay();      });
      step(probe, poll_window_events_key, [&] { frontend_.window().poll_events(); });
      step(probe, frame_delay_key,        [&] { frontend_.window().frame_delay(); });
      step(probe, core_run_key,           [&] { run_core(probe);                  });
      step(probe, video_render_key,       [&] { frontend_.video_render();         });
      step(probe, window_refresh_key,     [&] { frontend_.window_refresh();        });

      auto perf_metrics_start_time = Clock::gettime(CLOCK_MONOTONIC);
      probe.mark(final_key, Probe::FINAL, 0, perf_metrics_start_time);
      frontend_.collect_metrics(probe);
      frontend_.record_probe(probe);
      probe.clear();
      probe.mark(perf_metrics_key, 0, perf_metrics_start_time);
    }
  }

private:
  Frontend & frontend_;
  Context & ctx_;
};

}
