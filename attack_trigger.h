#pragma once

#include "util.h"
#include "furniture_type.h"

RICH_ENUM(AttackTriggerId,
  POWER,
  SELF_VICTIMS,
  ENEMY_POPULATION,
  GOLD,
  STOLEN_ITEMS,
  ROOM_BUILT,
  TIMER,
  NUM_CONQUERED,
  ENTRY,
  FINISH_OFF,
  PROXIMITY
);

class AttackTrigger : public EnumVariant<AttackTriggerId, TYPES(int, FurnitureType),
        ASSIGN(int,
            AttackTriggerId::ENEMY_POPULATION,
            AttackTriggerId::GOLD,
            AttackTriggerId::TIMER,
            AttackTriggerId::NUM_CONQUERED
        ),
        ASSIGN(FurnitureType,
            AttackTriggerId::ROOM_BUILT
        )> {
  using EnumVariant::EnumVariant;
};

struct TriggerInfo {
  AttackTrigger trigger;
  double value;
};

