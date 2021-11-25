#include "Config.hpp"
#include "Core.hpp"
#include "Frontend.hpp"
#include "Context.hpp"
#include "Loop.hpp"

#include "plugins/KeyHandler.hpp"
#include "plugins/Logger.hpp"
#include "plugins/Perflog.hpp"
#include "plugins/Savefile.hpp"
#include "plugins/GLFWGamepad.hpp"
#include "plugins/EvdevGamepad.hpp"
#include "plugins/GL.hpp"
#include "plugins/Sync.hpp"
#include "plugins/Framedelay.hpp"
#include "plugins/V4l2Stream.hpp"
#include "plugins/SSR.hpp"
#include "plugins/Netcmds.hpp"
#include "plugins/Rusage.hpp"
#include "plugins/Screensaver.hpp"

#ifdef HAVE_PORTAUDIO
#include "plugins/Portaudio.hpp"
#endif

#ifdef HAVE_ALSA
#include "plugins/ALSA.hpp"
#endif

#ifdef HAVE_LIBPULSE
#include "plugins/Pulseaudio.hpp"
#endif

#ifdef HAVE_GSTREAMER
#include "plugins/Gstreamer.hpp"
#include <gstreamermm/init.h>
#endif

#include "popl.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  using namespace fenestra;

#ifdef HAVE_GSTREAMER
  Gst::init(argc, argv);
#endif

  popl::OptionParser op("Allowed options");
  auto core_option = op.add<popl::Value<std::string>>("", "core", "Path to libretro core");
  auto game_option = op.add<popl::Value<std::string>>("", "game", "Path to game to load");
  auto config_option = op.add<popl::Value<std::string>>("", "config", "Path to config file", "fenestra.cfg");
  auto extra_config_option = op.add<popl::Value<std::string>>("", "extra-config", "Path to extra config file");
  auto help_option = op.add<popl::Switch>("h", "help", "Show this help message");
  op.parse(argc, argv);

  if (help_option->is_set()) {
    std::cout << op << std::endl;
    return 0;
  }

  auto core_filename = core_option->value();
  auto game_filename = game_option->value();

  Config config(config_option->value());

  if (extra_config_option->is_set()) {
    config.load(extra_config_option->value());
  }

  Core core(core_filename);

  std::map<std::string, bool> default_plugins {
    { "key-handler", true }, { "logger", true }, { "perf", true }, { "savefile", true },
    { "glfw-gamepad", true }, { "portaudio", true }, { "gl", true }, { "sync", true },
    { "framedelay", true }, { "v4l2stream", true }, { "gstreamer", true }, { "ssr", true },
    { "netcmds", true }, { "rusage", true }
  };

  auto const & plugins = config.fetch<std::map<std::string, bool>>("plugins", default_plugins);

  Frontend frontend("Fenestra", core, config, plugins);

  frontend.add_plugin<KeyHandler>("key-handler");
  frontend.add_plugin<Logger>("logger");
  frontend.add_plugin<Perflog>("perflog");
  frontend.add_plugin<Savefile>("savefile");
  frontend.add_plugin<GLFWGamepad>("glfw-gamepad");
  frontend.add_plugin<EvdevGamepad>("evdev-gamepad");
  frontend.add_plugin<GL>("gl");
  frontend.add_plugin<Sync>("sync");
  frontend.add_plugin<Framedelay>("framedelay");
  frontend.add_plugin<V4l2Stream>("v4l2stream");
  frontend.add_plugin<SSR>("ssr");
  frontend.add_plugin<Netcmds>("netcmds");
  frontend.add_plugin<Rusage>("rusage");
  frontend.add_plugin<Screensaver>("screensaver");

#ifdef HAVE_PORTAUDIO
  frontend.add_plugin<Portaudio>("portaudio");
#endif

#ifdef HAVE_ALSA
  frontend.add_plugin<ALSA>("alsa");
#endif

#ifdef HAVE_LIBPULSE
  frontend.add_plugin<Pulseaudio>("pulseaudio");
#endif

#ifdef HAVE_GSTREAMER
  frontend.add_plugin<Gstreamer>("gstreamer");
#endif

  Context ctx(frontend, core, config);
  ctx.load_game(game_filename);
  ctx.init();

  Loop loop(frontend, ctx);
  loop.run();
}
