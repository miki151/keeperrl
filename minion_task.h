#pragma once

#include "util.h"

RICH_ENUM(
    MinionTask,
    SLEEP,
    GRAVE,
    TRAIN,
    ARCHERY,
    EAT,
    CRAFT,
    STUDY,
    PRISON,
    LAIR,
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
    WORKER
);

class Position;

class MinionTasks {
  public:
  static WTask getExisting(WCollective, WCreature, MinionTask);
  static PTask generate(WCollective, WCreature, MinionTask);
  static optional<double> getDuration(WConstCreature, MinionTask);
  static vector<Position> getAllPositions(WConstCollective, WConstCreature, MinionTask, bool onlyActive = false);
  static const vector<FurnitureType>& getAllFurniture(MinionTask);
  static optional<MinionTask> getTaskFor(WConstCollective, WConstCreature, FurnitureType);
};
