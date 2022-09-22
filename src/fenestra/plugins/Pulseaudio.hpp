#pragma once

#include "fenestra/Plugin.hpp"
#include "fenestra/Clock.hpp"

#include <pulse/pulseaudio.h>

#include <stdexcept>
#include <sstream>

namespace fenestra {

class Pulseaudio
  : public Plugin
{
public:
  Pulseaudio(Config::Subtree const & config, std::string const & instance)
    : device_(config.fetch<std::string>("device", ""))
    , audio_maximum_latency_(config.fetch<int>("audio_maximum_latency", 64))
    , audio_suggested_latency_(config.fetch<int>("audio_suggested_latency", 64))
    , reconnect_(config.fetch<bool>("reconect", true))
    , instance_(instance)
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

  virtual void set_sample_rate(double sample_rate, double adjusted_rate) override {
    game_sample_rate_ = sample_rate;
    game_adjusted_rate_ = adjusted_rate;

    if (!(loop_ = pa_mainloop_new())) {
      throw std::runtime_error("pa_mainloop_new failed");
    }

    if (!(api_ = pa_mainloop_get_api(loop_))) {
      throw std::runtime_error("pa_mainloop_get_api failed");
    }

    connect_context();
  }

  void connect_context() {
    if (context_) {
      pa_context_unref(context_);
    }

    std::string client_name = "fenestra";
    if (instance_ != "") {
      client_name += ":";
      client_name += instance_;
    }

    if (!(context_ = pa_context_new(api_, client_name.c_str()))) {
      throw std::runtime_error("pa_context_new failed");
    }

    pa_context_set_event_callback(context_, context_event_callback, this);
    pa_context_set_state_callback(context_, context_state_callback, this);

    auto server = nullptr; // TODO
    auto flags = PA_CONTEXT_NOFAIL;
    if (pa_context_connect(context_, server, flags, nullptr) < 0) {
      throw std::runtime_error("pa_context_connect failed");
    }
  }

  virtual void pre_frame_delay(State const & state) override {
    auto now = Clock::gettime(CLOCK_REALTIME);
    if (next_reconnect_ != Timestamp() && now >= next_reconnect_) {
      try {
        // std::cout << "Reconnecting context..." << std::endl;
        connect_context();
        next_reconnect_ = Timestamp();
      } catch(...) {
        // std::cout << "Reconnect failed" << std::endl;
        if (reconnect_) {
          next_reconnect_ = now + reconnect_delay_;
        }
      }
    }

    int ret;
    int retval;
    while ((ret = pa_mainloop_iterate(loop_, false, &retval) > 0)) { }

    now = Clock::gettime(CLOCK_REALTIME);
    if (stream_ && now - last_update_ > Seconds(1)) {
      // dump_timing_info();
      // dump_latency();
      last_update_ = now;
    }
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

  virtual void collect_metrics(Probe & probe, Probe::Dictionary & dictionary) override {
    if (!overruns_key_) { overruns_key_ = dictionary["Audio Overruns" + metrics_suffix()]; }
    if (!underruns_key_) { underruns_key_ = dictionary["Audio Underruns" + metrics_suffix()]; }
    if (!latency_key_) { latency_key_ = dictionary["Audio Latency" + metrics_suffix()]; }

    pa_usec_t usec = 0;
    int negative;
    if (stream_ && pa_stream_get_latency(stream_, &usec, &negative) != 0) {
      usec = 0;
    }

    probe.meter(*overruns_key_, Probe::VALUE, 0, overruns_);
    probe.meter(*underruns_key_, Probe::VALUE, 0, underruns_);
    probe.meter(*latency_key_, Probe::VALUE, 0, usec / 1000);

    overruns_ = 0;
    underruns_ = 0;
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

    // std::cout << "Context state is now: " << c_str(state) << std::endl;

    if (state == PA_CONTEXT_READY) {
      std::string stream_name = "fenestra";
      if (instance_ != "") {
        stream_name += ":";
        stream_name += instance_;
      }

      if (stream_) {
        pa_stream_unref(stream_);
      }

      pa_sample_spec ss;
      ss.format = PA_SAMPLE_S16LE;
      ss.channels = 2;
      ss.rate = game_adjusted_rate_;
      if (!(stream_ = pa_stream_new(context_, stream_name.c_str(), &ss, nullptr))) {
        throw std::runtime_error("pa_stream_new failed");
      }

      pa_stream_set_state_callback(stream_, stream_state_callback, this);
      pa_stream_set_overflow_callback(stream_, stream_overflow_callback, this);
      pa_stream_set_underflow_callback(stream_, stream_underflow_callback, this);

      pa_buffer_attr buffer_attr;
      buffer_attr.maxlength = pa_usec_to_bytes(audio_maximum_latency_ * 1000, &ss);
      buffer_attr.tlength = pa_usec_to_bytes(audio_suggested_latency_ * 1000, &ss);
      buffer_attr.prebuf = -1;
      buffer_attr.minreq = -1;
      buffer_attr.fragsize = -1;
      auto flags = pa_stream_flags(PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY);
      int err;
      char const * dev = device_ == "" ? nullptr : device_.c_str();
      if ((err = pa_stream_connect_playback(stream_, dev, &buffer_attr, flags, nullptr, nullptr)) < 0) {
        std::stringstream strm;
        strm << "pa_stream_connect_playback failed: " << pa_strerror(err);
        throw std::runtime_error(strm.str());
      }
    } else if (state == PA_CONTEXT_FAILED) {
      if (reconnect_) {
        next_reconnect_ = Clock::gettime(CLOCK_REALTIME) + reconnect_delay_;
      }
    }
  }

  static void stream_state_callback(pa_stream * p, void * userdata) {
    auto self = static_cast<Pulseaudio *>(userdata);
    return self->stream_state_callback(p);
  }

  static char const * c_str(pa_stream_state state) {
    switch(state) {
      case PA_STREAM_UNCONNECTED: return "unconnected";
      case PA_STREAM_CREATING: return "creating";
      case PA_STREAM_READY: return "ready";
      case PA_STREAM_FAILED: return "failed";
      case PA_STREAM_TERMINATED: return "terminated";
      default: return "unknown";
    }
  }

  static char const * c_str(pa_context_state state) {
    switch(state) {
      case PA_CONTEXT_UNCONNECTED: return "unconnected";
      case PA_CONTEXT_CONNECTING: return "connecting";
      case PA_CONTEXT_AUTHORIZING: return "authorizing";
      case PA_CONTEXT_SETTING_NAME: return "setting name";
      case PA_CONTEXT_READY: return "ready";
      case PA_CONTEXT_FAILED: return "failed";
      case PA_CONTEXT_TERMINATED: return "terminated";
      default: return "unknown";
    }
  }

  void stream_state_callback(pa_stream * p) {
    auto state = pa_stream_get_state(p);
    // std::cout << "Stream state is now " << c_str(state) << std::endl;
    ready_ = state == PA_STREAM_READY;
  }

  static void stream_overflow_callback(pa_stream * p, void * userdata) {
    auto self = static_cast<Pulseaudio *>(userdata);
    return self->stream_overflow_callback(p);
  }

  void stream_overflow_callback(pa_stream * p) {
    ++overruns_;
  }

  static void stream_underflow_callback(pa_stream * p, void * userdata) {
    auto self = static_cast<Pulseaudio *>(userdata);
    return self->stream_underflow_callback(p);
  }

  void stream_underflow_callback(pa_stream * p) {
    ++underruns_;
  }

  void dump_timing_info() {
    auto const * timing_info = pa_stream_get_timing_info(stream_);
    if (timing_info) {
      std::stringstream strm;
      strm << "Timing info:"
           << " timestamp=" << timing_info->timestamp.tv_sec
                     << "." << std::setfill('0') << std::setw(6) << timing_info->timestamp.tv_usec
           << " synchronized_clocks=" << timing_info->synchronized_clocks
           << " sink_usec=" << timing_info->sink_usec
           << " source_usec=" << timing_info->source_usec
           << " transport_usec=" << timing_info->transport_usec
           << " playing=" << timing_info->playing
           << " write_index_corrupt=" << timing_info->write_index_corrupt
           << " write_index=" << timing_info->write_index
           << " read_index_corrupt=" << timing_info->read_index_corrupt
           << " read_index=" << timing_info->read_index
           << " configured_sink_usec=" << timing_info->configured_sink_usec
           << " configured_source_usec=" << timing_info->configured_source_usec
           << " since_underrun=" << timing_info->since_underrun
           << " ahead_by=" << (timing_info->write_index - timing_info->read_index);
      std::cout << strm.str() << std::endl;
    }
  }

  void dump_latency() {
    pa_usec_t usec;
    int negative;
    if (pa_stream_get_latency(stream_, &usec, &negative) == 0) {
      std::cout << "Latency: usec=" << usec << " negative=" << negative << std::endl;
    }
  }

  std::string metrics_suffix() const {
    if (instance_ == "") {
      return "";
    } else {
      return " (" + instance_ + ")";
    }
  }

private:
  std::string const & device_;
  int const & audio_maximum_latency_;
  int const & audio_suggested_latency_;
  bool & reconnect_;

  std::string instance_;

  pa_mainloop * loop_ = nullptr;
  double game_sample_rate_ = 0.0;
  double game_adjusted_rate_ = 0.0;
  pa_mainloop_api * api_ = nullptr;
  pa_context * context_ = nullptr;
  pa_stream * stream_ = nullptr;
  bool ready_ = false;
  Timestamp next_reconnect_ = Timestamp();
  Timestamp last_update_ = Timestamp();
  std::optional<Probe::Key> overruns_key_;
  std::optional<Probe::Key> underruns_key_;
  std::optional<Probe::Key> latency_key_;
  Probe::Value overruns_;
  Probe::Value underruns_;
  static constexpr auto reconnect_delay_ = Seconds(5);
};

}
