#pragma once

#include "body_part.h"
#include "item_type.h"

class ItemType;

struct IntrinsicAttack {
  IntrinsicAttack(ItemType, bool isExtraAttack = false);
  IntrinsicAttack(const IntrinsicAttack&);
  IntrinsicAttack& operator = (const IntrinsicAttack&);
  IntrinsicAttack(IntrinsicAttack&&) = default;
  IntrinsicAttack& operator = (IntrinsicAttack&&) = default;
  void initializeItem(const ContentFactory*);
  SERIALIZATION_DECL(IntrinsicAttack)
  PItem SERIAL(item);
  ItemType SERIAL(itemType);
  bool SERIAL(active) = true;
  bool SERIAL(isExtraAttack) = false;
};
