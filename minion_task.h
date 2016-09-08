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
  STUDY,
  PRISON,
  TORTURE,
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
  THRONE
);

class MinionTasks {
  public:
  static PTask generate(Collective*, Creature*, MinionTask);
  static optional<double> getDuration(MinionTask);
};
