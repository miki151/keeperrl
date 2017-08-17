#include "stdafx.h"
#include "attr_type.h"

string getName(AttrType attr) {
  switch (attr) {
    case AttrType::DAMAGE: return "damage";
    case AttrType::DEFENSE: return "defense";
    case AttrType::SPELL_DAMAGE: return "spell damage";
    case AttrType::RANGED_DAMAGE: return "ranged damage";
    case AttrType::SPEED: return "speed";
  }
}
