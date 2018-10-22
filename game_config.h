#pragma once

#include "util.h"
#include "directory_path.h"
#include "pretty_printing.h"
#include "file_path.h"

enum class GameConfigId {
  CAMPAIGN_VILLAINS,
  PLAYER_CREATURES
};

class GameConfig {
  public:
  GameConfig(DirectoryPath);
  template<typename T>
  T readObject(GameConfigId id) {
    auto file = path.file(getConfigName(id) + ".txt"_s);
    if (auto contents = file.readContents()) {
      T ret;
      if (auto error = PrettyPrinting::parseObject<T>(ret, removeFormatting(*contents))) {
        USER_FATAL << file.getPath() << " parsing error: " << *error;
        fail();
      } else
        return ret;
    } else {
      USER_FATAL << "Couldn't open file: " << file.getPath();
      fail();
    }
  }

  private:
  static const char* getConfigName(GameConfigId);
  static string removeFormatting(string contents);
  DirectoryPath path;
};
