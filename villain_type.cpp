#include "villain_type.h"
#include "t_string.h"

TStringId getName(VillainType type) {
  switch (type) {
    case VillainType::MAIN: return TStringId("VILLAIN_TYPE_MAIN");
    case VillainType::LESSER: return TStringId("VILLAIN_TYPE_LESSER");
    case VillainType::MINOR: return TStringId("VILLAIN_TYPE_MINOR");
    case VillainType::RETIRED: return TStringId("VILLAIN_TYPE_RETIRED");
    case VillainType::ALLY: return TStringId("VILLAIN_TYPE_ALLY");
    case VillainType::NONE: return TStringId("VILLAIN_TYPE_OTHER");
    case VillainType::PLAYER: return TStringId("VILLAIN_TYPE_PLAYER");
  }
}

bool blocksInfluence(VillainType type) {
  switch (type) {
    case VillainType::MAIN:
    case VillainType::LESSER:
      return true;
    default:
      return false;
  }
}
