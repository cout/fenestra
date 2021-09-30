#include "Config.hpp"
#include "Core.hpp"
#include "Frontend.hpp"
#include "Context.hpp"
#include "Netcmds.hpp"
#include "Loop.hpp"

// Plugins
#include "Audio.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  using namespace fenestra;

  if (argc < 3) {
    std::stringstream strm;
    strm << "usage: " << argv[0] << " <core> <game>";
    throw std::runtime_error(strm.str());
  }

  Core core(argv[1]);

  Config config;
  Frontend frontend("Fenestra", core, config);

  frontend.add_plugin<Audio>();

  Context ctx(frontend, core, config);
  ctx.load_game(argv[2]);
  ctx.init();

  Netcmds netcmds(core, config);

  Loop loop(config, frontend, ctx, netcmds);
  loop.run();
}
