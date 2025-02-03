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
  static optional<string> parseObject(T& object, const vector<string>&, vector<string> filename = {}, KeyVerifier* keyVerifier = nullptr, optional<GameConfigId> gameConfigId = none);

  template<typename T>
  static optional<string> parseObject(T& object, const string& text) {
    return parseObject(object, vector<string>(1, text));
  }

  template<typename T>
  static optional<string> parseObject(T& object, vector<FilePath> paths, KeyVerifier* keyVerifier, optional<GameConfigId> gameConfigId = none) {
    vector<string> allContent;
    vector<string> pathStrings;
    for (auto& path : paths) {
      pathStrings.push_back(path.getPath());
      if (auto contents = path.readContents())
        allContent.push_back(*contents);
      else
        return "Couldn't open file: "_s + path.getPath();
    }
    return PrettyPrinting::parseObject<T>(object, allContent, pathStrings, keyVerifier, gameConfigId);
  }
};
