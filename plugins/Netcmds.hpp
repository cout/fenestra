#pragma once

#include "Plugin.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdexcept>
#include <vector>
#include <string_view>
#include <cctype>
#include <cstring>
#include <charconv>

namespace fenestra {

class Netcmds
  : public Plugin
{
public:
  Netcmds(Config const & config)
    : port_(config.fetch<int>("netcmds.port", 55355))
  {
  }

  virtual void game_loaded(Core const & core, std::string const & filename) override {
    core_ = &core;
    this->start();
  }

  void start() {
    if (port_ > 0) {
      if ((sock_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        throw std::runtime_error("socket failed");
      }

      sockaddr_in addr;
      std::memset(&addr, 0, sizeof(addr));

      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(port_);

      if (bind(sock_, (sockaddr const *)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("bind failed");
      }

      if (fcntl(sock_, F_SETFL, O_NONBLOCK) < 0) {
        throw std::runtime_error("fcntl failed");
      }
    }
  }

  virtual void pre_frame_delay(State const & state) override {
    poll();
  }

  void poll() {
    if (sock_ > 0) {
      char buf[1500];
      sockaddr_in reply_addr;
      socklen_t reply_addr_len = sizeof(reply_addr);
      int n = recvfrom(sock_, buf, sizeof(buf), 0, (sockaddr *)&reply_addr, &reply_addr_len);

      if (n > 0) {
        auto const & vec = parse(buf, n);
        handle_command(vec, reply_addr, reply_addr_len);
      }
    }
  }

private:
  std::vector<std::string_view> const & parse(char const * buf, int n) {
    vec_.clear();

    auto const * p = buf;
    auto const * e = buf + n;
    auto const * s = p;

    while (e > p && std::isspace(*(e-1))) --e;

    for(; p < e; ++p) {
      if (std::isspace(*p)) {
        vec_.push_back(std::string_view(s, p - s));
        s = p + 1;
      }
      while (p != e && std::isspace(*p)) ++p;
    }

    vec_.push_back(std::string_view(s, p - s));

    return vec_;
  }

  void handle_command(std::vector<std::string_view> const & vec, sockaddr_in reply_addr, socklen_t reply_addr_len) {
    if (vec.empty()) {
      return;
    }

    auto cmd = vec[0];
    if (cmd == "READ_CORE_RAM" && vec.size() >= 3) {
      handle_read_core_ram(vec[1], vec[2], reply_addr, reply_addr_len);
    }
  }

  void handle_read_core_ram(std::string_view s_addr, std::string_view s_bytes, sockaddr_in reply_addr, socklen_t reply_addr_len) {
    unsigned int addr = 0;
    std::from_chars(s_addr.data(), s_addr.data() + s_addr.size(), addr, 16);

    unsigned int bytes = 0;
    std::from_chars(s_bytes.data(), s_bytes.data() + s_bytes.size(), bytes, 16);

    auto size = core_->get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    auto const * data = static_cast<uint8_t *>(core_->get_memory_data(RETRO_MEMORY_SYSTEM_RAM));

    auto const max_bytes = 1024;

    if (addr > size) {
      bytes = 0;
    }

    if (bytes > max_bytes) {
      bytes = max_bytes;
    }

    char reply[40 + 1024 * 3];
    char * r = reply;
    r += std::snprintf(reply, sizeof(reply), "READ_CORE_RAM %x", addr);

    for (auto i = 0u; i < bytes; ++i) {
      r += std::snprintf(r, sizeof(reply) - (r - reply), " %.2X", data[addr + i]);
    }

    r += std::snprintf(r, sizeof(reply) - (r - reply), "\n");

    send_reply(std::string_view(reply, r - reply), reply_addr);
  }

  void send_reply(std::string_view reply, sockaddr_in addr) {
    sendto(sock_, reply.data(), reply.size(), 0, (sockaddr *)&addr, sizeof(addr));
  }

private:
  Core const * core_;
  int & port_;
  int sock_ = -1;
  std::vector<std::string_view> vec_;
};

}
