#pragma once

#include "Plugin.hpp"

#include <pulse/pulseaudio.h>

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
    if (stream_) {
      pa_stream_unref(stream_);
    }

    if (context_) {
      pa_context_unref(context_);
    }

    if (loop_) {
      pa_mainloop_free(loop_);
    }
  }

  virtual void set_sample_rate(int sample_rate) override {
    if (!(loop_ = pa_mainloop_new())) {
      throw std::runtime_error("pa_mainloop_new failed");
    }

    if (!(api_ = pa_mainloop_get_api(loop_))) {
      throw std::runtime_error("pa_mainloop_get_api failed");
    }

    auto const * client_name = "fenestra";
    if (!(context_ = pa_context_new(api_, client_name))) {
      throw std::runtime_error("pa_context_new failed");
    }

    pa_context_set_event_callback(context_, context_event_callback, this);
    pa_context_set_state_callback(context_, context_state_callback, this);

    auto server = nullptr; // TODO
    auto flags = PA_CONTEXT_NOFLAGS; // TODO
    if (pa_context_connect(context_, server, flags, nullptr) < 0) {
      throw std::runtime_error("pa_context_connect failed");
    }

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 2;
    ss.rate = sample_rate;
    if (!(stream_ = pa_stream_new(context_, "fenestra", &ss, nullptr))) {
      throw std::runtime_error("pa_stream_new failed");
    }

    pa_stream_set_state_callback(stream_, stream_state_callback, this);
  }

  virtual void pre_frame_delay() override {
    int ret;
    while (pa_mainloop_iterate(loop_, false, &ret) > 0) { }
  }

  virtual void write_audio_sample(void const * buf, std::size_t frames) override {
    if (!stream_ || !ready_) {
      return;
    }

    auto bytes = frames * sizeof(std::int16_t) * 2;
    int err;
    if ((err = pa_stream_write(stream_, buf, bytes, nullptr, 0, PA_SEEK_RELATIVE)) != 0) {
      std::cout << "pa_stream_write failed: " << pa_strerror(err) << std::endl;
    }
  }

private:
  static void context_event_callback(pa_context * c, char const * name, pa_proplist * p, void * userdata) {
    auto self = static_cast<Pulseaudio *>(userdata);
    return self->context_event_callback_(c, name, p);
  }
  
  void context_event_callback_(pa_context * c, char const * name, pa_proplist * p) {
  }

  static void context_state_callback(pa_context * c, void * userdata) {
    auto self = static_cast<Pulseaudio *>(userdata);
    return self->context_state_callback(c);
  }

  void context_state_callback(pa_context * c) {
    auto state = pa_context_get_state(c);
    std::cout << "context state: " << state << "; ready: " << ready_ << std::endl;

    if (state == PA_CONTEXT_READY) {
      std::cout << "connecting playback" << std::endl;
      int err;
      if ((err = pa_stream_connect_playback(stream_, nullptr, nullptr, pa_stream_flags(0), nullptr, nullptr)) < 0) {
        std::stringstream strm;
        strm << "pa_stream_connect_playback failed: " << pa_strerror(err);
        throw std::runtime_error(strm.str());
      }
    }
  }

  static void stream_state_callback(pa_stream * p, void * userdata) {
    auto self = static_cast<Pulseaudio *>(userdata);
    return self->stream_state_callback(p);
  }

  void stream_state_callback(pa_stream * p) {
    auto state = pa_stream_get_state(p);
    ready_ = state == PA_STREAM_READY;
    std::cout << "stream state: " << state << "; ready: " << ready_ << std::endl;
  }

private:
  Config const & config_;
  pa_mainloop * loop_ = nullptr;
  pa_mainloop_api * api_ = nullptr;
  pa_context * context_ = nullptr;
  pa_stream * stream_ = nullptr;
  bool ready_ = false;
};

}
