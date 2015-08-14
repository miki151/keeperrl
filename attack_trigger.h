#ifndef _ATTACK_TRIGGER_H
#define _ATTACK_TRIGGER_H

#include "util.h"
#include "square_type.h"

RICH_ENUM(AttackTriggerId,
  POWER,
  SELF_VICTIMS,
  ENEMY_POPULATION,
  GOLD,
  STOLEN_ITEMS,
  ROOM_BUILT,
  TIMER
);

typedef EnumVariant<AttackTriggerId, TYPES(int, SquareType),
        ASSIGN(int, AttackTriggerId::ENEMY_POPULATION, AttackTriggerId::GOLD, AttackTriggerId::TIMER),
        ASSIGN(SquareType, AttackTriggerId::ROOM_BUILT)> AttackTrigger;

struct TriggerInfo {
  AttackTrigger trigger;
  double value;
};

#endif
