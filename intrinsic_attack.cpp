#include "stdafx.h"
#include "intrinsic_attack.h"
#include "item_type.h"
#include "item.h"

IntrinsicAttack::IntrinsicAttack(ItemType type, bool isExtraAttack) : itemType(type), isExtraAttack(isExtraAttack) {}

IntrinsicAttack::IntrinsicAttack(const IntrinsicAttack& o) : itemType(o.itemType), active(o.active), isExtraAttack(o.isExtraAttack) {
}

IntrinsicAttack& IntrinsicAttack::operator =(const IntrinsicAttack& o) {
  itemType = o.itemType;
  active = o.active;
  isExtraAttack = o.isExtraAttack;
  return *this;
}

void IntrinsicAttack::initializeItem(const ContentFactory* factory) {
  if (!item)
    item = itemType.get(factory);
}

SERIALIZATION_CONSTRUCTOR_IMPL(IntrinsicAttack)

SERIALIZE_DEF(IntrinsicAttack, item, itemType, active, isExtraAttack)

#include "pretty_archive.h"
template<>
void IntrinsicAttack::serialize(PrettyInputArchive& ar1, unsigned) {
  ar1(NAMED(itemType), OPTION(isExtraAttack));
}
