#pragma once

#include "util.h"

RICH_ENUM(MinionTask,
  SLEEP,
  GRAVE,
  TRAIN,
  EAT,
  WORKSHOP,
  FORGE,
  LABORATORY,
  JEWELER,
  STEEL_FURNACE,
  STUDY,
  PRISON,
  //SACRIFICE,
  EXECUTE,
  //WORSHIP,
  LAIR,
  EXPLORE,
  EXPLORE_NOCTURNAL,
  EXPLORE_CAVES,
  COPULATE,
  CONSUME,
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
  static PTask generate(Collective*, Creature*, MinionTask);
  static optional<double> getDuration(const Creature*, MinionTask);
  static vector<Position> getAllPositions(const Collective*, const Creature*, MinionTask);
  static const vector<FurnitureType>& getAllFurniture(MinionTask);
  static optional<MinionTask> getTaskFor(const Creature*, FurnitureType);


};
