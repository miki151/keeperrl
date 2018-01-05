#pragma once

#include "body_part.h"

class ItemType;

struct IntrinsicAttack {
  enum Active {
    ALWAYS,
    NO_WEAPON,
    NEVER
  };
  IntrinsicAttack(ItemType, Active = NO_WEAPON);
  SERIALIZATION_DECL(IntrinsicAttack)
  PItem SERIAL(item);
  Active SERIAL(active);
};
