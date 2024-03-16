#ifdef USE_STEAMWORKS

#include "steam_achievements.h"
#include "achievement_id.h"

#include "extern/steamworks/public/steam/isteamuserstats.h"
#include "extern/steamworks/public/steam/isteamuser.h"

SteamAchievements::SteamAchievements() {
  if (auto stats = SteamUserStats())
    if (auto user = SteamUser())
      if (user->BLoggedOn()) {
        stats->RequestCurrentStats();
      }
}

void SteamAchievements::achieve(AchievementId id) {
  if (auto stats = SteamUserStats()) {
    stats->SetAchievement(id.data());
    stats->StoreStats();
  }
}

#endif
