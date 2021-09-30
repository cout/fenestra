#pragma once

#include "libretro.h"

#include "Geometry.hpp"

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

class Capture {
public:
  struct Pixel_Format {
    std::uint32_t v4l2_pix_fmt;
    int bpp;
  };

  static inline std::map<unsigned int, Pixel_Format> const pixel_formats = {
    { RETRO_PIXEL_FORMAT_RGB565,   { V4L2_PIX_FMT_RGB565, 16 } },
  };

  Capture() {
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

  ~Capture() {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  bool set_pixel_format(retro_pixel_format format) {
    pixel_format_ = pixel_formats.at(format);
    return true;
  }

  void configure(Geometry const & geom) {
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

  void refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) {
    auto Bpp = pixel_format_.bpp / CHAR_BIT;
    buf_.clear();
    auto it = std::back_inserter(buf_);
    for (std::size_t i = 0; i < height; ++i) {
      auto start = static_cast<char const *>(data) + i * pitch;
      auto end = start + width * Bpp;
      it = std::copy(start, end, it);
    }
  }

  void render() {
    if (fd_ < 0) {
      return;
    }

    write(fd_, buf_.data(), buf_.size());
  }

private:
  void prime_format(v4l2_format & format) {
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd_, VIDIOC_G_FMT, &format) < 0) {
      throw std::runtime_error("ioctl for priming failed (VIDIOC_G_FMT)");
    }
  }

  void set_format(v4l2_format const & format) {
    std::cout << "Before: " << format.fmt.pix.bytesperline << std::endl;
    if (ioctl(fd_, VIDIOC_S_FMT, &format) < 0) {
      throw std::runtime_error("ioctl failed (VIDIOC_S_FMT)");
    }
    std::cout << "After: " << format.fmt.pix.bytesperline << std::endl;
  }

private:
  Pixel_Format pixel_format_;
  int fd_ = -1;
  std::vector<char> buf_;

  v4l2_capability cap_;
  v4l2_format format_;
};

}
