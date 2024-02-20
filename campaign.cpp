#include "stdafx.h"
#include "campaign.h"
#include "view_id.h"
#include "model.h"
#include "progress_meter.h"
#include "campaign_type.h"
#include "villain_type.h"
#include "pretty_archive.h"
#include "perlin_noise.h"
#include "content_factory.h"
#include "enemy_info.h"
#include "tribe.h"
#include "monster_ai.h"
#include "creature.h"

SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

SERIALIZE_DEF(Campaign, sites, playerPos, worldName, defeated, influencePos, type, mapZoom, minimapZoom, originalPlayerPos, belowMaxAgressorCutOff)

void VillainViewId::serialize(PrettyInputArchive& ar1, unsigned int) {
  if (ar1.peek() == "{" && ar1.peek(2) == "{")
    ar1(ids);
  else {
    ViewId id;
    ar1(id);
    ids.push_back(id);
  }
}

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

CampaignType Campaign::getType() const {
  return type;
}

bool Campaign::canTravelTo(Vec2 pos) const {
  return isInInfluence(pos) && !sites[pos].isEmpty();
}

Vec2 Campaign::getPlayerPos() const {
  return playerPos;
}

Vec2 Campaign::getOriginalPlayerPos() const {
  return originalPlayerPos;
}

BiomeId Campaign::getBaseBiome() const {
  return *sites[playerPos].biome;
}

Campaign::Campaign(Table<SiteInfo> s, CampaignType t, const string& w)
    : sites(s), worldName(w), defeated(sites.getBounds(), false), type(t) {
}

bool Campaign::isGoodStartPos(Vec2 pos) const {
  for (auto v : Rectangle::centered(pos, 1))
    if (v.inRectangle(sites.getBounds()) && !!sites[v].dweller &&
        !sites[v].dweller->contains<Campaign::KeeperInfo>())
      return false;
  return !!sites[pos].biome;
}

const string& Campaign::getWorldName() const {
  return worldName;
}

bool Campaign::isDefeated(Vec2 pos) const {
  return defeated[pos];
}

void Campaign::removeDweller(Vec2 pos) {
  sites[pos].dweller = none;
}

void Campaign::setDefeated(const ContentFactory* f, Vec2 pos) {
  defeated[pos] = true;
  refreshInfluencePos(f);
  refreshMaxAggressorCutOff();
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
    case VillainType::RETIRED: return "retired player";
    case VillainType::MINOR:
    case VillainType::NONE: return "";
  }
}

void Campaign::updateInhabitants(ContentFactory* factory) {
  for (auto pos : sites.getBounds()) {
    auto& site = sites[pos];
    site.inhabitants.clear();
    if (site.dweller)
      site.dweller->match(
          [&](const VillainInfo& info) {
            auto& inhabitants = factory->enemies.at(info.enemyId).settlement.inhabitants;
            auto creatures = inhabitants.leader.generate(Random,
                &factory->getCreatures(), TribeId::getMonster(), MonsterAIFactory::monster());
            creatures.append(inhabitants.fighters.generate(Random,
                &factory->getCreatures(), TribeId::getMonster(), MonsterAIFactory::monster()));
            auto exp = getBaseLevelIncrease(pos);
            for (auto& c : creatures) {
              c->setCombatExperience(exp);
              site.inhabitants.push_back(SavedGameInfo::MinionInfo::get(factory, c.get()));
              if (site.inhabitants.size() >= 4)
                break;
            }
          },
          [&](const RetiredInfo& info) { site.inhabitants = info.gameInfo.minions ;},
          [&](const KeeperInfo&) { site.inhabitants.clear(); });
  }
}

optional<Campaign::VillainInfo> Campaign::SiteInfo::getVillain() const {
  if (dweller)
    return dweller->getValueMaybe<VillainInfo>();
  return none;
}

optional<Campaign::KeeperInfo> Campaign::SiteInfo::getKeeper() const {
  if (dweller)
    return dweller->getValueMaybe<KeeperInfo>();
  return none;
}

optional<Campaign::RetiredInfo> Campaign::SiteInfo::getRetired() const {
  if (dweller)
    return dweller->getValueMaybe<RetiredInfo>();
  return none;
}

bool Campaign::SiteInfo::isEmpty() const {
  return !dweller;
}

optional<string> Campaign::SiteInfo::getDwellerDescription() const {
  if (dweller)
    return dweller->match(
        [](const VillainInfo& info) {
          auto desc = info.getDescription();
          return info.name + (desc.empty() ? ""_s : " (" + desc + ")");
        },
        [](const RetiredInfo& info) { return info.gameInfo.name + " (retired player)" ;},
        [](const KeeperInfo&)->string { return "This is your home site"; });
  else
    return none;
}

optional<string> Campaign::SiteInfo::getDwellerName() const {
  if (dweller)
    return dweller->match(
        [](const VillainInfo& info) { return info.name; },
        [](const RetiredInfo& info) { return info.gameInfo.name;},
        [](const KeeperInfo&)->string { return "Home site"; });
  else
    return none;
}

optional<VillainType> Campaign::SiteInfo::getVillainType() const {
  if (dweller)
    return dweller->match(
        [](const VillainInfo& info) { return info.type; },
        [](const RetiredInfo&) { return VillainType::RETIRED; },
        [](const KeeperInfo&) { return VillainType::PLAYER; });
  else
    return none;
}

optional<ViewIdList> Campaign::SiteInfo::getDwellingViewId() const {
  if (dweller)
    return dweller->match(
        [](const VillainInfo& info) { return ViewIdList{{info.dwellingId}}; },
        [](const RetiredInfo& info) { return ViewIdList{{ViewId("map_dungeon1")}}; },
        [](const KeeperInfo& info) { return ViewIdList{{ViewId("map_base1")}}; });
  else
    return none;
}

bool Campaign::SiteInfo::isEnemy() const {
  return getRetired() || (getVillain() && getVillain()->isEnemy());
}

bool Campaign::isInInfluence(Vec2 pos) const {
  return influencePos.count(pos);
}

int Campaign::getBaseLevelIncrease(Vec2 pos) const {
  double dist = pos.distD(playerPos);
  int res = 0;
  for (Vec2 v : sites.getBounds())
    if (blocksInfluence(sites[v].getVillainType().value_or(VillainType::NONE)) && v.distD(playerPos) < dist)
      ++res;
  return res * 3;
}

bool Campaign::passesMaxAggressorCutOff(Vec2 pos) {
  return belowMaxAgressorCutOff[pos];
}

constexpr int maxAggressorDiff = 10;

void Campaign::refreshMaxAggressorCutOff() {
  belowMaxAgressorCutOff = Table<bool>(sites.getBounds(), false);
  auto maxConquered = 0;
  for (Vec2 v : sites.getBounds())
    if (blocksInfluence(sites[v].getVillainType().value_or(VillainType::NONE)) && defeated[v])
      maxConquered = max(maxConquered, getBaseLevelIncrease(v));
  for (Vec2 v : sites.getBounds())
    belowMaxAgressorCutOff[v] = getBaseLevelIncrease(v) <= maxConquered + maxAggressorDiff;
}

void Campaign::refreshInfluencePos(const ContentFactory* f) {
  map<Vec2, Vec2> siteOwners;
  map<Vec2, HashSet<Vec2>> territories;
  influencePos.clear();
  const int initialRadius = f->campaignInfo.initialRadius;
  for (auto v : Rectangle::centered(originalPlayerPos, initialRadius))
    if (v.distD(originalPlayerPos) <= initialRadius + 0.5)
      influencePos.insert(v);
  bool hasDefeated = [&] {
    for (auto v : sites.getBounds())
      if (defeated[v])
        return true;
    return false;
  }();
  struct QueueElem {
    Vec2 pos;
    double dist;
    Vec2 owner;
    bool operator < (const QueueElem& e) const {
      return dist > e.dist;
    }
  };
  priority_queue<QueueElem> q;
  for (int x : sites.getBounds().getXRange()) {
    q.push(QueueElem{Vec2(x, 0), 0, Vec2(0, 0)});
    q.push(QueueElem{Vec2(x, sites.getBounds().bottom() - 1), 0, Vec2(0, 0)});
  }
  for (int y : sites.getBounds().getYRange()) {
    q.push(QueueElem{Vec2(0, y), 0, Vec2(0, 0)});
    q.push(QueueElem{Vec2(sites.getBounds().right() - 1, y), 0, Vec2(0, 0)});
  }
  for (Vec2 v : sites.getBounds())
    if (!!sites[v].dweller && blocksInfluence(*sites[v].getVillainType()))
      q.push(QueueElem{v, 0, v});
  RandomGen gen;
  gen.init(2134);
  auto noiseTable = genNoiseMap(gen, sites.getBounds(), NoiseInit { 1, 1, 1, 1, 1 }, 0.65);
  while (!q.empty()) {
    auto e = q.top();
    q.pop();
    if (siteOwners.count(e.pos))
      continue;
    siteOwners[e.pos] = e.owner;
    for (auto v : e.pos.neighbors4())
      if (v.inRectangle(sites.getBounds()) && !sites[v].blocked && !siteOwners.count(v))
        q.push(QueueElem({v, e.dist + 3 * noiseTable[v], e.owner}));
  }
  for (auto& elem : siteOwners)
    if (elem.second != Vec2(0, 0))
      territories[elem.second].insert(elem.first);
  for (auto& elem : territories) {
    for (auto pos : elem.second)
      for (auto v : pos.neighbors4())
        if (v.inRectangle(sites.getBounds()))
          if (auto owner = getValueMaybe(siteOwners, v))
            if (defeated[*owner]) {
              influencePos.insert(elem.second.begin(), elem.second.end());
              goto nextTerritory;
            }
    nextTerritory:
    continue;
  }
  CHECK(!influencePos.empty());
}

int Campaign::getNumNonEmpty() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (!sites[v].isEmpty())
      ++ret;
  return ret;
}

int Campaign::getMapZoom() const {
  return mapZoom;
}

int Campaign::getMinimapZoom() const {
  return minimapZoom;
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
        default: break;
      }
  auto gameType = EnumInfo<CampaignType>::getString(type);
  if (type == CampaignType::QUICK_MAP)
    gameType = "WARLORD";
  return {
    {"main", toString(numMain)},
    {"lesser", toString(numLesser)},
    {"allies", toString(numAlly)},
    {"retired", toString(numRetired)},
    {"game_type", gameType},
  };
}

