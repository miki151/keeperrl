#ifndef _CAMPAIGN_H
#define _CAMPAIGN_H

#include "util.h"
#include "main_loop.h"

class View;
class ProgressMeter;
class Options;

RICH_ENUM(EnemyId,
  KNIGHTS,
  WARRIORS,
  DWARVES,
  ELVES,
  ELEMENTALIST,
  LIZARDMEN,
  RED_DRAGON,
  GREEN_DRAGON,

  VILLAGE,
  BANDITS,
  ENTS,
  DRIADS,
  CYCLOPS,
  SHELOB,
  HYDRA,
  ANTS,

  DARK_ELVES,
  GNOMES,
  FRIENDLY_CAVE,
  ORC_VILLAGE,
  SOKOBAN
);

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(viewId);
    EnemyId SERIAL(enemyId);
    string SERIAL(name);
    enum Type { ALLY, LESSER, MAIN};
    string getDescription() const;
    bool isEnemy() const;
    Type SERIAL(type);
    SERIALIZE_ALL(viewId, name, enemyId, type);
  };
  struct PlayerInfo {
    ViewId SERIAL(viewId);
    SERIALIZE_ALL(viewId);
  };
  typedef MainLoop::RetiredSiteInfo RetiredSiteInfo;
  struct SiteInfo {
    vector<ViewId> SERIAL(viewId);
    optional<variant<VillainInfo, RetiredSiteInfo, PlayerInfo>> SERIAL(dweller);
    optional<VillainInfo> getVillain() const;
    optional<RetiredSiteInfo> getRetired() const;
    optional<PlayerInfo> getPlayer() const;
    bool isEnemy() const;
    bool isEmpty() const;
    bool SERIAL(blocked) = false;
    bool canEmbark() const;
    void setBlocked();
    optional<ViewId> getDwellerViewId() const;
    optional<string> getDwellerDescription() const;
    SERIALIZE_ALL(viewId, dweller, blocked);
  };

  const Table<SiteInfo>& getSites() const;
  void clearSite(Vec2);
  static optional<Campaign> prepareCampaign(View*, Options*, const vector<RetiredSiteInfo>&,
      function<string()> worldNameGen, RandomGen&);
  optional<Vec2> getPlayerPos() const;
  const string& getWorldName() const;
  int getNumGenVillains() const;
  int getNumRetVillains() const;
  bool isDefeated(Vec2) const;
  void setDefeated(Vec2);
  bool canTravelTo(Vec2) const;
  bool isInInfluence(Vec2) const;

  SERIALIZATION_DECL(Campaign);

  private:
  void refreshInfluencePos();
  Campaign(Vec2 size);
  Table<SiteInfo> SERIAL(sites);
  optional<Vec2> SERIAL(playerPos);
  string SERIAL(worldName);
  Table<bool> SERIAL(defeated);
  set<Vec2> SERIAL(influencePos);
  int SERIAL(influenceSize);
};


#endif
