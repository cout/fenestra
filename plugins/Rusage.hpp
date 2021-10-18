#pragma once

#include "Plugin.hpp"
#include "../Clock.hpp"

#include <optional>

#include <sys/time.h>
#include <sys/resource.h>

namespace fenestra {

class Rusage
  : public Plugin
{
public:
  Rusage(Config const & config) {
    getrusage(RUSAGE_SELF, &last_usage_);
  }

  virtual void collect_metrics(Probe & probe, Probe::Dictionary & dictionary) override {
    rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
      // TODO: how to handle this?
      std::cout << "getrusage failed" << std::endl;
    }

    if (!cs_key_) {
      cs_key_ = dictionary["Context switches"];
    }

    Probe::Value nvcsw = usage.ru_nvcsw - last_usage_.ru_nvcsw;
    Probe::Value nivcsw = usage.ru_nivcsw - last_usage_.ru_nivcsw;

    probe.meter(*cs_key_, Probe::VALUE, 0, nvcsw + nivcsw);

    last_usage_ = usage;
  }

private:
  std::optional<Probe::Key> cs_key_;
  rusage last_usage_;
};

}
