#pragma once

#include <json/json.h>

#include <set>
#include <map>
#include <string>
#include <fstream>

namespace fenestra::json {

inline void assign(bool & dest, Json::Value const & src) {
  dest = src.asBool();
}

inline void assign(int & dest, Json::Value const & src) {
  dest = src.asInt();
}

inline void assign(unsigned int & dest, Json::Value const & src) {
  dest = src.asUInt();
}

inline void assign(float & dest, Json::Value const & src) {
  dest = src.asFloat();
}

inline void assign(double & dest, Json::Value const & src) {
  dest = src.asDouble();
}

inline void assign(std::string & dest, Json::Value const & src) {
  dest = src.asString();
}

inline void assign(Milliseconds & dest, Json::Value const & src) {
  dest = Milliseconds(src.asDouble());
}

template <typename T>
inline void assign(std::map<std::string, T> & dest, Json::Value const & src) {
  std::map<std::string, T> result;
  auto it = src.begin();
  auto end = src.end();
  while (it != end) {
    T tmp;
    assign(tmp, *it);
    result[it.name()] = tmp;
    ++it;
  }
  dest = result;
}

inline
void
merge(Json::Value & target, Json::Value v) {
  for (auto it = v.begin(); it != v.end(); ++it) {
    auto & t = target[it.name()];
    if (it->type() == Json::objectValue && t.type() == Json::objectValue) {
      merge(t, *it);
    } else {
      t = *it;
    }
  }
}

inline
Json::Value
read_file(std::string const & filename) {
  Json::Value v;
  std::ifstream file(filename);
  file >> v;
  return v;
}

inline
Json::Value const *
deep_find(Json::Value const * v, std::string const & name) {
  auto it = name.begin();
  auto begin_word = it;
  auto end = name.end();

  while (it != end) {
    if (*it == '.') {
      v = v->find(&*begin_word, &*it);
      if (!v) return nullptr;
      ++it;
      begin_word = it;
    } else {
      ++it;
    }
  }

  return v->find(&*begin_word, &*it);
}

}
