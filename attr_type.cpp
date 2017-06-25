#include "stdafx.h"
#include "attr_type.h"

string getName(AttrType attr) {
  switch (attr) {
    case AttrType::DAMAGE: return "damage";
    case AttrType::DEFENSE: return "defense";
    case AttrType::SPELL_DAMAGE: return "spell damage";
    case AttrType::SPELL_DEFENSE: return "spell resistance";
    case AttrType::RANGED_DAMAGE: return "ranged damage";
    case AttrType::SPEED: return "speed";
  }
}

AttrType getCorrespondingDefense(AttrType type) {
  switch (type) {
    case AttrType::RANGED_DAMAGE:
    case AttrType::DAMAGE:
      return AttrType::DEFENSE;
    case AttrType::SPELL_DAMAGE:
      return AttrType::SPELL_DEFENSE;
    default:
      FATAL << "No defense for attribute " << EnumInfo<AttrType>::getString(type);
      return AttrType(0);
  }
}
