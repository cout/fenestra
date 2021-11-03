#pragma once

#include "libretro.h"

#include "DL.hpp"

#include <string>

namespace fenestra {

class Core {
public:
  Core(std::string const & sofile)
    : dl_(sofile, RTLD_LAZY)
  {
    *(void **)&set_environment = dl_.sym("retro_set_environment");
    *(void **)&set_video_refresh = dl_.sym("retro_set_video_refresh");
    *(void **)&set_audio_sample = dl_.sym("retro_set_audio_sample");
    *(void **)&set_audio_sample_batch = dl_.sym("retro_set_audio_sample_batch");
    *(void **)&set_input_poll = dl_.sym("retro_set_input_poll");
    *(void **)&set_input_state = dl_.sym("retro_set_input_state");
    *(void **)&init = dl_.sym("retro_init");
    *(void **)&deinit = dl_.sym("retro_deinit");
    *(void **)&api_version = dl_.sym("retro_api_version");
    *(void **)&get_system_info = dl_.sym("retro_get_system_info");
    *(void **)&get_system_av_info = dl_.sym("retro_get_system_av_info");
    *(void **)&set_controller_port_device = dl_.sym("retro_set_controller_port_device");
    *(void **)&reset = dl_.sym("retro_reset");
    *(void **)&run = dl_.sym("retro_run");
    *(void **)&serialize_size = dl_.sym("retro_serialize_size");
    *(void **)&serialize = dl_.sym("retro_serialize");
    *(void **)&unserialize = dl_.sym("retro_unserialize");
    *(void **)&cheat_reset = dl_.sym("retro_cheat_reset");
    *(void **)&cheat_set = dl_.sym("retro_cheat_set");
    *(void **)&load_game = dl_.sym("retro_load_game");
    *(void **)&unload_game = dl_.sym("retro_unload_game");
    *(void **)&get_region = dl_.sym("retro_get_region");
    *(void **)&get_memory_data = dl_.sym("retro_get_memory_data");
    *(void **)&get_memory_size = dl_.sym("retro_get_memory_size");
  }

  decltype(::retro_set_environment) * set_environment;
  decltype(::retro_set_video_refresh) * set_video_refresh;
  decltype(::retro_set_audio_sample) * set_audio_sample;
  decltype(::retro_set_audio_sample_batch) * set_audio_sample_batch;
  decltype(::retro_set_input_poll) * set_input_poll;
  decltype(::retro_set_input_state) * set_input_state;
  decltype(::retro_init) * init;
  decltype(::retro_deinit) * deinit;
  decltype(::retro_api_version) * api_version;
  decltype(::retro_get_system_info) * get_system_info;
  decltype(::retro_get_system_av_info) * get_system_av_info;
  decltype(::retro_set_controller_port_device) * set_controller_port_device;
  decltype(::retro_reset) * reset;
  decltype(::retro_run) * run;
  decltype(::retro_serialize_size) * serialize_size;
  decltype(::retro_serialize) * serialize;
  decltype(::retro_unserialize) * unserialize;
  decltype(::retro_cheat_reset) * cheat_reset;
  decltype(::retro_cheat_set) * cheat_set;
  decltype(::retro_load_game) * load_game;
  decltype(::retro_unload_game) * unload_game;
  decltype(::retro_get_region) * get_region;
  decltype(::retro_get_memory_data) * get_memory_data;
  decltype(::retro_get_memory_size) * get_memory_size;

private:
  DL dl_;
};

}
