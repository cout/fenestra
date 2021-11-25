#include "KeyHandler.hpp"

#include <filesystem>

void
fenestra::KeyHandler::game_loaded(Core const & core, std::string const & filename) {
  std::filesystem::create_directories(state_directory_);

  auto state_basename = std::filesystem::path(state_directory_) /
                       std::filesystem::path(filename).filename();
  state_basename.replace_extension(".state");

  state_basename_ = state_basename.native();
  core_ = &core;
}

void
fenestra::KeyHandler::unloading_game(Core const & core) {
  core_ = nullptr;
  state_basename_ = "";
}
