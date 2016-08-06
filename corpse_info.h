#pragma once
#include "unique_entity.h"

struct CorpseInfo {
  UniqueEntity<Creature>::Id SERIAL(victim);
  bool SERIAL(canBeRevived);
  bool SERIAL(hasHead);
  bool SERIAL(isSkeleton);
  SERIALIZE_ALL(canBeRevived, hasHead, isSkeleton);
};


