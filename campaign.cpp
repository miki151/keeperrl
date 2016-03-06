#include "stdafx.h"
#include "campaign.h"
#include "view.h"
#include "view_id.h"
#include "model_builder.h"
#include "model.h"
#include "progress_meter.h"

template <class Archive> 
void Campaign::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, sites, playerPos, worldName);
}

SERIALIZABLE(Campaign);
SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

bool Campaign::SiteInfo::canEmbark() const {
  return !villain && !blocked;
}

Vec2 Campaign::getPlayerPos() const {
  return playerPos;
}

Campaign::Campaign(Vec2 size) : sites(size, {}) {
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
  int numBlocked = random.get(4, 8);
  while (1) {
    Campaign campaign(size);
    campaign.worldName = worldName;
    for (Vec2 v : Rectangle(size)) {
      campaign.sites[v].viewId.push_back(ViewId::GRASS);
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
    for (int i : Range(numBlocked)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].blocked = true;
      campaign.sites[pos].viewId.push_back(ViewId::MAP_MOUNTAIN);
      if (freePos.empty())
        break;
    }
    CampaignAction action = view->prepareCampaign(campaign);
    switch (action.getId()) {
      case CampaignActionId::INC_WIDTH: size.x += action.get<int>(); break;
      case CampaignActionId::INC_HEIGHT: size.y += action.get<int>(); break;
      case CampaignActionId::CANCEL: return none;
      case CampaignActionId::CHOOSE_SITE:
          campaign.playerPos = action.get<Vec2>();
          campaign.sites[campaign.playerPos].villain = VillainInfo{ViewId::KEEPER, "keeper"};
          return campaign;
    }
  }
}

Table<PModel> Campaign::buildModels(ProgressMeter* meter, RandomGen& random, Options* options) const {
  Table<PModel> ret(sites.getBounds());
  for (Vec2 v : sites.getBounds()) {
    meter->addProgress();
    if (v == playerPos) {
      //ret[v] = ModelBuilder::campaignBaseModel(nullptr, random, options, "pok");
      ret[v] = ModelBuilder::quickModel(nullptr, random, options);
      ModelBuilder::spawnKeeper(ret[v].get(), options);
    } else if (sites[v].villain)
      ret[v] = ModelBuilder::quickModel(nullptr, random, options);
      //ret[v] = ModelBuilder::campaignSiteModel(nullptr, random, options, "pok");
  }
  return ret;
}
