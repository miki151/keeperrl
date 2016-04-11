#include "stdafx.h"
#include "village_control.h"

template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CollectiveControl);
  serializeAll(ar, villain, victims, myItems, stolenItemCount, attackSizes, entries, maxEnemyPower);
}

SERIALIZABLE(VillageControl);