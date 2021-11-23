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
  void step(Probe & probe, std::optional<Probe::Depth> depth, Probe::Key key, Fn && fn) {
    try {
      auto now = Clock::gettime(CLOCK_MONOTONIC);
      if (depth) probe.mark(key, *depth, now);
      std::forward<Fn>(fn)();
    } catch(std::exception const & ex) {
      std::cout << "error during " << frontend_.probe_dict()[key] << ": " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "error during " << frontend_.probe_dict()[key] << ": unknown error" << std::endl;
    }
  }

  void run_core(Probe & probe) {
    if (!frontend_.paused()) {
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
    auto video_key = frontend_.probe_dict()["Video"];
    auto render_key = frontend_.probe_dict()["Render"];
    auto update_delay_key = frontend_.probe_dict()["Update delay"];
    auto update_key = frontend_.probe_dict()["Update"];
    auto sync_key = frontend_.probe_dict()["Sync"];

    Probe probe;
    frontend_.start_metrics(probe);

    // Make sure this is always the first probe, otherwise we will show
    // the timing for the previous perf_record as if it were the one
    // that happened in this loop iteration.
    probe.mark(perf_metrics_key, 0, Clock::gettime(CLOCK_MONOTONIC));

    while (!frontend_.done()) {
      step(probe, 0, pre_frame_delay_key,     [&] { frontend_.pre_frame_delay();     });
      step(probe, 0, poll_window_events_key,  [&] { frontend_.poll_window_events();  });
      step(probe, 0, frame_delay_key,         [&] { frontend_.frame_delay();         });
      step(probe, 0, core_run_key,            [&] { run_core(probe);                 });
      step(probe, 0, video_key, [&] {
        step(probe, 1, render_key,          [&] { frontend_.video_render();        });
        step(probe, 1, update_delay_key,    [&] { frontend_.window_update_delay(); });
        step(probe, 1, update_key,          [&] { frontend_.window_update();       });
        step(probe, std::nullopt, sync_key, [&] { frontend_.window_sync();         });
      });

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
