#pragma once

#include "Plugin.hpp"

namespace fenestra {

class Sync
  : public Plugin
{
public:
  Sync(Config const & config)
    : config_(config)
    , vsync_(config.fetch<bool>("sync.vsync", true))
    , adaptive_vsync_(config.fetch<bool>("sync.adaptive_vsync", true))
    , glfinish_(config.fetch<bool>("sync.glfinish", false))
    , oml_sync_(config.fetch<bool>("sync.oml_sync", false))
    , sgi_sync_(config.fetch<bool>("sync.sgi_sync", false))
  {
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
      glXSwapBuffersMscOML(glXGetCurrentDisplay(), glXGetCurrentDrawable(), msc_ + 1, 0, 0);
    } else if (sgi_sync_) {
      glXSwapBuffers(glXGetCurrentDisplay(), glXGetCurrentDrawable());
      glXWaitVideoSyncSGI(2, 1 - (vsc_ & 1), &vsc_);
    } else {
      // TODO: This should be glfwSwapBuffers for maximum compatibility,
      // but we need access to the Window object
      glXSwapBuffers(glXGetCurrentDisplay(), glXGetCurrentDrawable());
    }

    // TODO: The glFinish is required to make glXDelayBeforeSwapNV work
    // correctly, but it has other downsides, like occasionally making
    // us wait until the next frame if we miss the swap window.
    //
    // The documentation says we can do glFinish just before calling
    // glXDelayBeforeSwapNV, but that doesn't work (it always sleeps for
    // 15ms when we do that, regardles of what delay we pass in).
    glClear(GL_COLOR_BUFFER_BIT);
    if (glfinish_ || config_.nv_delay_before_swap()) {
      glFinish();
    }
  }

private:
  Config const & config_;
  bool const & vsync_;
  bool const & adaptive_vsync_;
  bool const & glfinish_;
  bool const & oml_sync_;
  bool const & sgi_sync_;

  std::int64_t ust_ = 0;
  std::int64_t msc_ = 0;
  std::int64_t sbc_ = 0;
  unsigned int vsc_ = 0;
};

}
