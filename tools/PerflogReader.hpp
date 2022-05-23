#pragma once

#include "fenestra/Clock.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <sys/stat.h>
#include <unistd.h>

class PerflogReader {
public:
  class PerfQueue;

  PerflogReader(std::string const & filename)
    : filename_(filename)
  {
    this->stat();
    last_stat_time_ = fenestra::Clock::gettime(CLOCK_REALTIME);
  }

  auto const & queues() const { return queues_; }
  auto time() const { return time_; }

  void poll() {
    auto now = fenestra::Clock::gettime(CLOCK_REALTIME);
    if (now - last_stat_time_ > fenestra::Seconds(1.0)) {
      stat();
      last_stat_time_ = now;
    }

    file_.clear();
    file_.seekg(last_pos_);

    std::int64_t time;
    std::vector<std::uint32_t> deltas;
    while (read_record(&time, &deltas)) {
      handle(time, deltas);
    }
  }

private:
  void stat() {
    struct stat statbuf;
    if (lstat(filename_.c_str(), &statbuf) < 0) {
      // TODO: this might be a transient error, so there should be away
      // to ignore this and continue
      throw std::runtime_error("lstat failed");
    }

    if (statbuf.st_ino != statbuf_.st_ino || statbuf.st_size < statbuf_.st_size || queues_.size() == 0) {
      if (last_stat_time_ > fenestra::Timestamp()) {
        std::cout << "File was truncated; re-opening file" << std::endl;
      }
      open(filename_);
    }

    statbuf_ = statbuf;
  }

  void open(std::string const & filename) {
    queues_.clear();

    file_.close();
    file_.open(filename);
    filename_ = filename;

    std::string line;
    std::getline(file_, line);
    last_pos_ = file_.tellg();

    std::string name;
    bool first = true;
    for (auto c : line) {
      if (c == '\0') {
        if (first) {
          first = false;
        } else {
          queues_.emplace_back(name);
        }
        name = "";
      } else {
        name += c;
      }
    }

    auto n_deltas = queues_.size();
    time_ = 0;
    frames_ = 0;
    record_size_ = 8 + n_deltas * 4; // 64-bit time, 32-bit deltas

    if (n_deltas > 0) {
      queues_.emplace(queues_.begin(), "FPS");
      queues_.emplace(queues_.begin(), "Frame Time");
    }

    std::int64_t time;
    std::vector<std::uint32_t> deltas;
    while (read_record(&time, &deltas)) {
      handle(time, deltas);
    }
  }

  bool read_record(std::int64_t * time, std::vector<std::uint32_t> * deltas) {
    buf_.clear();
    buf_.resize(record_size_);
    file_.read(buf_.data(), record_size_);

    if (file_) {
      auto const * p = buf_.data();
      auto const * end = p + buf_.size();
      *time = reinterpret_cast<std::uint64_t const &>(*p);
      p += sizeof(std::uint64_t);

      deltas->clear();
      for (; p < end; p += sizeof(std::uint32_t)) {
        deltas->push_back(reinterpret_cast<std::uint32_t const &>(*p));
      }

      last_pos_ = file_.tellg();
      return true;
    }

    return false;
  }

  void handle(std::uint64_t time, std::vector<std::uint32_t> const & deltas);

private:
  std::string filename_;
  std::ifstream file_;
  std::ifstream::pos_type last_pos_;
  std::vector<PerfQueue> queues_;
  std::size_t record_size_ = 0;
  std::vector<char> buf_;
  std::uint64_t time_ = 0;
  std::uint64_t frames_ = 0;
  fenestra::Timestamp last_stat_time_ = fenestra::Nanoseconds::zero();
  struct stat statbuf_ { 0, 0 }; // dev, inode
};

class PerflogReader::PerfQueue {
public:
  PerfQueue(std::string const & name)
    : name_(name)
  {
  }

  auto const & name() const { return name_; }

  static constexpr inline std::size_t max_secs = 60;
  static constexpr inline std::size_t max_frames = 60 * max_secs;

  void record(std::uint32_t delta) {
    total_ += delta;
    deltas_.push_back(delta);
    if (deltas_.size() > max_frames) {
      total_ -= deltas_.front();
      deltas_.pop_front();
    }
  }

  auto total() const { return total_; }
  auto avg() const { return deltas_.size() == 0 ? 0 : total_ / deltas_.size(); }
  auto size() const { return deltas_.size(); }
  auto begin() const { return deltas_.begin(); }
  auto end() const { return deltas_.end(); }

  auto operator[](std::size_t i) const { return deltas_[i]; }

private:
  std::string name_;
  std::deque<std::uint32_t> deltas_;
  std::uint64_t total_ = 0;
};

void
PerflogReader::
handle(std::uint64_t time, std::vector<std::uint32_t> const & deltas) {
  ++frames_;

  // Ignore the first few frames
  if (frames_ < 4) {
    return;
  }

  std::uint32_t frame_time_us = 16'667;
  if (time_ > 0 && time != time_) {
    auto last_time = time_;
    auto time_delta_ns = time - last_time;
    frame_time_us = time_delta_ns / 1'000;
  }
  queues_[0].record(frame_time_us);

  float fps = 60.0;
  if (queues_[0].size() >= 8) {
    // Average fps over 8 frames
    auto it = std::prev(queues_[0].end());
    std::uint64_t eight_frame_time = 0;
    for (std::size_t i = 0; i < 8; ++i, --it) {
      eight_frame_time += *it;
    }
    auto avg_us = eight_frame_time / 8.0;
    auto avg_s = avg_us / 1'000'000;
    fps = 1.0 / avg_s;
  }
  queues_[1].record(fps * 1000);

  for (std::size_t i = 0; i < std::min(deltas.size(), queues_.size()); ++i) {
    queues_[i+2].record(deltas[i]); // +2 for frame time, fps
  }

  time_ = time;
}
