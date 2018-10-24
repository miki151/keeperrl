#include "stdafx.h"
#include "game_config.h"
#include "pretty_printing.h"

const char* GameConfig::getConfigName(GameConfigId id) {
  switch (id) {
    case GameConfigId::CAMPAIGN_VILLAINS:
      return "campaign_villains";
    case GameConfigId::PLAYER_CREATURES:
      return "player_creatures";
    case GameConfigId::BUILD_MENU:
      return "build_menu";
  }
}

GameConfig::GameConfig(DirectoryPath path) : path(std::move(path)) {
}
