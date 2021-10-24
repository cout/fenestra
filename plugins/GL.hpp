#pragma once

#include "Plugin.hpp"

#include <epoxy/gl.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <stdexcept>
#include <sstream>
#include <map>
#include <array>

namespace fenestra {

class GL
  : public Plugin
{
private:
  struct Pixel_Format {
    GLuint format = GL_RGB;
    GLuint type = GL_UNSIGNED_SHORT_5_5_5_1;
    GLuint bpp = 16;
  };

  static inline std::map<unsigned int, Pixel_Format> const pixel_formats = {
    { RETRO_PIXEL_FORMAT_0RGB1555, { GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1, 16 } },
    { RETRO_PIXEL_FORMAT_XRGB8888, { GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 32 } },
    { RETRO_PIXEL_FORMAT_RGB565,   { GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 16 } },
  };

public:
  GL(Config const & config)
    : log_errors_(config.fetch<bool>("gl.log_errors", false))
  {
  }

  ~GL()
  {
    if (tex_id_) {
      glDeleteTextures(1, &tex_id_);
    }
  }

  virtual void set_pixel_format(retro_pixel_format format) override {
    if (tex_id_) {
      throw std::runtime_error("Tried to change pixel format after initialization.");
    }

    pixel_format_ = pixel_formats.at(format);
  }

  virtual void set_geometry(Geometry const & geom) override {
    glEnable(GL_TEXTURE_2D);

    if (tex_id_)
      glDeleteTextures(1, &tex_id_);

    tex_id_ = 0;

    glGenTextures(1, &tex_id_);

    if (!tex_id_) {
      throw std::runtime_error("Failed to create the video texture");
    }

    glBindTexture(GL_TEXTURE_2D, tex_id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom.max_width(), geom.max_height(), 0,
	pixel_format_.format, pixel_format_.type, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    tex_w_ = geom.max_width();
    tex_h_ = geom.max_height();

    glGenQueries(render_query_ids_.size(), render_query_ids_.data());
    glGenQueries(sync_query_ids_.size(), sync_query_ids_.data());
  }

  virtual void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) override {
    next_render_query_idx_ = (render_query_idx_ + 1) % render_query_ids_.size();
    if (next_render_query_idx_ != render_result_idx_ && !render_query_started_) {
      flush_errors();
      glGetInteger64v(GL_TIMESTAMP, &render_start_time_[render_query_idx_]);
      log_errors("glGetInteger64v");
      render_query_started_ = true;
    }

    glBindTexture(GL_TEXTURE_2D, tex_id_);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / (pixel_format_.bpp / CHAR_BIT));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, pixel_format_.format, pixel_format_.type, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    render_w_ = width;
    render_h_ = height;
  }

  virtual void video_render() override {
    glBindTexture(GL_TEXTURE_2D, tex_id_);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    GLfloat vertexes[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,  1.0f };
    glVertexPointer(2, GL_FLOAT, 0, vertexes);

    GLfloat texcoords[8] = { 0.0f, render_h_ / tex_h_, 0.0f,  0.0f, render_w_ / tex_w_, render_h_ / tex_h_, render_w_ / tex_w_,  0.0f };
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    if (render_query_started_) {
      flush_errors();
      glQueryCounter(render_query_ids_[render_query_idx_], GL_TIMESTAMP);
      log_errors("glQueryCounter");
      render_query_idx_ = next_render_query_idx_;
      render_query_started_ = false;
    }
  }

  virtual void collect_metrics(Probe & probe, Probe::Dictionary & dictionary) override {
    if (!render_latency_key_) { render_latency_key_ = dictionary.define("Render latency", 1000); }
    if (!sync_latency_key_) { sync_latency_key_ = dictionary.define("Sync latency", 1000); }

    GLint available = 0;
    glGetQueryObjectiv(render_query_ids_[render_result_idx_], GL_QUERY_RESULT_AVAILABLE, &available);

    if (available) {
      GLint64 render_finish_time = 0;
      glGetQueryObjecti64v(render_query_ids_[render_result_idx_], GL_QUERY_RESULT, &render_finish_time);

      Probe::Value render_latency = (render_finish_time - render_start_time_[render_result_idx_]) / 1000;
      probe.meter(*render_latency_key_, Probe::VALUE, 0, render_latency);

      render_result_idx_ = (render_result_idx_ + 1) % render_query_ids_.size();

      next_sync_query_idx_ = (sync_query_idx_ + 1) % sync_query_ids_.size();
      if (next_sync_query_idx_ != sync_result_idx_ && !sync_query_started_) {
	sync_start_time_[sync_query_idx_] = render_finish_time;
	sync_query_started_ = true;
      }
    } else {
      probe.meter(*render_latency_key_, Probe::VALUE, 0, 0);
    }

    available = 0;
    glGetQueryObjectiv(sync_query_ids_[sync_result_idx_], GL_QUERY_RESULT_AVAILABLE, &available);

    if (available) {
      GLint64 sync_finish_time = 0;
      glGetQueryObjecti64v(sync_query_ids_[sync_result_idx_], GL_QUERY_RESULT, &sync_finish_time);

      Probe::Value sync_latency = (sync_finish_time - sync_start_time_[sync_result_idx_]) / 1000;
      probe.meter(*sync_latency_key_, Probe::VALUE, 0, sync_latency);

      sync_result_idx_ = (sync_result_idx_ + 1) % sync_query_ids_.size();
    } else {
      probe.meter(*sync_latency_key_, Probe::VALUE, 0, 0);
    }

    log_errors("GL");
  }

  virtual void window_refreshed() override {
    if (sync_query_started_) {
      flush_errors();
      glQueryCounter(sync_query_ids_[sync_query_idx_], GL_TIMESTAMP);
      log_errors("glQueryCounter");
      sync_query_idx_ = next_sync_query_idx_;
      sync_query_started_ = false;
    }
  }

  void flush_errors() {
    if (!log_errors_) return;

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) { }
  }

  void log_errors(char const * op) {
    if (!log_errors_) return;

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
      std::cout << op << ": " << gluErrorString(err) << " (" << err << ")" << std::endl;
    }
  }

private:
  bool const & log_errors_;

  GLuint tex_id_ = 0;
  GLint tex_w_ = 0;
  GLint tex_h_ = 0;
  GLfloat render_w_ = 0;
  GLfloat render_h_ = 0;

  Pixel_Format pixel_format_;

  std::size_t render_result_idx_ = 0;
  std::size_t render_query_idx_ = 0;
  std::size_t next_render_query_idx_ = 0;
  std::array<GLint64, 16> render_start_time_;
  std::array<GLuint, 16> render_query_ids_;
  bool render_query_started_ = false;

  std::size_t sync_result_idx_ = 0;
  std::size_t sync_query_idx_ = 0;
  std::size_t next_sync_query_idx_ = 0;
  std::array<GLint64, 16> sync_start_time_;
  std::array<GLuint, 16> sync_query_ids_;
  bool sync_query_started_ = false;

  std::optional<Probe::Key> render_latency_key_;
  std::optional<Probe::Key> sync_latency_key_;
};

}
