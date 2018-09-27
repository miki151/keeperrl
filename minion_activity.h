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
  static WTask getExisting(WCollective, WCreature, MinionActivity);
  static PTask generate(WCollective, WCreature, MinionActivity);
  static PTask generateDropTask(WCollective, WCreature, MinionActivity);
  static optional<TimeInterval> getDuration(WConstCreature, MinionActivity);
  static vector<Position> getAllPositions(WConstCollective, WConstCreature, MinionActivity);
  static const vector<FurnitureType>& getAllFurniture(MinionActivity);
  static optional<MinionActivity> getActivityFor(WConstCollective, WConstCreature, FurnitureType);
};
