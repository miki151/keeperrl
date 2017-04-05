#pragma once

#include "util.h"

RICH_ENUM(MinionTask,
  SLEEP,
  GRAVE,
  TRAIN,
  EAT,
  CRAFT,
  STUDY,
  PRISON,
  //SACRIFICE,
  //WORSHIP,
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
  BE_TORTURED
);

class Position;

class MinionTasks {
  public:
  static PTask generate(WCollective, WCreature, MinionTask);
  static optional<double> getDuration(WConstCreature, MinionTask);
  static vector<Position> getAllPositions(WConstCollective, WConstCreature, MinionTask, bool onlyActive = false);
  static const vector<FurnitureType>& getAllFurniture(MinionTask);
  static optional<MinionTask> getTaskFor(WConstCreature, FurnitureType);


};
