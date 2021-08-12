#pragma once

#include "util.h"

struct DungeonLevel {
  void onKilledVillain(VillainType);
  void onKilledWave();
  int numResearchAvailable() const;
  void increaseLevel();
  bool popKeeperUpdate();
  double numPromotionsAvailable() const;
  static double getProgress(VillainType type);
  static int getNecessaryProgress(int level);

  int SERIAL(level) = 0;
  int SERIAL(consumedLevels) = 0;
  double SERIAL(consumedPromotions) = 0;
  double SERIAL(progress) = 0;
  SERIALIZE_ALL(level, consumedLevels, progress, consumedPromotions)

  private:
  void addAbsoluteProgress(double amount);
};
