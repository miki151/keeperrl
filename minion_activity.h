#pragma once

#include "util.h"
#include "game_time.h"

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
    IDLE
);

class Position;

class MinionActivities {
  public:
  static WTask getExisting(WCollective, Creature*, MinionActivity);
  static PTask generate(WCollective, Creature*, MinionActivity);
  static PTask generateDropTask(WCollective, Creature*, MinionActivity);
  static optional<TimeInterval> getDuration(const Creature*, MinionActivity);
  static vector<Position> getAllPositions(WConstCollective, const Creature*, MinionActivity);
  static const vector<FurnitureType>& getAllFurniture(MinionActivity);
  static optional<MinionActivity> getActivityFor(WConstCollective, const Creature*, FurnitureType);
};
