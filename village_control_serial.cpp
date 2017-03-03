#include "stdafx.h"
#include "village_control.h"
#include "event_proxy.h"
#include "attack_trigger.h"

template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CollectiveControl);
  serializeAll(ar, villain, victims, myItems, stolenItemCount, attackSizes, entries, maxEnemyPower, eventProxy);
}

SERIALIZABLE(VillageControl);
