#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"
#include "effect.h"
#include "level.h"
#include "item.h"
#include "item_factory.h"
#include "statistics.h"
#include "technology.h"
#include "monster.h"
#include "options.h"
#include "trigger.h"
#include "model.h"
#include "game.h"
#include "spell.h"
#include "location.h"
#include "view_id.h"
#include "equipment.h"
#include "view_index.h"
#include "minion_equipment.h"
#include "task_map.h"
#include "collective_teams.h"
#include "known_tiles.h"
#include "construction_map.h"
#include "minion_task_map.h"
#include "tribe.h"
#include "collective_config.h"
#include "creature_name.h"
#include "modifier_type.h"
#include "cost_info.h"
#include "monster_ai.h"
#include "task.h"
#include "territory.h"
#include "collective_attack.h"
#include "gender.h"
#include "collective_name.h"
#include "creature_attributes.h"
#include "event_proxy.h"
#include "villain_type.h"
#include "workshops.h"
#include "attack_trigger.h"
#include "spell_map.h"
#include "body.h"
#include "furniture.h"
#include "furniture_factory.h"
#include "tile_efficiency.h"
#include "zones.h"
#include "experience_type.h"
#include "furniture_usage.h"
#include "collective_warning.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(TaskCallback);
  serializeAll(ar, creatures, leader, taskMap, tribe, control, byTrait, bySpawnType);
  serializeAll(ar, territory, alarmInfo, markedItems, constructions, minionEquipment);
  serializeAll(ar, surrendering, delayedPos, knownTiles, technologies, kills, points, currentTasks);
  serializeAll(ar, credit, level, pregnancies, minionAttraction, teams, name);
  serializeAll(ar, config, warnings, banished, equipmentUpdates);
  serializeAll(ar, villainType, eventProxy, workshops, zones, tileEfficiency);
}

SERIALIZABLE(Collective);

SERIALIZATION_CONSTRUCTOR_IMPL(Collective);

Collective::Collective(Level* l, const CollectiveConfig& cfg, TribeId t, EnumMap<ResourceId, int> _credit,
    const CollectiveName& n) 
  : eventProxy(this, l->getModel()), credit(_credit), control(CollectiveControl::idle(this)),
    tribe(t), level(NOTNULL(l)), name(n), config(cfg), workshops(config->getWorkshops()) {
}

const CollectiveName& Collective::getName() const {
  return *name;
}

void Collective::setVillainType(VillainType t) {
  villainType = t;
}

optional<VillainType> Collective::getVillainType() const {
  return villainType;
}

Collective::~Collective() {
}

double Collective::getAttractionOccupation(const MinionAttraction& attraction) {
  double res = 0;
  for (auto& elem : minionAttraction)
    for (auto& info : elem.second)
      if (info.attraction == attraction)
        res += info.amountClaimed;
  return res;
}

double Collective::getAttractionValue(const MinionAttraction& attraction) {
  switch (attraction.getId()) {
    case AttractionId::FURNITURE: {
      double ret = constructions->getBuiltCount(attraction.get<FurnitureType>());
      for (auto type : ENUM_ALL(FurnitureType))
        if (FurnitureFactory::isUpgrade(attraction.get<FurnitureType>(), type))
          ret += constructions->getBuiltCount(type);
      return ret;
    }
    case AttractionId::ITEM_INDEX: 
      return getNumItems(attraction.get<ItemIndex>(), false);
  }
}

namespace {

class LeaderControlOverride : public Creature::MoraleOverride {
  public:
  LeaderControlOverride(Collective* col) : collective(col) {}

  virtual optional<double> getMorale(const Creature* creature) override {
    for (auto team : collective->getTeams().getContaining(collective->getLeader()))
      if (collective->getTeams().isActive(team) && collective->getTeams().contains(team, creature) &&
          collective->getTeams().getLeader(team) == collective->getLeader())
        return 1;
    return none;
  }

  SERIALIZATION_CONSTRUCTOR(LeaderControlOverride);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Creature::MoraleOverride) & SVAR(collective);
  }

  private:
  Collective* SERIAL(collective);
};

}

void Collective::addCreatureInTerritory(PCreature creature, EnumSet<MinionTrait> traits) {
  for (Position pos : Random.permutation(territory->getAll()))
    if (pos.canEnter(creature.get())) {
      addCreature(std::move(creature), pos, traits);
      return;
    }
}

void Collective::addCreature(PCreature creature, Position pos, EnumSet<MinionTrait> traits) {
  if (config->getStripSpawns())
    creature->getEquipment().removeAllItems();
  Creature* c = creature.get();
  pos.addCreature(std::move(creature));
  addCreature(c, traits);
}

void Collective::addCreature(Creature* c, EnumSet<MinionTrait> traits) {
  if (!traits.contains(MinionTrait::FARM_ANIMAL))
    c->setController(PController(new Monster(c, MonsterAIFactory::collective(this))));
  if (traits.contains(MinionTrait::WORKER))
    c->getAttributes().getMinionTasks().clear();
  if (!leader)
    leader = c;
  CHECK(c->getTribeId() == *tribe);
  if (Game* game = getGame())
    for (Collective* col : getGame()->getCollectives())
      if (contains(col->getCreatures(), c))
        col->removeCreature(c);
  creatures.push_back(c);
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
  if (auto spawnType = c->getAttributes().getSpawnType())
    bySpawnType[*spawnType].push_back(c);
  for (const Item* item : c->getEquipment().getItems())
    minionEquipment->own(c, item);
  if (traits.contains(MinionTrait::FIGHTER)) {
    c->setMoraleOverride(Creature::PMoraleOverride(new LeaderControlOverride(this)));
  }
}

void Collective::removeCreature(Creature* c) {
  removeElement(creatures, c);
  minionAttraction.erase(c);
  returnResource(taskMap->freeFromTask(c));
  if (auto spawnType = c->getAttributes().getSpawnType())
    removeElement(bySpawnType[*spawnType], c);
  for (auto team : teams->getContaining(c))
    teams->remove(team, c);
  for (MinionTrait t : ENUM_ALL(MinionTrait))
    if (contains(byTrait[t], c))
      removeElement(byTrait[t], c);
  c->setMoraleOverride(nullptr);
}

void Collective::banishCreature(Creature* c) {
  decreaseMoraleForBanishing(c);
  removeCreature(c);
  vector<Position> exitTiles = territory->getExtended(10, 20);
  vector<PTask> tasks;
  vector<Item*> items = c->getEquipment().getItems();
  if (!items.empty())
    tasks.push_back(Task::dropItems(items));
  if (!exitTiles.empty())
    tasks.push_back(Task::goToTryForever(Random.choose(exitTiles)));
  tasks.push_back(Task::disappear());
  c->setController(PController(new Monster(c, MonsterAIFactory::singleTask(Task::chain(std::move(tasks))))));
  banished.insert(c);
}

bool Collective::wasBanished(const Creature* c) const {
  return banished.contains(c);
}

vector<Creature*> Collective::getRecruits() const {
  vector<Creature*> ret;
  vector<Creature*> possibleRecruits = filter(getCreatures(MinionTrait::FIGHTER),
      [] (const Creature* c) { return c->getAttributes().getRecruitmentCost() > 0; });
  if (auto minPop = config->getRecruitingMinPopulation())
    for (int i = *minPop; i < possibleRecruits.size(); ++i)
      ret.push_back(possibleRecruits[i]);
  return ret;
}

void Collective::recruit(Creature* c, Collective* to) {
  removeCreature(c);
  c->setTribe(to->getTribeId());
  to->addCreature(c, {MinionTrait::FIGHTER});
}

vector<Item*> Collective::getTradeItems() const {
  vector<Item*> ret;
  for (Position pos : territory->getAll())
    append(ret, pos.getItems(ItemIndex::FOR_SALE));
  return ret;
}

PItem Collective::buyItem(Item* item) {
  for (Position pos : territory->getAll())
    for (Item* it : pos.getItems(ItemIndex::FOR_SALE))
      if (it == item) {
        PItem ret = pos.removeItem(it);
        ret->setShopkeeper(nullptr);
        return ret;
      }
  FAIL << "Couldn't find item";
  return nullptr;
}

vector<TriggerInfo> Collective::getTriggers(const Collective* against) const {
  return control->getTriggers(against);
}

const Creature* Collective::getLeader() const {
  return leader;
}

Creature* Collective::getLeader() {
  return leader;
}

bool Collective::hasLeader() const {
  return leader && !leader->isDead();
}

Level* Collective::getLevel() {
  return level;
}

const Level* Collective::getLevel() const {
  return level;
}

Game* Collective::getGame() const {
  return level->getModel()->getGame();
}

CollectiveControl* Collective::getControl() const {
  return control.get();
}

TribeId Collective::getTribeId() const {
  return *tribe;
}

Tribe* Collective::getTribe() const {
  return getGame()->getTribe(*tribe);
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

MoveInfo Collective::getDropItems(Creature *c) {
  if (territory->contains(c->getPosition())) {
    vector<Item*> items = c->getEquipment().getItems([this, c](const Item* item) {
        return minionEquipment->isItemUseful(item) && !minionEquipment->isOwner(item, c); });
    if (!items.empty())
      return c->drop(items);
  }
  return NoMove;
}

MoveInfo Collective::getWorkerMove(Creature* c) {
  if (Task* task = taskMap->getTask(c))
    return task->getMove(c);
  if (Task* closest = taskMap->getClosestTask(c, MinionTrait::WORKER)) {
    taskMap->takeTask(c, closest);
    return closest->getMove(c);
  } else {
    if (config->getWorkerFollowLeader() && hasLeader() && territory->isEmpty()) {
      Position leaderPos = getLeader()->getPosition();
      if (leaderPos.dist8(c->getPosition()) < 3)
        return NoMove;
      if (auto action = c->moveTowards(leaderPos))
        return {1.0, action};
      else
        return NoMove;
    } else if (!hasTrait(c, MinionTrait::NO_RETURNING) &&  !territory->isEmpty() &&
               !territory->contains(c->getPosition()))
        return c->moveTowards(Random.choose(territory->getAll()));
      return NoMove;
  }
}

void Collective::setMinionTask(const Creature* c, MinionTask task) {
  if (auto duration = MinionTasks::getDuration(c, task))
    currentTasks.set(c, {task, c->getLocalTime() + *duration});
  else
    currentTasks.set(c, {task, none});
}

optional<MinionTask> Collective::getMinionTask(const Creature* c) const {
  if (auto current = currentTasks.getMaybe(c))
    return current->task;
  else
    return none;
}

bool Collective::isTaskGood(const Creature* c, MinionTask task, bool ignoreTaskLock) const {
  if (c->getAttributes().getMinionTasks().getValue(task, ignoreTaskLock) == 0)
    return false;
  switch (task) {
    case MinionTask::CROPS:
    case MinionTask::EXPLORE:
        return getGame()->getSunlightInfo().getState() == SunlightState::DAY;
    case MinionTask::SLEEP:
        if (!config->sleepOnlyAtNight())
          return true;
        // break skipped on purpose
    case MinionTask::EXPLORE_NOCTURNAL:
        return getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
    default: return true;
  }
}

void Collective::setRandomTask(const Creature* c) {
  vector<pair<MinionTask, double>> goodTasks;
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (isTaskGood(c, t))
      goodTasks.push_back({t, c->getAttributes().getMinionTasks().getValue(t)});
  if (!goodTasks.empty())
    setMinionTask(c, Random.choose(goodTasks));
}

bool Collective::isMinionTaskPossible(Creature* c, MinionTask task) {
  return isTaskGood(c, task, true) && MinionTasks::generate(this, c, task);
}

PTask Collective::getStandardTask(Creature* c) {
  if (!c->getAttributes().getMinionTasks().hasAnyTask())
    return nullptr;
  auto current = currentTasks.getMaybe(c);
  if (!current || (current->finishTime && *current->finishTime < c->getLocalTime()) || !isTaskGood(c, current->task)) {
    currentTasks.erase(c);
    setRandomTask(c);
  }
  if (auto current = currentTasks.getMaybe(c)) {
    MinionTask task = current->task;
    auto& info = config->getTaskInfo(task);
    PTask ret = MinionTasks::generate(this, c, task);
    if (!current->finishTime) // see comment in header
      currentTasks.getOrFail(c).finishTime = -1000;
    if (info.warning && !territory->isEmpty())
      warnings->setWarning(*info.warning, !ret);
    if (!ret)
      currentTasks.erase(c);
    return ret;
  } else
    return nullptr;
}

bool Collective::isConquered() const {
  return getCreatures(MinionTrait::FIGHTER).empty() && !hasLeader();
}

vector<Creature*> Collective::getConsumptionTargets(Creature* consumer) const {
  vector<Creature*> ret;
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    if (consumer->canConsume(c) && c != getLeader())
      ret.push_back(c);
  return ret;
}

void Collective::orderConsumption(Creature* consumer, Creature* who) {
  CHECK(contains(getConsumptionTargets(consumer), who));
  setTask(consumer, Task::consume(this, who));
}

void Collective::ownItem(const Creature* c, const Item* it) {
  minionEquipment->own(c, it);
  equipmentUpdates.insert(c);
}

PTask Collective::getEquipmentTask(Creature* c) {
  if (!Random.roll(20) && !equipmentUpdates.contains(c))
    return nullptr;
  equipmentUpdates.erase(c);
  autoEquipment(c, Random.roll(10));
  vector<PTask> tasks;
  for (Item* it : c->getEquipment().getItems())
    if (!c->getEquipment().isEquipped(it) && c->getEquipment().canEquip(it))
      tasks.push_back(Task::equipItem(it));
  for (Position v : zones->getPositions(ZoneId::STORAGE_EQUIPMENT)) {
    vector<Item*> it = filter(v.getItems(ItemIndex::MINION_EQUIPMENT), 
        [this, c] (const Item* it) { return minionEquipment->isOwner(it, c) && it->canEquip(); });
    if (!it.empty())
      tasks.push_back(Task::pickAndEquipItem(this, v, it[0]));
    it = filter(v.getItems(ItemIndex::MINION_EQUIPMENT), 
        [this, c] (const Item* it) { return minionEquipment->isOwner(it, c); });
    if (!it.empty())
      tasks.push_back(Task::pickItem(this, v, it));
  }
  if (!tasks.empty())
    return Task::chain(std::move(tasks));
  return nullptr;
}

void Collective::considerHealingTask(Creature* c) {
  if (c->getBody().canHeal() && !c->isAffected(LastingEffect::POISON))
    for (MinionTask t : {MinionTask::SLEEP, MinionTask::GRAVE, MinionTask::LAIR})
      if (c->getAttributes().getMinionTasks().getValue(t) > 0) {
        cancelTask(c);
        setMinionTask(c, t);
        return;
      }
}

void Collective::clearLeader() {
  leader = nullptr;
}

MoveInfo Collective::getTeamMemberMove(Creature* c) {
  for (auto team : teams->getContaining(c))
    if (teams->isActive(team)) {
      const Creature* leader = teams->getLeader(team);
      if (c != leader) {
        if (leader->getPosition().dist8(c->getPosition()) > 1)
          return c->moveTowards(leader->getPosition());
        else
          return c->wait();
      } else
        if (c == leader && !teams->isPersistent(team))
          return c->wait();
    }
  return NoMove;
}

void Collective::setTask(const Creature *c, PTask task) {
  returnResource(taskMap->freeFromTask(c));
  taskMap->addTaskFor(std::move(task), c);
}

bool Collective::hasTask(const Creature* c) const {
  return taskMap->hasTask(c);
}

void Collective::cancelTask(const Creature* c) {
  if (Task* task = taskMap->getTask(c))
    taskMap->removeTask(task);
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  CHECK(!c->isDead());
/*  CHECK(contains(c->getPosition().getModel()->getLevels(), c->getPosition().getLevel())) <<
      c->getPosition().getLevel()->getName() << " " << c->getName().bare();*/
  if (Task* task = taskMap->getTask(c))
    if (taskMap->isPriorityTask(task))
      return task->getMove(c);
  if (MoveInfo move = getTeamMemberMove(c))
    return move;
  if (hasTrait(c, MinionTrait::WORKER))
    return getWorkerMove(c);
  if (config->getFetchItems())
    if (MoveInfo move = getDropItems(c))
      return move;
  if (hasTrait(c, MinionTrait::FIGHTER)) {
    if (MoveInfo move = getAlarmMove(c))
      return move;
  }
  considerHealingTask(c);
  if (Task* task = taskMap->getTask(c))
    return task->getMove(c);
  if (Task* closest = taskMap->getClosestTask(c, MinionTrait::FIGHTER)) {
    taskMap->takeTask(c, closest);
    return closest->getMove(c);
  }
  if (usesEquipment(c))
    if (PTask t = getEquipmentTask(c))
      if (t->getMove(c))
        return taskMap->addTaskFor(std::move(t), c)->getMove(c);
  if (PTask t = getStandardTask(c))
    if (t->getMove(c))
      return taskMap->addTaskFor(std::move(t), c)->getMove(c);
  if (!hasTrait(c, MinionTrait::NO_RETURNING) && !territory->isEmpty() &&
      !territory->contains(c->getPosition()) && teams->getActive(c).empty()) {
    if (c->getPosition().getModel() == getLevel()->getModel())
      return c->moveTowards(Random.choose(territory->getAll()));
    else
      if (PTask t = Task::transferTo(getLevel()->getModel()))
        return taskMap->addTaskFor(std::move(t), c)->getMove(c);
  }
  return NoMove;
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

vector<Position> Collective::getEnemyPositions() const {
  vector<Position> enemyPos;
  for (Position pos : territory->getExtended(10))
    if (const Creature* c = pos.getCreature())
      if (getTribe()->isEnemy(c))
        enemyPos.push_back(pos);
  return enemyPos;
}

static int countNeighbor(Position pos, const set<Position>& squares) {
  int num = 0;
  for (Position v : pos.neighbors8())
    num += squares.count(v);
  return num;
}

static optional<Position> chooseBedPos(const set<Position>& lair, const set<Position>& beds) {
  vector<Position> res;
  for (Position v : lair) {
    if (countNeighbor(v, beds) > 2)
      continue;
    bool bad = false;
    for (Position n : v.neighbors8())
      if (beds.count(n) && countNeighbor(n, beds) >= 2) {
        bad = true;
        break;
      }
    if (!bad)
      res.push_back(v);
  }
  if (!res.empty())
    return Random.choose(res);
  else
    return none;
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
      Debug() << "Couldn't spawn immigrant " << c->getName().bare();
      return {};
    } else
      spawnPos.push_back(pos);
  }
  return spawnPos;
}

bool Collective::considerNonSpawnImmigrant(const ImmigrantInfo& info, vector<PCreature> immigrants) {
  CHECK(!info.spawnAtDorm);
  auto creatureRefs = extractRefs(immigrants);
  vector<Position> spawnPos;
  if (!config->activeImmigrantion(getGame())) {
    for (Position v : Random.permutation(territory->getAll()))
      if (v.canEnter(immigrants[spawnPos.size()].get())) {
        spawnPos.push_back(v);
        if (spawnPos.size() >= immigrants.size())
          break;
      }
  } else 
    spawnPos = getSpawnPos(creatureRefs, territory->getExtended(10, 20));
  if (spawnPos.size() < immigrants.size())
    return false;
  if (info.autoTeam)
    teams->activate(teams->createPersistent(extractRefs(immigrants)));
  for (int i : All(immigrants)) {
    Creature* c = immigrants[i].get();
    addCreature(std::move(immigrants[i]), spawnPos[i], info.traits);
    minionAttraction.set(c, info.attractions);
  }
  addNewCreatureMessage(creatureRefs);
  return true;
}

static CostInfo getSpawnCost(SpawnType type, int howMany) {
  switch (type) {
    case SpawnType::UNDEAD:
      return {CollectiveResourceId::CORPSE, howMany};
    default:
      return CostInfo::noCost();
  }
}

bool Collective::considerImmigrant(const ImmigrantInfo& info) {
  if (info.techId && !hasTech(*info.techId))
    return false;
  vector<PCreature> immigrants;
  int groupSize = info.groupSize ? Random.get(*info.groupSize) : 1;
  groupSize = min(groupSize, getMaxPopulation() - getPopulationSize());
  for (int i : Range(groupSize))
    immigrants.push_back(CreatureFactory::fromId(info.id, getTribeId(), MonsterAIFactory::collective(this)));
  if (!immigrants[0]->getAttributes().getSpawnType() || info.ignoreSpawnType)
    return considerNonSpawnImmigrant(info, std::move(immigrants));
  SpawnType spawnType = *immigrants[0]->getAttributes().getSpawnType();
  FurnitureType bedType = config->getDormInfo()[spawnType].bedType;
  if (!hasResource(getSpawnCost(spawnType, groupSize)))
    return false;
  vector<Position> spawnPos = getSpawnPos(extractRefs(immigrants), info.spawnAtDorm ?
      asVector<Position>(constructions->getBuiltPositions(bedType)) : territory->getExtended(10, 20));
  groupSize = min<int>(groupSize, spawnPos.size());
  int neededBeds = max<int>(0, bySpawnType[spawnType].size() + groupSize
      - constructions->getBuiltCount(config->getDormInfo()[spawnType].bedType));
  groupSize -= neededBeds;
  if (groupSize < 1)
    return false;
  if (immigrants.size() > groupSize)
    immigrants.resize(groupSize);
  takeResource(getSpawnCost(spawnType, immigrants.size()));
  if (info.autoTeam && groupSize > 1)
    teams->activate(teams->createPersistent(extractRefs(immigrants)));
  addNewCreatureMessage(extractRefs(immigrants));
  for (int i : All(immigrants)) {
    Creature* c = immigrants[i].get();
    if (i == 0 && groupSize > 1) // group leader
      c->getAttributes().increaseBaseExpLevel(2);
    addCreature(std::move(immigrants[i]), spawnPos[i], info.traits);
    minionAttraction.set(c, info.attractions);
  }
  return true;
}

void Collective::addNewCreatureMessage(const vector<Creature*>& immigrants) {
  if (immigrants.size() == 1)
    control->addMessage(PlayerMessage(immigrants[0]->getName().a() + " joins your forces.")
        .setCreature(immigrants[0]->getUniqueId()));
  else {
    control->addMessage(PlayerMessage("A " + immigrants[0]->getName().multiple(immigrants.size()) + 
          " joins your forces.").setCreature(immigrants[0]->getUniqueId()));
  }
}

double Collective::getImmigrantChance(const ImmigrantInfo& info) {
  if (info.limit && info.limit != getGame()->getSunlightInfo().getState())
    return 0;
  double result = 0;
  if (info.attractions.empty())
    return -1;
  for (auto& attraction : info.attractions) {
    double value = getAttractionValue(attraction.attraction);
    if (value < 0.001 && attraction.mandatory)
      return 0;
    result += max(0.0, value - getAttractionOccupation(attraction.attraction) - attraction.minAmount);
  }
  return result * info.frequency;
}

void Collective::considerImmigration() {
  if (getPopulationSize() >= getMaxPopulation() || !hasLeader())
    return;
  vector<double> weights;
  bool ok = false;
  for (auto& elem : config->getImmigrantInfo()) {
    weights.push_back(getImmigrantChance(elem));
    if (weights.back() > 0 || weights.back() < -0.9)
      ok = true;
  }
  double avgWeight = 0; 
  int numPositive = 0;
  for (double elem : weights)
    if (elem > 0) {
      ++numPositive;
      avgWeight += elem;
    }
  for (double& elem : weights)
    if (elem < -0.9) {
      if (numPositive == 0)
        elem = 1;
      else
        elem = avgWeight / numPositive;
    }
  if (!ok)
    return;
  for (int i : Range(10))
    if (considerImmigrant(Random.choose(config->getImmigrantInfo(), weights)))
      break;
}

void Collective::considerBirths() {
  for (Creature* c : getCreatures())
    if (pregnancies.contains(c) && !c->isAffected(LastingEffect::PREGNANT)) {
      pregnancies.erase(c);
      if (getPopulationSize() < getMaxPopulation()) {
        vector<pair<CreatureId, double>> candidates;
        for (auto& elem : config->getBirthSpawns())
          if (!elem.tech || hasTech(*elem.tech)) 
            candidates.emplace_back(elem.id, elem.frequency);
        if (candidates.empty())
          return;
        PCreature spawn = CreatureFactory::fromId(Random.choose(candidates), getTribeId());
        for (Position pos : c->getPosition().neighbors8(Random))
          if (pos.canEnter(spawn.get())) {
            control->addMessage(c->getName().a() + " gives birth to " + spawn->getName().a());
            c->playerMessage("You give birth to " + spawn->getName().a() + "!");
            c->monsterMessage(c->getName().the() + " gives birth to "  + spawn->getName().a());
            addCreature(std::move(spawn), pos, {MinionTrait::FIGHTER});
            return;
          }
      }
    }
}

void Collective::decayMorale() {
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(-c->getMorale() * 0.0008);
}

void Collective::update(bool currentlyActive) {
  control->update(currentlyActive);
  if (currentlyActive == config->activeImmigrantion(getGame()) &&
      Random.chance(config->getImmigrantFrequency()))
    considerImmigration();
}

void Collective::tick() {
  control->tick();
  zones->tick();
  considerBirths();
  decayMorale();
  if (config->getWarnings() && Random.roll(5))
    warnings->considerWarnings(this);
  if (config->getEnemyPositions() && Random.roll(5)) {
    vector<Position> enemyPos = getEnemyPositions();
    if (!enemyPos.empty())
      delayDangerousTasks(enemyPos, getLocalTime() + 20);
    else {
      alarmInfo.reset();
      control->onNoEnemies();
    }
    bool allSurrender = true;
    vector<Creature*> surrenderingVec;
    for (Position v : enemyPos)
      if (!surrendering.contains(NOTNULL(v.getCreature()))) {
        allSurrender = false;
        break;
      } else
        surrenderingVec.push_back(v.getCreature());
    if (allSurrender) {
      for (Creature* c : surrenderingVec) {
        if (!c->isDead() && territory->contains(c->getPosition())) {
          Position pos = c->getPosition();
          PCreature prisoner = CreatureFactory::fromId(CreatureId::PRISONER, getTribeId(),
              MonsterAIFactory::collective(this));
          if (pos.canEnterEmpty(prisoner.get())) {
            pos.globalMessage(c->getName().the() + " surrenders.");
            control->addMessage(PlayerMessage(c->getName().a() + " surrenders.").setPosition(c->getPosition()));
            c->die(nullptr, true, false);
            addCreature(std::move(prisoner), pos, {MinionTrait::PRISONER, MinionTrait::NO_LIMIT});
          }
        }
        surrendering.erase(c);
      }
    }
  }
  if (config->getConstructions())
    updateConstructions();
  if (config->getFetchItems() && Random.roll(5))
    for (const ItemFetchInfo& elem : CollectiveConfig::getFetchInfo()) {
      for (Position pos : territory->getAll())
        fetchItems(pos, elem);
      for (Position pos : zones->getPositions(ZoneId::FETCH_ITEMS))
        fetchItems(pos, elem);
      for (Position pos : zones->getPositions(ZoneId::PERMANENT_FETCH_ITEMS))
        fetchItems(pos, elem);
    }
  if (config->getManageEquipment() && Random.roll(10))
    minionEquipment->updateOwners(getAllItems(true), getCreatures());
  workshops->scheduleItems(this);
}

const vector<Creature*>& Collective::getCreatures(MinionTrait trait) const {
  return byTrait[trait];
}

const vector<Creature*>& Collective::getCreatures(SpawnType type) const {
  return bySpawnType[type];
}

bool Collective::hasTrait(const Creature* c, MinionTrait t) const {
  return contains(byTrait[t], c);
}

bool Collective::hasAnyTrait(const Creature* c, EnumSet<MinionTrait> traits) const {
  for (MinionTrait t : traits)
    if (hasTrait(c, t))
      return true;
  return false;
}

void Collective::setTrait(Creature* c, MinionTrait t) {
  if (!hasTrait(c, t))
    byTrait[t].push_back(c);
}

vector<Creature*> Collective::getCreaturesAnyOf(EnumSet<MinionTrait> trait) const {
  set<Creature*> ret;
  for (MinionTrait t : trait)
    for (Creature* c : byTrait[t])
      ret.insert(c);
  return vector<Creature*>(ret.begin(), ret.end());
}

vector<Creature*> Collective::getCreatures(EnumSet<MinionTrait> with, EnumSet<MinionTrait> without) const {
  vector<Creature*> ret;
  for (Creature* c : creatures) {
    bool ok = true;
    for (MinionTrait t : with)
      if (!hasTrait(c, t)) {
        ok = false;
        break;
      }
    for (MinionTrait t : without)
      if (hasTrait(c, t)) {
        ok = false;
        break;
      }
    if (ok)
      ret.push_back(c);
  }
  return ret;
}

double Collective::getKillManaScore(const Creature* victim) const {
  return 0;
/*  int ret = victim->getDifficultyPoints() / 3;
  if (victim->isAffected(LastingEffect::SLEEP))
    ret *= 2;
  return ret;*/
}

void Collective::addMoraleForKill(const Creature* killer, const Creature* victim) {
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(c == killer ? 0.25 : 0.015);
}

void Collective::decreaseMoraleForKill(const Creature* killer, const Creature* victim) {
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(victim == getLeader() ? -2 : -0.015);
}

void Collective::decreaseMoraleForBanishing(const Creature*) {
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(-0.05);
}

void Collective::onKillCancelled(Creature* c) {
}

void Collective::onEvent(const GameEvent& event) {
  switch (event.getId()) {
    case EventId::ALARM: {
        Position pos = event.get<Position>();
        static const int alarmTime = 100;
        if (getTerritory().contains(pos)) {
          control->addMessage(PlayerMessage("An alarm goes off.", MessagePriority::HIGH).setPosition(pos));
          alarmInfo = {getGlobalTime() + alarmTime, pos };
          for (Creature* c : byTrait[MinionTrait::FIGHTER])
            if (c->isAffected(LastingEffect::SLEEP))
              c->removeEffect(LastingEffect::SLEEP);
        }
      }
      break;
    case EventId::KILLED: {
        Creature* victim = event.get<EventInfo::Attacked>().victim;
        Creature* killer = event.get<EventInfo::Attacked>().attacker;
        if (contains(creatures, victim))
          onMinionKilled(victim, killer);
        if (contains(creatures, killer))
          onKilledSomeone(killer, victim);
      }
      break;
    case EventId::TORTURED:
      if (contains(creatures, event.get<EventInfo::Attacked>().attacker))
        returnResource({ResourceId::MANA, 1});
      break;
    case EventId::SURRENDERED: {
        Creature* victim = event.get<EventInfo::Attacked>().victim;
        Creature* attacker = event.get<EventInfo::Attacked>().attacker;
        if (contains(getCreatures(), attacker) && !contains(getCreatures(), victim) &&
            victim->getBody().isHumanoid())
          surrendering.insert(victim);
      }
      break;
    case EventId::TRAP_TRIGGERED: {
        Position pos = event.get<Position>();
        if (constructions->containsTrap(pos)) {
          constructions->getTrap(pos).reset();
          if (constructions->getTrap(pos).getType() == TrapType::SURPRISE)
            handleSurprise(pos);
        }
      }
      break;
    case EventId::TRAP_DISARMED: {
        Position pos = event.get<EventInfo::TrapDisarmed>().position;
        Creature* who = event.get<EventInfo::TrapDisarmed>().creature;
        if (constructions->containsTrap(pos)) {
          control->addMessage(PlayerMessage(who->getName().a() + " disarms a " 
                + Item::getTrapName(constructions->getTrap(pos).getType()) + " trap.",
                MessagePriority::HIGH).setPosition(pos));
          constructions->getTrap(pos).reset();
        }
      }
      break;
    case EventId::FURNITURE_DESTROYED: {
        auto info = event.get<EventInfo::FurnitureEvent>();
        if (constructions->containsFurniture(info.position, info.layer))
          constructions->onFurnitureDestroyed(info.position, info.layer);
        tileEfficiency->update(info.position);
      }
      break;
    case EventId::EQUIPED:
      minionEquipment->own(event.get<EventInfo::ItemsHandled>().creature,
          getOnlyElement(event.get<EventInfo::ItemsHandled>().items));
      break;
    case EventId::CONQUERED_ENEMY: {
      Collective* col = event.get<Collective*>();
      if (col->getVillainType() == VillainType::MAIN || col->getVillainType() == VillainType::LESSER) {
        control->addMessage(PlayerMessage("The tribe of " + col->getName().getFull() + " is destroyed.",
            MessagePriority::CRITICAL));
        auto mana = config->getManaForConquering(*col->getVillainType());
        addMana(mana);
        control->addMessage(PlayerMessage("You feel a surge of power (+" + toString(mana) + " mana)",
            MessagePriority::HIGH));
      }
    }
    break;
    default:
      break;
  }
}

void Collective::onMinionKilled(Creature* victim, Creature* killer) {
  control->onMemberKilled(victim, killer);
  if (hasTrait(victim, MinionTrait::PRISONER) && killer && contains(getCreatures(), killer))
    returnResource({ResourceId::PRISONER_HEAD, 1});
  if (victim == leader)
    for (Creature* c : getCreatures(MinionTrait::SUMMONED)) // shortcut to get rid of summons when summonner dies
      c->disappear().perform(c);
  if (!hasTrait(victim, MinionTrait::FARM_ANIMAL)) {
    decreaseMoraleForKill(killer, victim);
    if (killer)
      control->addMessage(PlayerMessage(victim->getName().a() + " is killed by " + killer->getName().a(),
            MessagePriority::HIGH).setPosition(victim->getPosition()));
    else
      control->addMessage(PlayerMessage(victim->getName().a() + " is killed.", MessagePriority::HIGH)
          .setPosition(victim->getPosition()));
  }
  bool fighterKilled = hasTrait(victim, MinionTrait::FIGHTER) || victim == getLeader();
  removeCreature(victim);
  if (isConquered() && fighterKilled)
    getGame()->addEvent({EventId::CONQUERED_ENEMY, this});
}

void Collective::onKilledSomeone(Creature* killer, Creature* victim) {
  if (victim->getTribe() != getTribe()) {
    addMana(getKillManaScore(victim));
    addMoraleForKill(killer, victim);
    kills.insert(victim);
    int difficulty = victim->getDifficultyPoints();
    CHECK(difficulty >=0 && difficulty < 100000) << difficulty << " " << victim->getName().bare();
    points += difficulty;
    control->addMessage(PlayerMessage(victim->getName().a() + " is killed by " + killer->getName().a())
        .setPosition(victim->getPosition()));
  }
}

double Collective::getEfficiency(const Creature* c) const {
  return pow(2.0, c->getMorale());
}

const Territory& Collective::getTerritory() const {
  return *territory;
}

bool Collective::canClaimSquare(Position pos) const {
  return getKnownTiles().isKnown(pos) &&
      pos.isCovered() &&
      pos.canConstruct(FurnitureType::BED);
}

void Collective::claimSquare(Position pos) {
  //CHECK(canClaimSquare(pos));
  territory->insert(pos);
  for (auto furniture : pos.getFurniture())
    if (!constructions->containsFurniture(pos, furniture->getLayer()))
      constructions->addFurniture(pos, ConstructionMap::FurnitureInfo::getBuilt(furniture->getType()));
  control->onClaimedSquare(pos);
}

const KnownTiles& Collective::getKnownTiles() const {
  return *knownTiles;
}

const TileEfficiency& Collective::getTileEfficiency() const {
  return *tileEfficiency;
}

MoveInfo Collective::getAlarmMove(Creature* c) {
  if (alarmInfo && alarmInfo->finishTime > getGlobalTime())
    if (auto action = c->moveTowards(alarmInfo->position))
      return {1.0, action};
  return NoMove;
}

double Collective::getLocalTime() const {
  return getLevel()->getModel()->getLocalTime();
}

double Collective::getGlobalTime() const {
  return getGame()->getGlobalTime();
}

int Collective::numResource(ResourceId id) const {
  int ret = credit[id];
  if (auto itemIndex = config->getResourceInfo(id).itemIndex)
    if (auto storageType = config->getResourceInfo(id).storageDestination)
      for (Position pos : storageType(this))
        ret += pos.getItems(*itemIndex).size();
  return ret;
}

int Collective::numResourcePlusDebt(ResourceId id) const {
  return numResource(id) - getDebt(id);
}

int Collective::getDebt(ResourceId id) const {
  int ret = 0;
  for (auto& pos : constructions->getAllFurniture()) {
    auto& furniture = constructions->getFurniture(pos.first, pos.second);
    if (!furniture.isBuilt() && furniture.getCost().id == id)
      ret += furniture.getCost().value;
  }
  for (auto& elem : taskMap->getCompletionCosts())
    if (elem.second.id == id && !elem.first->isDone())
      ret -= elem.second.value;
  ret += workshops->getDebt(id);
  return ret;
}

bool Collective::hasResource(const CostInfo& cost) const {
  return numResource(cost.id) >= cost.value;
}

void Collective::takeResource(const CostInfo& cost) {
  int num = cost.value;
  if (num == 0)
    return;
  CHECK(num > 0);
  if (credit[cost.id]) {
    if (credit[cost.id] >= num) {
      credit[cost.id] -= num;
      return;
    } else {
      num -= credit[cost.id];
      credit[cost.id] = 0;
    }
  }
  if (auto itemIndex = config->getResourceInfo(cost.id).itemIndex)
    if (auto storageType = config->getResourceInfo(cost.id).storageDestination)
      for (Position pos : storageType(this)) {
        vector<Item*> goldHere = pos.getItems(*itemIndex);
        for (Item* it : goldHere) {
          pos.removeItem(it);
          if (--num == 0)
            return;
        }
      }
  FAIL << "Not enough " << config->getResourceInfo(cost.id).name << " missing " << num << " of " << cost.value;
}

void Collective::returnResource(const CostInfo& amount) {
  if (amount.value == 0)
    return;
  CHECK(amount.value > 0);
  if (auto storageType = config->getResourceInfo(amount.id).storageDestination) {
    const set<Position>& destination = storageType(this);
    if (!destination.empty()) {
      Random.choose(destination).dropItems(ItemFactory::fromId(
            config->getResourceInfo(amount.id).itemId, amount.value));
      return;
    }
  }
  credit[amount.id] += amount.value;
}

vector<pair<Item*, Position>> Collective::getTrapItems(TrapType type, const vector<Position>& squares) const {
  vector<pair<Item*, Position>> ret;
  for (Position pos : squares) {
    vector<Item*> v = filter(pos.getItems(ItemIndex::TRAP),
        [type, this](Item* it) { return it->getTrapType() == type && !isItemMarked(it); });
    for (Item* it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}

bool Collective::usesEquipment(const Creature* c) const {
  return config->getManageEquipment()
    && c->getBody().isHumanoid() && !hasTrait(c, MinionTrait::NO_EQUIPMENT)
    && !hasTrait(c, MinionTrait::PRISONER);
}

Item* Collective::getWorstItem(const Creature* c, vector<Item*> items) const {
  Item* ret = nullptr;
  for (Item* it : items)
    if (!minionEquipment->isLocked(c, it->getUniqueId()) &&
        (ret == nullptr || minionEquipment->getItemValue(it) < minionEquipment->getItemValue(ret)))
      ret = it;
  return ret;
}

void Collective::autoEquipment(Creature* creature, bool replace) {
  map<EquipmentSlot, vector<Item*>> slots;
  vector<Item*> myItems = filter(getAllItems(ItemIndex::CAN_EQUIP), [&](const Item* it) {
      return minionEquipment->isOwner(it, creature);});
  for (Item* it : myItems) {
    EquipmentSlot slot = it->getEquipmentSlot();
    if (slots[slot].size() < creature->getEquipment().getMaxItems(slot)) {
      slots[slot].push_back(it);
    } else  // a rare occurence that minion owns too many items of the same slot,
          //should happen only when an item leaves the fortress and then is braught back
      if (!minionEquipment->isLocked(creature, it->getUniqueId()))
        minionEquipment->discard(it);
  }
  vector<Item*> possibleItems = filter(getAllItems(ItemIndex::MINION_EQUIPMENT, false), [&](const Item* it) {
      return minionEquipment->needs(creature, it, false, replace) && !minionEquipment->getOwner(it); });
  sortByEquipmentValue(possibleItems);
  for (Item* it : possibleItems) {
    if (!it->canEquip()) {
      minionEquipment->own(creature, it);
      if (it->getClass() != ItemClass::AMMO)
        break;
      else
        continue;  
    }
    Item* replacedItem = getWorstItem(creature, slots[it->getEquipmentSlot()]);
    int slotSize = creature->getEquipment().getMaxItems(it->getEquipmentSlot());
    int numInSlot = slots[it->getEquipmentSlot()].size();
    if (numInSlot < slotSize ||
        (replacedItem && minionEquipment->getItemValue(replacedItem) < minionEquipment->getItemValue(it))) {
      if (numInSlot == slotSize) {
        minionEquipment->discard(replacedItem);
        removeElement(slots[it->getEquipmentSlot()], replacedItem); 
      }
      minionEquipment->own(creature, it);
      slots[it->getEquipmentSlot()].push_back(it);
      break;
    }
  }
}

vector<Item*> Collective::getAllItems(bool includeMinions) const {
  vector<Item*> allItems;
  for (Position v : territory->getAll())
    append(allItems, v.getItems());
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems());
  return allItems;
}

void Collective::sortByEquipmentValue(vector<Item*>& items) {
  sort(items.begin(), items.end(), [](const Item* it1, const Item* it2) {
      int diff = MinionEquipment::getItemValue(it1) - MinionEquipment::getItemValue(it2);
      if (diff == 0)
        return it1->getUniqueId() < it2->getUniqueId();
      else
        return diff > 0;
    });
}

vector<Item*> Collective::getAllItems(ItemPredicate predicate, bool includeMinions) const {
  vector<Item*> allItems;
  for (Position v : territory->getAll())
    append(allItems, v.getItems(predicate));
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems(predicate));
  return allItems;
}

vector<Item*> Collective::getAllItems(ItemIndex index, bool includeMinions) const {
  vector<Item*> allItems;
  for (Position v : territory->getAll())
    append(allItems, v.getItems(index));
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems(index));
  return allItems;
}

int Collective::getNumItems(ItemIndex index, bool includeMinions) const {
  int ret = 0;
  for (Position v : territory->getAll())
    ret += v.getItems(index).size();
  if (includeMinions)
    for (Creature* c : getCreatures())
      ret += c->getEquipment().getItems(index).size();
  return ret;
}

void Collective::orderExecution(Creature* c) {
  taskMap->addTask(Task::kill(this, c), c->getPosition(), MinionTrait::FIGHTER);
  setTask(c, Task::goToAndWait(c->getPosition(), 100));
}

bool Collective::isItemMarked(const Item* it) const {
  return markedItems.contains(it);
}

void Collective::markItem(const Item* it) {
  markedItems.insert(it);
}

void Collective::unmarkItem(UniqueEntity<Item>::Id id) {
  markedItems.erase(id);
}

void Collective::removeTrap(Position pos) {
  constructions->removeTrap(pos);
}

bool Collective::canAddFurniture(Position position, FurnitureType type) const {
  return knownTiles->isKnown(position)
      && (territory->contains(position) ||
          canClaimSquare(position) ||
          CollectiveConfig::canBuildOutsideTerritory(type))
      && !getConstructions().containsTrap(position)
      && !getConstructions().containsFurniture(position, Furniture::getLayer(type))
      && position.canConstruct(type);
}

void Collective::removeFurniture(Position pos, FurnitureLayer layer) {
  if (constructions->getFurniture(pos, layer).hasTask())
    returnResource(taskMap->removeTask(constructions->getFurniture(pos, layer).getTask()));
  constructions->removeFurniture(pos, layer);
}

void Collective::destroySquare(Position pos, FurnitureLayer layer) {
  if (auto furniture = pos.modFurniture(layer))
    if (furniture->getTribe() == getTribeId()) {
      furniture->destroy(pos, DestroyAction::Type::BASH);
      tileEfficiency->update(pos);
    }
  if (constructions->containsFurniture(pos, layer))
    removeFurniture(pos, layer);
  if (layer != FurnitureLayer::FLOOR) {
    zones->eraseZones(pos);
    if (constructions->containsTorch(pos))
      removeTorch(pos);
    if (constructions->containsTrap(pos))
      removeTrap(pos);
    pos.removeTriggers();
  }
}

void Collective::addFurniture(Position pos, FurnitureType type, const CostInfo& cost, bool noCredit) {
  if (type == FurnitureType::MOUNTAIN && (pos.isChokePoint({MovementTrait::WALK}) ||
        constructions->getTotalCount(type) - constructions->getBuiltCount(type) > 0))
    return;
  if (!noCredit || hasResource(cost)) {
    constructions->addFurniture(pos, ConstructionMap::FurnitureInfo(type, cost));
    updateConstructions();
    if (canClaimSquare(pos))
      claimSquare(pos);
  }
}

const ConstructionMap& Collective::getConstructions() const {
  return *constructions;
}

void Collective::cancelMarkedTask(Position pos) {
  taskMap->removeTask(taskMap->getMarked(pos));
}

bool Collective::isMarked(Position pos) const {
  return taskMap->getMarked(pos);
}

HighlightType Collective::getMarkHighlight(Position pos) const {
  return taskMap->getHighlightType(pos);
}

void Collective::setPriorityTasks(Position pos) {
  taskMap->setPriorityTasks(pos);
}

bool Collective::hasPriorityTasks(Position pos) const {
  return taskMap->hasPriorityTasks(pos);
}

static HighlightType getHighlight(const DestroyAction& action) {
  switch (action.getType()) {
    case DestroyAction::Type::CUT:
      return HighlightType::CUT_TREE;
    default:
      return HighlightType::DIG;
  }
}

void Collective::orderDestruction(Position pos, const DestroyAction& action) {
  auto f = NOTNULL(pos.getFurniture(FurnitureLayer::MIDDLE));
  CHECK(f->canDestroy(action));
  taskMap->markSquare(pos, getHighlight(action), Task::destruction(this, pos, f, action));
}

void Collective::addTrap(Position pos, TrapType type) {
  constructions->addTrap(pos, ConstructionMap::TrapInfo(type));
  updateConstructions();
}

void Collective::onAppliedItem(Position pos, Item* item) {
  CHECK(item->getTrapType());
  if (constructions->containsTrap(pos))
    constructions->getTrap(pos).setArmed();
}

void Collective::onAppliedItemCancel(Position pos) {
  if (constructions->containsTrap(pos))
    constructions->getTrap(pos).reset();
}

void Collective::onTorchBuilt(Position pos, Trigger* t) {
  if (!constructions->containsTorch(pos)) {
    if (canPlaceTorch(pos))
      addTorch(pos);
    else
      return;
  }
  constructions->getTorch(pos).setBuilt(t);
}

bool Collective::isConstructionReachable(Position pos) {
  for (Position v : pos.neighbors8())
    if (knownTiles->isKnown(v))
      return true;
  return false;
}

void Collective::onConstructed(Position pos, FurnitureType type) {
  tileEfficiency->update(pos);
  if (pos.getFurniture(type)->isWall()) {
    constructions->removeFurniture(pos, Furniture::getLayer(type));
    if (territory->contains(pos))
      territory->remove(pos);
    return;
  }
  constructions->onConstructed(pos, type);
  control->onConstructed(pos, type);
  if (Task* task = taskMap->getMarked(pos))
    taskMap->removeTask(task);
}

void Collective::onDestructed(Position pos, FurnitureType type, const DestroyAction& action) {
  tileEfficiency->update(pos);
  switch (action.getType()) {
    case DestroyAction::Type::CUT:
      zones->setZone(pos, ZoneId::FETCH_ITEMS);
      break;
    case DestroyAction::Type::DIG:
      territory->insert(pos);
      for (Position v : pos.neighbors4())
        if (constructions->containsTorch(v) &&
            constructions->getTorch(v).getAttachmentDir() == v.getDir(pos).getCardinalDir())
          removeTorch(v);
      break;
    default:
      break;
  }
  control->onDestructed(pos, action);
}

void Collective::handleTrapPlacementAndProduction() {
  EnumMap<TrapType, vector<pair<Item*, Position>>> trapItems(
      [this] (TrapType type) { return getTrapItems(type, territory->getAll());});
  EnumMap<TrapType, int> missingTraps;
  for (auto elem : constructions->getTraps())
    if (!elem.second.isArmed() && !elem.second.isMarked() && !isDelayed(elem.first)) {
      vector<pair<Item*, Position>>& items = trapItems[elem.second.getType()];
      if (!items.empty()) {
        Position pos = items.back().second;
        taskMap->addTask(Task::applyItem(this, pos, items.back().first, elem.first), pos);
        markItem(items.back().first);
        items.pop_back();
        constructions->getTrap(elem.first).setMarked();
      } else
        ++missingTraps[elem.second.getType()];
    }
  for (TrapType type : ENUM_ALL(TrapType))
    scheduleAutoProduction([type](const Item* it) { return it->getTrapType() == type;}, missingTraps[type]);
}

void Collective::scheduleAutoProduction(function<bool(const Item*)> itemPredicate, int count) {
  if (count > 0)
    for (auto workshopType : ENUM_ALL(WorkshopType))
      for (auto& item : workshops->get(workshopType).getQueued())
        if (itemPredicate(ItemFactory::fromId(item.type).get()))
          count -= item.number * item.batchSize;
  if (count > 0)
    for (auto workshopType : ENUM_ALL(WorkshopType)) {
      auto& options = workshops->get(workshopType).getOptions();
      for (int index : All(options))
        if (itemPredicate(ItemFactory::fromId(options[index].type).get())) {
          workshops->get(workshopType).queue(index, (count + options[index].batchSize - 1) / options[index].batchSize);
          return;
        }
    }
}

void Collective::updateResourceProduction() {
  for (ResourceId resourceId : ENUM_ALL(ResourceId))
    if (auto index = config->getResourceInfo(resourceId).itemIndex) {
      int needed = getDebt(resourceId) - getNumItems(*index);
      if (needed > 0)
        scheduleAutoProduction([resourceId] (const Item* it) { return it->getResourceId() == resourceId; }, needed);
  }
}

void Collective::updateConstructions() {
  handleTrapPlacementAndProduction();
  for (auto& pos : constructions->getAllFurniture()) {
    auto& construction = constructions->getFurniture(pos.first, pos.second);
    if (!isDelayed(pos.first) &&
        !construction.hasTask() &&
        !construction.isBuilt() &&
        hasResource(construction.getCost())) {
      construction.setTask(
          taskMap->addTaskCost(Task::construction(this, pos.first, construction.getFurnitureType()), pos.first,
          construction.getCost())->getUniqueId());
      takeResource(construction.getCost());
    }
  }
  for (auto& elem : constructions->getTorches())
    if (!isDelayed(elem.first) && !elem.second.hasTask() && !elem.second.isBuilt())
      constructions->getTorch(elem.first).setTask(taskMap->addTask(
          Task::buildTorch(this, elem.first, elem.second.getAttachmentDir()), elem.first)->getUniqueId());
}

void Collective::delayDangerousTasks(const vector<Position>& enemyPos1, double delayTime) {
  vector<Vec2> enemyPos = transform2<Vec2>(filter(enemyPos1,
        [=] (const Position& p) { return p.isSameLevel(level); }),
      [] (const Position& p) { return p.getCoord();});
  int infinity = 1000000;
  int radius = 10;
  Table<int> dist(Rectangle::boundingBox(enemyPos)
      .minusMargin(-radius)
      .intersection(getLevel()->getBounds()), infinity);
  queue<Vec2> q;
  for (Vec2 v : enemyPos) {
    dist[v] = 0;
    q.push(v);
  }
  while (!q.empty()) {
    Vec2 pos = q.front();
    q.pop();
    delayedPos[Position(pos, level)] = delayTime;
    if (dist[pos] >= radius)
      continue;
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(dist.getBounds()) && dist[v] == infinity && territory->contains(Position(v, level))) {
        dist[v] = dist[pos] + 1;
        q.push(v);
      }
  }
}

bool Collective::isDelayed(Position pos) {
  return delayedPos.count(pos) && delayedPos.at(pos) > getLocalTime();
}

void Collective::fetchItems(Position pos, const ItemFetchInfo& elem) {
  if (isDelayed(pos) || !pos.canEnterEmpty(MovementTrait::WALK) || elem.destinationFun(this).count(pos))
    return;
  vector<Item*> equipment = filter(pos.getItems(elem.index),
      [this, &elem] (const Item* item) { return elem.predicate(this, item); });
  if (!equipment.empty()) {
    const set<Position>& destination = elem.destinationFun(this);
    if (!destination.empty()) {
      warnings->setWarning(elem.warning, false);
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      taskMap->addTask(Task::bringItem(this, pos, equipment, destination), pos);
      for (Item* it : equipment)
        markItem(it);
    } else
      warnings->setWarning(elem.warning, true);
  }
}

void Collective::handleSurprise(Position pos) {
  Vec2 rad(8, 8);
  bool wasMsg = false;
  Creature* c = pos.getCreature();
  for (Position v : Random.permutation(pos.getRectangle(Rectangle(-rad, rad + Vec2(1, 1)))))
    if (Creature* other = v.getCreature())
      if (hasTrait(other, MinionTrait::FIGHTER) && other->getPosition().dist8(pos) > 1) {
        for (Position dest : pos.neighbors8(Random))
          if (other->getPosition().canMoveCreature(dest)) {
            other->getPosition().moveCreature(dest);
            other->playerMessage("Surprise!");
            if (!wasMsg) {
              c->playerMessage("Surprise!");
              wasMsg = true;
            }
            break;
          }
      }
}

void Collective::onTaskPickedUp(Position pos, EntitySet<Item> items) {
  for (auto id : items)
    unmarkItem(id);
}

void Collective::onCantPickItem(EntitySet<Item> items) {
  for (auto id : items)
    unmarkItem(id);
}

void Collective::limitKnownTilesToModel() {
  knownTiles->limitToModel(getLevel()->getModel());
}

CollectiveWarnings& Collective::getWarnings() {
  return *warnings;
}

const CollectiveConfig& Collective::getConfig() const {
  return *config;
}

bool Collective::addKnownTile(Position pos) {
  if (!knownTiles->isKnown(pos)) {
    pos.setNeedsRenderUpdate(true);
    knownTiles->addTile(pos);
    if (pos.getLevel() == level)
      if (Task* task = taskMap->getMarked(pos))
        if (task->isBogus())
          taskMap->removeTask(task);
    return true;
  } else
    return false;
}

void Collective::addMana(double value) {
  if ((manaRemainder += value) >= 1) {
    returnResource({ResourceId::MANA, int(manaRemainder)});
    manaRemainder -= int(manaRemainder);
  }
}

void Collective::addProducesMessage(const Creature* c, const vector<PItem>& items) {
  if (items.size() > 1)
    control->addMessage(c->getName().a() + " produces " + toString(items.size())
        + " " + items[0]->getName(true));
  else
    control->addMessage(c->getName().a() + " produces " + items[0]->getAName());
}

void Collective::onAppliedSquare(Creature* c, Position pos) {
  if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE)) {
    // Furniture have variable apply time, so just multiply by it to be independent of changes.
    double efficiency = tileEfficiency->getEfficiency(pos) * pos.getApplyTime() * getEfficiency(c);
    switch (furniture->getType()) {
      case FurnitureType::BOOK_SHELF: {
        addMana(0.1 * efficiency * c->getAttributes().getSkills().getValue(SkillId::MANA));
        auto availableSpells = Technology::getAvailableSpells(this);
        if (Random.chance(efficiency / 60) && !availableSpells.empty()) {
          for (int i : Range(30)) {
            Spell* spell = Random.choose(Technology::getAvailableSpells(this));
            if (!c->getAttributes().getSpellMap().contains(spell)) {
              c->getAttributes().getSpellMap().add(spell);
              control->addMessage(c->getName().a() + " learns the spell of " + spell->getName());
              break;
            }
          }
        }
        break;
      }
      case FurnitureType::THRONE:
        if (c == getLeader())
          addMana(0.2 * efficiency * c->getAttributes().getSkills().getValue(SkillId::MANA));
        break;
      case FurnitureType::WHIPPING_POST:
        taskMap->addTask(Task::whipping(pos, c), pos, MinionTrait::FIGHTER);
        break;
      case FurnitureType::TORTURE_TABLE:
        taskMap->addTask(Task::torture(this, c), pos, MinionTrait::FIGHTER);
        break;
      default:
        break;
    }
    if (auto usage = furniture->getUsageType())
      switch (*usage) {
        case FurnitureUsageType::TRAIN:
          c->increaseExpLevel(ExperienceType::TRAINING, 0.005 * efficiency);
          break;
        default:
          break;
      }
    if (auto workshopType = config->getWorkshopType(furniture->getType())) {
      auto& info = config->getWorkshopInfo(*workshopType);
      vector<PItem> items =
          workshops->get(*workshopType).addWork(efficiency * c->getAttributes().getSkills().getValue(info.skill));
      if (!items.empty()) {
        if (items[0]->getClass() == ItemClass::WEAPON)
          getGame()->getStatistics().add(StatId::WEAPON_PRODUCED);
        if (items[0]->getClass() == ItemClass::ARMOR)
          getGame()->getStatistics().add(StatId::ARMOR_PRODUCED);
        if (items[0]->getClass() == ItemClass::POTION)
          getGame()->getStatistics().add(StatId::POTION_PRODUCED);
        addProducesMessage(c, items);
        c->getPosition().dropItems(std::move(items));
      }
    }
  }
}

optional<FurnitureType> Collective::getMissingTrainingDummy(const Creature* c) const {
  if (c->getAttributes().getMinionTasks().getValue(MinionTask::TRAIN) == 0)
    return none;
  optional<FurnitureType> requiredDummy;
  for (auto dummyType : MinionTasks::getAllFurniture(MinionTask::TRAIN)) {
    bool canTrain = *config->getTrainingMaxLevelIncrease(dummyType) >
        c->getAttributes().getExpIncrease(ExperienceType::TRAINING);
    bool hasDummy = getConstructions().getBuiltCount(dummyType) > 0;
    if (canTrain && hasDummy)
      return none;
    if (!requiredDummy && canTrain && !hasDummy)
      requiredDummy = dummyType;
  }
  return requiredDummy;
}

double Collective::getDangerLevel() const {
  double ret = 0;
  for (const Creature* c : getCreatures(MinionTrait::FIGHTER))
    ret += c->getDifficultyPoints();
  ret += constructions->getBuiltCount(FurnitureType::IMPALED_HEAD) * 150;
  return ret;
}

bool Collective::hasTech(TechId id) const {
  return contains(technologies, id);
}

double Collective::getTechCost(Technology* t) {
  return t->getCost();
}

void Collective::acquireTech(Technology* tech) {
  technologies.push_back(tech->getId());
  Technology::onAcquired(tech->getId(), this);
}

vector<Technology*> Collective::getTechnologies() const {
  return transform2<Technology*>(technologies, [] (const TechId t) { return Technology::get(t); });
}

const EntitySet<Creature>& Collective::getKills() const {
  return kills;
}

int Collective::getPoints() const {
  return points;
}

void Collective::onRansomPaid() {
  control->onRansomPaid();
}

void Collective::ownItems(const Creature* who, const vector<Item*> items) {
  for (const Item* it : items)
    if (minionEquipment->isItemUseful(it))
      minionEquipment->own(who, it);
}

void Collective::onCopulated(Creature* who, Creature* with) {
  if (with->getName().bare() == "vampire")
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a()
        + " with a monkey on " + who->getAttributes().getGender().his() + " knee");
  else
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a());
  if (contains(getCreatures(), with))
    with->addMorale(1);
  if (!pregnancies.contains(who)) {
    pregnancies.insert(who);
    who->addEffect(LastingEffect::PREGNANT, Random.get(200, 300));
  }
}

MinionEquipment& Collective::getMinionEquipment() {
  return *minionEquipment;
}

const MinionEquipment& Collective::getMinionEquipment() const {
  return *minionEquipment;
}

Workshops& Collective::getWorkshops() {
  return *workshops;
}

const Workshops& Collective::getWorkshops() const {
  return *workshops;
}

void Collective::addAttack(const CollectiveAttack& attack) {
  control->addAttack(attack);
}

CollectiveTeams& Collective::getTeams() {
  return *teams;
}

const CollectiveTeams& Collective::getTeams() const {
  return *teams;
}

void Collective::freeTeamMembers(TeamId id) {
  for (Creature* c : teams->getMembers(id)) {
    if (c->isAffected(LastingEffect::SLEEP))
      c->removeEffect(LastingEffect::SLEEP);
  }
}

static optional<Vec2> getAdjacentWall(Position pos) {
  for (Position p : pos.neighbors4(Random))
    if (p.isWall())
      return pos.getDir(p);
  return none;
}

bool Collective::isPlannedTorch(Position pos) const {
  return constructions->containsTorch(pos) && !constructions->getTorch(pos).isBuilt();
}

void Collective::removeTorch(Position pos) {
  if (constructions->getTorch(pos).hasTask())
    taskMap->removeTask(constructions->getTorch(pos).getTask());
  if (auto trigger = constructions->getTorch(pos).getTrigger())
    pos.removeTrigger(trigger);
  constructions->removeTorch(pos);
}

void Collective::addTorch(Position pos) {
  CHECK(canPlaceTorch(pos));
  constructions->addTorch(pos, ConstructionMap::TorchInfo(getAdjacentWall(pos)->getCardinalDir()));
}

Zones& Collective::getZones() {
  return *zones;
}

const Zones& Collective::getZones() const {
  return *zones;
}

bool Collective::canPlaceTorch(Position pos) const {
  return getAdjacentWall(pos) && !constructions->containsTorch(pos) &&
    knownTiles->isKnown(pos) && pos.canEnterEmpty({MovementTrait::WALK});
}

const TaskMap& Collective::getTaskMap() const {
  return *taskMap;
}

int Collective::getPopulationSize() const {
  return getCreatures().size() - getCreatures(MinionTrait::NO_LIMIT).size();
}

int Collective::getMaxPopulation() const {
  int ret = config->getMaxPopulation();
  for (auto& elem : config->getPopulationIncreases()) {
    int sz = getConstructions().getBuiltPositions(elem.type).size();
    ret += min<int>(elem.maxIncrease, elem.increasePerSquare * sz);
  }
  return ret;
}

template <class Archive>
void Collective::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, LeaderControlOverride);
  REGISTER_TYPE(ar, Collective);
}

REGISTER_TYPES(Collective::registerTypes);
