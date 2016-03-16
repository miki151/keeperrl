#include "stdafx.h"
#include "campaign.h"
#include "view.h"
#include "view_id.h"
#include "model_builder.h"
#include "model.h"
#include "progress_meter.h"

template <class Archive> 
void Campaign::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, sites, playerPos, worldName, defeated);
}

SERIALIZABLE(Campaign);
SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

bool Campaign::SiteInfo::canEmbark() const {
  return !villain && !blocked;
}

optional<Vec2> Campaign::getPlayerPos() const {
  return playerPos;
}

Campaign::Campaign(Vec2 size) : sites(size, {}), defeated(size, false) {
}

const string& Campaign::getWorldName() const {
  return worldName;
}

static Campaign::VillainInfo getRandomVillain(RandomGen& random) {
  return random.choose<Campaign::VillainInfo>({
      {ViewId::AVATAR, EnemyId::KNIGHTS, "knights"},
      {ViewId::ELF_LORD, EnemyId::ELVES, "elves"},
      {ViewId::DWARF_BARON, EnemyId::DWARVES, "dwarves"},
      {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "red dragon"},
      {ViewId::ELEMENTALIST, EnemyId::ELEMENTALIST, "elementalist"},
      {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "green dragon"},
      {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "dark elves"},
      {ViewId::GNOME_BOSS, EnemyId::GNOMES, "gnomes"},
      {ViewId::BANDIT, EnemyId::BANDITS, "bandits"},
      {ViewId::UNKNOWN_MONSTER, EnemyId::FRIENDLY_CAVE, "unknown"},
      {ViewId::UNKNOWN_MONSTER, EnemyId::SOKOBAN, "unknown"},
      });
}

bool Campaign::isDefeated(Vec2 pos) const {
  return defeated[pos];
}

void Campaign::setDefeated(Vec2 pos) {
  defeated[pos] = true;
}

int Campaign::getNumVillains() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (!!sites[v].villain)
      ++ret;
  return ret;
}

int Campaign::getMaxVillains() const {
  return 12;
}

int Campaign::getMinVillains() const {
  return 0;
}

optional<Campaign> Campaign::prepareCampaign(View* view, function<string()> worldNameGen, RandomGen& random) {
  Vec2 size(8, 5);
  int numVillains = 4;
  int numBlocked = random.get(4, 8);
  string worldName;
  while (1) {
    Campaign campaign(size);
    if (!worldName.empty())
      campaign.worldName = worldName;
    else
      campaign.worldName = worldNameGen();
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
      if (freePos.size() <= 1)
        break;
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].blocked = true;
      campaign.sites[pos].viewId.push_back(ViewId::MAP_MOUNTAIN);
    }
    while (1) {
      bool reroll = false;
      CampaignAction action = view->prepareCampaign(campaign);
      switch (action.getId()) {
        case CampaignActionId::INC_WIDTH:
            size.x += action.get<int>(); reroll = true;
            break;
        case CampaignActionId::INC_HEIGHT:
            size.y += action.get<int>(); reroll = true;
            break;
        case CampaignActionId::NUM_VILLAINS:
            numVillains = max(0, numVillains + action.get<int>());
            reroll = true;
            break;
        case CampaignActionId::REROLL_MAP:
            reroll = true; break;
        case CampaignActionId::CANCEL:
            return none;
        case CampaignActionId::WORLD_NAME:
            if (auto name = view->getText("Enter world name", worldName, 15,
                  "Leave blank to use a random name.")) {
              if (!name->empty())
                campaign.worldName = worldName = *name;
              else {
                worldName = "";
                campaign.worldName = worldNameGen();
              }
            }
            break;
        case CampaignActionId::CHOOSE_SITE:
            campaign.playerPos = action.get<Vec2>();
            campaign.sites[*campaign.playerPos].player = {ViewId::KEEPER};
            return campaign;
      }
      if (reroll)
        break;
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
      //ret[v] = ModelBuilder::quickModel(nullptr, random, options);
      ret[v] = ModelBuilder::campaignSiteModel(nullptr, random, options, "pok", sites[v].villain->enemyId);
  }
  return ret;
}
