#pragma once

#include "stdafx.h"
#include "util.h"
#include "item_type.h"
#include "health_type.h"

struct BodyMaterial {
  string SERIAL(name);
  unordered_set<LastingOrBuff, CustomHash<LastingOrBuff>> SERIAL(intrinsicallyAffected);
  unordered_set<LastingOrBuff, CustomHash<LastingOrBuff>> SERIAL(immuneTo);
  bool SERIAL(killedByBoulder) = true;
  bool SERIAL(canCopulate) = false;
  bool SERIAL(hasBrain) = false;
  bool SERIAL(flamable) = false;
  bool SERIAL(undead) = false;
  bool SERIAL(canLoseBodyParts) = true;
  bool SERIAL(losingHeadsMeansDeath) = false;
  string SERIAL(deathDescription);
  optional<HealthType> SERIAL(healthType);
  optional<ItemType> SERIAL(bodyPartItem);
  SERIALIZE_ALL(OPTION(losingHeadsMeansDeath), OPTION(canLoseBodyParts), NAMED(name), OPTION(intrinsicallyAffected), OPTION(immuneTo), OPTION(killedByBoulder), OPTION(canCopulate), OPTION(hasBrain), OPTION(flamable), OPTION(undead), NAMED(deathDescription), OPTION(healthType), NAMED(bodyPartItem))
};