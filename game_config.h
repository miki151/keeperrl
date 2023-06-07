#pragma once

#include "util.h"
#include "directory_path.h"
#include "pretty_printing.h"
#include "file_path.h"

constexpr auto gameConfigSubdir = "mods";

enum class GameConfigId {
  CAMPAIGN_VILLAINS,
  KEEPER_CREATURES,
  ADVENTURER_CREATURES,
  BUILD_MENU,
  WORKSHOPS_MENU,
  IMMIGRATION,
  TECHNOLOGY,
  CREATURE_ATTRIBUTES,
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
  NAMES,
  BIOMES,
  CAMPAIGN_INFO,
  WORKSHOP_INFO,
  RESOURCE_INFO,
  LAYOUT_MAPPING,
  RANDOM_LAYOUTS,
  STORAGE_IDS,
  TILE_GAS_TYPES,
  PROMOTIONS,
  DANCE_POSITIONS,
  EQUIPMENT_GROUPS,
  HELP,
  ATTR_INFO,
  BUFFS,
  BODY_MATERIALS,
  KEYS,
  WORLD_MAPS
};

class GameConfig {
  public:
  GameConfig(vector<DirectoryPath> modDirs);
  template<typename T>
  [[nodiscard]] optional<string> readObject(T& object, GameConfigId id, KeyVerifier* keyVerifier) const {
    vector<FilePath> paths;
    string fileName = getConfigName(id) + ".txt"_s;
    for (auto& dir : dirs) {
      auto path = dir.file(fileName);
      if (path.exists())
        paths.push_back(std::move(path));
    }
    return PrettyPrinting::parseObject<T>(object, paths, keyVerifier);
  }

  static const char* getConfigName(GameConfigId);
  vector<DirectoryPath> dirs;
};
