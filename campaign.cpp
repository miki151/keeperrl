#include "stdafx.h"
#include "campaign.h"
#include "view_id.h"
#include "model.h"
#include "progress_meter.h"
#include "campaign_type.h"
#include "player_role.h"
#include "villain_type.h"

SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

SERIALIZE_DEF(Campaign, sites, playerPos, worldName, defeated, influencePos, influenceSize, playerRole, type)

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

bool Campaign::canEmbark(Vec2 pos) const {
  if (type == CampaignType::CAMPAIGN)
    return false;
  switch (playerRole) {
    case PlayerRole::ADVENTURER: return !!sites[pos].dweller;
    case PlayerRole::KEEPER: return !sites[pos].dweller && !sites[pos].blocked;
  }
}

CampaignType Campaign::getType() const {
  return type;
}

bool Campaign::canTravelTo(Vec2 pos) const {
  return (isInInfluence(pos) || playerPos == pos) && !sites[pos].isEmpty();
}

optional<Vec2> Campaign::getPlayerPos() const {
  return playerPos;
}

Campaign::Campaign(Table<SiteInfo> s, CampaignType t, PlayerRole r, const string& w)
    : sites(s), worldName(w), defeated(sites.getBounds(), false), playerRole(r), type(t) {
}

const string& Campaign::getWorldName() const {
  return worldName;
}

void Campaign::clearSite(Vec2 v) {
  sites[v] = SiteInfo{};
  sites[v].viewId = {ViewId::GRASS};
}

bool Campaign::isDefeated(Vec2 pos) const {
  return defeated[pos];
}

void Campaign::setDefeated(Vec2 pos) {
  defeated[pos] = true;
  refreshInfluencePos();
}

bool Campaign::VillainInfo::isEnemy() const {
  return type != VillainType::ALLY;
}

string Campaign::VillainInfo::getDescription() const {
  switch (type) {
    case VillainType::ALLY: return "ally";
    case VillainType::MAIN: return "main villain";
    case VillainType::LESSER: return "lesser villain";
    case VillainType::PLAYER: return "player";
  }
}

optional<Campaign::VillainInfo> Campaign::SiteInfo::getVillain() const {
  if (dweller)
    if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::KeeperInfo> Campaign::SiteInfo::getKeeper() const {
  if (dweller)
    if (const KeeperInfo* info = boost::get<KeeperInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::RetiredInfo> Campaign::SiteInfo::getRetired() const {
  if (dweller)
    if (const RetiredInfo* info = boost::get<RetiredInfo>(&(*dweller)))
      return *info;
  return none;
}
 
bool Campaign::SiteInfo::isEmpty() const {
  return !dweller;
}

optional<string> Campaign::SiteInfo::getDwellerDescription() const {
  if (dweller)
    return applyVisitor(*dweller, makeVisitor<string>(
        [](const VillainInfo& info) { return info.name + " (" + info.getDescription() + ")"; },
        [](const RetiredInfo& info) { return info.gameInfo.getName() + " (main villain)" ;},
        [](const KeeperInfo&) { return "This is your home site"; }));
  else
    return none;
}

optional<VillainType> Campaign::SiteInfo::getVillainType() const {
  if (dweller)
    return applyVisitor(*dweller, makeVisitor<VillainType>(
        [](const VillainInfo& info) { return info.type; },
        [](const RetiredInfo&) { return VillainType::MAIN; },
        [](const KeeperInfo&) { return VillainType::PLAYER; }));
  else
    return none;
}

optional<ViewId> Campaign::SiteInfo::getDwellerViewId() const {
  if (dweller)
    return applyVisitor(*dweller, makeVisitor<ViewId>(
        [](const VillainInfo& info) { return info.viewId; },
        [](const RetiredInfo& info) { return info.gameInfo.getViewId(); },
        [](const KeeperInfo& info) { return info.viewId; }));
  else
    return none;
}

bool Campaign::SiteInfo::isEnemy() const {
  return getRetired() || (getVillain() && getVillain()->isEnemy());
}

void Campaign::SiteInfo::setBlocked() {
  blocked = true;
  viewId.push_back(Random.choose(ViewId::MAP_MOUNTAIN1, ViewId::MAP_MOUNTAIN2, ViewId::MAP_MOUNTAIN3,
        ViewId::CANIF_TREE, ViewId::DECID_TREE));
}

bool Campaign::isInInfluence(Vec2 pos) const {
  return influencePos.count(pos);
}

void Campaign::refreshInfluencePos() {
  influencePos.clear();
  if (!playerPos)
    return;
  for (double r = 1; r <= max(sites.getWidth(), sites.getHeight()); r += 0.1) {
    for (Vec2 v : sites.getBounds())
      if ((sites[v].getVillain() || sites[v].getRetired()) && v.distD(*playerPos) <= r)
        influencePos.insert(v);
    int numEnemies = 0;
    for (Vec2 v : influencePos)
      if (sites[v].isEnemy() && !defeated[v])
        ++numEnemies;
    if (numEnemies >= influenceSize)
      break;
  }
}

int Campaign::getNumNonEmpty() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (!sites[v].isEmpty())
      ++ret;
  return ret;
}

map<string, string> Campaign::getParameters() const {
  int numMain = 0;
  int numLesser = 0;
  int numAlly = 0;
  int numRetired = 0;
  for (Vec2 v : sites.getBounds())
    if (sites[v].getRetired())
      ++numRetired;
    else if (auto villain = sites[v].getVillain())
      switch (villain->type) {
        case VillainType::ALLY: ++numAlly; break;
        case VillainType::MAIN: ++numMain; break;
        case VillainType::LESSER: ++numLesser; break;
        case VillainType::PLAYER: break;
      }
  return {
    {"main", toString(numMain)},
    {"lesser", toString(numLesser)},
    {"allies", toString(numAlly)},
    {"retired", toString(numRetired)},
    {"player_role", EnumInfo<PlayerRole>::getString(playerRole)},
    {"game_type", EnumInfo<CampaignType>::getString(type)},
  };
}

PlayerRole Campaign::getPlayerRole() const {
  return playerRole;
}
