#pragma once

#include "stdafx.h"
#include "util.h"
#include "file_path.h"

class Effect;
class ItemType;

class PrettyPrinting {
  public:
  template<typename T>
  static optional<string> parseObject(T& object, const string&, optional<string> filename = none);

  template<typename T>
  static optional<string> parseObject(T& object, FilePath path) {
    if (auto contents = path.readContents())
      return PrettyPrinting::parseObject<T>(object, *contents,  string(path.getFileName()));
    else
      return "Couldn't open file: "_s + path.getPath();
  }
};
