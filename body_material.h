#pragma once

#include "stdafx.h"
#include "util.h"
#include "item_type.h"
#include "health_type.h"
#include "lasting_or_buff.h"

struct BodyMaterial {
  string SERIAL(name);
  HashSet<LastingOrBuff> SERIAL(intrinsicallyAffected);
  HashSet<LastingOrBuff> SERIAL(immuneTo);
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