#pragma once

#include <string>
#include <algorithm>
#include <string_view>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <iostream>

namespace fenestra {

class Savefile {
public:
  Savefile(std::string const & filename, std::size_t size)
    : size_(size)
  {
    if (size_ == 0) {
      return;
    }

    if ((fd_ = ::open(filename.c_str(), O_CREAT|O_RDWR|O_NOATIME, 0664)) < 0) {
      throw std::runtime_error("open failed");
    }

    if (::ftruncate(fd_, size) != 0) {
      throw std::runtime_error("ftruncate failed");
    }

    if ((p_ = ::mmap(nullptr, size_, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE|MAP_POPULATE, fd_, 0)) == MAP_FAILED) {
      throw std::runtime_error("mmap failed");
    }
  }

  ~Savefile() {
    if (p_) {
      ::msync(p_, size_, MS_SYNC);
      ::munmap(p_, size_);
    }

    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  std::uint8_t * data() { return static_cast<std::uint8_t *>(p_); }
  std::size_t size() const { return size_; }

  auto begin() { return data(); }
  auto end() { return data() + size(); }

  void sync_if_changed(std::uint8_t const * data, std::size_t size) {
    last_sync_bytes_.reserve(size);
    if (!std::equal(data, data + size, last_sync_bytes_.begin())) {
      sync(data, size);
      std::copy(data, data + size, last_sync_bytes_.begin());
    }
  }

  void sync(std::uint8_t const * data, std::size_t size) {
    std::cout << "Sync SRAM" << std::endl;
    std::copy(data, data + std::min(size, this->size()), this->data());
  }

private:
  std::size_t size_;
  int fd_ = -1;
  void * p_ = nullptr;
  std::vector<std::uint8_t> last_sync_bytes_;
};

}
