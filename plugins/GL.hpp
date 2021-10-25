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

  class Stopwatch {
  public:
    Stopwatch() {
      glGenQueries(1, &query_id_);
      if (query_id_ == 0) {
	throw std::runtime_error("glGenQueries failed");
      }
    }

    ~Stopwatch() {
      glDeleteQueries(1, &query_id_);
    }

    void start() {
      glGetInteger64v(GL_TIMESTAMP, &start_time_);
      running_ = true;
    }

    void start(Nanoseconds start_time) {
      start_time_ = start_time.count();
      running_ = true;
    }

    void stop() {
      if (!running_) {
	throw std::runtime_error("Cannot stop stopwatch before it is started");
      }

      glQueryCounter(query_id_, GL_TIMESTAMP);
      stopping_ = true;
    }

    bool running() const { return running_; }
    bool stopping() const { return stopping_; }

    Nanoseconds start_time() {
      return Nanoseconds(start_time_);
    }

    Nanoseconds stop_time() {
      get_stop_time();
      return Nanoseconds(stop_time_);
    }

    Nanoseconds duration() {
      get_stop_time();
      if (start_time_ != 0 && stop_time_ != 0) {
	return Nanoseconds(stop_time_ - start_time_);
      } else {
	return Nanoseconds::zero();
      }
    }

    void reset() {
      if (stopping_) {
	throw std::runtime_error("Cannot reset stopwatch while in progress");
      }

      running_ = false;
      start_time_ = 0;
      stop_time_ = 0;
    }

  private:
    void get_stop_time() {
      if (stopping_) {
	GLint available = 0;
	glGetQueryObjectiv(query_id_, GL_QUERY_RESULT_AVAILABLE, &available);

	if (available) {
	  glGetQueryObjecti64v(query_id_, GL_QUERY_RESULT, &stop_time_);
	}

	if (stop_time_ != 0) {
	  running_ = false;
	  stopping_ = false;
	}
      }
    }

  private:
    GLuint query_id_ = 0;
    GLint64 start_time_ = 0;
    GLint64 stop_time_ = 0;
    bool running_ = false;
    bool stopping_ = false;
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

    render_timers_.resize(16);
    sync_timers_.resize(16);
  }

  virtual void video_refresh(const void * data, unsigned int width, unsigned int height, std::size_t pitch) override {
    next_render_query_idx_ = (render_query_idx_ + 1) % render_timers_.size();
    if (next_render_query_idx_ != render_result_idx_ && !render_timers_[render_query_idx_].running()) {
      flush_errors();
      render_timers_[render_query_idx_].start();
      log_errors("glGetInteger64v");
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

    if (render_timers_[render_query_idx_].running()) {
      flush_errors();
      render_timers_[render_query_idx_].stop();
      log_errors("glQueryCounter");
      render_query_idx_ = next_render_query_idx_;
    }
  }

  virtual void collect_metrics(Probe & probe, Probe::Dictionary & dictionary) override {
    if (!render_latency_key_) { render_latency_key_ = dictionary.define("Render latency", 1000); }
    if (!sync_latency_key_) { sync_latency_key_ = dictionary.define("Sync latency", 1000); }

    auto render_latency_ns = render_timers_[render_result_idx_].duration();
    if (render_latency_ns != Nanoseconds::zero()) {
      next_sync_query_idx_ = (sync_query_idx_ + 1) % sync_timers_.size();
      if (next_sync_query_idx_ != sync_result_idx_ && !sync_timers_[sync_query_idx_].running()) {
	sync_timers_[sync_query_idx_].start(render_timers_[render_result_idx_].stop_time());
      }

      render_result_idx_ = (render_result_idx_ + 1) % render_timers_.size();
    }

    auto sync_latency_ns = sync_timers_[sync_result_idx_].duration();
    if (sync_latency_ns != Nanoseconds::zero()) {
      sync_result_idx_ = (sync_result_idx_ + 1) % sync_timers_.size();
    }

    Probe::Value render_latency = render_latency_ns.count() / 1000;
    Probe::Value sync_latency = sync_latency_ns.count() / 1000;

    probe.meter(*render_latency_key_, Probe::VALUE, 0, render_latency);
    probe.meter(*sync_latency_key_, Probe::VALUE, 0, sync_latency);

    log_errors("GL");
  }

  virtual void window_refreshed() override {
    if (sync_timers_[sync_query_idx_].running()) {
      flush_errors();
      sync_timers_[sync_query_idx_].stop();
      log_errors("glQueryCounter");
      sync_query_idx_ = next_sync_query_idx_;
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
  std::size_t next_render_query_idx_ = 0;
  std::size_t render_query_idx_ = 0;
  std::vector<Stopwatch> render_timers_;

  std::size_t sync_query_idx_ = 0;
  std::size_t sync_result_idx_ = 0;
  std::size_t next_sync_query_idx_ = 0;
  std::vector<Stopwatch> sync_timers_;

  std::optional<Probe::Key> render_latency_key_;
  std::optional<Probe::Key> sync_latency_key_;
};

}
