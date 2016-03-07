#ifndef _CAMPAIGN_H
#define _CAMPAIGN_H

#include "util.h"

class View;
class ProgressMeter;
class Options;

enum class EnemyId {
  KNIGHTS,
  DWARVES,
  ELVES,
  RED_DRAGON,
  ELEMENTALIST,
  KEEPER,
};

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(viewId);
    EnemyId SERIAL(enemyId);
    string SERIAL(name);
    SERIALIZE_ALL(viewId, name, enemyId);
  };

  struct SiteInfo {
    string SERIAL(description);
    vector<ViewId> SERIAL(viewId);
    optional<VillainInfo> SERIAL(villain);
    bool SERIAL(blocked);
    bool canEmbark() const;
    SERIALIZE_ALL(description, viewId, villain, blocked);
  };

  Table<PModel> buildModels(ProgressMeter*, RandomGen&, Options*) const;
  const Table<SiteInfo>& getSites() const;
  static optional<Campaign> prepareCampaign(View*, const string& worldName, RandomGen&);
  Vec2 getPlayerPos() const;
  const string& getWorldName() const;

  SERIALIZATION_DECL(Campaign);

  private:
  Campaign(Vec2 size);
  Table<SiteInfo> SERIAL(sites);
  Vec2 SERIAL(playerPos);
  string SERIAL(worldName);
};


#endif
