#include "stdafx.h"
#include "intrinsic_attack.h"
#include "item_type.h"
#include "item.h"

IntrinsicAttack::IntrinsicAttack(ItemType type, IntrinsicAttack::Active a) : item(type.get()), active(a) {
}

SERIALIZATION_CONSTRUCTOR_IMPL(IntrinsicAttack)

SERIALIZE_DEF(IntrinsicAttack, item, active)
