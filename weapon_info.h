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
  vector<Effect> SERIAL(victimEffect);
  vector<Effect> SERIAL(attackerEffect);
  AttackMsg SERIAL(attackMsg) = AttackMsg::SWING;
  SERIALIZE_ALL(OPTION(twoHanded), OPTION(attackType), OPTION(meleeAttackAttr), OPTION(victimEffect), OPTION(attackerEffect), OPTION(attackMsg))
};
