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
