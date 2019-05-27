#pragma once

#include "util.h"
#include "furniture_type.h"
#include "enum_variant.h"

struct RoomTrigger {
  FurnitureType type;
  double probPerSquare;
  COMPARE_ALL(type, probPerSquare)
};

EMPTY_STRUCT(Power);
EMPTY_STRUCT(SelfVictims);
EMPTY_STRUCT(StolenItems);
EMPTY_STRUCT(MiningInProximity);
EMPTY_STRUCT(FinishOff);
EMPTY_STRUCT(Proximity);

struct EnemyPopulation {
  int value;
  COMPARE_ALL(value)
};

struct Gold {
  int value;
  COMPARE_ALL(value)
};

struct Timer {
  int value;
  COMPARE_ALL(value)
};

struct NumConquered {
  int value;
  COMPARE_ALL(value)
};

MAKE_VARIANT2(AttackTrigger, RoomTrigger, Power, SelfVictims, StolenItems, MiningInProximity, FinishOff, Proximity, EnemyPopulation,
    Gold, Timer, NumConquered);

struct TriggerInfo {
  AttackTrigger trigger;
  double value;
};

