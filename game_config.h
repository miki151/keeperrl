#pragma once

#include "util.h"
#include "directory_path.h"
#include "pretty_printing.h"
#include "file_path.h"

constexpr auto gameConfigSubdir = "game_config";

enum class GameConfigId {
  CAMPAIGN_VILLAINS,
  PLAYER_CREATURES,
  BUILD_MENU,
  WORKSHOPS_MENU,
  IMMIGRATION,
  TECHNOLOGY,
  CREATURE_ATTRIBUTES,
  CREATURE_INVENTORY,
  SPELL_SCHOOLS,
  SPELLS,
  Z_LEVELS,
  RESOURCE_COUNTS,
  GAME_INTRO_TEXT,
  TILES,
  FURNITURE,
  FURNITURE_LISTS,
  ENEMIES,
  ITEM_LISTS,
  EXTERNAL_ENEMIES,
  ITEMS,
  BUILDING_INFO,
  NAMES
};

class GameConfig {
  public:
  GameConfig(DirectoryPath modsPath, string modName);
  template<typename T>
  [[nodiscard]] optional<string> readObject(T& object, GameConfigId id, KeyVerifier* keyVerifier) const {
    return PrettyPrinting::parseObject<T>(object, path.file(getConfigName(id) + ".txt"_s), keyVerifier);
  }

  const DirectoryPath& getPath() const;
  const string& getModName() const;

  private:
  static const char* getConfigName(GameConfigId);
  DirectoryPath path;
  string modName;
};
