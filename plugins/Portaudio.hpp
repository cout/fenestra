#pragma once

#include "Plugin.hpp"

#include <portaudiocpp/PortAudioCpp.hxx>
#include <portaudio.h>

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

namespace fenestra {

class Portaudio
  : public Plugin
{
public:
  Portaudio(Config const & config)
    : config_(config)
  {
  }

  ~Portaudio() {
  }

  std::map<std::string, std::vector<std::string>> list_devices() const {
    auto & system = portaudio::System::instance();
    std::map<std::string, std::vector<std::string>> devices;

    for (auto host_api_it = system.hostApisBegin(); host_api_it != system.hostApisEnd(); ++host_api_it) {
      std::vector<std::string> host_api_devices;
      for (auto it = host_api_it->devicesBegin(); it != host_api_it->devicesEnd(); ++it) {
        host_api_devices.push_back(it->name());
      }
      devices.emplace(host_api_it->name(), host_api_devices);
    }

    return devices;
  }

  virtual void set_sample_rate(int sample_rate) override {
    auto & system = portaudio::System::instance();
    auto & host_api = this->host_api(system, config_.audio_api());
    auto & device = this->device(host_api, config_.audio_device());

    auto suggested_latency = config_.audio_suggested_latency() * 0.001;
    portaudio::DirectionSpecificStreamParameters in_params(system.nullDevice(), 0, portaudio::INVALID_FORMAT, 0, 0, 0);
    portaudio::DirectionSpecificStreamParameters out_params(device, 2, portaudio::INT16, true, suggested_latency, nullptr);
    portaudio::StreamParameters params(in_params, out_params, sample_rate, 0, paNoFlag);
    stream_.open(params);
  }

  virtual void write_audio_sample(void const * buf, std::size_t frames) override {
    if (stream_.isStopped()) {
      stream_.start();
    }

    try {
      stream_.write(buf, frames);
    } catch(std::exception const & ex) {
      std::cout << "ERROR: " << ex.what() << std::endl;
    }
  }

private:
  portaudio::HostApi & host_api(portaudio::System & system, std::string_view name) {
    for (auto it = system.hostApisBegin(); it != system.hostApisEnd(); ++it) {
      if (name == it->name()) {
        return *it;
      }
    }

    std::stringstream strm;
    strm << "Could not find audio api `" << name << "'";
    throw std::runtime_error(strm.str());
  }

  portaudio::Device & device(portaudio::HostApi & host_api, std::string_view name) {
    for (auto it = host_api.devicesBegin(); it != host_api.devicesEnd(); ++it) {
      if (name == it->name()) {
        return *it;
      }
    }

    std::stringstream strm;
    strm << "Could not find device `" << name << "'";
    throw std::runtime_error(strm.str());
  }

private:
  Config const & config_;
  portaudio::AutoSystem auto_system_;
  portaudio::BlockingStream stream_;
};

}
