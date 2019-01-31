#pragma once

#include "util.h"
#include "attack_type.h"
#include "attr_type.h"
#include "effect.h"
#include "msg_type.h"

RICH_ENUM(AttackMsg,
  SWING,
  THRUST,
  WAVE,
  KICK,
  BITE,
  TOUCH,
  CLAW,
  SPELL
);

struct WeaponInfo {
  bool SERIAL(twoHanded) = false;
  AttackType SERIAL(attackType) = AttackType::HIT;
  AttrType SERIAL(meleeAttackAttr) = AttrType::DAMAGE;
  optional<Effect> SERIAL(victimEffect);
  optional<Effect> SERIAL(attackerEffect);
  AttackMsg SERIAL(attackMsg) = AttackMsg::SWING;
  SERIALIZE_ALL(NAMED(twoHanded), NAMED(attackType), NAMED(meleeAttackAttr), NAMED(victimEffect), NAMED(attackerEffect), NAMED(attackMsg))
};
