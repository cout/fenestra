#pragma once

#include "libretro.h"

namespace fenestra {

class Geometry {
public:
  Geometry(retro_game_geometry const & geom, float scale)
    : geom_(geom)
    , scale_(scale)
  {
    if (geom_.base_width < geom_.base_height) {
      adjusted_height_ = geom.base_height;
      adjusted_width_ = geom_.base_height * geom_.aspect_ratio;
    } else {
      adjusted_width_ = geom.base_width;
      adjusted_height_ = geom_.base_width / geom_.aspect_ratio;
    }
  }

  auto & geom() { return geom_; }
  auto const & geom() const { return geom_; }

  auto base_width() const { return geom_.base_width; }
  auto base_height() const { return geom_.base_height; }
  auto max_width() const { return geom_.max_width; }
  auto max_height() const { return geom_.max_height; }
  auto aspect_ratio() const { return geom_.aspect_ratio; }

  auto adjusted_width() const { return adjusted_width_; }
  auto adjusted_height() const { return adjusted_height_; }

  auto scaled_width() const { return scale_ * adjusted_width_; }
  auto scaled_height() const { return scale_ * adjusted_height_; }

private:
  retro_game_geometry geom_;
  float scale_;
  unsigned int adjusted_width_;
  unsigned int adjusted_height_;
};

}
