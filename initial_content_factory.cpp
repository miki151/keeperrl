#include "stdafx.h"
#include "initial_content_factory.h"
#include "game_config.h"
#include "player_role.h"
#include "tribe_alignment.h"

static optional<string> checkGroupCounts(const map<string, vector<ImmigrantInfo>>& immigrants) {
  for (auto& group : immigrants)
    for (auto& elem : group.second)
      if (elem.getGroupSize().isEmpty())
        return "Bad immigrant group size: " + toString(elem.getGroupSize()) +
            ". Lower bound is inclusive, upper bound exclusive.";
  return none;
}

optional<string> InitialContentFactory::readVillainsTuple(const GameConfig* gameConfig) {
  if (auto error = gameConfig->readObject(villains, GameConfigId::CAMPAIGN_VILLAINS))
    return "Error reading campaign villains definition"_s + *error;
  auto has = [](vector<Campaign::VillainInfo> v, VillainType type) {
    return std::any_of(v.begin(), v.end(), [type](const auto& elem){ return elem.type == type; });
  };
  for (auto villainType : {VillainType::ALLY, VillainType::MAIN, VillainType::LESSER})
    for (auto role : ENUM_ALL(PlayerRole))
      for (auto alignment : ENUM_ALL(TribeAlignment)) {
        int index = [&] {
          switch (role) {
            case PlayerRole::KEEPER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return 0;
                case TribeAlignment::LAWFUL:
                  return 1;
              }
            case PlayerRole::ADVENTURER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return 2;
                case TribeAlignment::LAWFUL:
                  return 3;
              }
          }
        }();
        if (!has(villains[index], villainType))
          return "Empty " + EnumInfo<VillainType>::getString(villainType) + " villain list for alignment: "
                    + EnumInfo<TribeAlignment>::getString(alignment);
      }
  return none;
}

InitialContentFactory::InitialContentFactory(const GameConfig* gameConfig) {
  while (1) {
    if (auto error = gameConfig->readObject(technology, GameConfigId::TECHNOLOGY)) {
      USER_INFO << *error;
      continue;
    }
    for (auto& tech : technology.techs)
      for (auto& preq : tech.second.prerequisites)
        if (!technology.techs.count(preq)) {
          USER_INFO << "Technology prerequisite \"" << preq << "\" of \"" << tech.first << "\" is not available";
          continue;
        }
    if (auto error = gameConfig->readObject(workshopGroups, GameConfigId::WORKSHOPS_MENU)) {
      USER_INFO << *error;
      continue;
    }
    if (auto error = gameConfig->readObject(immigrantsData, GameConfigId::IMMIGRATION)) {
      USER_INFO << *error;
      continue;
    }
    if (auto error = checkGroupCounts(immigrantsData)) {
      USER_INFO << *error;
      continue;
    }
    if (auto error = gameConfig->readObject(buildInfo, GameConfigId::BUILD_MENU)) {
      USER_INFO << *error;
      continue;
    }
    if (auto error = readVillainsTuple(gameConfig)) {
      USER_INFO << *error;
      continue;
    }
    if (auto error = gameConfig->readObject(gameIntros, GameConfigId::GAME_INTRO_TEXT)) {
      USER_INFO << *error;
      continue;
    }
    break;
  }
}
