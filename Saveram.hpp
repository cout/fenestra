#pragma once

#include "libretro.h"

#include <cstdint>
#include <cstddef>

namespace fenestra {

class Saveram {
public:
  Saveram(Core & core) {
    size_ = core.get_memory_size(RETRO_MEMORY_SAVE_RAM);

    if (size_ > 0) {
      data_ = static_cast<std::uint8_t *>(core.get_memory_data(RETRO_MEMORY_SAVE_RAM));
    }
  }

  std::uint8_t * data() { return data_; }
  std::size_t size() const { return size_; }

  auto begin() { return data(); }
  auto end() { return data() + size(); }

private:
  std::uint8_t * data_ = nullptr;
  std::size_t size_ = 0;
};

}
