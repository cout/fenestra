#pragma once

#include "libretro.h"

#include "DL.hpp"

#include <string>

namespace fenestra {

class Core {
public:
  Core(std::string const & sofile)
    : dl_(sofile, RTLD_LAZY)
    , set_environment(dl_.sym<decltype(set_environment)>("retro_set_environment"))
    , set_video_refresh(dl_.sym<decltype(set_video_refresh)>("retro_set_video_refresh"))
    , set_audio_sample(dl_.sym<decltype(set_audio_sample)>("retro_set_audio_sample"))
    , set_audio_sample_batch(dl_.sym<decltype(set_audio_sample_batch)>("retro_set_audio_sample_batch"))
    , set_input_poll(dl_.sym<decltype(set_input_poll)>("retro_set_input_poll"))
    , set_input_state(dl_.sym<decltype(set_input_state)>("retro_set_input_state"))
    , init(dl_.sym<decltype(init)>("retro_init"))
    , deinit(dl_.sym<decltype(deinit)>("retro_deinit"))
    , api_version(dl_.sym<decltype(api_version)>("retro_api_version"))
    , get_system_info(dl_.sym<decltype(get_system_info)>("retro_get_system_info"))
    , get_system_av_info(dl_.sym<decltype(get_system_av_info)>("retro_get_system_av_info"))
    , set_controller_port_device(dl_.sym<decltype(set_controller_port_device)>("retro_set_controller_port_device"))
    , reset(dl_.sym<decltype(reset)>("retro_reset"))
    , run(dl_.sym<decltype(run)>("retro_run"))
    , serialize_size(dl_.sym<decltype(serialize_size)>("retro_serialize_size"))
    , serialize(dl_.sym<decltype(serialize)>("retro_serialize"))
    , unserialize(dl_.sym<decltype(unserialize)>("retro_unserialize"))
    , cheat_reset(dl_.sym<decltype(cheat_reset)>("retro_cheat_reset"))
    , cheat_set(dl_.sym<decltype(cheat_set)>("retro_cheat_set"))
    , load_game(dl_.sym<decltype(load_game)>("retro_load_game"))
    , unload_game(dl_.sym<decltype(unload_game)>("retro_unload_game"))
    , get_region(dl_.sym<decltype(get_region)>("retro_get_region"))
    , get_memory_data(dl_.sym<decltype(get_memory_data)>("retro_get_memory_data"))
    , get_memory_size(dl_.sym<decltype(get_memory_size)>("retro_get_memory_size"))
  {
  }

private:
  DL dl_;

public:
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
};

}
