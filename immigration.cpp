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
#include "level.h"
#include "game.h"
#include "collective_name.h"
#include "clock.h"
#include "tutorial.h"
#include "container_range.h"
#include "creature_factory.h"
#include "resource_info.h"

template <class Archive>
void Immigration::serialize(Archive& ar, const unsigned int) {
  ar & SUBCLASS(OwnedObject<Immigration>);
  ar(available, minionAttraction, idCnt, collective, generated, candidateTimeout, initialized, nextImmigrantTime, autoState);
}
SERIALIZABLE(Immigration);

SERIALIZATION_CONSTRUCTOR_IMPL(Immigration)

static unordered_map<AttractionType, int, CustomHash<AttractionType>> empty;

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
  return attraction.match(
        [&](FurnitureType type) {
          auto& constructions = collective->getConstructions();
          int ret = constructions.getBuiltCount(type);
          for (auto upgrade : FurnitureFactory::getUpgrades(type))
            ret += constructions.getBuiltCount(upgrade);
          return ret;
        },
        [&](ItemIndex index) {
          return collective->getNumItems(index, false);
  });
}

const vector<ImmigrantInfo>& Immigration::getImmigrants() const {
  return collective->getConfig().getImmigrantInfo();
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
  vector<string> ret = getMissingRequirements(Group {available.immigrantIndex, (int)available.getCreatures().size()});
  int groupSize = available.getCreatures().size();
  if (!available.getInfo().getTraits().contains(MinionTrait::NO_LIMIT) &&
      groupSize > collective->getMaxPopulation() - collective->getPopulationSize())
    ret.push_back("Exceeds population limit");
  if (ret.empty() && available.getSpawnPositions().size() < groupSize)
    ret.push_back("Not enough room to spawn.");
  return ret;
}

void Immigration::setAutoState(int index, optional<ImmigrantAutoState> s) {
  if (!getImmigrants()[index].isNoAuto()) {
    if (!s)
      autoState.erase(index);
    else
      autoState[index] = *s;
  }
}

optional<ImmigrantAutoState> Immigration::getAutoState(int index) const {
  return getValueMaybe(autoState, index);
}

vector<string> Immigration::getMissingRequirements(const Group& group) const {
  vector<string> ret;
  auto& immigrantInfo = getImmigrants()[group.immigrantIndex];
   auto visitor = makeVisitor(
      [&](const AttractionInfo& attraction) {
        return visitAttraction(*this, attraction,
            [&](int total, int available) {
              int required = attraction.amountClaimed * group.count - available;
              if (required > 0) {
                const char* extra = total > 0 ? "more " : "";
                ret.push_back("Requires " + toString(required) + " " + extra +
                    combineWithOr(attraction.types.transform(
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
        if (!collective->hasResource(calculateCost(group.immigrantIndex, cost) * group.count))
          ret.push_back("Not enough " + CollectiveConfig::getResourceInfo(cost.base.id).name);
      },
      [&](const FurnitureType& type) {
        if (collective->getConstructions().getBuiltCount(type) == 0)
          ret.push_back("Requires " + Furniture::getName(type));
      },
      [&](const Pregnancy&) {
        for (WCreature c : collective->getCreatures())
          if (c->isAffected(LastingEffect::PREGNANT))
            return;
        ret.push_back("Requires a pregnant succubus");
      },
      [&](const RecruitmentInfo& info) {
        auto col = info.findEnemy(collective->getGame());
        if (!col)
          ret.push_back("Ally doesn't exist");
        if (!collective->isKnownVillainLocation(col))
          ret.push_back("Ally hasn't been discovered");
        else if (info.getAvailableRecruits(collective->getGame(), immigrantInfo.getId(0)).empty())
          ret.push_back("Ally doesn't have recruits available at this moment");
      },
      [&](const TutorialRequirement& t) {
        if (!t.tutorial->showImmigrant(immigrantInfo))
          ret.push_back("Tutorial not there yet");
      }
  );
  immigrantInfo.visitRequirements(visitor);
  return ret;
}

double Immigration::getRequirementMultiplier(const Group& group) const {
  double ret = 1;
  auto& immigrantInfo = getImmigrants()[group.immigrantIndex];
  auto visitor = makeVisitor(
      [&](const AttractionInfo& attraction, double prob) {
        visitAttraction(*this, attraction,
            [&](int, int available) { if (available < attraction.amountClaimed) ret *= prob; });
      },
      [&](const TechId& techId, double prob) {
        if (!collective->hasTech(techId))
          ret *= prob;
      },
      [&](const SunlightState& state, double prob) {
        if (state != collective->getGame()->getSunlightInfo().getState())
          ret *= prob;
      },
      [&](const FurnitureType& type, double prob) {
        if (collective->getConstructions().getBuiltCount(type) == 0)
          ret *= prob;
      },
      [&](const CostInfo& cost, double prob) {
        if (!collective->hasResource(cost * group.count))
          ret *= prob;
      },
      [&](const ExponentialCost& cost, double prob) {
        if (!collective->hasResource(calculateCost(group.immigrantIndex, cost) * group.count))
          ret *= prob;
      },
      [&](const Pregnancy&, double prob) {
        for (WCreature c : collective->getCreatures())
          if (c->isAffected(LastingEffect::PREGNANT))
            return;
        ret *= prob;
      },
      [&](const RecruitmentInfo& info, double prob) {
        WCollective col = info.findEnemy(collective->getGame());
        if (!col || !collective->isKnownVillainLocation(col) ||
            info.getAvailableRecruits(collective->getGame(), getImmigrants()[group.immigrantIndex].getId(0)).empty())
          ret *= prob;
      },
      [&](const TutorialRequirement& t, double prob) {
        if (!t.tutorial->showImmigrant(immigrantInfo))
          ret *= prob;
      }
    );
  getImmigrants()[group.immigrantIndex].visitRequirementsAndProb(visitor);
  return ret;
}

void Immigration::occupyRequirements(WConstCreature c, int index) {
  auto visitor = makeVisitor(
      [&](const AttractionInfo& attraction) { occupyAttraction(c, attraction); },
      [&](const TechId&) {},
      [&](const SunlightState&) {},
      [&](const FurnitureType&) {},
      [&](const CostInfo& cost) {
        collective->takeResource(cost);
      },
      [&](const ExponentialCost& cost) {
        collective->takeResource(calculateCost(index, cost));
      },
      [&](const Pregnancy&) {
        for (WCreature c : collective->getCreatures())
          if (c->isAffected(LastingEffect::PREGNANT)) {
            c->removeEffect(LastingEffect::PREGNANT);
            break;
          }
      },
      [&](const RecruitmentInfo&) {},
      [&](const TutorialRequirement&) {}
  );
  getImmigrants()[index].visitRequirements(visitor);
}

void Immigration::occupyAttraction(WConstCreature c, const AttractionInfo& attraction) {
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
  auto& info = getImmigrants()[group.immigrantIndex];
  if (info.isPersistent() || !info.isAvailable(getNumGeneratedAndCandidates(group.immigrantIndex) + group.count - 1))
    return 0.0;
  else
    return info.getFrequency() * getRequirementMultiplier(group);
}

Immigration::Immigration(WCollective c)
    : collective(c), candidateTimeout(c->getConfig().getImmigrantTimeout()) {
}

map<int, std::reference_wrapper<const Immigration::Available>> Immigration::getAvailable() const {
  map<int, std::reference_wrapper<const Immigration::Available>> ret;
  for (auto& elem : available)
    if (!elem.second.isUnavailable() &&
        getValueMaybe(autoState, elem.second.immigrantIndex) != ImmigrantAutoState::AUTO_REJECT &&
        getRequirementMultiplier({elem.second.immigrantIndex, (int)elem.second.getCreatures().size()}) > 0)
      ret.emplace(elem.first, std::cref(elem.second));
  return ret;
}

static vector<Position> pickSpawnPositions(const vector<WCreature>& creatures, vector<Position> allPositions) {
  if (allPositions.empty() || creatures.empty())
    return {};
  for (auto& pos : copyOf(allPositions))
    if (!pos.canEnter(creatures[0]))
      for (auto& neighbor : pos.neighbors8())
        if (neighbor.canEnter(creatures[0]))
          allPositions.push_back(neighbor);
  vector<Position> spawnPos;
  for (auto c : creatures) {
    Position pos;
    int cnt = 100;
    do {
      pos = Random.choose(allPositions);
    } while ((!pos.canEnter(c) || spawnPos.contains(pos)) && --cnt > 0);
    if (cnt == 0) {
      INFO << "Couldn't spawn immigrant " << c->getName().bare();
      return {};
    } else
      spawnPos.push_back(pos);
  }
  return spawnPos;
}

vector<Position> Immigration::Available::getSpawnPositions() const {
  vector<Position> positions = getInfo().getSpawnLocation().match(
    [&] (FurnitureType type) {
      return asVector<Position>(immigration->collective->getConstructions().getBuiltPositions(type));
    },
    [&] (OutsideTerritory) {
      auto ret = immigration->collective->getTerritory().getExtended(10, 20);
      if (ret.empty())
        ret = immigration->collective->getTerritory().getAll();
      auto leader = immigration->collective->getLeader();
      if (ret.empty() && leader)
        ret = {leader->getPosition()};
      return ret;
    },
    [&] (NearLeader) -> vector<Position> {
      if (auto leader = immigration->collective->getLeader())
        return {leader->getPosition()};
      else
        return {};
    },
    [&] (Pregnancy) {
      vector<Position> ret;
      for (WCreature c : immigration->collective->getCreatures())
        if (c->isAffected(LastingEffect::PREGNANT))
          ret.push_back(c->getPosition());
      return ret;
    }
  );
  return pickSpawnPositions(getCreatures(), positions);
}

const ImmigrantInfo& Immigration::Available::getInfo() const {
  return immigration->getImmigrants()[immigrantIndex];
}

optional<GlobalTime> Immigration::Available::getEndTime() const {
  return endTime;
}

optional<CostInfo> Immigration::Available::getCost() const {
  optional<CostInfo> ret;
  auto visitor = makeVisitor(
      [&](const CostInfo& cost) {
        ret = cost;
      },
      [&](const ExponentialCost& cost) {
        ret = immigration->calculateCost(immigrantIndex, cost);
      },
      [&](const auto&) {}
  );
  getInfo().visitRequirements(visitor);
  return ret;
}

CostInfo Immigration::calculateCost(int index, const ExponentialCost& cost) const {
  EntitySet<Creature> existing;
  for (WCreature c : collective->getCreatures())
    existing.insert(c);
  int numType = 0;
  if (auto gen = getReferenceMaybe(generated, index))
    for (auto id : *gen)
      if (existing.contains(id))
        ++numType;
  if (numType < cost.numFree)
    return CostInfo::noCost();
  else
    return CostInfo(cost.base.id, cost.base.value * pow(2, double(numType - cost.numFree) / cost.numToDoubleCost));
}

/*const ImmigrantInfo& Immigration::Available::getImmigrantInfo() const {
  return info;
}*/

Immigration::Available::Available(WImmigration im, vector<PCreature> c, int ind, optional<GlobalTime> t)
  : creatures(std::move(c)), immigrantIndex(ind), endTime(t), immigration(im) {
}

void Immigration::Available::addAllCreatures(const vector<Position>& spawnPositions) {
  const ImmigrantInfo& info = immigration->getImmigrants()[immigrantIndex];
  bool addedRecruits = false;
  info.visitRequirements(makeVisitor(
      [&](const RecruitmentInfo& recruitmentInfo) {
        auto recruits = recruitmentInfo.getAllRecruits(immigration->collective->getGame(), info.getId(0));
        if (!recruits.empty()) {
          WCreature c = recruits[0];
          immigration->collective->addCreature(c, {MinionTrait::FIGHTER});
          WModel target = immigration->collective->getModel();
          if (c->getPosition().getModel() != target)
            c->getGame()->transferCreature(c, target);
          addedRecruits = true;
        }
      },
      [](const auto&) {}
  ));
  if (!addedRecruits)
    for (auto creature : Iter(creatures)) {
      immigration->collective->addCreature(std::move(*creature), spawnPositions[creature.index()],
          getInfo().getTraits());
      creature.markToErase();
    }
}

void Immigration::accept(int id, bool withMessage) {
  CHECK(!collective->isConquered());
  if (!getAvailable().count(id))
    return;
  auto& candidate = available.at(id);
  auto& immigrantInfo = getImmigrants()[candidate.immigrantIndex];
  if (!getMissingRequirements(candidate).empty())
    return;
  vector<WCreature> creatures = candidate.getCreatures();
  const int groupSize = creatures.size();
  CHECK(groupSize >= 1);
  vector<Position> spawnPos = candidate.getSpawnPositions();
  if (spawnPos.size() < groupSize)
    return;
  if (immigrantInfo.isAutoTeam() && groupSize > 1)
    collective->getTeams().activate(collective->getTeams().createPersistent(creatures));
  if (withMessage)
    collective->addNewCreatureMessage(creatures);
  for (int i : All(creatures)) {
    WCreature c = creatures[i];
    if (i == 0 && groupSize > 1) // group leader
      c->getAttributes().increaseBaseExpLevel(ExperienceType::MELEE, 2);
    occupyRequirements(c, candidate.immigrantIndex);
    generated[candidate.immigrantIndex].insert(c);
  }
  candidate.addAllCreatures(spawnPos);
  if (!immigrantInfo.isPersistent())
    rejectIfNonPersistent(id);
  else if (immigrantInfo.isAvailable(generated[candidate.immigrantIndex].getSize()))
    available[id] = Available::generate(this, candidate.immigrantIndex);
}

void Immigration::rejectIfNonPersistent(int id) {
  if (auto immigrant = getReferenceMaybe(available, id))
    if (!immigrant->getInfo().isPersistent())
      immigrant->endTime = GlobalTime(-1);
}

SERIALIZE_DEF(Immigration::Available, creatures, immigrantIndex, endTime, immigration)
SERIALIZATION_CONSTRUCTOR_IMPL2(Immigration::Available, Available)

Immigration::Available Immigration::Available::generate(WImmigration immigration, int index) {
  return generate(immigration, Group {index, Random.get(immigration->getImmigrants()[index].getGroupSize()) });
}

int Immigration::getNumGeneratedAndCandidates(int index) const {
  int ret = 0;
  if (auto gen = getReferenceMaybe(generated, index))
    ret = gen->getSize();
  for (auto& available : getAvailable())
    if (available.second.get().getImmigrantIndex() == index)
      ++ret;
  return ret;
}

Immigration::Available Immigration::Available::generate(WImmigration immigration, const Group& group) {
  const ImmigrantInfo& info = immigration->getImmigrants()[group.immigrantIndex];
  vector<PCreature> immigrants;
  int numGenerated = immigration->generated[group.immigrantIndex].getSize();
  for (int i : Range(group.count))
    immigrants.push_back(CreatureFactory::fromId(info.getId(numGenerated), immigration->collective->getTribeId(),
        MonsterAIFactory::collective(immigration->collective)));
  return Available(
    immigration,
    std::move(immigrants),
    group.immigrantIndex,
    !info.isPersistent()
        ? immigration->collective->getGame()->getGlobalTime() + immigration->candidateTimeout
        : optional<GlobalTime>(none)
  );
}

vector<WCreature> Immigration::Available::getCreatures() const {
  return getWeakPointers(creatures);
}

bool Immigration::Available::isUnavailable() const {
  return creatures.empty() || (endTime && *endTime < immigration->collective->getGame()->getGlobalTime());
}

optional<milliseconds> Immigration::Available::getCreatedTime() const {
  return createdTime;
}

int Immigration::Available::getImmigrantIndex() const {
  return immigrantIndex;
}

void Immigration::initializePersistent() {
  for (auto elem : Iter(getImmigrants()))
    if (elem->isPersistent()) {
      available.emplace(++idCnt, Available::generate(this, Group{elem.index(), 1}));
      auto& elem = available.at(idCnt);
      for (int i : Range(elem.getInfo().getInitialRecruitment()))
        accept(idCnt, false);
    }
}

void Immigration::resetImmigrantTime() {
  auto interval = collective->getConfig().getImmigrantInterval();
  auto lastImmigrantIndex = floor(nextImmigrantTime.value_or(GlobalTime(-1)).getDouble() /
      interval.getDouble());
  nextImmigrantTime = max(
      collective->getGlobalTime(),
      GlobalTime((int) (interval.getDouble() * (Random.getDouble() + 1 + lastImmigrantIndex))));
}

void Immigration::update() {
  if (!initialized) {
    initialized = true;
    initializePersistent();
    resetImmigrantTime();
  }
  for (auto elem : Iter(available))
    if (elem->second.isUnavailable())
      elem.markToErase();
  for (auto elem : Iter(available))
    if (getValueMaybe(autoState, elem->second.immigrantIndex) == ImmigrantAutoState::AUTO_ACCEPT)
      accept(elem->first);
  if (!nextImmigrantTime || *nextImmigrantTime < collective->getGlobalTime()) {
    vector<Group> immigrantInfo;
    for (auto elem : Iter(getImmigrants()))
      immigrantInfo.push_back(Group {elem.index(), Random.get(elem->getGroupSize())});
    vector<double> weights = immigrantInfo.transform(
        [&](const Group& group) { return getImmigrantChance(group);});
    if (std::accumulate(weights.begin(), weights.end(), 0.0) > 0) {
      ++idCnt;
      available.emplace(idCnt, Available::generate(this, Random.choose(immigrantInfo, weights)));
      available[idCnt].createdTime = Clock::getRealMillis();
      resetImmigrantTime();
    }
  }
}
