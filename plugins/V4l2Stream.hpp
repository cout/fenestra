#pragma once

#include "Plugin.hpp"

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <stdexcept>
#include <sstream>
#include <map>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <cstring>

namespace fenestra {

class V4l2Stream
  : public Plugin
{
public:
  struct Pixel_Format {
    std::uint32_t v4l2_pix_fmt;
    int bpp;
  };

  static inline std::map<unsigned int, Pixel_Format> const pixel_formats = {
    { RETRO_PIXEL_FORMAT_RGB565,   { V4L2_PIX_FMT_RGB565, 16 } },
  };

  V4l2Stream(Config const & config) {
    if (config.v4l2_device() != "") {
      open(config.v4l2_device());
    }
  }

  void open(std::string const & filename) {
    if ((fd_ = ::open(filename.c_str(), O_RDWR | O_NONBLOCK)) < 0) {
      throw std::runtime_error(std::string("open failed for ") + filename + ": " + std::strerror(errno));
    }

    // Make sure this is a v4l2 device
    if (ioctl(fd_, VIDIOC_QUERYCAP, &cap_) < 0) {
      throw std::runtime_error("ioctl failed (VIDIOC_QUERYCAP)");
    }

    if (!(cap_.capabilities & V4L2_CAP_STREAMING)) {
      throw std::runtime_error("v4l2 device does not allow streaming");
    }

    if (!(cap_.capabilities & V4L2_CAP_STREAMING)) {
      throw std::runtime_error("v4l2 device is not writable");
    }
  }

  virtual ~V4l2Stream() override {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  virtual void set_pixel_format(retro_pixel_format format) override {
    pixel_format_ = pixel_formats.at(format);
  }

  virtual void set_geometry(Geometry const & geom) override {
    if (fd_ < 0) {
      return;
    }

    prime_format(format_);
    format_.fmt.pix.width = geom.base_width();
    format_.fmt.pix.height = geom.base_height();
    format_.fmt.pix.pixelformat = pixel_format_.v4l2_pix_fmt;
    format_.fmt.pix.bytesperline = 0;
    format_.fmt.pix.sizeimage = format_.fmt.pix.bytesperline * geom.base_height();

    set_format(format_);
  }

  virtual void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) {
    auto Bpp = pixel_format_.bpp / CHAR_BIT;
    buf_.clear();
    auto size = width * height * Bpp;
    buf_.resize(size);
    auto dest = buf_.data();
    for (std::size_t i = 0; i < height; ++i) {
      auto start = static_cast<char const *>(data) + i * pitch;
      auto end = start + width * Bpp;
      dest = std::copy(start, end, dest);
    }
  }

  virtual void pre_frame_delay() {
    if (fd_ < 0) {
      return;
    }

    [&] { return write(fd_, buf_.data(), buf_.size()); }();
  }

private:
  void prime_format(v4l2_format & format) {
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd_, VIDIOC_G_FMT, &format) < 0) {
      throw std::runtime_error("ioctl for priming failed (VIDIOC_G_FMT)");
    }
  }

  void set_format(v4l2_format const & format) {
    std::cout << "Before: " << to_string(format.fmt.pix) << std::endl;
    if (ioctl(fd_, VIDIOC_S_FMT, &format) < 0) {
      throw std::runtime_error("ioctl failed (VIDIOC_S_FMT)");
    }
    std::cout << "After: " << to_string(format.fmt.pix) << std::endl;
  }

  std::string to_string(v4l2_pix_format const & pix) {
    std::stringstream strm;
    strm << "width=" << pix.width << ",height=" << pix.height <<
      ",pixelformat=" << pix.pixelformat << ",field=" << pix.field <<
      ",bytesperline=" << pix.bytesperline << ",sizeimage=" <<
      pix.sizeimage << ",colorspace=" << pix.colorspace << ",priv=" <<
      pix.priv << ",flags=" << pix.flags << ",quantization=" <<
      pix.quantization << ",xfer_func=" << pix.xfer_func;
    return strm.str();
  }

private:
  Pixel_Format pixel_format_;
  int fd_ = -1;

  using Buffer = std::vector<char>;
  Buffer buf_;

  v4l2_capability cap_;
  v4l2_format format_;
};

}
