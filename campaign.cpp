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
  return !dweller && !blocked;
}

bool Campaign::canTravelTo(Vec2 pos) const {
  return (getInfluencePos().count(pos) || playerPos == pos) && !sites[pos].isEmpty();
}

optional<Vec2> Campaign::getPlayerPos() const {
  return playerPos;
}

Campaign::Campaign(Vec2 size) : sites(size, {}), defeated(size, false) {
}

const string& Campaign::getWorldName() const {
  return worldName;
}

void Campaign::clearSite(Vec2 v) {
  sites[v] = SiteInfo{};
  sites[v].viewId = {ViewId::GRASS};
  sites[v].description = "site " + toString(v);
}

static Campaign::VillainInfo getRandomVillain(RandomGen& random) {
  return random.choose<Campaign::VillainInfo>({
      {ViewId::AVATAR, EnemyId::KNIGHTS, "Knights", true},
      {ViewId::ELF_LORD, EnemyId::ELVES, "Elves", true},
      {ViewId::DWARF_BARON, EnemyId::DWARVES, "Dwarves", true},
      {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "Red dragon", true},
      {ViewId::ELEMENTALIST, EnemyId::ELEMENTALIST, "Elementalist", true},
      {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "Green dragon", true},
      {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", false},
      {ViewId::GNOME_BOSS, EnemyId::GNOMES, "Gnomes", false},
      {ViewId::LIZARDLORD, EnemyId::LIZARDMEN, "Lizardmen", true},
      {ViewId::BANDIT, EnemyId::BANDITS, "Bandits", true},
      {ViewId::ENT, EnemyId::ENTS, "Tree spirits", true},
      {ViewId::DRIAD, EnemyId::DRIADS, "Driads", true},
      {ViewId::CYCLOPS, EnemyId::CYCLOPS, "Cyclops", true},
      {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", true},
      {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", true},
      {ViewId::UNKNOWN_MONSTER, EnemyId::FRIENDLY_CAVE, "Unknown", false},
      {ViewId::UNKNOWN_MONSTER, EnemyId::SOKOBAN, "Unknown", false},
      });
}

bool Campaign::isDefeated(Vec2 pos) const {
  return defeated[pos];
}

void Campaign::setDefeated(Vec2 pos) {
  defeated[pos] = true;
}

optional<Campaign::VillainInfo> Campaign::SiteInfo::getVillain() const {
  if (dweller)
    if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::PlayerInfo> Campaign::SiteInfo::getPlayer() const {
  if (dweller)
    if (const PlayerInfo* info = boost::get<PlayerInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::RetiredSiteInfo> Campaign::SiteInfo::getRetired() const {
  if (dweller)
    if (const RetiredSiteInfo* info = boost::get<RetiredSiteInfo>(&(*dweller)))
      return *info;
  return none;
}
 
bool Campaign::SiteInfo::isEmpty() const {
  return !dweller;
}

optional<string> Campaign::SiteInfo::getDwellerDescription() const {
  if (!dweller)
    return none;
  if (const PlayerInfo* info = boost::get<PlayerInfo>(&(*dweller)))
    return string("This is your home site");
  if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
    return info->name + " " + (info->hostile ? "(hostile)" : "(ally)");
  if (const RetiredSiteInfo* info = boost::get<RetiredSiteInfo>(&(*dweller)))
    return info->name + " (hostile)";
  return none;
}

optional<ViewId> Campaign::SiteInfo::getDwellerViewId() const {
  if (!dweller)
    return none;
  if (const PlayerInfo* info = boost::get<PlayerInfo>(&(*dweller)))
    return info->viewId;
  if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
    return info->viewId;
  if (const RetiredSiteInfo* info = boost::get<RetiredSiteInfo>(&(*dweller)))
    return info->viewId;
  return none;
}

bool Campaign::SiteInfo::isEnemy() const {
  return getRetired() || (getVillain() && getVillain()->hostile);
}
set<Vec2> Campaign::getInfluencePos() const {
  if (!playerPos)
    return {};
  set<Vec2> ret;
  for (double r = 1; r <= max(sites.getWidth(), sites.getHeight()); r += 0.1) {
    for (Vec2 v : sites.getBounds())
      if ((sites[v].getVillain() || sites[v].getRetired()) && v.distD(*playerPos) <= r)
        ret.insert(v);
    int numEnemies = 0;
    for (Vec2 v : ret)
      if (sites[v].isEnemy() && !defeated[v])
        ++numEnemies;
    if (numEnemies >= 3)
      break;
  }
  return ret;
}

int Campaign::getNumGenVillains() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (sites[v].getVillain())
      ++ret;
  return ret;
}

int Campaign::getNumRetVillains() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (sites[v].getRetired())
      ++ret;
  return ret;
}

optional<Campaign> Campaign::prepareCampaign(View* view, const vector<RetiredSiteInfo>& retired,
    function<string()> worldNameGen, RandomGen& random) {
  Vec2 size(8, 5);
  int numGenVillains = 4;
  int numRetired = min<int>(1, retired.size());
  int numBlocked = random.get(4, 8);
  string worldName;
  while (1) {
    Campaign campaign(size);
    if (!worldName.empty())
      campaign.worldName = worldName;
    else
      campaign.worldName = worldNameGen();
    for (Vec2 v : Rectangle(size))
      campaign.clearSite(v);
    vector<Vec2> freePos;
    for (Vec2 v : Rectangle(size))
      if (campaign.sites[v].canEmbark())
        freePos.push_back(v);
    while (freePos.size() < numGenVillains + numRetired + 1)
      --numGenVillains;
    for (int i : Range(numGenVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = getRandomVillain(random);
    }
    for (int i : Range(numRetired)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = random.choose(retired);
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
      CampaignAction action = view->prepareCampaign(campaign, CampaignSetupInfo{0, 12, (int)retired.size()});
      switch (action.getId()) {
        case CampaignActionId::INC_WIDTH:
            size.x += action.get<int>(); reroll = true;
            break;
        case CampaignActionId::INC_HEIGHT:
            size.y += action.get<int>(); reroll = true;
            break;
        case CampaignActionId::NUM_GEN_VILLAINS:
            numGenVillains = max(0, numGenVillains + action.get<int>());
            reroll = true;
            break;
        case CampaignActionId::NUM_RET_VILLAINS:
            numRetired = max(0, numRetired + action.get<int>());
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
            if (campaign.playerPos)
              campaign.clearSite(*campaign.playerPos);
            campaign.playerPos = action.get<Vec2>();
            campaign.sites[*campaign.playerPos].dweller = PlayerInfo{ViewId::KEEPER};
            break;
        case CampaignActionId::CONFIRM:
            return campaign;
      }
      if (reroll)
        break;
    }
  }
}

