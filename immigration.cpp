#include "stdafx.h"
#include "immigration.h"
#include "collective.h"
#include "collective_config.h"
#include "monster_ai.h"
#include "creature.h"
#include "creature_attributes.h"
#include "territory.h"
#include "game.h"
#include "furniture_factory.h"
#include "construction_map.h"
#include "collective_teams.h"
#include "furniture.h"
#include "item_index.h"
#include "technology.h"

SERIALIZE_DEF(Immigration, available, minionAttraction, idCnt, collective, generated)

SERIALIZATION_CONSTRUCTOR_IMPL(Immigration)

static map<AttractionType, int> empty;

int Immigration::getAttractionOccupation(const AttractionType& type) const {
  int res = 0;
  for (auto creature : collective->getCreatures()) {
    auto& elems = minionAttraction.getOrElse(creature, empty);
    auto it = elems.find(type);
    if (it != elems.end())
      res += it->second;
  }
  return res;
}

int Immigration::getAttractionValue(const AttractionType& attraction) const {
  return apply_visitor(attraction, make_lambda_visitor<int>(
        [&](FurnitureType type) {
          auto& constructions = collective->getConstructions();
          int ret = constructions.getBuiltCount(type);
          for (auto upgrade : FurnitureFactory::getUpgrades(type))
            ret += constructions.getBuiltCount(upgrade);
          return ret;
        },
        [&](ItemIndex index) {
          return collective->getNumItems(index, false);
        }));
}

static void visitAttraction(const Immigration& immigration, const AttractionInfo& attraction,
    function<void(int total, int available)> visit) {
  int value = 0;
  int occupation = 0;
  for (auto& type : attraction.types) {
    int thisValue = immigration.getAttractionValue(type);
    value += thisValue;
    occupation += min(thisValue, immigration.getAttractionOccupation(type));
  }
  visit(value, max(0, value - occupation));
}

vector<string> Immigration::getMissingRequirements(const Available& available) const {
  vector<string> ret = getMissingRequirements(Group {available.info, (int)available.creatures.size()});
  int groupSize = available.creatures.size();
  if (groupSize > collective->getMaxPopulation() - collective->getPopulationSize())
    ret.push_back("Exceeds population limit");
  auto spawnType = available.creatures[0]->getAttributes().getSpawnType();
  if (ret.empty() && available.getSpawnPositions().size() < groupSize)
    ret.push_back("Not enough room to spawn.");
  return ret;
}

vector<string> Immigration::getMissingRequirements(const Group& group) const {
  vector<string> ret;
  auto visitor = make_lambda_visitor<void>(
      [&](const AttractionInfo& attraction) {
        return visitAttraction(*this, attraction,
            [&](int total, int available) {
              int required = attraction.amountClaimed * group.count - available;
              if (required > 0) {
                const char* extra = total > 0 ? "more " : "";
                ret.push_back("Requires " + toString(required) + " " + extra +
                    combineWithOr(transform2<string>(attraction.types,
                        [&](const AttractionType& type) { return AttractionInfo::getAttractionName(type, required); })));
              }
            });
      },
      [&](const TechId& techId) {
        if (!collective->hasTech(techId))
          ret.push_back("Missing technology: " + Technology::get(techId)->getName());
      },
      [&](const SunlightState& state) {
        if (state != collective->getGame()->getSunlightInfo().getState())
          ret.push_back("Immigrant won't join during the "_s + collective->getGame()->getSunlightInfo().getText());
      },
      [&](const CostInfo& cost) {
        if (!collective->hasResource(cost * group.count))
          ret.push_back("Not enough " + CollectiveConfig::getResourceInfo(cost.id).name);
      },
      [&](const ExponentialCost& cost) {
        if (!collective->hasResource(calculateCost(group.info, cost) * group.count))
          ret.push_back("Not enough " + CollectiveConfig::getResourceInfo(cost.base.id).name);
      },
      [&](const FurnitureType& type) {
        if (collective->getConstructions().getBuiltCount(type) == 0)
          ret.push_back("Requires " + Furniture::getName(type));
      }
  );
  group.info.visitRequirements(visitor);
  return ret;
}

vector<string> Immigration::getAllRequirements(const ImmigrantInfo& info) {
  vector<string> ret;

  return ret;
}

bool Immigration::preliminaryRequirementsMet(const Group& group) const {
  bool ret = true;
  auto visitor = make_lambda_visitor<void>(
      [&](const AttractionInfo& attraction) {
        visitAttraction(*this, attraction,
            [&](int, int available) { return ret &= (available > 0); });
      },
      [&](const TechId& techId) { ret &= collective->hasTech(techId); },
      [&](const SunlightState& state) { ret &= (state == collective->getGame()->getSunlightInfo().getState()); },
      [&](const FurnitureType& type) { ret &= (collective->getConstructions().getBuiltCount(type) > 0); },
      [&](const CostInfo& cost) {
        ret &= collective->hasResource(cost * group.count);
      },
      [&](const ExponentialCost& cost) {
        ret &= collective->hasResource(calculateCost(group.info, cost) * group.count);
      }
  );
  group.info.visitPreliminaryRequirements(visitor);
  return ret;
}

void Immigration::occupyRequirements(const Creature* c, const ImmigrantInfo& info) {
  auto visitor = make_lambda_visitor<void>(
      [&](const AttractionInfo& attraction) { occupyAttraction(c, attraction); },
      [&](const TechId&) {},
      [&](const SunlightState&) {},
      [&](const FurnitureType&) {},
      [&](const CostInfo& cost) {
        collective->takeResource(cost);
      },
      [&](const ExponentialCost& cost) {
        collective->takeResource(calculateCost(info, cost));
      }
  );
  info.visitRequirements(visitor);
}

void Immigration::occupyAttraction(const Creature* c, const AttractionInfo& attraction) {
  int toOccupy = attraction.amountClaimed;
  vector<AttractionType> occupied;
  for (auto& type : attraction.types) {
    int nowOccupy = min(toOccupy, max(0,
        getAttractionValue(type) - getAttractionOccupation(type)));
    if (nowOccupy > 0) {
      CHECK(nowOccupy <= toOccupy) << nowOccupy << " " << toOccupy;
      toOccupy -= nowOccupy;
      minionAttraction.getOrInit(c)[type] += nowOccupy;
    }
    if (toOccupy == 0)
      break;
  }
  CHECK(toOccupy == 0) << toOccupy;
}

double Immigration::getImmigrantChance(const Group& group) const {
  if (!preliminaryRequirementsMet(group))
    return 0.0;
  else
    return group.info.frequency.get_value_or(0);
}

Immigration::Immigration(Collective* c) : collective(c) {
  for (auto& elem : collective->getConfig().getImmigrantInfo())
    if (!elem.frequency) {
      available.emplace(++idCnt, Available::generate(this, Group{elem, 1}));
    }
}

map<int, std::reference_wrapper<const Immigration::Available>> Immigration::getAvailable() const {
  map<int, std::reference_wrapper<const Immigration::Available>> ret;
  for (auto& elem : available)
    if (!isUnavailable(elem.second))
      ret.emplace(elem.first, std::cref(elem.second));
  return ret;
}

static vector<Position> getSpawnPos(const vector<Creature*>& creatures, vector<Position> allPositions) {
  if (allPositions.empty() || creatures.empty())
    return {};
  for (auto& pos : copyOf(allPositions))
    if (!pos.canEnterEmpty(creatures[0]))
      for (auto& neighbor : pos.neighbors8())
        if (neighbor.canEnterEmpty(creatures[0]))
          allPositions.push_back(neighbor);
  vector<Position> spawnPos;
  for (auto c : creatures) {
    Position pos;
    int cnt = 100;
    do {
      pos = Random.choose(allPositions);
    } while ((!pos.canEnter(c) || contains(spawnPos, pos)) && --cnt > 0);
    if (cnt == 0) {
      INFO << "Couldn't spawn immigrant " << c->getName().bare();
      return {};
    } else
      spawnPos.push_back(pos);
  }
  return spawnPos;
}

vector<Position> Immigration::Available::getSpawnPositions() const {
  return apply_visitor(info.spawnLocation, make_lambda_visitor<vector<Position>>(
    [&] (FurnitureType type) {
      return asVector<Position>(immigration->collective->getConstructions().getBuiltPositions(type));
    },
    [&] (OutsideTerritory) {
      return immigration->collective->getTerritory().getExtended(10, 20);
    },
    [&] (NearLeader) {
      return filter(immigration->collective->getLeader()->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), 2)),
        [&](const Position& pos) { return pos.canEnter(creatures[0].get());});
    }
  ));
}

vector<Creature*> Immigration::Available::getCreatures() const {
  return extractRefs(creatures);
}

optional<double> Immigration::Available::getEndTime() const {
  return endTime;
}

optional<CostInfo> Immigration::Available::getCost() const {
  optional<CostInfo> ret;
  auto visitor = make_lambda_visitor<void>(
      [&](const AttractionInfo&) {},
      [&](const TechId&) {},
      [&](const SunlightState&) {},
      [&](const FurnitureType&) {},
      [&](const CostInfo& cost) {
        ret = cost;
      },
      [&](const ExponentialCost& cost) {
        ret = immigration->calculateCost(info, cost);
      }
  );
  info.visitRequirements(visitor);
  return ret;
}

CostInfo Immigration::calculateCost(const ImmigrantInfo& info, const ExponentialCost& cost) const {
  EntitySet<Creature> existing;
  for (Creature* c : collective->getCreatures())
    existing.insert(c);
  int numType = 0;
  if (auto gen = getMaybe(generated, info.id))
    for (Creature* c : *gen)
      if (existing.contains(c))
        ++numType;
  if (numType < cost.numFree)
    return CostInfo::noCost();
  else
    return CostInfo(cost.base.id, cost.base.value * pow(2, double(numType - cost.numFree) / cost.numToDoubleCost));
}

const ImmigrantInfo& Immigration::Available::getImmigrantInfo() const {
  return info;
}

Immigration::Available::Available(Immigration* im, vector<PCreature> c, ImmigrantInfo i, optional<double> t)
  : creatures(std::move(c)), info(i), endTime(t), immigration(im) {
}

void Immigration::accept(int id) {
  if (!available.count(id))
    return;
  auto& immigrants = available.at(id);
  if (!getMissingRequirements(immigrants).empty())
    return;
  const int groupSize = immigrants.creatures.size();
  auto& creatures = immigrants.creatures;
  vector<Position> spawnPos = getSpawnPos(extractRefs(creatures), immigrants.getSpawnPositions());
  if (spawnPos.size() < immigrants.creatures.size())
    return;
  if (immigrants.info.autoTeam && groupSize > 1)
    collective->getTeams().activate(collective->getTeams().createPersistent(extractRefs(creatures)));
  collective->addNewCreatureMessage(extractRefs(creatures));
  for (int i : All(immigrants.creatures)) {
    Creature* c = immigrants.creatures[i].get();
    if (i == 0 && groupSize > 1) // group leader
      c->getAttributes().increaseBaseExpLevel(2);
    collective->addCreature(std::move(immigrants.creatures[i]), spawnPos[i], immigrants.info.traits);
    occupyRequirements(c, immigrants.info);
    generated[immigrants.info.id].push_back(c);
  }
  if (immigrants.info.frequency) // this means it's not a persistent candidate
    reject(id);
  else
    immigrants.regenerate();
}

void Immigration::reject(int id) {
  if (available.count(id))
    available.at(id).endTime = -1;
}

SERIALIZE_DEF(Immigration::Available, creatures, info, endTime, immigration)
SERIALIZATION_CONSTRUCTOR_IMPL2(Immigration::Available, Available)

Immigration::Available Immigration::Available::generate(Immigration* immigration, const Group& group) {
  vector<PCreature> immigrants;
  for (int i : Range(group.count))
    immigrants.push_back(CreatureFactory::fromId(group.info.id, immigration->collective->getTribeId(),
        MonsterAIFactory::collective(immigration->collective)));
  return Available(
    immigration,
    std::move(immigrants),
    group.info,
    group.info.frequency ? immigration->collective->getGame()->getGlobalTime() + 500 : optional<double>(none)
  );
}

void Immigration::Available::regenerate() {
  creatures.clear();
  for (int i : Range(info.groupSize ? Random.get(*info.groupSize) : 1))
    creatures.push_back(CreatureFactory::fromId(info.id, immigration->collective->getTribeId(),
        MonsterAIFactory::collective(immigration->collective)));
}


bool Immigration::isUnavailable(const Available& available) const {
  return (available.endTime && available.endTime < collective->getGame()->getGlobalTime()) ||
      !preliminaryRequirementsMet({available.info, (int)available.creatures.size()});
}

void Immigration::update() {
  for (auto elem : Iter(available))
    if (isUnavailable(elem->second))
      elem.markToErase();
  auto immigrantInfo = transform2<Group>(collective->getConfig().getImmigrantInfo(),
      [](const ImmigrantInfo& info) { return Group {info, info.groupSize ? Random.get(*info.groupSize) : 1};});
  vector<double> weights = transform2<double>(immigrantInfo,
      [&](const Group& group) { return getImmigrantChance(group);});
  if (std::accumulate(weights.begin(), weights.end(), 0.0) > 0)
    available.emplace(++idCnt, Available::generate(this, Random.choose(immigrantInfo, weights)));
}
