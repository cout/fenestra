#pragma once

#include "fenestra/Plugin.hpp"
#include "fenestra/Clock.hpp"

#include <optional>

#include <sys/time.h>
#include <sys/resource.h>

namespace fenestra {

class Rusage
  : public Plugin
{
public:
  Rusage(Config::Subtree const & config) {
    getrusage(RUSAGE_SELF, &last_usage_);
  }

  virtual void collect_metrics(Probe & probe, Probe::Dictionary & dictionary) override {
    rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
      // TODO: how to handle this?
      std::cout << "getrusage failed" << std::endl;
    }

    if (!cs_key_) { cs_key_ = dictionary["Context switches"]; }
    if (!flt_key_) { flt_key_ = dictionary["Page faults"]; }

    auto nvcsw = usage.ru_nvcsw - last_usage_.ru_nvcsw;
    auto nivcsw = usage.ru_nivcsw - last_usage_.ru_nivcsw;
    Probe::Value cs = nvcsw + nivcsw;
    auto minflt = usage.ru_minflt - last_usage_.ru_minflt;
    auto majflt = usage.ru_majflt - last_usage_.ru_majflt;
    Probe::Value flt = minflt + majflt;

    probe.meter(*cs_key_, Probe::VALUE, 0, cs);
    probe.meter(*flt_key_, Probe::VALUE, 0, flt);

    last_usage_ = usage;
    ++updates_;
  }

private:
  std::optional<Probe::Key> cs_key_;
  std::optional<Probe::Key> flt_key_;
  rusage last_usage_;
  std::uint64_t updates_= 0;
};

}
