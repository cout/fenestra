#pragma once

#include "Plugin.hpp"

#include "ssr/SSRVideoStreamWriter.h"

#include <stdexcept>
#include <memory>
#include <cstring>

namespace fenestra {

class SSR
  : public Plugin
{
public:
  struct Pixel_Format {
    int bpp;
    retro_pixel_format format;
  };

  static inline std::map<unsigned int, Pixel_Format> const pixel_formats = {
    { RETRO_PIXEL_FORMAT_RGB565,   { 16, RETRO_PIXEL_FORMAT_RGB565 } },
  };

  SSR(Config const & config) {
    if (config.ssr_channel() != "") {
      open(config.ssr_channel());
    }
  }

  void open(std::string const & channel) {
    ssr_ = std::make_unique<SSRVideoStreamWriter>(channel, "libretro");
  }

  virtual ~SSR() override {
  }

  virtual void set_pixel_format(retro_pixel_format format) override {
    pixel_format_ = pixel_formats.at(format);
  }

  virtual void set_geometry(Geometry const & geom) override {
  }

  virtual void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) {
    if (!ssr_) return;

    if (width != width_ || height != height_ || pitch_ != pitch) {
      ssr_->UpdateSize(width, height, width * 4);
      width_ = width;
      height_ = height;
      pitch_ = pitch;
    }

    unsigned int flags = 0;
    ptr_ = ssr_->NewFrame(&flags);

    if (ptr_) {
      auto Bpp = pixel_format_.bpp / CHAR_BIT;
      auto size = Bpp * height * pitch;

      if (buf_.size() != size) {
        buf_.resize(size);
      }

      // Copy to a temporary buffer so we can do pixel format conversion
      // during the frame delay
      auto src = static_cast<std::uint8_t const *>(data);
      auto dest = buf_.data();
      std::memcpy(dest, src, size);
    }
  }

  virtual void pre_frame_delay() {
    if (!ssr_) return;

    if (pixel_format_.format != RETRO_PIXEL_FORMAT_RGB565) {
      // Only 565 is implemented, for now
      return;
    }

    bool captured = ptr_;

    if (captured) {
      auto Bpp = pixel_format_.bpp / CHAR_BIT;
      auto * dest = static_cast<std::uint8_t *>(ptr_);
      for (std::size_t y = 0; y < height_; ++y) {
        auto start = buf_.data() + y * pitch_;
        auto end = start + width_ * Bpp;

        for (auto * src = start; src < end; src += Bpp) {
          // Retroarch pixel formats are always native endianness
          auto i = *(reinterpret_cast<std::uint16_t const *>(src));

          auto blue  = (i >>  0) & 0x3f;
          auto green = (i >>  5) & 0x7f;
          auto red   = (i >> 11) & 0x3f;

          *dest++ = blue << 3;
          *dest++ = green << 2;
          *dest++ = red << 3;
          *dest++ = 255;
        }
      }

      ssr_->NextFrame();
      ptr_ = nullptr;
    }
  }

private:
  Pixel_Format pixel_format_;
  std::unique_ptr<SSRVideoStreamWriter> ssr_;
  unsigned int width_ = 0;
  unsigned int height_ = 0;
  std::size_t pitch_ = 0;
  std::vector<char> buf_;
  void * ptr_ = nullptr;
};

}
