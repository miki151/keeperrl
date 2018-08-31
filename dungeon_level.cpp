#include "dungeon_level.h"
#include "villain_type.h"

static double getProgress(VillainType type) {
  switch (type) {
    case VillainType::ALLY:
      return 3;
    case VillainType::NONE:
      return 1.5;
    case VillainType::LESSER:
      return 11;
    case VillainType::MAIN:
      return 25;
    default:
      FATAL << "Villain type not handled: " + EnumInfo<VillainType>::getString(type);
      return 0;
  }
}

void DungeonLevel::onKilledVillain(VillainType type) {
  addAbsoluteProgress(getProgress(type));
}

void DungeonLevel::onKilledWave() {
  increaseLevel();
}

void DungeonLevel::onLibraryWork(double amount) {
  addAbsoluteProgress(0.0003 * amount);
}

bool DungeonLevel::canConsumeLevel() const {
  return consumedLevels < level;
}

int DungeonLevel::getNecessaryProgress(int level) const {
  return 2 * level + 1;
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
