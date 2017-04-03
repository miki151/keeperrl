#include "stdafx.h"
#include "village_control.h"
#include "attack_trigger.h"

template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CollectiveControl) & SUBCLASS(EventListener);
  serializeAll(ar, villain, victims, myItems, stolenItemCount, attackSizes, entries, maxEnemyPower);
}

SERIALIZABLE(VillageControl);
REGISTER_TYPE(ListenerTemplate<VillageControl>)
