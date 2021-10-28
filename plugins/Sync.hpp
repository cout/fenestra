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
    , adaptive_sync_(config.fetch<bool>("sync.adaptive", true))
    , glfinish_draw_(config.fetch<bool>("sync.glfinish_draw", false))
    , glfinish_sync_(config.fetch<bool>("sync.glfinish_sync", false))
    , oml_sync_(config.fetch<bool>("sync.oml_sync", false))
    , sgi_sync_(config.fetch<bool>("sync.sgi_sync", false))
    , fence_sync_(config.fetch<bool>("sync.fence_sync", true))
    , probe_(&dummy_probe_)
  {
  }

  virtual void start_metrics(Probe & probe, Probe::Dictionary & dictionary) {
    probe_ = &probe;
    swap_key_ = dictionary["Swap buffers"];
    sync_key_ = dictionary["Sync"];
    glfinish_draw_key_ = dictionary["Glfinish draw"];
    glfinish_sync_key_ = dictionary["Glfinish sync"];
  }

  virtual void window_created() override {
    if (vsync_ && adaptive_sync_) {
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
    glfwSwapInterval(vsync_ ? 1 : 0);
  }

  virtual void video_rendered() override {
    if (fence_sync_) {
      fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
  }

  virtual void pre_frame_delay() override {
    if (oml_sync_) {
      glXGetSyncValuesOML(glXGetCurrentDisplay(), glXGetCurrentDrawable(), &ust_, &msc_, &sbc_);
    } else if (sgi_sync_) {
      if (vsc_ == 0 || !adaptive_sync_) {
        glXGetVideoSyncSGI(&vsc_);
      }
    }
  }

  virtual void window_refresh(State & state) override {
    if (fence_sync_) {
      glClientWaitSync(fence_, 0, 16700000);
    }

    if (glfinish_draw_) {
      probe_->mark(glfinish_draw_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      glFinish();
      probe_->mark(glfinish_draw_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    }

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
      if (adaptive_sync_) {
        unsigned int vsc = vsc_;
        glXGetVideoSyncSGI(&vsc);
        state.synchronized = vsc < vsc_ + 1;
        if (state.synchronized) {
          glXWaitVideoSyncSGI(2, 1 - (vsc_ & 1), &vsc);
        }
        vsc_ = vsc;
      } else {
        glXWaitVideoSyncSGI(2, 1 - (vsc_ & 1), &vsc_);
      }
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

    if (glfinish_sync_) {
      probe_->mark(glfinish_sync_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
      glFinish();
      probe_->mark(glfinish_sync_key_, Probe::END, 1, Clock::gettime(CLOCK_MONOTONIC));
    }
  }

private:
  bool const & vsync_;
  bool const & adaptive_sync_;
  bool const & glfinish_draw_;
  bool const & glfinish_sync_;
  bool const & oml_sync_;
  bool const & sgi_sync_;
  bool const & fence_sync_;

  Probe::Key swap_key_;
  Probe::Key sync_key_;
  Probe::Key glfinish_draw_key_;
  Probe::Key glfinish_sync_key_;
  Probe dummy_probe_;
  Probe * probe_;

  std::int64_t ust_ = 0;
  std::int64_t msc_ = 0;
  std::int64_t sbc_ = 0;
  unsigned int vsc_ = 0;
  GLsync fence_ = 0;
};

}
