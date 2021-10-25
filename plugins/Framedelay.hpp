#pragma once

#include "Plugin.hpp"

#include <epoxy/gl.h>

namespace fenestra {

class Framedelay
  : public Plugin
{
public:
  Framedelay(Config const & config)
    : frame_delay_(config.fetch<Milliseconds>("framedelay.milliseconds", Milliseconds(4)))
    , nv_delay_before_swap_(config.fetch<bool>("framedelay.nv_delay_before_swap", false))
  {
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
      auto & glfinish_sync = config.fetch<bool>("sync.glfinish_sync", false);
      glfinish_sync = true;
    }
  }

  virtual void frame_delay() override {
    if (nv_delay_before_swap_) {
      Seconds delay_before_swap = Milliseconds(16.7) - frame_delay_;
      glXDelayBeforeSwapNV(glXGetCurrentDisplay(), glXGetCurrentDrawable(), delay_before_swap.count());
    } else {
      Clock::nanosleep_until(last_refresh_ + frame_delay_, CLOCK_MONOTONIC);
    }
  }

  virtual void window_refreshed() override {
    // TODO: I do not understand why the glFinish call is needed here;
    // without it, sync latency shows as 15.8ms, but with it, sync
    // latency is close to 16.67ms - frame_delay.
    if (nv_delay_before_swap_) {
      glFinish();
    }

    last_refresh_ = Clock::gettime(CLOCK_MONOTONIC);
  }

private:
  Milliseconds const & frame_delay_;
  bool const & nv_delay_before_swap_;

  Timestamp last_refresh_;
};

}
