#include "dungeon_level.h"
#include "villain_type.h"

double DungeonLevel::getProgress(VillainType type) {
  switch (type) {
    case VillainType::ALLY:
      return 3;
    case VillainType::MINOR:
    case VillainType::NONE:
      return 1.5;
    case VillainType::LESSER:
      return 6;
    case VillainType::RETIRED:
    case VillainType::MAIN:
      return 12;
    case VillainType::PLAYER:
      FATAL << "Villain type not handled: " + ENUM_STRING(type);
      return 0;
  }
}

int DungeonLevel::getNecessaryProgress(int level) {
  return 2 * level + 1;
}

void DungeonLevel::onKilledVillain(VillainType type) {
  addAbsoluteProgress(getProgress(type));
}

void DungeonLevel::onKilledWave() {
  increaseLevel();
}

int DungeonLevel::numResearchAvailable() const {
  return level - consumedLevels;
}

double DungeonLevel::numPromotionsAvailable() const {
  return level - consumedPromotions;
}

void DungeonLevel::addAbsoluteProgress(double amount) {
  double relative = amount / getNecessaryProgress(level);
  relative = min(1 - progress, relative);
  progress += relative;
  amount -= relative * getNecessaryProgress(level);
  if (progress >= 1.0 || amount > 0.0001) {
    increaseLevel();
    addAbsoluteProgress(amount);
  }
}

void DungeonLevel::increaseLevel() {
  ++level;
  progress = 0;
}
