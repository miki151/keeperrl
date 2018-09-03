#pragma once

#include "util.h"

struct DungeonLevel {
  void onKilledVillain(VillainType);
  void onKilledWave();
  void onLibraryWork(double amount);
  bool canConsumeLevel() const;
  void increaseLevel();
  int SERIAL(level) = 0;
  int SERIAL(consumedLevels) = 0;
  double SERIAL(progress) = 0;
  SERIALIZE_ALL(level, consumedLevels, progress)

  private:
  int getNecessaryProgress(int level) const;
  void addAbsoluteProgress(double amount);
};
