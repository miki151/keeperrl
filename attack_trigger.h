#pragma once

#include "util.h"
#include "furniture_type.h"
#include "enum_variant.h"
#include "pretty_archive.h"

struct RoomTrigger {
  FurnitureType SERIAL(type);
  double SERIAL(probPerSquare);
  COMPARE_ALL(type, probPerSquare)
};

EMPTY_STRUCT(Power);
EMPTY_STRUCT(SelfVictims);
EMPTY_STRUCT(StolenItems);
EMPTY_STRUCT(MiningInProximity);
EMPTY_STRUCT(FinishOff);
EMPTY_STRUCT(Proximity);
EMPTY_STRUCT(Immediate);
EMPTY_STRUCT(AggravatingMinions);

struct EnemyPopulation {
  int SERIAL(value);
  COMPARE_ALL(value)
};

struct Gold {
  int SERIAL(value);
  COMPARE_ALL(value)
};

struct Timer {
  int SERIAL(value);
  COMPARE_ALL(value)
};

struct NumConquered {
  int SERIAL(value);
  COMPARE_ALL(value)
};

#define ATTACK_TRIGGERS_LIST\
  X(RoomTrigger, 0)\
  X(Power, 1)\
  X(SelfVictims, 2)\
  X(StolenItems, 3)\
  X(MiningInProximity, 4)\
  X(FinishOff, 5)\
  X(Proximity, 6)\
  X(EnemyPopulation, 7)\
  X(Gold, 8)\
  X(Timer, 9)\
  X(NumConquered, 10)\
  X(Immediate, 11)\
  X(AggravatingMinions, 12)

#define VARIANT_TYPES_LIST ATTACK_TRIGGERS_LIST
#define VARIANT_NAME AttackTrigger

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

struct TriggerInfo {
  AttackTrigger trigger;
  double value;
};

