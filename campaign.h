#ifndef _CAMPAIGN_H
#define _CAMPAIGN_H

#include "util.h"

class View;

class Campaign {
  public:
  struct VillainInfo {
    ViewId SERIAL(viewId);
    string SERIAL(name);
    SERIALIZE_ALL(viewId, name);
  };

  struct SiteInfo {
    string SERIAL(description);
    ViewId SERIAL(viewId);
    optional<VillainInfo> SERIAL(villain);
    SERIALIZE_ALL(description, viewId, villain);
  };

  const Table<SiteInfo>& getSites() const;
  Vec2 chooseSite(View*, const string& message);
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
