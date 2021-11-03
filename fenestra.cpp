#include <gstreamermm.h>

#include "Config.hpp"
#include "Core.hpp"
#include "Frontend.hpp"
#include "Context.hpp"
#include "Loop.hpp"

#include "plugins/Logger.hpp"
#include "plugins/Perflog.hpp"
#include "plugins/Savefile.hpp"
#include "plugins/Portaudio.hpp"
#include "plugins/ALSA.hpp"
#include "plugins/Pulseaudio.hpp"
#include "plugins/GL.hpp"
#include "plugins/Sync.hpp"
#include "plugins/Framedelay.hpp"
#include "plugins/V4l2Stream.hpp"
#include "plugins/Gstreamer.hpp"
#include "plugins/SSR.hpp"
#include "plugins/Netcmds.hpp"
#include "plugins/Rusage.hpp"

#include "popl.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  using namespace fenestra;

  Gst::init(argc, argv);

  popl::OptionParser op("Allowed options");
  auto core_option = op.add<popl::Value<std::string>>("", "core", "Path to libretro core");
  auto game_option = op.add<popl::Value<std::string>>("", "game", "Path to game to load");
  auto help_option = op.add<popl::Switch>("h", "help", "Show this help message");
  op.parse(argc, argv);

  if (help_option->is_set()) {
    std::cout << op << std::endl;
    return 0;
  }

  auto core_filename = core_option->value();
  auto game_filename = game_option->value();

  Core core(core_filename);

  Config config("fenestra.cfg");
  Frontend frontend("Fenestra", core, config);

  frontend.add_plugin<Logger>("logger");
  frontend.add_plugin<Perflog>("perflog");
  frontend.add_plugin<Savefile>("savefile");
  frontend.add_plugin<Portaudio>("portaudio");
  frontend.add_plugin<ALSA>("alsa");
  frontend.add_plugin<Pulseaudio>("pulseaudio");
  frontend.add_plugin<GL>("gl");
  frontend.add_plugin<Sync>("sync");
  frontend.add_plugin<Framedelay>("framedelay");
  frontend.add_plugin<V4l2Stream>("v4l2stream");
  frontend.add_plugin<Gstreamer>("gstreamer");
  frontend.add_plugin<SSR>("ssr");
  frontend.add_plugin<Netcmds>("netcmds");
  frontend.add_plugin<Rusage>("rusage");

  Context ctx(frontend, core, config);
  ctx.load_game(game_filename);
  ctx.init();

  Loop loop(frontend, ctx);
  loop.run();
}
