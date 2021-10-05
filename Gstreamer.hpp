#pragma once

#include "libretro.h"

#include "Geometry.hpp"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gstreamermm.h>

#include <stdexcept>
#include <map>
#include <cstdint>

namespace fenestra {

class Gstreamer
  : public Plugin
{
public:
  struct Pixel_Format {
    std::string caps_format;
    int bpp;
  };

  static inline std::map<unsigned int, Pixel_Format> const pixel_formats = {
    { RETRO_PIXEL_FORMAT_RGB565,   { "RGB16", 16 } },
  };

  Gstreamer(Config const & config)
    : config_(config)
  {
    if (config_.gstreamer_sink_pipeline() != "") {
      open(config_.gstreamer_sink_pipeline());
    }
  }

  ~Gstreamer() {
  }

  void open(std::string const & sink_pipeline) {
    pipeline_ = Gst::Pipeline::create("fenestra-stream-pipeline");

    source_ = Gst::ElementFactory::create_element("appsrc", "libretro-source");
    appsrc_ = decltype(appsrc_)::cast_dynamic(source_);
    pipeline_->add(source_);

    conv_ = Gst::ElementFactory::create_element("videoconvert", "videoconvert");
    pipeline_->add(conv_);

    sink_ = Gst::Parse::create_bin(sink_pipeline, true);
    sink_->set_property("name", std::string("capture-sink"));
    pipeline_->add(sink_);
  }

  virtual void set_pixel_format(retro_pixel_format format) override {
    pixel_format_ = pixel_formats.at(format);
  }

  virtual void set_geometry(Geometry const & geom) override {
    if (!source_ || !pipeline_) {
      return;
    }

    auto caps = Gst::Caps::create_simple(
        "video/x-raw",
        "format", pixel_format_.caps_format,
        "width", int(geom.base_width()),
        "height", int(geom.base_height()),
        "framerate", Gst::Fraction(0, 1)); // variable framerate

    source_->set_property("caps", caps);

    source_->link(conv_)->link(sink_);

    source_->set_property("stream_type", 0);
    source_->set_property("is-live", true);

    pipeline_->set_state(Gst::STATE_PLAYING);
  }

  virtual void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) override {
    if (!appsrc_) {
      return;
    }

    auto Bpp = pixel_format_.bpp / CHAR_BIT;
    auto size = width * height * Bpp;
    buf_ = Gst::Buffer::create(size);

    for (std::size_t i = 0; i < height; ++i) {
      auto start_idx = i * pitch;
      auto end_idx = start_idx + width * Bpp;
      auto start = static_cast<char const *>(data) + start_idx;
      auto end = static_cast<char const *>(data) + end_idx;

      buf_->fill(i * width * Bpp, start, end - start);
    }
  }

  virtual void pre_frame_delay() override {
    if (buf_ && appsrc_) {
      auto ret = appsrc_->push_buffer(buf_);
      if (ret != Gst::FLOW_OK) {
        throw std::runtime_error("push_buffer failed");
      }
      [&]{ return buf_.release(); }();
    }
  }

private:
  Config const & config_;

  Pixel_Format pixel_format_;

  Glib::RefPtr<Gst::Pipeline> pipeline_;
  Glib::RefPtr<Gst::AppSrc> appsrc_;
  Glib::RefPtr<Gst::Element> source_;
  Glib::RefPtr<Gst::Element> conv_;
  Glib::RefPtr<Gst::Element> sink_;
  Glib::RefPtr<Gst::Buffer> buf_;
};

}
