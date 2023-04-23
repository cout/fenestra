#pragma once

#include "fenestra/Plugin.hpp"

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
  Netcmds(Config::Subtree const & config, std::string const & instance)
    : port_(config.fetch<int>("port", 55355))
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
    poll(state);
  }

  void poll(State const & state) {
    if (sock_ > 0) {
      char buf[1500];
      sockaddr_in reply_addr;
      socklen_t reply_addr_len = sizeof(reply_addr);

      for (;;) {
        int n = recvfrom(sock_, buf, sizeof(buf), 0, (sockaddr *)&reply_addr, &reply_addr_len);
        if (n <= 0) {
          break;
        }

        std::string_view str(buf, n);
        handle_packet(str, reply_addr, reply_addr_len, state);
      }
    }
  }

private:
  template<typename F>
  static void split(std::string_view const & str, std::vector<std::string_view> & res, F issep) {
    res.clear();

    auto const * p = str.data();
    auto const * e = p + str.size();
    auto const * s = p;

    // Ignore trailing separators
    while (e > p && issep(*(e-1))) --e;

    for(; p < e; ++p) {
      // Each time we find a separator, add the current token to the list
      if (issep(*p)) {
        res.push_back(std::string_view(s, p - s));
        s = p + 1;
      }

      // Find the next separator
      while (p != e && issep(*p)) ++p;
    }

    // Add the final token to the list
    res.push_back(std::string_view(s, p - s));
  }

  std::vector<std::string_view> const & parse(std::string_view str) {
    split(str, args_, [](auto c) { return std::isspace(c); });
    return args_;
  }

  void handle_packet(std::string_view str, sockaddr_in reply_addr, socklen_t reply_addr_len, State const & state) {
    split(str, cmds_, [](auto c) { return c == '\n'; });
    for (auto const & cmd : cmds_ ) {
      auto const & args = parse(cmd);
      handle_command(args, reply_addr, reply_addr_len, state);
    }
  }

  void handle_command(std::vector<std::string_view> const & args, sockaddr_in reply_addr, socklen_t reply_addr_len, State const & state) {
    if (args.empty()) {
      return;
    }

    auto cmd = args[0];
    if (cmd == "READ_CORE_RAM" && args.size() >= 3) {
      handle_read_core_ram(args[1], args[2], reply_addr, reply_addr_len, state);
    } else if (cmd == "WRITE_CORE_RAM" && args.size() >= 1) {
      handle_write_core_ram(args[1], std::next(std::begin(args)), std::end(args));
    } else if (cmd == "GET_STATUS") {
      handle_get_status(reply_addr, reply_addr_len, state);
    } else if (cmd == "VERSION") {
      handle_get_version(reply_addr, reply_addr_len);
    } else {
      std::cerr << "Unknown network command: " << cmd << std::endl;
    }
  }

  void handle_read_core_ram(std::string_view s_addr, std::string_view s_bytes, sockaddr_in reply_addr, socklen_t reply_addr_len, State const & state) {
    unsigned int addr = 0;
    std::from_chars(s_addr.data(), s_addr.data() + s_addr.size(), addr, 16);

    unsigned int bytes = 0;
    std::from_chars(s_bytes.data(), s_bytes.data() + s_bytes.size(), bytes, 10);

    auto size = core_->get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    auto const * data = static_cast<uint8_t *>(core_->get_memory_data(RETRO_MEMORY_SYSTEM_RAM));
    auto sram_size = core_->get_memory_size(RETRO_MEMORY_SAVE_RAM);
    auto const * sram_data = static_cast<uint8_t *>(core_->get_memory_data(RETRO_MEMORY_SAVE_RAM));

    auto const max_bytes = 2000;

    if (bytes > max_bytes) {
      bytes = max_bytes;
    }

    reply_.resize(40 + bytes * 3);
    char * r = reply_.data();
    r += std::snprintf(reply_.data(), reply_.size(), "READ_CORE_RAM %x", addr);

    if (addr < size) {
      for (auto i = 0u; i < bytes; ++i) {
        r += std::snprintf(r, reply_.size() - (r - reply_.data()), " %.2X", data[addr + i]);
      }
    } else if (addr < size + sram_size) {
      // Hack: return sram for addresses beyond system ram (this
      // probably only works for SNES)
      addr -= size;
      for (auto i = 0u; i < bytes; ++i) {
        r += std::snprintf(r, reply_.size() - (r - reply_.data()), " %.2X", sram_data[addr + i]);
      }
    } else {
      // TODO: This might be a request for a ROM read, but we don't
      // support it.
      r += std::snprintf(r, reply_.size() - (r - reply_.data()), " -1");
    }

    r += std::snprintf(r, reply_.size() - (r - reply_.data()), "\n");

    send_reply(std::string_view(reply_.data(), r - reply_.data()), reply_addr);
  }

  template<typename It>
  void handle_write_core_ram(std::string_view s_addr, It begin, It end) {
    unsigned int addr = 0;
    std::from_chars(s_addr.data(), s_addr.data() + s_addr.size(), addr, 16);

    auto size = core_->get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    auto * data = static_cast<uint8_t *>(core_->get_memory_data(RETRO_MEMORY_SYSTEM_RAM));

    for (auto it = begin, i = 0u; it != end; ++it, ++i) {
      unsigned int value = 0;
      std::from_chars(it->data(), it->data() + it->size(), value, 10);

      if (addr + i < size) {
        data[addr + i] = value;
      }
    }
  }

  void handle_get_status(sockaddr_in reply_addr, socklen_t reply_addr_len, State const & state) {
    reply_.clear();
    append(reply_, "GET_STATUS ");
    append(reply_, state.paused ? "PAUSED " : "PLAYING ");
    append(reply_, "TODO_system_id");
    append(reply_, ",");
    append(reply_, "TODO_content_name");
    append(reply_, ",crc32=");
    append(reply_, "TODO_content_crc32");

    send_reply(std::string_view(reply_.data(), reply_.size()), reply_addr);
  }

  void handle_get_version(sockaddr_in reply_addr, socklen_t reply_addr_len) {
    reply_.clear();

    // 1.7.2 tells clients that we don't support READ_CORE_MEMORY.
    append(reply_, "1.7.2");

    send_reply(std::string_view(reply_.data(), reply_.size()), reply_addr);
  }

  static void append(std::vector<char> & dest, std::string_view s) {
    dest.insert(dest.end(), s.begin(), s.end());
  }

  void send_reply(std::string_view reply, sockaddr_in addr) {
    sendto(sock_, reply.data(), reply.size(), 0, (sockaddr *)&addr, sizeof(addr));
  }

private:
  Core const * core_;
  int & port_;
  int sock_ = -1;
  std::vector<std::string_view> cmds_;
  std::vector<std::string_view> args_;
  std::vector<char> reply_;
};

}
