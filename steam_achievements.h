#pragma once

#include "stdafx.h"

class AchievementId;

struct SteamAchievements {
  SteamAchievements();
  void achieve(AchievementId);
};