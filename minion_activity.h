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
    BE_WHIPPED,
    BE_TORTURED,
    BE_EXECUTED,
    CONSTRUCTION,
    DIGGING,
    WORKING,
    HAULING,
    IDLE,
    POETRY,
    DISTILLATION,
    MINION_ABUSE,
    GUARDING1,
    GUARDING2,
    GUARDING3,
    PHYLACTERY
);

class Position;
class ContentFactory;

class MinionActivities {
  public:
  MinionActivities(const ContentFactory*);
  static WTask getExisting(Collective*, Creature*, MinionActivity);
  PTask generate(Collective*, Creature*, MinionActivity) const;
  static PTask generateDropTask(Collective*, Creature*, MinionActivity);
  static optional<TimeInterval> getDuration(const Creature*, MinionActivity);
  vector<pair<Position, FurnitureLayer>> getAllPositions(const Collective*, const Creature*, MinionActivity) const;
  const vector<FurnitureType>& getAllFurniture(MinionActivity) const;
  optional<MinionActivity> getActivityFor(const Collective*, const Creature*, FurnitureType) const;

  SERIALIZATION_DECL(MinionActivities)

  private:
  EnumMap<MinionActivity, vector<FurnitureType>> SERIAL(allFurniture);
  unordered_map<FurnitureType, MinionActivity, CustomHash<FurnitureType>> SERIAL(activities);
};
