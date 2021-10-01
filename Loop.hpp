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
  Loop(Frontend & frontend, Context & ctx)
    : frontend_(frontend)
    , ctx_(ctx)
    , perf_()
  {
  }

  template <typename Fn>
  void step(Probe & probe, Probe::Key key, Fn && fn) {
    try {
      auto now = Clock::gettime(CLOCK_MONOTONIC);
      probe.mark(key, 0, now);
      std::forward<Fn>(fn)();
    } catch(...) {
      std::cout << "error during " << probe_names_[key] << std::endl;
    }
  }

  void run_core() {
    if (!frontend_.window().paused()) {
      ctx_.run_core();
    }
  }

  void record_probe(Probe const & probe) {
    Perf::PerfID next_id;
    Timestamp last_timestamp;

    for (auto const & stamp : probe) {
      if (last_timestamp != Timestamp()) {
        auto delta = stamp.time - last_timestamp;
        auto & counter = perf_.get_counter(next_id);
        counter.record(delta);
      }

      next_id = probe_key_to_counter_id_[stamp.key];
      last_timestamp = stamp.time;
    }
  }

  void run() {
    auto perf_metrics_key = probe_names_.emplace(0, "Perf metrics").first->first;
    auto sync_savefile_key = probe_names_.emplace(1, "Sync savefile").first->first;
    auto pre_frame_delay_key = probe_names_.emplace(2, "Pre frame delay").first->first;
    auto poll_window_events_key = probe_names_.emplace(3, "Poll window events").first->first;
    auto frame_delay_key = probe_names_.emplace(4, "Frame delay").first->first;
    auto core_run_key = probe_names_.emplace(5, "Core run").first->first;
    auto video_render_key = probe_names_.emplace(6, "Video render").first->first;
    auto window_refresh_key = probe_names_.emplace(7, "Window refresh").first->first;
    auto glfinish_key = probe_names_.emplace(8, "Glfinish").first->first;

    for (auto const & [ key, name ] : probe_names_) {
      probe_key_to_counter_id_[key] = perf_.add_counter(name);
    }

    Probe probe;

    while (!frontend_.window().done()) {
      step(probe, sync_savefile_key,      [&] { ctx_.sync_savefile();             });
      step(probe, pre_frame_delay_key,    [&] { frontend_.pre_frame_delay();      });
      step(probe, poll_window_events_key, [&] { frontend_.window().poll_events(); });
      step(probe, frame_delay_key,        [&] { frontend_.window().frame_delay(); });
      step(probe, core_run_key,           [&] { run_core();                       });
      step(probe, video_render_key,       [&] { frontend_.video_render();         });
      step(probe, window_refresh_key,     [&] { frontend_.window().refresh();     });
      step(probe, glfinish_key,           [&] { frontend_.window().glfinish();    });

      auto perf_metrics_time = Clock::gettime(CLOCK_MONOTONIC);

      record_probe(probe);

      perf_.loop_done(perf_metrics_time);

      probe.clear();
      probe.mark(perf_metrics_key, 0, perf_metrics_time);
    }
  }

private:
  Frontend & frontend_;
  Context & ctx_;
  std::map<Probe::Key, std::string> probe_names_;
  std::map<Probe::Key, Perf::PerfID> probe_key_to_counter_id_;
  Perf perf_;
};

}
