#pragma once

#include "util.h"

RICH_ENUM(
  MsgType,
  FEEL, // deprectated
  COLLAPSE,
  FALL, // deprecated
  FALL_ASLEEP,
  PANIC,
  RAGE,
  DIE_OF,
  ARE, // bleeding
  YOUR, // your head is cut off
  WAKE_UP,
  DIE, //
  TELE_APPEAR,
  TELE_DISAPPEAR,
  ARE_NO_LONGER, // deprectated
  CAN_SEE_HIDING,
  HIT,
  TOUCH,
  CRAWL,
  TRIGGER_TRAP,
  DISARM_TRAP,
  GET_HIT_NODAMAGE, // body part
  HIT_THROWN_ITEM,
  HIT_THROWN_ITEM_PLURAL,
  MISS_THROWN_ITEM,
  MISS_THROWN_ITEM_PLURAL,
  ITEM_CRASHES,
  ITEM_CRASHES_PLURAL,
  STAND_UP,
  TURN_INVISIBLE,
  TURN_VISIBLE,
  ENTER_PORTAL,
  HAPPENS_TO, // deprecated
  BURN,
  DROWN,
  SET_UP_TRAP, // deprecated
  DECAPITATE,
  TURN, // deprecated
  BECOME,
  KILLED_BY, // deprecated
  BREAK_FREE,
  MISS_ATTACK, //deprecated
  PRAY, // deprecated
  SACRIFICE, // deprecated
  COPULATE,
  CONSUME
);

