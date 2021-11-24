#pragma once

#include <vector>
#include <cstddef>
#include <utility>
#include <sstream>

#include <zstd.h>

namespace fenestra {

class CoreState {
public:
  CoreState(std::vector<char> data)
    : data_(std::move(data))
  {
  }

  static CoreState serialize(Core const & core) {
    auto size = core.serialize_size();
    std::vector<char> data;
    data.resize(size);
    if (!core.serialize(data.data(), size)) {
      throw std::runtime_error("Core failed to serialize");
    }
    return CoreState(data);
  }

  void unserialize(Core const & core) {
    if (!core.unserialize(data_.data(), data_.size())) {
      throw std::runtime_error("Core failed to unserialize");
    }
  }

  static CoreState load(std::string const & filename) {
    std::ifstream in(filename);
    std::stringstream sstr;
    while (in >> sstr.rdbuf()) { }

    auto size = ZSTD_getFrameContentSize(sstr.str().data(), sstr.str().length());

    if (size == ZSTD_CONTENTSIZE_ERROR) {
      throw std::runtime_error("Content is not zstd-compressed");
    }

    if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
      throw std::runtime_error("Content size is unknown");
    }

    std::vector<char> data;
    data.resize(size);
    ZSTD_decompress(data.data(), data.size(), sstr.str().data(), sstr.str().length());

    return CoreState(data);
  }

  void save(std::string const & filename) {
    std::ofstream out(filename);

    std::vector<char> buf;
    buf.resize(ZSTD_compressBound(data_.size()));

    auto csize = ZSTD_compress(buf.data(), buf.size(), data_.data(), data_.size(), 1);

    if (ZSTD_isError(csize)) {
      throw std::runtime_error(ZSTD_getErrorName(csize));
    }

    out.write(buf.data(), csize);
  }

private:
  std::vector<char> data_;
};

}
