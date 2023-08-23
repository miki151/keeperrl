#include "villain_type.h"

const char* getName(VillainType type) {
  switch (type) {
    case VillainType::MAIN: return "Main villain";
    case VillainType::LESSER: return "Lesser villain";
    case VillainType::MINOR: return "Minor villain";
    case VillainType::ALLY: return "Ally";
    case VillainType::NONE: return "Other";
    case VillainType::PLAYER: return "Player";
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
