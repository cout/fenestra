#include "Savefile.hpp"

#include <filesystem>

void
fenestra::Savefile::game_loaded(Core const & core, std::string const & filename) {
  saveram_ = std::make_unique<Saveram>(core);
  std::cout << "Saveram size is " << saveram_->size() << std::endl;

  auto save_filename = std::filesystem::path(save_directory_) /
                       std::filesystem::path(filename).filename();
  save_filename.replace_extension(".srm");

  this->open(save_filename.native(), saveram_->size());

  std::copy(begin(), end(), saveram_->begin());

  std::cout << "Loaded savefile " << save_filename.native() << std::endl;
  std::cout << p_ << std::endl;
  std::cout << fd_ << std::endl;
}
