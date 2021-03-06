#pragma once

#include "fenestra/Plugin.hpp"

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

  static inline std::map<retro_pixel_format, Pixel_Format> const pixel_formats = {
    { RETRO_PIXEL_FORMAT_XRGB8888, { 32, RETRO_PIXEL_FORMAT_XRGB8888 } },
    { RETRO_PIXEL_FORMAT_RGB565,   { 16, RETRO_PIXEL_FORMAT_RGB565 } },
  };

  SSR(Config::Subtree const & config, std::string const & instance)
    : channel_(config.fetch<std::string>("channel", ""))
  {
    if (channel_ != "") {
      open(channel_);
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
      auto size = height * pitch;

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

  virtual void pre_frame_delay(State const & state) {
    if (!ssr_) return;

    bool captured = ptr_;

    if (captured) {
      auto Bpp = pixel_format_.bpp / CHAR_BIT;
      auto * dest = static_cast<std::uint8_t *>(ptr_);
      for (std::size_t y = 0; y < height_; ++y) {
        auto start = buf_.data() + y * pitch_;
        auto end = start + width_ * Bpp;
        dest = serialize(start, end, dest, pixel_format_.format);
      }

      ssr_->NextFrame();
      ptr_ = nullptr;
    }
  }

private:
  struct RGB565 {
    static constexpr inline auto bshift = 0;
    static constexpr inline auto gshift = 5;
    static constexpr inline auto rshift = 11;
    static constexpr inline auto bmask = 0x3f;
    static constexpr inline auto gmask = 0x7f;
    static constexpr inline auto rmask = 0x3f;

    static inline std::uint8_t bscale[256];
    static inline std::uint8_t gscale[256];
    static inline std::uint8_t rscale[256];

    static inline std::uint8_t scaleb(std::uint8_t blue)  { return bscale[blue];  }
    static inline std::uint8_t scaleg(std::uint8_t green) { return gscale[green]; }
    static inline std::uint8_t scaler(std::uint8_t red)   { return rscale[red];   }
  };

  struct RGB565_Init {
    RGB565_Init() {
      for (int i = 0; i < 256; ++i) {

        RGB565::bscale[i] = (i << 3) | (i >> 2);
        RGB565::gscale[i] = (i << 2) | (i >> 4);
        RGB565::rscale[i] = (i << 3) | (i >> 2);
      }
    }
  };

  static inline RGB565_Init rgb565_init_;

  struct XRGB8888 {
    static constexpr inline auto bshift = 0;
    static constexpr inline auto gshift = 8;
    static constexpr inline auto rshift = 16;
    static constexpr inline auto bmask = 0xff;
    static constexpr inline auto gmask = 0xff;
    static constexpr inline auto rmask = 0xff;

    static inline std::uint8_t scaleb(std::uint8_t blue)  { return blue;  }
    static inline std::uint8_t scaleg(std::uint8_t green) { return green; }
    static inline std::uint8_t scaler(std::uint8_t red)   { return red;   }
  };

  template<typename T, typename Params>
  static inline std::uint8_t * serialize_bgra(char const * start, char const * end, std::uint8_t * dest) {
    for (auto * src = start; src < end; src += sizeof(T)) {
      dest = serialize_bgra_pixel<T, Params>(src, dest);
    }
    return dest;
  }

  template<typename T, typename Params>
  static inline std::uint8_t * serialize_bgra_pixel(char const * src, std::uint8_t * dest) {
    // Retroarch pixel formats are always native endianness
    auto i = *(reinterpret_cast<T const *>(src));

    auto blue  = (i >> Params::bshift) & Params::bmask;
    auto green = (i >> Params::gshift) & Params::gmask;
    auto red   = (i >> Params::rshift) & Params::rmask;

    *dest++ = Params::scaleb(blue);
    *dest++ = Params::scaleg(green);
    *dest++ = Params::scaler(red);
    *dest++ = 255;

    return dest;
  }

  static inline std::uint8_t * serialize(char const * start, char const * end, std::uint8_t * dest, retro_pixel_format format) {
    switch (format) {
      case RETRO_PIXEL_FORMAT_RGB565:
        return serialize_bgra<std::uint16_t, RGB565>(start, end, dest);

      case RETRO_PIXEL_FORMAT_XRGB8888:
        return serialize_bgra<std::uint32_t, XRGB8888>(start, end, dest);

      case RETRO_PIXEL_FORMAT_0RGB1555:
      case RETRO_PIXEL_FORMAT_UNKNOWN:
        // not implemented
        break;
    }

    return dest;
  }

private:
  std::string const & channel_;
  Pixel_Format pixel_format_;
  std::unique_ptr<SSRVideoStreamWriter> ssr_;
  unsigned int width_ = 0;
  unsigned int height_ = 0;
  std::size_t pitch_ = 0;
  std::vector<char> buf_;
  void * ptr_ = nullptr;
};

}
