#pragma once

#include "fenestra/Plugin.hpp"

#include <epoxy/gl.h>

namespace fenestra {

class Framedelay
  : public Plugin
{
public:
  Framedelay(Config const & config)
    : frame_delay_(config.fetch<Milliseconds>("framedelay.milliseconds", Milliseconds(4)))
    , adaptive_(config.fetch<bool>("framedelay.adaptive", true))
    , nv_delay_before_swap_(config.fetch<bool>("framedelay.nv_delay_before_swap", false))
    , glfinish_sync_(config.fetch<bool>("sync.glfinish_sync", false))
  {
  }

  virtual void window_created() override {
    // We have to glfinish before capturing timing information,
    // (either before or after swap), otherwise sync latency will be 1
    // frame longer.
    //
    // TODO: The documentation says I should call glFinish just before
    // glXDelayBeforeSwapNV, but when I do that, frame delay always
    // takes 15ms, regardless of what frame delay I use.
    //
    // That's probably the correct behavior -- glFinish is delayed until
    // the last possible moment, and so is the swap, but:
    //   1. This means we can't tell the difference between a
    //      longer-than-normal frame delay and a longer-than-normal
    //      vsync
    //   2. It causes the sync latency measurement to be one frame
    //      longer, because we are no longer measuring sync at the vsync
    //      time; we are measuring it when the buffer swap is initiated.
    //      Since the frame has not yet been rendered at that time, the
    //      sync latency appears to be one frame longer than it actually
    //      is.
    //
    // For now, calling glFinish immediately after swap helps get better
    // measurements.  But it has some downsides:
    //   - If rendering takes longer than normal, then we'll block until
    //     the pipeline is empty.  But since we still have some work to
    //     do (up to 1ms on some frames, at least on my hardware), this
    //     will cause the event loop to stall longer than normal, and we
    //     risk missing the deadline for rendering the next frame.
    //   - If we miss vsync glFinish may stall even longer, and we'll
    //     end up a frame behind; this may or not be corrected by
    //     "adaptive" vsync.
    //
    // We can possibly reorder the event loop so Sync always comes
    // immediately before Frame delay.  This would be a more efficient
    // event loop, even when not using nv_delay_before_swap, because we
    // could initiate the buffer swap, do work, and then synchronize
    // when the work is done.  The problem is that this does not play
    // nicely with frame delay, because then there is no good way to
    // know when the vsync event happened -- so we end up overshooting
    // on the frame delay by the same amount of work that was done.
    //
    // Another option is to use a thread to wait for vsync; we can
    // initiate swap/sync, then do work, then find out later when the
    // sync completed.
    if (nv_delay_before_swap_) {
      if (epoxy_has_glx_extension(glXGetCurrentDisplay(), 0, "GLX_NV_delay_before_swap")) {
	glfinish_sync_ = true;
      } else {
        std::cout << "GLX_NV_delay_before_swap not found; not using nv_delay_before_swap" << std::endl;
	nv_delay_before_swap_ = false;
      }
    }
  }

  virtual void frame_delay() override {
    if (nv_delay_before_swap_) {
      Seconds delay_before_swap = Milliseconds(16.7) - frame_delay_;
      glXDelayBeforeSwapNV(glXGetCurrentDisplay(), glXGetCurrentDrawable(), delay_before_swap.count());
    } else {
      Clock::nanosleep_until(next_delay_time_, CLOCK_MONOTONIC);
    }
  }

  virtual void window_synced(State const & state) override {
    // TODO: I do not understand why the glFinish call is needed here;
    // without it, sync latency shows as 15.8ms, but with it, sync
    // latency is close to 16.67ms - frame_delay.
    if (nv_delay_before_swap_) {
      glFinish();
    }

    auto now = Clock::gettime(CLOCK_MONOTONIC);
    if (adaptive_) {
      next_delay_time_ = state.synchronized ? now + frame_delay_ : now;
    } else {
      next_delay_time_ = now + frame_delay_;
    }
    last_refresh_ = now;
  }

private:
  Milliseconds const & frame_delay_;
  bool const & adaptive_;
  bool & nv_delay_before_swap_;
  bool & glfinish_sync_;

  Timestamp last_refresh_ = Timestamp::zero();
  Timestamp next_delay_time_ = Timestamp::zero();
};

}
