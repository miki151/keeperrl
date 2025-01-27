#pragma once

#include "util.h"

RICH_ENUM(
  MinionTrait,
  LEADER,
  FIGHTER,
  WORKER,
  PRISONER,
  NO_EQUIPMENT,
  NO_LIMIT,
  FARM_ANIMAL,
  SUMMONED,
  NO_AUTO_EQUIPMENT,
  DOESNT_TRIGGER,
  INCREASE_POPULATION,
  AUTOMATON,
  NO_LEISURE_ZONE
);

class TStringId;
optional<TStringId> getImmigrantDescription(MinionTrait);
