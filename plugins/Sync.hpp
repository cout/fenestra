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
    , delay_with_fence_(config.fetch<bool>("sync.delay.fence", false))
    , delay_with_glfinish_(config.fetch<bool>("sync.delay.glfinish", false))
    , delay_with_query_object_(config.fetch<bool>("sync.delay.query_object", false))
    , delay_with_nanosleep_(config.fetch<bool>("sync.delay.nanosleep", false))
    , delay_margin_(config.fetch<float>("sync.delay.magin", 2.0f))
    , glfinish_sync_(config.fetch<bool>("sync.glfinish_sync", false))
    , oml_sync_(config.fetch<bool>("sync.oml_sync", false))
    , sgi_sync_(config.fetch<bool>("sync.sgi_sync", false))
    , probe_(&dummy_probe_)
  {
  }

  ~Sync() {
    if (query_id_ != 0) {
      glDeleteQueries(1, &query_id_);
    }
  }

  virtual void start_metrics(Probe & probe, Probe::Dictionary & dictionary) {
    probe_ = &probe;
    sync_key_ = dictionary["Sync"];
    glfinish_sync_key_ = dictionary["Glfinish sync"];
  }

  virtual void window_created() override {
    if (delay_with_query_object_) {
      glGenQueries(1, &query_id_);
      if (query_id_ == 0) {
        throw std::runtime_error("glGenQueries failed");
      }
    }

    if (sgi_sync_) {
      if (!epoxy_has_glx_extension(glXGetCurrentDisplay(), 0, "GLX_SGI_video_sync")) {
        sgi_sync_ = false;
        if (vsync_) {
          std::cout << "GLX_SGI_video_sync not found; not using sgi_sync" << std::endl;
        } else {
          vsync_ = true;
          std::cout << "GLX_SGI_video_sync not found; using vsync instead" << std::endl;
        }
      }
    }

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
    if (delay_with_fence_) {
      if (fence_) glDeleteSync(fence_);
      fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      glFlush();
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

  virtual void window_update_delay() override {
    auto next_sync_time = last_sync_time_ + Milliseconds(16.667);
    auto delay_time = next_sync_time - Milliseconds(delay_margin_);

    if (delay_with_fence_) {
      auto now = Clock::gettime(CLOCK_MONOTONIC);
      auto duration = delay_time > now ? (delay_time - now).count() : 0;
      glClientWaitSync(fence_, 0, duration);
    }

    if (delay_with_glfinish_) {
      glFinish();
    }

    if (delay_with_query_object_) {
      glQueryCounter(query_id_, GL_TIMESTAMP);

      GLint available = 0;
      glGetQueryObjectiv(query_id_, GL_QUERY_RESULT_AVAILABLE, &available);
      while (!available && Clock::gettime(CLOCK_MONOTONIC) < delay_time) {
        Clock::nanosleep(Milliseconds(0.1), CLOCK_MONOTONIC);
        glGetQueryObjectiv(query_id_, GL_QUERY_RESULT_AVAILABLE, &available);
      }

      GLint64 timestamp;
      glGetQueryObjecti64v(query_id_, GL_QUERY_RESULT, &timestamp);
    }

    if (delay_with_nanosleep_) {
      glFlush();
      if (synchronized_) {
        Clock::nanosleep_until(delay_time, CLOCK_MONOTONIC);
      }
    }
  }

  virtual void window_update() override {
    if (oml_sync_) {
      // TODO: I do not have hardware to test this on; it should
      // probably be updated to work similar to glXGetVideoSyncSGI with
      // adaptive_sync_.
      glXSwapBuffersMscOML(glXGetCurrentDisplay(), glXGetCurrentDrawable(), msc_ + 1, 0, 0);
    } else if (sgi_sync_) {
      glXSwapBuffers(glXGetCurrentDisplay(), glXGetCurrentDrawable());
    } else {
      // TODO: This should be glfwSwapBuffers for maximum compatibility,
      // but we need access to the Window object
      glXSwapBuffers(glXGetCurrentDisplay(), glXGetCurrentDrawable());
    }
  }

  virtual void window_sync(State & state) override {
    if (sgi_sync_) {
      probe_->mark(sync_key_, Probe::START, 1, Clock::gettime(CLOCK_MONOTONIC));
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

    last_sync_time_ = Clock::gettime(CLOCK_MONOTONIC);
    synchronized_ = state.synchronized;
  }

private:
  bool & vsync_;
  bool const & adaptive_sync_;
  bool const & delay_with_fence_;
  bool const & delay_with_glfinish_;
  bool const & delay_with_query_object_;
  bool const & delay_with_nanosleep_;
  float const & delay_margin_;
  bool const & glfinish_sync_;
  bool const & oml_sync_;
  bool & sgi_sync_;

  Probe::Key sync_key_;
  Probe::Key glfinish_sync_key_;
  Probe dummy_probe_;
  Probe * probe_;

  std::int64_t ust_ = 0;
  std::int64_t msc_ = 0;
  std::int64_t sbc_ = 0;
  unsigned int vsc_ = 0;
  GLsync fence_ = 0;
  GLuint query_id_ = 0;

  Timestamp last_sync_time_ = Timestamp::zero();
  bool synchronized_ = true;
};

}
