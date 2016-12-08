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

SERIALIZE_DEF(Immigration, available, minionAttraction, idCnt)

double Immigration::getAttractionOccupation(const Collective* collective, const MinionAttraction& attraction) const {
  double res = 0;
  for (auto creature : collective->getCreatures())
    if (auto attractions = minionAttraction.getMaybe(creature))
      for (auto& info : *attractions)
        if (info.attraction == attraction)
          res += info.amountClaimed;
  return res;
}

double Immigration::getAttractionValue(const Collective* collective, const MinionAttraction& attraction) const {
  auto& constructions = collective->getConstructions();
  switch (attraction.getId()) {
    case AttractionId::FURNITURE: {
      double ret = constructions.getBuiltCount(attraction.get<FurnitureType>());
      for (auto type : ENUM_ALL(FurnitureType))
        if (FurnitureFactory::isUpgrade(attraction.get<FurnitureType>(), type))
          ret += constructions.getBuiltCount(type);
      return ret;
    }
    case AttractionId::ITEM_INDEX:
      return collective->getNumItems(attraction.get<ItemIndex>(), false);
  }
}


optional<double> Immigration::getImmigrantChance(const Collective* collective, const ImmigrantInfo& info) const {
  if (info.sunlightState && info.sunlightState != collective->getGame()->getSunlightInfo().getState())
    return 0.0;
  double result = 0;
  if (info.attractions.empty())
    return none;
  for (auto& attraction : info.attractions) {
    double value = getAttractionValue(collective, attraction.attraction);
    if (value < 0.001 && attraction.mandatory)
      return 0.0;
    result += max(0.0, value - getAttractionOccupation(collective, attraction.attraction) - attraction.minAmount);
  }
  return result * info.frequency;
}

void Immigration::update(Collective* collective) {
  for (auto elem : Iter(available))
    if (elem->second.isUnavailable(collective))
      elem.markToErase();
  vector<optional<double>> weights;
  double avgWeight = 0;
  int numPositive = 0;
  for (auto& elem : collective->getConfig().getImmigrantInfo()) {
    auto weight = getImmigrantChance(collective, elem);
    weights.push_back(weight);
    if (weight && *weight > 0) {
      ++numPositive;
      avgWeight += *weight;
    }
  }
  if (numPositive == 0)
    avgWeight = 1;
  else
    avgWeight /= numPositive;
  vector<double> allWeights = transform2<double>(weights,
      [&](const optional<double>& w){ return w.get_value_or(avgWeight); });
  for (auto& w : allWeights)
    if (w > 0) {
      for (int i : Range(10)) {
        auto candidate = Available::generate(collective,
            Random.choose(collective->getConfig().getImmigrantInfo(), allWeights));
        if (canAccept(collective, candidate)) {
          available.emplace(++idCnt, std::move(candidate));
          break;
        }
      }
      break;
    }
}

map<int, std::reference_wrapper<const Immigration::Available>> Immigration::getAvailable(const Collective* collective) const {
  map<int, std::reference_wrapper<const Immigration::Available>> ret;
  for (auto& elem : available)
    if (!elem.second.isUnavailable(collective))
      ret.emplace(elem.first, std::cref(elem.second));
  return ret;
}

static CostInfo getSpawnCost(SpawnType type, int howMany) {
  switch (type) {
    case SpawnType::UNDEAD:
      return {CollectiveResourceId::CORPSE, howMany};
    default:
      return CostInfo::noCost();
  }
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

CostInfo Immigration::Available::getCost() const {
  if (auto spawnType = creatures[0]->getAttributes().getSpawnType())
    return getSpawnCost(*spawnType, creatures.size());
  else
    return CostInfo::noCost();
}

Immigration::Available::Available(vector<PCreature> c, ImmigrantInfo i, double t)
    : creatures(std::move(c)), info(i), endTime(t) {
}

void Immigration::accept(Collective* collective, int id) {
  if (!available.count(id))
    return;
  auto& immigrants = available.at(id);
  if (!canAccept(collective, immigrants))
    return;
  const int groupSize = immigrants.creatures.size();
  auto& creatures = immigrants.creatures;
  collective->takeResource(immigrants.getCost());
  if (immigrants.info.autoTeam && groupSize > 1)
    collective->getTeams().activate(collective->getTeams().createPersistent(extractRefs(creatures)));
  collective->addNewCreatureMessage(extractRefs(creatures));
  vector<Position> spawnPos = getSpawnPos(extractRefs(creatures), immigrants.getSpawnPositions(collective));
  if (spawnPos.size() < immigrants.creatures.size())
    return;
  for (int i : All(immigrants.creatures)) {
    Creature* c = immigrants.creatures[i].get();
    if (i == 0 && groupSize > 1) // group leader
      c->getAttributes().increaseBaseExpLevel(2);
    collective->addCreature(std::move(immigrants.creatures[i]), spawnPos[i], immigrants.info.traits);
    minionAttraction.set(c, immigrants.info.attractions);
  }
  reject(id);
}

void Immigration::reject(int id) {
  if (available.count(id))
    available.at(id).endTime = -1;
}

SERIALIZE_DEF(Immigration::Available, creatures, info, endTime)
SERIALIZATION_CONSTRUCTOR_IMPL2(Immigration::Available, Available)

Immigration::Available Immigration::Available::generate(Collective* collective, const ImmigrantInfo& info) {
  vector<PCreature> immigrants;
  int groupSize = info.groupSize ? Random.get(*info.groupSize) : 1;
  for (int i : Range(groupSize))
    immigrants.push_back(CreatureFactory::fromId(info.id, collective->getTribeId(), MonsterAIFactory::collective(collective)));
  return Available (
    std::move(immigrants),
    info,
    collective->getGame()->getGlobalTime() + 500
  );
}

bool Immigration::canAccept(const Collective* collective, Available& immigrants) const {
  if (getImmigrantChance(collective, immigrants.info).get_value_or(1) == 0)
    return false;
  int groupSize = immigrants.creatures.size();
  if (immigrants.info.techId && !collective->hasTech(*immigrants.info.techId))
    return false;
  if (groupSize > collective->getMaxPopulation() - collective->getPopulationSize())
    return false;
  auto spawnType = immigrants.creatures[0]->getAttributes().getSpawnType();
  if (!collective->getConfig().bedsLimitImmigration())
    spawnType = none;
  if (!collective->hasResource(immigrants.getCost()))
    return false;
  if (immigrants.getSpawnPositions(collective).size() < groupSize)
    return false;
  if (spawnType && collective->getCreatures(*spawnType).size() + groupSize >
      collective->getConstructions().getBuiltCount(collective->getConfig().getDormInfo()[*spawnType].bedType))
    return false;
  else
    return true;
}

bool Immigration::Available::isUnavailable(const Collective* col) const {
  return endTime < col->getGame()->getGlobalTime();
}
