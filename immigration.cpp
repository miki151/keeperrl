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
#include "equipment.h"
#include "player_control.h"
#include "view_object.h"
#include "content_factory.h"
#include "immigrant_info.h"
#include "special_trait.h"
#include "item.h"
#include "effect_type.h"
#include "storage_id.h"

template <class Archive>
void Immigration::serialize(Archive& ar, const unsigned int) {
  ar & SUBCLASS(OwnedObject<Immigration>);
  ar(available, minionAttraction, idCnt, collective, generated, candidateTimeout, initialized, nextImmigrantTime, autoState, immigrants);
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
          for (auto upgrade : collective->getGame()->getContentFactory()->furniture.getUpgrades(type))
            ret += constructions.getBuiltCount(upgrade);
          return ret;
        },
        [&](ItemIndex index) {
          return collective->getNumItems(index, false);
  });
}

template <typename Visitor>
static auto visitAttraction(const Immigration& immigration, const AttractionInfo& attraction, Visitor visit) {
  int value = 0;
  int occupation = 0;
  for (auto& type : attraction.types) {
    int thisValue = immigration.getAttractionValue(type);
    value += thisValue;
    occupation += min(thisValue, immigration.getAttractionOccupation(type));
  }
  return visit(value, max(0, value - occupation));
}

vector<string> Immigration::getMissingRequirements(const Available& available) const {
  vector<string> ret = getMissingRequirements(Group {available.immigrantIndex, (int)available.getCreatures().size()});
  int groupSize = available.getCreatures().size();
  if (!available.getInfo().getTraits().contains(MinionTrait::NO_LIMIT) &&
      collective->getPopulationSize() >= collective->getMaxPopulation())
    ret.push_back("Exceeds population limit");
  if (ret.empty() && available.getSpawnPositions().size() < groupSize)
    ret.push_back("Not enough room to spawn.");
  return ret;
}

void Immigration::setAutoState(int index, optional<ImmigrantAutoState> s) {
  if (!immigrants[index].isNoAuto()) {
    if (!s)
      autoState.erase(index);
    else
      autoState[index] = *s;
  }
}

optional<ImmigrantAutoState> Immigration::getAutoState(int index) const {
  return getValueMaybe(autoState, index);
}

optional<string> Immigration::getMissingRequirement(const ImmigrantRequirement& requirement, const Group& group) const {
  PROFILE;
  auto& immigrantInfo = immigrants[group.immigrantIndex];
  auto visitor = makeVisitor(
      [&](const AttractionInfo& attraction) -> optional<string> {
        return visitAttraction(*this, attraction,
            [&](int total, int available) -> optional<string> {
              int required = attraction.amountClaimed * group.count - available;
              if (required > 0) {
                const char* extra = total > 0 ? "more " : "";
                return "Requires " + toString(required) + " " + extra +
                    combineWithOr(attraction.types.transform([&](const AttractionType& type) {
                      return AttractionInfo::getAttractionName(collective->getGame()->getContentFactory(), type, required); }));
              } else
                return none;
            });
      },
      [&](const TechId& techId) -> optional<string> {
        if (!collective->getTechnology().researched.count(techId))
          return "Missing technology: "_s + techId.data();
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
          return "Not enough " + collective->getResourceInfo(cost.id).name;
        else
          return none;
      },
      [&](const ExponentialCost& cost) -> optional<string> {
        if (!collective->hasResource(calculateCost(group.immigrantIndex, cost) * group.count))
          return "Not enough " + collective->getResourceInfo(cost.base.id).name;
        else
          return none;
      },
      [&](const FurnitureType& type) -> optional<string> {
        if (collective->getConstructions().getBuiltCount(type) == 0)
          return "Requires " + collective->getGame()->getContentFactory()->furniture.getData(type).getName();
        else
          return none;
      },
      [&](const MinTurnRequirement& type) -> optional<string> {
        if (collective->getGlobalTime() < type.turn)
          return "Not available until turn " + toString(type.turn);
        else
          return none;
      },
      [&](const Pregnancy&) -> optional<string> {
        for (Creature* c : collective->getCreatures())
          if (c->isAffected(LastingEffect::PREGNANT))
            return none;
        return "Requires a pregnant succubus"_s;
      },
      [&](const RecruitmentInfo& info) -> optional<string> {
        auto cols = info.findEnemy(collective->getGame());
        if (cols.empty())
          return "Ally doesn't exist"_s;
        optional<string> result;
        for (auto col : cols) {
          if (!collective->isKnownVillainLocation(col))
            result = "Ally hasn't been discovered"_s;
          else if (info.getAvailableRecruits(collective->getGame(), immigrantInfo.getNonRandomId(0)).empty())
            result = "Ally doesn't have recruits available at this moment"_s;
          else
            return none;
        }
        return result;
      },
      [&](const TutorialRequirement& t) -> optional<string> {
        if ((int) collective->getGame()->getPlayerControl()->getTutorial()->getState() < (int) t.state)
          return "Tutorial not there yet"_s;
        else
          return none;
      },
      [&](const NegateRequirement& t) -> optional<string> {
        if (getMissingRequirement(*t.r, group))
          return none;
        else
          return "Not available"_s;
      },
      [&](const ImmigrantFlag& t) -> optional<string> {
        if (collective->getGame()->effectFlags.count(t.value))
          return none;
        else
          return "Requires " + t.value;
      }
  );
  return requirement.visit<optional<string>>(visitor);
}

vector<string> Immigration::getMissingRequirements(const Group& group) const {
  PROFILE;
  vector<string> ret;
  auto& immigrantInfo = immigrants[group.immigrantIndex];
  for (auto& requirement : immigrantInfo.requirements)
    if (auto req = getMissingRequirement(requirement.type, group))
      ret.push_back(*req);
  return ret;
}

double Immigration::getRequirementMultiplier(const Group& group) const {
  PROFILE;
  double ret = 1;
  auto& immigrantInfo = immigrants[group.immigrantIndex];
  for (auto& requirement : immigrantInfo.requirements)
    if (getMissingRequirement(requirement.type, group))
      ret *= requirement.candidateProb;
  return ret;
}

void Immigration::occupyRequirements(const Creature* c, int index) {
  PROFILE;
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
        for (Creature* c : collective->getCreatures())
          if (c->isAffected(LastingEffect::PREGNANT)) {
            c->removeEffect(LastingEffect::PREGNANT);
            break;
          }
      },
      [&](const RecruitmentInfo&) {},
      [&](const MinTurnRequirement&) {},
      [&](const TutorialRequirement&) {},
      [&](const NegateRequirement&) {},
      [&](const ImmigrantFlag&) {}
  );
  immigrants[index].visitRequirements(visitor);
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
  auto& info = immigrants[group.immigrantIndex];
  if (info.isPersistent() || !info.isAvailable(getNumGeneratedAndCandidates(group.immigrantIndex) + group.count - 1))
    return 0.0;
  else
    return info.getFrequency() * getRequirementMultiplier(group);
}

Immigration::Immigration(Collective* c, vector<ImmigrantInfo> immigrants)
  : collective(c), candidateTimeout(c->getConfig().getImmigrantTimeout()), immigrants(std::move(immigrants)) {
}

Immigration::~Immigration() {
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

static vector<Position> pickSpawnPositions(const vector<Creature*>& creatures, const vector<Position>& allPositions) {
  PROFILE;
  if (allPositions.empty() || creatures.empty())
    return {};
  vector<Position> spawnPos;
  auto game = allPositions.front().getGame();
  for (auto c : creatures) {
    PROFILE_BLOCK("Best for creature");
    auto movementType = c->getMovementType(game);
    auto goodPos = [&] (const Position& pos) -> optional<Position> {
      if (pos.canEnter(movementType) && !spawnPos.contains(pos))
        return pos;
      for (auto& neighbor : pos.neighbors8())
        if (neighbor.canEnter(movementType) && !spawnPos.contains(neighbor))
          return neighbor;
      for (auto& neighbor : pos.getRectangle(Rectangle::centered(3)))
        if (neighbor.canEnter(movementType) && !spawnPos.contains(neighbor))
          return neighbor;
      return none;
    };
    optional<Position> mySpawnPos;
    for (int i : Range(100))
      if (auto pos = goodPos(Random.choose(allPositions))) {
        mySpawnPos = pos;
        break;
      }
    if (!mySpawnPos) {
      INFO << "Couldn't spawn immigrant " << c->getName().bare();
      return {};
    } else
      spawnPos.push_back(*mySpawnPos);
  }
  return spawnPos;
}

vector<Position> Immigration::Available::getSpawnPositions() const {
  PROFILE;
  vector<Position> positions = getInfo().getSpawnLocation().visit<vector<Position>>(
    [&] (FurnitureType type) {
      return asVector<Position>(immigration->collective->getConstructions().getBuiltPositions(type));
    },
    [&] (OutsideTerritory) {
      auto ret = immigration->collective->getTerritory().getExtended(10, 20);
      auto tryLimitingToTopLevel = [&] {
        auto tmp = ret.filter([](Position pos) { return pos.getLevel() == pos.getModel()->getTopLevel(); });
        if (!tmp.empty())
          ret = tmp;
      };
      tryLimitingToTopLevel();
      if (ret.empty())
        ret = immigration->collective->getTerritory().getAll();
      tryLimitingToTopLevel();
      if (ret.empty())
        ret = immigration->collective->getLeaders().transform([](auto c) { return c->getPosition(); });
      return ret;
    },
    [&] (InsideTerritory) {
      auto ret = immigration->collective->getTerritory().getAll();
      if (ret.empty())
        ret = immigration->collective->getLeaders().transform([](auto c) { return c->getPosition(); });
      return ret;
    },
    [&] (NearLeader) -> vector<Position> {
      return immigration->collective->getLeaders().transform([](auto c) { return c->getPosition(); });
    },
    [&] (Pregnancy) {
      vector<Position> ret;
      for (Creature* c : immigration->collective->getCreatures())
        if (c->isAffected(LastingEffect::PREGNANT))
          ret.push_back(c->getPosition());
      return ret;
    }
  );
  return pickSpawnPositions(getCreatures(), positions);
}

const ImmigrantInfo& Immigration::Available::getInfo() const {
  return immigration->immigrants[immigrantIndex];
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
  for (Creature* c : collective->getCreatures())
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

Immigration::Available::Available(WImmigration im, vector<PCreature> c, int ind, optional<GlobalTime> t,
    vector<SpecialTrait> specialTraits)
  : creatures(std::move(c)), immigrantIndex(ind), endTime(t), immigration(im), specialTraits(std::move(specialTraits)) {
}

void Immigration::Available::addAllCreatures(const vector<Position>& spawnPositions) {
  const ImmigrantInfo& info = immigration->immigrants[immigrantIndex];
  bool addedRecruits = false;
  vector<Creature*> immigrants;
  info.visitRequirements(makeVisitor(
      [&](const RecruitmentInfo& recruitmentInfo) {
        auto recruits = recruitmentInfo.getAllRecruits(immigration->collective->getGame(), info.getId(0));
        if (!recruits.empty()) {
          Creature* c = recruits[0];
          immigration->collective->addCreature(c, info.getTraits());
          WModel target = immigration->collective->getModel();
          if (c->getPosition().getModel() != target)
            c->getGame()->transferCreature(c, target);
          immigrants.push_back(c);
          addedRecruits = true;
        }
      },
      [](const auto&) {}
  ));
  if (!addedRecruits) {
    auto creatureRefs = getWeakPointers(creatures);
    for (auto creature : Iter(creatures)) {
      if (auto c = immigration->collective->addCreature(std::move(*creature), spawnPositions[creature.index()],
          getInfo().getTraits()))
        immigrants.push_back(c);
      creature.markToErase();
    }
    if (!getInfo().getTraits().contains(MinionTrait::NO_LIMIT))
      immigration->collective->setPopulationGroup(creatureRefs);
  }
  for (auto& trait : specialTraits) {
/*  if (specialTrait.colorVariant)
      for (auto& c : immigrants)
        c->modViewObject().setColorVariant(*specialTrait.colorVariant);*/
    applySpecialTrait(immigration->collective->getGame()->getGlobalTime(), trait, immigrants.back(),
         immigration->collective->getGame()->getContentFactory());
  }
}

void Immigration::accept(int id, bool withMessage) {
  CHECK(!collective->isConquered());
  if (!getAvailable().count(id))
    return;
  auto& candidate = available.at(id);
  auto& immigrantInfo = immigrants[candidate.immigrantIndex];
  if (!getMissingRequirements(candidate).empty())
    return;
  vector<Creature*> creatures = candidate.getCreatures();
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
    Creature* c = creatures[i];
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
  if (auto immigrant = getReferenceMaybe(available, id)) {
    if (!immigrant->getInfo().isPersistent())
      immigrant->endTime = GlobalTime(-1);
    else if (!immigrant->getInfo().getLimit()) // if there is no limit then we allow cycling through the ids by rejecting
     available[id] = Available::generate(this, immigrant->immigrantIndex);
  }
}

SERIALIZE_DEF(Immigration::Available, creatures, immigrantIndex, endTime, immigration, specialTraits)
SERIALIZATION_CONSTRUCTOR_IMPL2(Immigration::Available, Available)

Immigration::Available Immigration::Available::generate(WImmigration immigration, int index) {
  return generate(immigration, Group {index, Random.get(immigration->immigrants[index].getGroupSize()) });
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
  const ImmigrantInfo& info = immigration->immigrants[group.immigrantIndex];
  vector<PCreature> immigrants;
  int numGenerated = immigration->generated[group.immigrantIndex].getSize();
  vector<SpecialTrait> specialTraits;
  auto contentFactory = immigration->collective->getGame()->getContentFactory();
  for (int i : Range(group.count)) {
    immigrants.push_back(contentFactory->getCreatures().
        fromId(info.getId(numGenerated), immigration->collective->getTribeId(),
            MonsterAIFactory::collective(immigration->collective)));
    if (immigration->collective->getConfig().getStripSpawns() && info.stripEquipment)
      immigrants.back()->getEquipment().removeAllItems(immigrants.back().get());
    for (auto& specialTrait : info.getSpecialTraits())
      if (Random.chance(specialTrait.prob)) {
        for (auto& trait1 : specialTrait.traits) {
          auto trait = transformBeforeApplying(trait1);
          specialTraits.push_back(trait);
        }
      }
  }
  return Available(
    immigration,
    std::move(immigrants),
    group.immigrantIndex,
    !info.isPersistent()
        ? immigration->collective->getGame()->getGlobalTime() + immigration->candidateTimeout
        : optional<GlobalTime>(none),
    specialTraits
  );
}

vector<Creature*> Immigration::Available::getCreatures() const {
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

const vector<SpecialTrait>& Immigration::Available::getSpecialTraits() const {
  return specialTraits;
}

void Immigration::initializePersistent() {
  for (auto elem : Iter(immigrants))
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
    for (auto elem : Iter(immigrants))
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

const vector<ImmigrantInfo>& Immigration::getImmigrants() const {
  return immigrants;
}
