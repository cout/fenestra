#pragma once

#include <vector>
#include <cstddef>
#include <utility>

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
    std::size_t size;
    std::vector<char> data;
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    data.resize(size);
    in.read(data.data(), size);
    return CoreState(data);
  }

  void save(std::string const & filename) {
    std::ofstream out(filename);
    auto * data = data_.data();
    std::size_t size = data_.size();
    out.write(reinterpret_cast<char *>(&size), sizeof(size));
    out.write(data, size);
  }

private:
  std::vector<char> data_;
};

}
