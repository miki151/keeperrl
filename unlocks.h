#pragma once

#include "stdafx.h"
#include "util.h"
#include "file_path.h"
#include "achievement_id.h"

class Options;

class Unlocks {
  public:
  Unlocks(Options*, FilePath);
  static Unlocks allUnlocked();
  bool isUnlocked(UnlockId) const;
  void unlock(UnlockId);
  void achieve(AchievementId);
  bool isAchieved(AchievementId) const;

  private:
  Unlocks();
  Options* options = nullptr;
  optional<FilePath> path;
};