#pragma once

#include "util.h"
#include "game_time.h"
#include "furniture_type.h"

RICH_ENUM(
    MinionActivity,
    SLEEP,
    TRAIN,
    ARCHERY,
    EAT,
    CRAFT,
    STUDY,
    EXPLORE,
    EXPLORE_NOCTURNAL,
    EXPLORE_CAVES,
    COPULATE,
    RITUAL,
    CROPS,
    SPIDER,
    THRONE,
    BE_WHIPPED,
    BE_TORTURED,
    BE_EXECUTED,
    CONSTRUCTION,
    DIGGING,
    WORKING,
    HAULING,
    IDLE,
    POETRY,
    MINION_ABUSE,
    GUARDING
);

class Position;
class ContentFactory;

class MinionActivities {
  public:
  MinionActivities(const ContentFactory*);
  static WTask getExisting(WCollective, Creature*, MinionActivity);
  PTask generate(WCollective, Creature*, MinionActivity) const;
  static PTask generateDropTask(WCollective, Creature*, MinionActivity);
  static optional<TimeInterval> getDuration(const Creature*, MinionActivity);
  vector<pair<Position, FurnitureLayer>> getAllPositions(WConstCollective, const Creature*, MinionActivity) const;
  const vector<FurnitureType>& getAllFurniture(MinionActivity) const;
  optional<MinionActivity> getActivityFor(WConstCollective, const Creature*, FurnitureType) const;

  SERIALIZATION_DECL(MinionActivities)

  private:
  EnumMap<MinionActivity, vector<FurnitureType>> SERIAL(allFurniture);
  unordered_map<FurnitureType, MinionActivity, CustomHash<FurnitureType>> SERIAL(activities);
};
