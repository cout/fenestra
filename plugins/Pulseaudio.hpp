#pragma once

#include "Plugin.hpp"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <stdexcept>
#include <sstream>

namespace fenestra {

class Pulseaudio
  : public Plugin
{
public:
  Pulseaudio(Config const & config)
    : config_(config)
  {
  }

  ~Pulseaudio() {
    if (s_) {
      pa_simple_free(s_);
    }
  }

  virtual void set_sample_rate(int sample_rate) override {
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 2;
    ss.rate = sample_rate;

    int err;
    s_ = pa_simple_new(
        nullptr,            // default server
        "fenestra",         // application name
        PA_STREAM_PLAYBACK,
        nullptr,            // default device
        "fenestra",         // stream description
        &ss,                // sample format
        nullptr,            // default channel map
        nullptr,            // defualt buffering attributes
        &err);              // ignore error code

    if (!s_) {
      std::stringstream strm;
      strm << "pa_simple_new failed: " << pa_strerror(err);
      throw std::runtime_error(strm.str());
    }
  }

  virtual void write_audio_sample(void const * buf, std::size_t frames) override {
    auto bytes = frames * sizeof(std::int16_t) * 2;
    int err = 0;
    if (pa_simple_write(s_, buf, bytes, &err)) {
      std::cout << "pa_simple_write failed: " << pa_strerror(err) << std::endl;
    }
  }

private:
  Config const & config_;
  pa_simple * s_;
};

}
