#pragma once

#include "enum_variant.h"
#include "creature_factory.h"

enum class AttackBehaviourId {
  KILL_LEADER,
  KILL_MEMBERS,
  STEAL_GOLD,
  CAMP_AND_SPAWN,
  HALLOWEEN_KIDS
};

class AttackBehaviour : public EnumVariant<AttackBehaviourId, TYPES(int, CreatureFactory),
        ASSIGN(int, AttackBehaviourId::KILL_MEMBERS),
        ASSIGN(CreatureFactory, AttackBehaviourId::CAMP_AND_SPAWN)> {
  public:
  using EnumVariant::EnumVariant;
};
