#pragma once

#include "Plugin.hpp"

#include <epoxy/gl.h>

namespace fenestra {

class Sync
  : public Plugin
{
public:
  Sync(Config const & config)
    : vsync_(config.fetch<bool>("sync.vsync", true))
    , adaptive_vsync_(config.fetch<bool>("sync.adaptive_vsync", true))
    , glfinish_(config.fetch<bool>("sync.glfinish", false))
    , oml_sync_(config.fetch<bool>("sync.oml_sync", false))
    , sgi_sync_(config.fetch<bool>("sync.sgi_sync", false))
    , probe_(&dummy_probe_)
  {
  }

  virtual void start_metrics(Probe & probe, Probe::Dictionary & dictionary) {
    probe_ = &probe;
    swap_key_ = dictionary["Swap Buffers"];
    sync_key_ = dictionary["Sync"];
    glfinish_key_ = dictionary["Glfinish"];
  }

  virtual void window_created() override {
    if (adaptive_vsync_) {
      if (epoxy_has_glx_extension(glXGetCurrentDisplay(), 0, "GLX_EXT_swap_control_tear")) {
        glfwSwapInterval(-1);
        return;
      } else {
        std::cout << "GLX_EXT_swap_control_tear not found; not using adaptive vsync" << std::endl;
      }
    }

    // TODO: If vsync is off, we need another sync method, either a swap
    // rate or using audio sync.  The portaudio plugin does blocking, so
    // it will sync, but the pulseaudio plugin does not block, so it
    // will not sync.
    bool vsync = vsync_ || adaptive_vsync_;
    glfwSwapInterval(vsync ? 1 : 0);
  }

  virtual void pre_frame_delay() override {
    if (oml_sync_) {
      glXGetSyncValuesOML(glXGetCurrentDisplay(), glXGetCurrentDrawable(), &ust_, &msc_, &sbc_);
    } else if (sgi_sync_) {
      glXGetVideoSyncSGI(&vsc_);
    }
  }

  virtual void window_refresh() override {
    if (oml_sync_) {
      // TODO: Obviously this is not ideal.  One problem is we don't
      // know how long it takes to swap buffers, so we can't push the
      // frame delay too for or we might miss deadline for swapping even
      // if we attempt to swap before the vertical retrace.
      //
      // This also doesn't achieve the goal of preferring visual
      // artifacts over jitter.
      probe_->mark(swap_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      glXSwapBuffersMscOML(glXGetCurrentDisplay(), glXGetCurrentDrawable(), msc_ + 1, 0, 0);
      probe_->mark(swap_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    } else if (sgi_sync_) {
      probe_->mark(swap_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      glXSwapBuffers(glXGetCurrentDisplay(), glXGetCurrentDrawable());
      auto now = Clock::gettime(CLOCK_MONOTONIC);
      probe_->mark(swap_key_, Probe::END, 1, now);
      probe_->mark(sync_key_, Probe::START, 1, now);
      glXWaitVideoSyncSGI(2, 1 - (vsc_ & 1), &vsc_);
      probe_->mark(sync_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    } else {
      // TODO: This should be glfwSwapBuffers for maximum compatibility,
      // but we need access to the Window object
      probe_->mark(swap_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      glXSwapBuffers(glXGetCurrentDisplay(), glXGetCurrentDrawable());
      probe_->mark(swap_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    }

    // TODO: The glFinish is required to make glXDelayBeforeSwapNV work
    // correctly, but it has other downsides, like occasionally making
    // us wait until the next frame if we miss the swap window.
    //
    // The documentation says we can do glFinish just before calling
    // glXDelayBeforeSwapNV, but that doesn't work (it always sleeps for
    // 15ms when we do that, regardles of what delay we pass in).
    glClear(GL_COLOR_BUFFER_BIT);
    if (glfinish_) {
      probe_->mark(glfinish_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      glFinish();
      probe_->mark(glfinish_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    }
  }

private:
  bool const & vsync_;
  bool const & adaptive_vsync_;
  bool const & glfinish_;
  bool const & oml_sync_;
  bool const & sgi_sync_;

  Probe::Key swap_key_;
  Probe::Key sync_key_;
  Probe::Key glfinish_key_;
  Probe dummy_probe_;
  Probe * probe_;

  std::int64_t ust_ = 0;
  std::int64_t msc_ = 0;
  std::int64_t sbc_ = 0;
  unsigned int vsc_ = 0;
};

}