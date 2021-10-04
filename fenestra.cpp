#include "Config.hpp"
#include "Core.hpp"
#include "Frontend.hpp"
#include "Context.hpp"
#include "Netcmds.hpp"
#include "Loop.hpp"

// Plugins
#include "Logger.hpp"
#include "Audio.hpp"
#include "Video.hpp"
#include "Capture.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  using namespace fenestra;

  if (argc < 3) {
    std::stringstream strm;
    strm << "usage: " << argv[0] << " <core> <game>";
    throw std::runtime_error(strm.str());
  }

  Core core(argv[1]);

  Config config("fenestra.cfg");
  Perf perf;
  Frontend frontend("Fenestra", core, config, perf);

  frontend.add_plugin<Logger>("Logger");
  frontend.add_plugin<Audio>("Audio");
  frontend.add_plugin<Video>("Video");
  frontend.add_plugin<Capture>("Capture");
  frontend.add_plugin<Netcmds>("Netcmds");

  Context ctx(frontend, core, config);
  ctx.load_game(argv[2]);
  ctx.init();

  Loop loop(frontend, ctx, perf);
  loop.run();
}
