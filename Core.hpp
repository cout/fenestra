#pragma once

#include "libretro.h"

#include "DL.hpp"

namespace fenestra {

class Core {
public:
  Core(const char * sofile)
    : dl_(sofile, RTLD_LAZY)
  {
    *(void **)&retro_set_environment = dl_.sym("retro_set_environment");
    *(void **)&retro_set_video_refresh = dl_.sym("retro_set_video_refresh");
    *(void **)&retro_set_audio_sample = dl_.sym("retro_set_audio_sample");
    *(void **)&retro_set_audio_sample_batch = dl_.sym("retro_set_audio_sample_batch");
    *(void **)&retro_set_input_poll = dl_.sym("retro_set_input_poll");
    *(void **)&retro_set_input_state = dl_.sym("retro_set_input_state");
    *(void **)&retro_init = dl_.sym("retro_init");
    *(void **)&retro_deinit = dl_.sym("retro_deinit");
    *(void **)&retro_api_version = dl_.sym("retro_api_version");
    *(void **)&retro_get_system_info = dl_.sym("retro_get_system_info");
    *(void **)&retro_get_system_av_info = dl_.sym("retro_get_system_av_info");
    *(void **)&retro_set_controller_port_device = dl_.sym("retro_set_controller_port_device");
    *(void **)&retro_reset = dl_.sym("retro_reset");
    *(void **)&retro_run = dl_.sym("retro_run");
    *(void **)&retro_serialize_size = dl_.sym("retro_serialize_size");
    *(void **)&retro_serialize = dl_.sym("retro_serialize");
    *(void **)&retro_unserialize = dl_.sym("retro_unserialize");
    *(void **)&retro_cheat_reset = dl_.sym("retro_cheat_reset");
    *(void **)&retro_cheat_set = dl_.sym("retro_cheat_set");
    *(void **)&retro_load_game = dl_.sym("retro_load_game");
    *(void **)&retro_unload_game = dl_.sym("retro_unload_game");
    *(void **)&retro_get_region = dl_.sym("retro_get_region");
    *(void **)&retro_get_memory_data = dl_.sym("retro_get_memory_data");
    *(void **)&retro_get_memory_size = dl_.sym("retro_get_memory_size");
  }

  decltype(::retro_set_environment) * retro_set_environment;
  decltype(::retro_set_video_refresh) * retro_set_video_refresh;
  decltype(::retro_set_audio_sample) * retro_set_audio_sample;
  decltype(::retro_set_audio_sample_batch) * retro_set_audio_sample_batch;
  decltype(::retro_set_input_poll) * retro_set_input_poll;
  decltype(::retro_set_input_state) * retro_set_input_state;
  decltype(::retro_init) * retro_init;
  decltype(::retro_deinit) * retro_deinit;
  decltype(::retro_api_version) * retro_api_version;
  decltype(::retro_get_system_info) * retro_get_system_info;
  decltype(::retro_get_system_av_info) * retro_get_system_av_info;
  decltype(::retro_set_controller_port_device) * retro_set_controller_port_device;
  decltype(::retro_reset) * retro_reset;
  decltype(::retro_run) * retro_run;
  decltype(::retro_serialize_size) * retro_serialize_size;
  decltype(::retro_serialize) * retro_serialize;
  decltype(::retro_unserialize) * retro_unserialize;
  decltype(::retro_cheat_reset) * retro_cheat_reset;
  decltype(::retro_cheat_set) * retro_cheat_set;
  decltype(::retro_load_game) * retro_load_game;
  decltype(::retro_unload_game) * retro_unload_game;
  decltype(::retro_get_region) * retro_get_region;
  decltype(::retro_get_memory_data) * retro_get_memory_data;
  decltype(::retro_get_memory_size) * retro_get_memory_size;

private:
  DL dl_;
};

}
