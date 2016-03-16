#ifndef _CAMPAIGN_H
#define _CAMPAIGN_H

#include "util.h"

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

  struct SiteInfo {
    string SERIAL(description);
    vector<ViewId> SERIAL(viewId);
    optional<VillainInfo> SERIAL(villain);
    optional<PlayerInfo> SERIAL(player);
    bool SERIAL(blocked);
    bool canEmbark() const;
    SERIALIZE_ALL(description, viewId, villain, player, blocked);
  };

  Table<PModel> buildModels(ProgressMeter*, RandomGen&, Options*) const;
  const Table<SiteInfo>& getSites() const;
  static optional<Campaign> prepareCampaign(View*, function<string()> worldNameGen, RandomGen&);
  optional<Vec2> getPlayerPos() const;
  const string& getWorldName() const;
  int getNumVillains() const;
  int getMaxVillains() const;
  int getMinVillains() const;
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
