#pragma once

#include "fenestra/Plugin.hpp"
#include "fenestra/DL.hpp"

namespace fenestra {

class Screensaver
  : public Plugin
{
public:
  Screensaver(Config::Subtree const & config)
    : xresetscreensaver_(config.fetch<bool>("inhibit.xresetscreensaver", true))
    , dl_(RTLD_LAZY)
  {
    XResetScreenSaver_ = dl_.sym<decltype(XResetScreenSaver_)>("XResetScreenSaver", false);

    if (xresetscreensaver_ && !XResetScreenSaver_) {
      xresetscreensaver_ = false;
    }
  }

  virtual void pre_frame_delay(State const & state) override {
    if (!state.paused) {
      reset_screensaver();
    }
  }

private:
  void reset_screensaver() {
    if (xresetscreensaver_) {
      auto * dpy = glXGetCurrentDisplay();
      if (dpy) {
	XResetScreenSaver_(dpy);
      }
    }
  }

private:
  bool & xresetscreensaver_;
  DL dl_;
  void (*XResetScreenSaver_)(void *);
};

}
