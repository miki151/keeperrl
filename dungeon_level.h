#pragma once

#include "util.h"

struct DungeonLevel {
  void onKilledVillain(VillainType);
  void onKilledWave();
  void onLibraryWork(double amount);
  int numResearchAvailable() const;
  void increaseLevel();
  bool popKeeperUpdate();
  static double getProgress(VillainType type);
  static int getNecessaryProgress(int level);

  int SERIAL(level) = 0;
  int SERIAL(consumedLevels) = 0;
  double SERIAL(progress) = 0;
  SERIALIZE_ALL(level, consumedLevels, progress)

  private:
  void addAbsoluteProgress(double amount);
};
