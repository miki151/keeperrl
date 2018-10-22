#pragma once

#include "util.h"
#include "directory_path.h"
#include "pretty_printing.h"
#include "file_path.h"

enum class GameConfigId {
  CAMPAIGN_VILLAINS
};

class GameConfig {
  public:
  GameConfig(DirectoryPath);
  template<typename T>
  vector<T> readObjects(int count, GameConfigId id) {
    auto file = path.file(getConfigName(id) + ".txt"_s);
    if (auto contents = file.readContents())
      return PrettyPrinting::parseObjects<T>(count, removeFormatting(*contents));
    else {
      USER_FATAL << "Couldn't open game config file: " << file.getPath();
      return {};
    }
  }

  private:
  static const char* getConfigName(GameConfigId);
  static string removeFormatting(string contents);
  DirectoryPath path;
};
