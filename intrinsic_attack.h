#pragma once

#include "body_part.h"
#include "item_type.h"

class ItemType;

struct IntrinsicAttack {
  enum Active {
    ALWAYS,
    NO_WEAPON,
    NEVER
  };
  IntrinsicAttack(ItemType, Active = NO_WEAPON);
  IntrinsicAttack(const IntrinsicAttack&);
  IntrinsicAttack& operator = (const IntrinsicAttack&);
  SERIALIZATION_DECL(IntrinsicAttack)
  PItem SERIAL(item);
  ItemType SERIAL(itemType);
  Active SERIAL(active) = NO_WEAPON;
};
