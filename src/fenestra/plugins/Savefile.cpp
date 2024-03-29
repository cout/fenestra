#include "Savefile.hpp"

#include <filesystem>

void
fenestra::Savefile::open_or_create(Core const & core, std::string const & filename) {
  std::filesystem::create_directories(save_directory_);

  saveram_ = std::make_unique<Saveram>(core);
  std::cout << "Saveram size is " << saveram_->size() << std::endl;

  auto save_filename = std::filesystem::path(save_directory_) /
                       std::filesystem::path(filename).filename();
  save_filename.replace_extension(".srm");

  this->open(save_filename.native(), saveram_->size());

  std::cout << "Using savefile " << save_filename.native() << std::endl;
}
