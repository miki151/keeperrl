#pragma once

#include "util.h"
#include "directory_path.h"
#include "pretty_printing.h"
#include "file_path.h"

enum class GameConfigId {
  CAMPAIGN_VILLAINS,
  PLAYER_CREATURES,
  BUILD_MENU
};

class GameConfig {
  public:
  GameConfig(DirectoryPath);
  template<typename T>
  optional<string> readObject(T& object, GameConfigId id) {
    auto file = path.file(getConfigName(id) + ".txt"_s);
    if (auto contents = file.readContents())
      return PrettyPrinting::parseObject<T>(object, removeFormatting(*contents));
    else
      return "Couldn't open file: "_s + file.getPath();
  }

  private:
  static const char* getConfigName(GameConfigId);
  static string removeFormatting(string contents);
  DirectoryPath path;
};
