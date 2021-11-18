#pragma once

#include "libretro.h"

#include "Frontend.hpp"
#include "Core.hpp"

#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <map>

namespace fenestra {

class Context {
public:
  Context(Frontend & frontend, Core & core, Config const & config)
    : frontend_(frontend)
    , core_(core)
    , system_directory_(config.fetch<std::string>("system_directory", "."))
    , save_directory_(config.fetch<std::string>("save_directory", "."))
  {
    current_ = this;
    core.set_environment(environment);
    core.set_video_refresh(video_refresh);
    core.set_audio_sample(audio_sample);
    core.set_audio_sample_batch(audio_sample_batch);
    core.set_input_poll(input_poll);
    core.set_input_state(input_state);
    core.init();
    core_initialized_ = true;
  }

  ~Context() {
    if (game_loaded_) {
      unload_game();
    }
    if (core_initialized_) {
      core_.deinit();
    }
  }

  static Context * current() { return current_; }

  auto & frontend() { return frontend_; }
  auto & core() { return core_; }

  auto system_info() {
    struct retro_system_info system_info;
    core_.get_system_info(&system_info);
    return system_info;
  }

  auto av_info() {
    struct retro_system_av_info av_info;
    core_.get_system_av_info(&av_info);
    return av_info;
  }

  void load_game(std::string filename) {
    retro_game_info info = { filename.c_str(), nullptr, 0, "" };
    std::string str;

    if (!system_info().need_fullpath) {
      std::ifstream file(filename.data());
      std::stringstream buf;
      buf << file.rdbuf();
      str = buf.str();

      info.data = str.data();
      info.size = str.length();
    }

    if (!core_.load_game(&info)) {
      throw std::runtime_error("retro_load_game failed");
    }

    game_loaded_ = true;

    frontend_.game_loaded(filename);
  }

  void init() {
    auto av = av_info();
    frontend_.init(av);
  }

  void unload_game() {
    if (game_loaded_) {
      frontend_.unloading_game();
      core_.unload_game();
      game_loaded_ = false;
      frontend_.game_unloaded();
    }
  }

  void run_core() {
    core_.run();
  }

private:
  static void log(enum retro_log_level level, const char *fmt, ...) {
    try {
      std::va_list ap;
      va_start(ap, fmt);
      Context::current()->frontend().log_libretro(level, fmt, ap);
      va_end(ap);
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
    }
  }

  static bool environment(unsigned int cmd, void * data) {
    try {
      auto * current = Context::current();

      switch (cmd) {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        {
          auto * cb = static_cast<retro_log_callback *>(data);
          cb->log = Context::log;
          return true;
        }

        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
          *static_cast<bool *>(data) = true;
          return true;

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
          return current->frontend().video_set_pixel_format(*static_cast<retro_pixel_format *>(data));

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
          *static_cast<char const * *>(data) = current->system_directory_.c_str();
          return true;

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
          *static_cast<char const * *>(data) = current->save_directory_.c_str();
          return true;

        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: // 8
        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 11
        case RETRO_ENVIRONMENT_GET_VARIABLE: // 15
        case RETRO_ENVIRONMENT_SET_VARIABLES: // 16
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: // 17
        case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO: // 34
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: // 35
        case RETRO_ENVIRONMENT_SET_GEOMETRY: // 37
        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: // 52
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: // 36 (experimental)
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS: // 42 (experimental)
        case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: // 47 (experimental)
          // TODO
          return false;

        default:
          warn_unknown_environment(cmd);
          return false;
      }
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
    }
    return false;
  }

  static void warn_unknown_environment(unsigned int cmd) {
    if (!warned_unknown_environment_[cmd]) {
      std::cout << "Uknown environment " << cmd << std::endl;
      warned_unknown_environment_[cmd] = true;
    }
  }

  static void video_refresh(void const * data, unsigned int width, unsigned int height, std::size_t pitch) {
    try {
      Context::current()->frontend().video_refresh(data, width, height, pitch);
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
    }
  }

  static void input_poll(void) {
    try {
      Context::current()->frontend().poll_input();
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
    }
  }

  static std::int16_t input_state(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
    try {
      return Context::current()->frontend().input_state(port, device, index, id);
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
      return 0;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
      return 0;
    }
  }

  static void audio_sample(std::int16_t left, std::int16_t right) {
    try {
      return Context::current()->frontend().audio_sample(left, right);
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
    }
  }

  static std::size_t audio_sample_batch(const std::int16_t * data, std::size_t frames) {
    try {
      return Context::current()->frontend().audio_sample_batch(data, frames);
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    } catch(...) {
      std::cout << "Unexpected error" << std::endl;
    }
    return 0;
  }

private:
  static inline Context * current_ = 0;

  Frontend & frontend_;
  Core & core_;

  std::string const & system_directory_;
  std::string const & save_directory_;

  bool core_initialized_ = false;
  bool game_loaded_ = false;

  static inline std::map<unsigned int, bool> warned_unknown_environment_;
};

}
