#include "stdafx.h"
#include "intrinsic_attack.h"
#include "item_type.h"
#include "item.h"

IntrinsicAttack::IntrinsicAttack(ItemType type, IntrinsicAttack::Active a) : itemType(type), active(a) {}

IntrinsicAttack::IntrinsicAttack(const IntrinsicAttack& o) : itemType(o.itemType), active(o.active) {
}

IntrinsicAttack& IntrinsicAttack::operator =(const IntrinsicAttack& o) {
  itemType = o.itemType;
  active = o.active;
  return *this;
}

void IntrinsicAttack::initializeItem(const ContentFactory* factory) {
  item = itemType.get(factory);
}

SERIALIZATION_CONSTRUCTOR_IMPL(IntrinsicAttack)

SERIALIZE_DEF(IntrinsicAttack, item, itemType, active)

#include "pretty_archive.h"
template<>
void IntrinsicAttack::serialize(PrettyInputArchive& ar1, unsigned) {
  ar1(itemType);
}
