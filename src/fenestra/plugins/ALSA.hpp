#pragma once

#include "fenestra/Plugin.hpp"

#include <alsa/asoundlib.h>

#include <stdexcept>
#include <sstream>

namespace fenestra {

class ALSA
  : public Plugin
{
public:
  ALSA(Config const & config)
    : audio_device_(config.fetch<std::string>("alsa.audio_device", "default"))
    , audio_suggested_latency_(config.fetch<int>("alsa.audio_suggested_latency", 64))
    , audio_nonblock_(config.fetch<bool>("alsa.audio_nonblock", 64))
  {
  }

  ~ALSA() {
    if (pcm) {
      snd_pcm_close(pcm);
    }
  }

  virtual void set_sample_rate(double sample_rate, double adjusted_rate) override {
    int err;

    int flags = 0;
    if (audio_nonblock_) flags |= SND_PCM_NONBLOCK;
    if ((err = snd_pcm_open(&pcm, audio_device_.c_str(), SND_PCM_STREAM_PLAYBACK, flags)) < 0) {
      std::stringstream strm;
      strm << "snd_pcm_open failed: " << snd_strerror(err);
      throw std::runtime_error(strm.str());
    }

    err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 2, adjusted_rate, 1, audio_suggested_latency_ * 1000);

    if (err < 0) {
      std::stringstream strm;
      strm << "snd_pcm_set_params failed: " << snd_strerror(err);
      throw std::runtime_error(strm.str());
    }
  }

  virtual void write_audio_sample(void const * buf, std::size_t frames) override {
    int written = snd_pcm_writei(pcm, buf, frames);

    if (written < 0) {
      printf("Alsa warning/error #%i: ", -written);
      snd_pcm_recover(pcm, written, 0);
    }
  }

private:
  std::string const & audio_device_;
  int const & audio_suggested_latency_;
  bool const & audio_nonblock_;

  snd_pcm_t * pcm = 0;
};

}
