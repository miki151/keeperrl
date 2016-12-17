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

SERIALIZE_DEF(Immigration, available, minionAttraction, idCnt)

static map<AttractionType, int> empty;

int Immigration::getAttractionOccupation(const Collective* collective, const AttractionType& type) const {
  int res = 0;
  for (auto creature : collective->getCreatures()) {
    auto& elems = minionAttraction.getOrElse(creature, empty);
    auto it = elems.find(type);
    if (it != elems.end())
      res += it->second;
  }
  return res;
}

int Immigration::getAttractionValue(const Collective* collective, const AttractionType& attraction) const {
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

static string getAttractionName(const AttractionType& attraction, int count) {
  return apply_visitor(attraction, make_lambda_visitor<string>(
        [&](FurnitureType type) {
          return Furniture::getName(type, count);
        },
        [&](ItemIndex index) {
          return getName(index, count);
        }));
}

static string combineWithOr(const vector<string>& elems) {
  string ret;
  for (auto& elem : elems) {
    if (!ret.empty())
      ret += " or ";
    ret += elem;
  }
  return ret;
}

template <typename Value, typename Aggregate, typename Visitor>
Aggregate visitRequirements(
    const ImmigrantInfo& info,
    const Visitor& visitor,
    function<void(Aggregate&, const Value&)> aggregator,
    Aggregate result,
    bool onlyPreliminary) {
  for (auto& requirement : info.requirements)
    if (requirement.preliminary || !onlyPreliminary) {
      Value value = apply_visitor(requirement.type, visitor);
      aggregator(result, value);
    }
  return result;
}

template <typename Value>
Value visitAttraction(const Immigration& immigration, const Collective* collective, const AttractionInfo& attraction,
    function<Value(int total, int available)> visit) {
  int value = 0;
  int occupation = 0;
  for (auto& type : attraction.types) {
    int thisValue = immigration.getAttractionValue(collective, type);
    value += thisValue;
    occupation += min(thisValue, immigration.getAttractionOccupation(collective, type));
  }
  return visit(value, max(0, value - occupation));
}

vector<string> Immigration::getMissingRequirements(const Collective* collective, const Available& available) const {
  vector<string> ret = getMissingRequirements(collective, Group {available.info, (int)available.creatures.size()});
  int groupSize = available.creatures.size();
  if (groupSize > collective->getMaxPopulation() - collective->getPopulationSize())
    ret.push_back("Exceeds population limit");
  auto spawnType = available.creatures[0]->getAttributes().getSpawnType();
  if (ret.empty() && available.getSpawnPositions(collective).size() < groupSize)
    ret.push_back("Not enough room to spawn.");
  return ret;
}

vector<string> Immigration::getMissingRequirements(const Collective* collective, const Group& group) const {
  auto visitor = make_lambda_visitor<optional<string>>(
      [&](const AttractionInfo& attraction) {
        return visitAttraction<optional<string>>(*this, collective, attraction,
            [&](int total, int available) -> optional<string> {
              int required = attraction.amountClaimed * group.count - available;
              if (required > 0) {
                const char* extra = total > 0 ? "more " : "";
                return "Requires " + toString(required) + " " + extra +
                    combineWithOr(transform2<string>(attraction.types,
                        [&](const AttractionType& type) { return getAttractionName(type, required); }));
              } else
                return none;
            });
      },
      [&](const TechId& techId) -> optional<string> {
        if (!collective->hasTech(techId))
          return "Missing technology: " + Technology::get(techId)->getName();
        else
          return none;
      },
      [&](const SunlightState& state) -> optional<string> {
        if (state != collective->getGame()->getSunlightInfo().getState())
          return "Immigrant won't join during the "_s + collective->getGame()->getSunlightInfo().getText();
        else
          return none;
      },
      [&](const CostInfo& cost) -> optional<string> {
        if (!collective->hasResource(cost * group.count))
          return "Not enough " + CollectiveConfig::getResourceInfo(cost.id).name;
        else
          return none;
      },
      [&](const FurnitureType& type) -> optional<string> {
        if (collective->getConstructions().getBuiltCount(type) == 0)
          return "Requires " + Furniture::getName(type);
        else
          return none;
      }
  );
  return visitRequirements<optional<string>, vector<string>>(group.info, visitor,
      [](vector<string>& all, const optional<string>& value) { if (value) all.push_back(*value);}, {}, false);
}

vector<string> Immigration::getAllRequirements(const ImmigrantInfo& info) {
  auto visitor = make_lambda_visitor<optional<string>>(
      [&](const AttractionInfo& attraction) {
          int required = attraction.amountClaimed;
          return "Requires " + toString(required) + " " +
              combineWithOr(transform2<string>(attraction.types,
                  [&](const AttractionType& type) { return getAttractionName(type, required); }));
      },
      [&](const TechId& techId) -> optional<string> {
          return "Requires technology: " + Technology::get(techId)->getName();
      },
      [&](const SunlightState& state) -> optional<string> {
          return "Will only join during the "_s + SunlightInfo::getText(state);
      },
      [&](const CostInfo& cost) -> optional<string> {
          return "Costs " + toString(cost.value) + " " + CollectiveConfig::getResourceInfo(cost.id).name;
      },
      [&](const FurnitureType& type) -> optional<string> {
          return "Requires at least one " + Furniture::getName(type);
      }
  );
  return visitRequirements<optional<string>, vector<string>>(info, visitor,
      [](vector<string>& all, const optional<string>& value) { if (value) all.push_back(*value);}, {}, false);
}

bool Immigration::preliminaryRequirementsMet(const Collective* collective, const Group& group) const {
  auto visitor = make_lambda_visitor<bool>(
      [&](const AttractionInfo& attraction) {
        return visitAttraction<bool>(*this, collective, attraction,
            [&](int, int available) { return available > 0; });
      },
      [&](const TechId& techId) { return collective->hasTech(techId); },
      [&](const SunlightState& state) { return state == collective->getGame()->getSunlightInfo().getState(); },
      [&](const CostInfo& cost) { return collective->hasResource(cost * group.count); },
      [&](const FurnitureType& type) { return collective->getConstructions().getBuiltCount(type) > 0; }
  );
  return visitRequirements<bool, bool>(group.info, visitor, [](bool& all, const bool& value) { all &= value;}, true, true);
}

void Immigration::occupyRequirements(Collective* collective, const Creature* c, const ImmigrantInfo& info) {
  auto visitor = make_lambda_visitor<int>(
      [&](const AttractionInfo& attraction) { occupyAttraction(collective, c, attraction); return 0;},
      [&](const TechId&) { return 0;},
      [&](const SunlightState&) { return 0;},
      [&](const FurnitureType&) { return 0;},
      [&](const CostInfo& cost) { collective->takeResource(cost); return 0; }
  );
  visitRequirements<int, int>(info, visitor, [](int&, const int&) { return 0;}, 0, false);
}

void Immigration::occupyAttraction(const Collective* collective, const Creature* c, const AttractionInfo& attraction) {
  int toOccupy = attraction.amountClaimed;
  vector<AttractionType> occupied;
  for (auto& type : attraction.types) {
    int nowOccupy = min(toOccupy, max(0,
        getAttractionValue(collective, type) - getAttractionOccupation(collective, type)));
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

double Immigration::getImmigrantChance(const Collective* collective, const Group& group) const {
  if (!preliminaryRequirementsMet(collective, group))
    return 0.0;
  else
    return group.info.frequency;
}

void Immigration::update(Collective* collective) {
  for (auto elem : Iter(available))
    if (isUnavailable(elem->second, collective))
      elem.markToErase();
  auto immigrantInfo = transform2<Group>(collective->getConfig().getImmigrantInfo(),
      [](const ImmigrantInfo& info) { return Group {info, info.groupSize ? Random.get(*info.groupSize) : 1};});
  vector<double> weights = transform2<double>(immigrantInfo,
      [&](const Group& group) { return getImmigrantChance(collective, group);});
  if (std::accumulate(weights.begin(), weights.end(), 0.0) > 0)
    available.emplace(++idCnt, Available::generate(collective, Random.choose(immigrantInfo, weights)));
}

map<int, std::reference_wrapper<const Immigration::Available>> Immigration::getAvailable(const Collective* col) const {
  map<int, std::reference_wrapper<const Immigration::Available>> ret;
  for (auto& elem : available)
    if (!isUnavailable(elem.second, col))
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

vector<Position> Immigration::Available::getSpawnPositions(const Collective* collective) const {
  if (auto spawnType = creatures[0]->getAttributes().getSpawnType()) {
    FurnitureType bedType = collective->getConfig().getDormInfo()[*spawnType].bedType;
    if (info.spawnAtDorm)
      return asVector<Position>(collective->getConstructions().getBuiltPositions(bedType));
  }
  return collective->getTerritory().getExtended(10, 20);
}

vector<Creature*> Immigration::Available::getCreatures() const {
  return extractRefs(creatures);
}

double Immigration::Available::getEndTime() const {
  return endTime;
}

Immigration::Available::Available(vector<PCreature> c, ImmigrantInfo i, double t)
    : creatures(std::move(c)), info(i), endTime(t) {
}

void Immigration::accept(Collective* collective, int id) {
  if (!available.count(id))
    return;
  auto& immigrants = available.at(id);
  if (!getMissingRequirements(collective, immigrants).empty())
    return;
  const int groupSize = immigrants.creatures.size();
  auto& creatures = immigrants.creatures;
  vector<Position> spawnPos = getSpawnPos(extractRefs(creatures), immigrants.getSpawnPositions(collective));
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
    occupyRequirements(collective, c, immigrants.info);
  }
  reject(id);
}

void Immigration::reject(int id) {
  if (available.count(id))
    available.at(id).endTime = -1;
}

SERIALIZE_DEF(Immigration::Available, creatures, info, endTime)
SERIALIZATION_CONSTRUCTOR_IMPL2(Immigration::Available, Available)

Immigration::Available Immigration::Available::generate(Collective* collective, const Group& group) {
  vector<PCreature> immigrants;
  for (int i : Range(group.count))
    immigrants.push_back(CreatureFactory::fromId(group.info.id, collective->getTribeId(),
        MonsterAIFactory::collective(collective)));
  return Available (
    std::move(immigrants),
    group.info,
    collective->getGame()->getGlobalTime() + 500
  );
}

bool Immigration::isUnavailable(const Available& available, const Collective* col) const {
  return available.endTime < col->getGame()->getGlobalTime() ||
      !preliminaryRequirementsMet(col, {available.info, (int)available.creatures.size()});
}
