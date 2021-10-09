#include <gstreamermm.h>

#include "Config.hpp"
#include "Core.hpp"
#include "Frontend.hpp"
#include "Context.hpp"
#include "Loop.hpp"

#include "plugins/Logger.hpp"
#include "plugins/Perf.hpp"
#include "plugins/Perflog.hpp"
#include "plugins/Savefile.hpp"
#include "plugins/Portaudio.hpp"
#include "plugins/ALSA.hpp"
#include "plugins/Video.hpp"
#include "plugins/V4l2Stream.hpp"
#include "plugins/Gstreamer.hpp"
#include "plugins/SSR.hpp"
#include "plugins/Netcmds.hpp"

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
  Frontend frontend("Fenestra", core, config);

  frontend.add_plugin<Logger>("logger");
  frontend.add_plugin<Perf>("perf");
  frontend.add_plugin<Perflog>("perflog");
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

  Loop loop(frontend, ctx);
  loop.run();
}
