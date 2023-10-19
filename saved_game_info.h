#pragma once

#include "util.h"
#include "view_id.h"
#include "enemy_id.h"

class ContentFactory;

struct SavedGameInfo {
  struct MinionInfo {
    ViewIdList SERIAL(viewId);
    int SERIAL(level);
    static MinionInfo get(const ContentFactory*, const Creature*);
    SERIALIZE_ALL(viewId, level)
  };
  ViewIdList getViewId() const;
  vector<MinionInfo> SERIAL(minions);
  struct RetiredEnemyInfo {
    EnemyId SERIAL(enemyId);
    VillainType SERIAL(villainType);
    SERIALIZE_ALL(enemyId, villainType)
  };
  optional<RetiredEnemyInfo> SERIAL(retiredEnemyInfo);
  string SERIAL(name);
  int SERIAL(progressCount);
  vector<string> SERIAL(spriteMods);
  SERIALIZE_ALL(minions, retiredEnemyInfo, name, progressCount, spriteMods)
};

struct OldSavedGameInfo {
  ViewIdList getViewId() const;
  vector<SavedGameInfo::MinionInfo> SERIAL(minions);
  double SERIAL(dangerLevel);
  string SERIAL(name);
  int SERIAL(progressCount);
  vector<string> SERIAL(spriteMods);
  SERIALIZE_ALL(minions, dangerLevel, name, progressCount, spriteMods)
};

OldSavedGameInfo getOldInfo(const SavedGameInfo&);
SavedGameInfo fromOldInfo(const OldSavedGameInfo&);
