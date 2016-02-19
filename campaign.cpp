#include "stdafx.h"
#include "campaign.h"
#include "view.h"
#include "view_id.h"

template <class Archive> 
void Campaign::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, sites, playerPos, worldName);
}

SERIALIZABLE(Campaign);
SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

Vec2 Campaign::chooseSite(View*, const string& message) {
  return Vec2(0, 0);
}

Campaign::Campaign(Vec2 size) : sites(size) {
}

const string& Campaign::getWorldName() const {
  return worldName;
}

static Campaign::VillainInfo getRandomVillain(RandomGen& random) {
  return random.choose<Campaign::VillainInfo>({
      {ViewId::AVATAR, "knights"},
      {ViewId::ELF_LORD, "elves"},
      {ViewId::DWARF_BARON, "dwarves"},
      {ViewId::RED_DRAGON, "dragon"},
      
      });
}

optional<Campaign> Campaign::prepareCampaign(View* view, const string& worldName, RandomGen& random) {
  Vec2 size(8, 5);
  int numVillains = 4;
  while (1) {
    Campaign campaign(size);
    campaign.worldName = worldName;
    for (Vec2 v : Rectangle(size)) {
      campaign.sites[v].viewId = ViewId::GRASS;
      campaign.sites[v].description = "site " + toString(v);
    }
    vector<Vec2> freePos;
    for (Vec2 v : Rectangle(size))
      if (!campaign.sites[v].villain)
        freePos.push_back(v);
    while (freePos.size() < numVillains + 1)
      --numVillains;
    for (int i : Range(numVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].villain = getRandomVillain(random);
    }
    CampaignAction action = view->prepareCampaign(campaign);
    switch (action.getId()) {
      case CampaignActionId::INC_WIDTH: size.x += action.get<int>(); break;
      case CampaignActionId::INC_HEIGHT: size.y += action.get<int>(); break;
      case CampaignActionId::CANCEL: return none;
      case CampaignActionId::CHOOSE_SITE: campaign.playerPos = action.get<Vec2>(); return campaign;
    }
  }
}

