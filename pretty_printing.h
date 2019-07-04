#pragma once

#include "stdafx.h"
#include "util.h"
#include "file_path.h"

class Effect;
class ItemType;
class KeyVerifier;

class PrettyPrinting {
  public:
  template<typename T>
  static optional<string> parseObject(T& object, const string&, optional<string> filename = none, KeyVerifier* keyVerifier = nullptr);

  template<typename T>
  static optional<string> parseObject(T& object, FilePath path, KeyVerifier* keyVerifier) {
    if (auto contents = path.readContents())
      return PrettyPrinting::parseObject<T>(object, *contents,  string(path.getFileName()), keyVerifier);
    else
      return "Couldn't open file: "_s + path.getPath();
  }
};
