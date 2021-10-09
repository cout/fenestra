#include <gstreamermm.h>

#include "Config.hpp"
#include "Core.hpp"
#include "Frontend.hpp"
#include "Context.hpp"
#include "Loop.hpp"

// Plugins
#include "Logger.hpp"
#include "Savefile.hpp"
#include "Portaudio.hpp"
#include "ALSA.hpp"
#include "Video.hpp"
#include "V4l2Stream.hpp"
#include "Gstreamer.hpp"
#include "SSR.hpp"
#include "Netcmds.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  using namespace fenestra;

  Gst::init(argc, argv);

  if (argc < 3) {
    std::stringstream strm;
    strm << "usage: " << argv[0] << " <core> <game>";
    throw std::runtime_error(strm.str());
  }

  Core core(argv[1]);

  Config config("fenestra.cfg");
  Perf perf;
  Frontend frontend("Fenestra", core, config);

  frontend.add_plugin<Logger>("logger");
  frontend.add_plugin<Savefile>("savefile");
  frontend.add_plugin<Portaudio>("portaudio");
  frontend.add_plugin<ALSA>("alsa");
  frontend.add_plugin<Video>("video");
  frontend.add_plugin<V4l2Stream>("v4l2stream");
  frontend.add_plugin<Gstreamer>("gstreamer");
  frontend.add_plugin<SSR>("ssr");
  frontend.add_plugin<Netcmds>("netcmds");

  Context ctx(frontend, core, config);
  ctx.load_game(argv[2]);
  ctx.init();

  Loop loop(frontend, ctx, perf);
  loop.run();
}
