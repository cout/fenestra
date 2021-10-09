#pragma once

#include "Plugin.hpp"

#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fenestra {

class Perflog
  : public Plugin
{
public:
  Perflog(Config const & config) {
    if (config.perflog_filename() != "") {
      open(config.perflog_filename());
    }
  }

  void open(std::string const & filename) {
    if ((fd_ = ::open(filename.c_str(), O_CREAT|O_RDWR|O_NOATIME, 0664)) < 0) {
      throw std::runtime_error("open failed");
    }
  }

  virtual void record_probe(Probe const & probe, Probe::Dictionary const & dictionary) override {
    if (probe_dict_.version() != dictionary.version()) {
      serialize(dictionary);
      probe_dict_ = dictionary;
    }
    serialize(probe);
  }

private:
  void serialize(Probe::Dictionary const & dictionary) {
    if (fd_ < 0) {
      return;
    }

    if (dictionary_written_) {
      throw std::runtime_error("Cannot write dictionary more than once");
    }

    buf_.clear();

    for (auto const & [ key, name ] : dictionary) {
      buf_.insert(buf_.end(), reinterpret_cast<char const *>(&key), reinterpret_cast<char const *>(&key) + sizeof(key));
      buf_.insert(buf_.end(), name.c_str(), name.c_str() + name.length() + 1);
    }

    if (::write(fd_, buf_.data(), buf_.size()) < 0) {
      throw std::runtime_error("write failed");
    }

    dictionary_written_ = true;
  }

  void serialize(Probe const & probe) {
    if (fd_ < 0) {
      return;
    }

    buf_.clear();

    for (auto const & stamp : probe) {
      buf_.insert(buf_.end(), reinterpret_cast<char const *>(&stamp), reinterpret_cast<char const *>(&stamp) + sizeof(stamp));
    }

    if (::write(fd_, buf_.data(), buf_.size()) < 0) {
      throw std::runtime_error("write failed");
    }
  }

private:
  Probe::Dictionary probe_dict_;
  Probe last_;
  int fd_ = -1;
  bool dictionary_written_ = false;
  std::vector<char> buf_;
};

}
