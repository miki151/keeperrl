#include "stdafx.h"
#include "initial_content_factory.h"
#include "game_config.h"

static optional<string> checkGroupCounts(const map<string, vector<ImmigrantInfo>>& immigrants) {
  for (auto& group : immigrants)
    for (auto& elem : group.second)
      if (elem.getGroupSize().isEmpty())
        return "Bad immigrant group size: " + toString(elem.getGroupSize()) +
            ". Lower bound is inclusive, upper bound exclusive.";
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
    break;
  }
}
