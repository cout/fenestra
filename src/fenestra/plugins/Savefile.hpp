#pragma once

#include "fenestra/Plugin.hpp"
#include "fenestra/Saveram.hpp"

#include <string>
#include <algorithm>
#include <string_view>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <iostream>
#include <memory>

namespace fenestra {

class Savefile
  : public Plugin
{
public:
  Savefile(Config::Subtree const & config)
    : save_directory_(config.root().fetch<std::string>("paths.save_directory", "."))
  {
  }

  virtual ~Savefile() override {
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

  virtual void game_loaded(Core const & core, std::string const & filename) override;

  virtual void unloading_game(Core const & core) override {
    sync(saveram_->data(), saveram_->size());
  }

  virtual void pre_frame_delay(State const & state) override {
    sync_if_changed(saveram_->data(), saveram_->size());
  }

private:
  void open(std::string const & filename, std::size_t size) {
    if (size == 0) {
      return;
    }

    if ((fd_ = ::open(filename.c_str(), O_CREAT|O_RDWR|O_NOATIME, 0664)) < 0) {
      throw std::runtime_error("open failed");
    }

    if (::ftruncate(fd_, size) != 0) {
      throw std::runtime_error("ftruncate failed");
    }

    if ((p_ = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE|MAP_POPULATE, fd_, 0)) == MAP_FAILED) {
      throw std::runtime_error("mmap failed");
    }

    size_ = size;
  }

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
  std::string const & save_directory_;

  std::unique_ptr<Saveram> saveram_;
  std::size_t size_ = 0;
  int fd_ = -1;
  void * p_ = nullptr;
  std::vector<std::uint8_t> last_sync_bytes_;
};

}
