#pragma once

#include "Plugin.hpp"

#include <alsa/asoundlib.h>

#include <stdexcept>
#include <sstream>

namespace fenestra {

class ALSA
  : public Plugin
{
public:
  ALSA(Config const & config)
    : config_(config)
  {
  }

  ~ALSA() {
    if (pcm) {
      snd_pcm_close(pcm);
    }
  }

  virtual void set_sample_rate(int sample_rate) override {
    int err;

    std::string audio_device(config_.audio_device().data());
    int flags = 0;
    if (config_.audio_nonblock()) flags |= SND_PCM_NONBLOCK;
    if ((err = snd_pcm_open(&pcm, audio_device.c_str(), SND_PCM_STREAM_PLAYBACK, flags)) < 0) {
      std::stringstream strm;
      strm << "snd_pcm_open failed: " << snd_strerror(err);
      throw std::runtime_error(strm.str());
    }

    err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 2, sample_rate, 1, config_.audio_suggested_latency() * 1000);

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
  Config const & config_;
  snd_pcm_t * pcm = 0;
};

}
