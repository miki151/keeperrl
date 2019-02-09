#pragma once

#include "enum_variant.h"
#include "creature_group.h"

enum class AttackBehaviourId {
  KILL_LEADER,
  KILL_MEMBERS,
  STEAL_GOLD,
  CAMP_AND_SPAWN,
  HALLOWEEN_KIDS
};

class AttackBehaviour : public EnumVariant<AttackBehaviourId, TYPES(int, CreatureGroup),
        ASSIGN(int, AttackBehaviourId::KILL_MEMBERS),
        ASSIGN(CreatureGroup, AttackBehaviourId::CAMP_AND_SPAWN)> {
  public:
  using EnumVariant::EnumVariant;
};
