#include "villain_type.h"

const char* getName(VillainType type) {
  switch (type) {
    case VillainType::MAIN: return "Main villain";
    case VillainType::LESSER: return "Lesser villain";
    case VillainType::ALLY: return "Ally";
    case VillainType::NONE: return "Other";
    case VillainType::PLAYER: return "Player";
  }
}
