#pragma once
#include "unique_entity.h"
#include "serialization.h"

class Creature;

struct CorpseInfo {
  UniqueEntity<Creature>::Id SERIAL(victim);
  bool SERIAL(canBeRevived);
  bool SERIAL(hasHead);
  bool SERIAL(isSkeleton);
  SERIALIZE_ALL(victim, canBeRevived, hasHead, isSkeleton);
};


