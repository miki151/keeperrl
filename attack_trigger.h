#pragma once

#include "util.h"
#include "furniture_type.h"
#include "enum_variant.h"

struct RoomTriggerInfo {
  FurnitureType type;
  double probPerSquare;
  COMPARE_ALL(type, probPerSquare)
};

RICH_ENUM(AttackTriggerId,
  POWER,
  SELF_VICTIMS,
  ENEMY_POPULATION,
  GOLD,
  STOLEN_ITEMS,
  ROOM_BUILT,
  TIMER,
  NUM_CONQUERED,
  MINING_IN_PROXIMITY,
  FINISH_OFF,
  PROXIMITY
);

class AttackTrigger : public EnumVariant<AttackTriggerId, TYPES(int, RoomTriggerInfo),
        ASSIGN(int,
            AttackTriggerId::ENEMY_POPULATION,
            AttackTriggerId::GOLD,
            AttackTriggerId::TIMER,
            AttackTriggerId::NUM_CONQUERED
        ),
        ASSIGN(RoomTriggerInfo,
            AttackTriggerId::ROOM_BUILT
        )> {
  using EnumVariant::EnumVariant;
};

struct TriggerInfo {
  AttackTrigger trigger;
  double value;
};

