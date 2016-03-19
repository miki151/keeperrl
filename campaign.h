#ifndef _CAMPAIGN_H
#define _CAMPAIGN_H

#include "util.h"
#include "main_loop.h"

class View;
class ProgressMeter;
class Options;

RICH_ENUM(EnemyId,
  KNIGHTS,
  DWARVES,
  ELVES,
  DARK_ELVES,
  GNOMES,
  RED_DRAGON,
  GREEN_DRAGON,
  ELEMENTALIST,
  BANDITS,
  FRIENDLY_CAVE,
  SOKOBAN
);

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(viewId);
    EnemyId SERIAL(enemyId);
    string SERIAL(name);
    SERIALIZE_ALL(viewId, name, enemyId);
  };
  struct PlayerInfo {
    ViewId SERIAL(viewId);
    SERIALIZE_ALL(viewId);
  };
  typedef MainLoop::RetiredSiteInfo RetiredSiteInfo;
  struct SiteInfo {
    string SERIAL(description);
    vector<ViewId> SERIAL(viewId);
    optional<variant<VillainInfo, RetiredSiteInfo, PlayerInfo>> SERIAL(dweller);
    optional<VillainInfo> getVillain() const;
    optional<RetiredSiteInfo> getRetired() const;
    optional<PlayerInfo> getPlayer() const;
    bool isEmpty() const;
    bool SERIAL(blocked) = false;
    bool canEmbark() const;
    optional<ViewId> getDwellerViewId() const;
    SERIALIZE_ALL(description, viewId, dweller, blocked);
  };

  const Table<SiteInfo>& getSites() const;
  static optional<Campaign> prepareCampaign(View*, const vector<RetiredSiteInfo>&, function<string()> worldNameGen,
      RandomGen&);
  optional<Vec2> getPlayerPos() const;
  const string& getWorldName() const;
  int getNumGenVillains() const;
  int getNumRetVillains() const;
  bool isDefeated(Vec2) const;
  void setDefeated(Vec2);

  SERIALIZATION_DECL(Campaign);

  private:
  Campaign(Vec2 size);
  Table<SiteInfo> SERIAL(sites);
  optional<Vec2> SERIAL(playerPos);
  string SERIAL(worldName);
  Table<bool> SERIAL(defeated);
};


#endif
