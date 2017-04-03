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
#include "immigration.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(TaskCallback) & SUBCLASS(EventListener);
  serializeAll(ar, creatures, leader, taskMap, tribe, control, byTrait, bySpawnType);
  serializeAll(ar, territory, alarmInfo, markedItems, constructions, minionEquipment);
  serializeAll(ar, surrendering, delayedPos, knownTiles, technologies, kills, points, currentTasks);
  serializeAll(ar, credit, level, immigration, teams, name);
  serializeAll(ar, config, warnings, knownVillains, knownVillainLocations, banished);
  serializeAll(ar, villainType, enemyId, workshops, zones, tileEfficiency);
}

SERIALIZABLE(Collective);

SERIALIZATION_CONSTRUCTOR_IMPL(Collective);

Collective::Collective(Private, WLevel l, TribeId t, const CollectiveName& n) : tribe(t), level(NOTNULL(l)), name(n) {
}

PCollective Collective::create(WLevel level, TribeId tribe, const CollectiveName& name) {
  auto ret = makeOwner<Collective>(Private {}, level, tribe, name);
  ret->subscribeTo(level->getModel());
  return ret;
}

void Collective::init(CollectiveConfig&& cfg, Immigration&& im) {
  config.reset(std::move(cfg));
  immigration.reset(std::move(im));
  credit = cfg.getStartingResource();
  workshops = config->getWorkshops();
}

const CollectiveName& Collective::getName() const {
  return *name;
}

void Collective::setVillainType(VillainType t) {
  villainType = t;
}

void Collective::setEnemyId(EnemyId id) {
  enemyId = id;
}

optional<VillainType> Collective::getVillainType() const {
  return villainType;
}

optional<EnemyId> Collective::getEnemyId() const {
  return enemyId;
}

Collective::~Collective() {
}

namespace {

class LeaderControlOverride : public Creature::MoraleOverride {
  public:
  LeaderControlOverride(WCollective col) : collective(col) {}

  virtual optional<double> getMorale(WConstCreature creature) override {
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
  WCollective SERIAL(collective);
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
    creature->getEquipment().removeAllItems(creature.get());
  WCreature c = creature.get();
  pos.addCreature(std::move(creature));
  addCreature(c, traits);
}

void Collective::addCreature(WCreature c, EnumSet<MinionTrait> traits) {
  if (!traits.contains(MinionTrait::FARM_ANIMAL) && !c->getController()->isCustomController())
    c->setController(makeOwner<Monster>(c, MonsterAIFactory::collective(this)));
  if (traits.contains(MinionTrait::WORKER))
    c->getAttributes().getMinionTasks().clear();
  if (!leader)
    leader = c;
  if (c->getTribeId() != *tribe)
    c->setTribe(*tribe);
  if (WGame game = getGame())
    for (WCollective col : getGame()->getCollectives())
      if (contains(col->getCreatures(), c))
        col->removeCreature(c);
  creatures.push_back(c);
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
  if (auto spawnType = c->getAttributes().getSpawnType())
    bySpawnType[*spawnType].push_back(c);
  for (WItem item : c->getEquipment().getItems())
    minionEquipment->own(c, item);
  if (traits.contains(MinionTrait::FIGHTER)) {
    c->setMoraleOverride(Creature::PMoraleOverride(new LeaderControlOverride(this)));
  }
  control->onMemberAdded(c);
}

void Collective::removeCreature(WCreature c) {
  removeElement(creatures, c);
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

void Collective::banishCreature(WCreature c) {
  decreaseMoraleForBanishing(c);
  removeCreature(c);
  vector<Position> exitTiles = territory->getExtended(10, 20);
  vector<PTask> tasks;
  vector<WItem> items = c->getEquipment().getItems();
  if (!items.empty())
    tasks.push_back(Task::dropItems(items));
  if (!exitTiles.empty())
    tasks.push_back(Task::goToTryForever(Random.choose(exitTiles)));
  tasks.push_back(Task::disappear());
  c->setController(makeOwner<Monster>(c, MonsterAIFactory::singleTask(Task::chain(std::move(tasks)))));
  banished.insert(c);
}

bool Collective::wasBanished(WConstCreature c) const {
  return banished.contains(c);
}

/*vector<WCreature> Collective::getRecruits() const {
  vector<WCreature> ret;
  vector<WCreature> possibleRecruits = filter(getCreatures(MinionTrait::FIGHTER),
      [] (WConstCreature c) { return c->getAttributes().getRecruitmentCost() > 0; });
  if (auto minPop = config->getRecruitingMinPopulation())
    for (int i = *minPop; i < possibleRecruits.size(); ++i)
      ret.push_back(possibleRecruits[i]);
  return ret;
}*/

bool Collective::hasTradeItems() const {
  for (Position pos : territory->getAll())
    if (!pos.getItems(ItemIndex::FOR_SALE).empty())
      return true;
  return false;
}

//kocham Cię

vector<WItem> Collective::getTradeItems() const {
  vector<WItem> ret;
  for (Position pos : territory->getAll())
    append(ret, pos.getItems(ItemIndex::FOR_SALE));
  return ret;
}

PItem Collective::buyItem(WItem item) {
  for (Position pos : territory->getAll())
    for (WItem it : pos.getItems(ItemIndex::FOR_SALE))
      if (it == item) {
        PItem ret = pos.removeItem(it);
        ret->setShopkeeper(nullptr);
        return ret;
      }
  FATAL << "Couldn't find item";
  return nullptr;
}

vector<TriggerInfo> Collective::getTriggers(WConstCollective against) const {
  return control->getTriggers(against);
}

WConstCreature Collective::getLeader() const {
  return leader;
}

WCreature Collective::getLeader() {
  return leader;
}

bool Collective::hasLeader() const {
  return leader && !leader->isDead();
}

WLevel Collective::getLevel() const {
  return level;
}

WGame Collective::getGame() const {
  return level->getModel()->getGame();
}

WCollectiveControl Collective::getControl() const {
  return control.get();
}

TribeId Collective::getTribeId() const {
  return *tribe;
}

Tribe* Collective::getTribe() const {
  return getGame()->getTribe(*tribe);
}

WModel Collective::getModel() const {
  return getLevel()->getModel();
}

const vector<WCreature>& Collective::getCreatures() const {
  return creatures;
}

MoveInfo Collective::getDropItems(WCreature c) {
  if (territory->contains(c->getPosition())) {
    vector<WItem> items = c->getEquipment().getItems([this, c](const WItem item) {
        return minionEquipment->isItemUseful(item) && !minionEquipment->isOwner(item, c); });
    if (!items.empty())
      return c->drop(items);
  }
  return NoMove;
}

MoveInfo Collective::getWorkerMove(WCreature c) {
  if (WTask task = taskMap->getTask(c))
    return task->getMove(c);
  if (WTask closest = taskMap->getClosestTask(c, MinionTrait::WORKER)) {
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

void Collective::setMinionTask(WConstCreature c, MinionTask task) {
  if (auto duration = MinionTasks::getDuration(c, task))
    currentTasks.set(c, {task, c->getLocalTime() + *duration});
  else
    currentTasks.set(c, {task, none});
}

optional<MinionTask> Collective::getMinionTask(WConstCreature c) const {
  if (auto current = currentTasks.getMaybe(c))
    return current->task;
  else
    return none;
}

bool Collective::isTaskGood(WConstCreature c, MinionTask task, bool ignoreTaskLock) const {
  if (c->getAttributes().getMinionTasks().getValue(task, ignoreTaskLock) == 0)
    return false;
  switch (task) {
    case MinionTask::CROPS:
    case MinionTask::EXPLORE:
        return getGame()->getSunlightInfo().getState() == SunlightState::DAY;
    case MinionTask::SLEEP:
        if (!config->sleepOnlyAtNight())
          return true;
      FALLTHROUGH;
    case MinionTask::EXPLORE_NOCTURNAL:
        return getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
    default: return true;
  }
}

void Collective::setRandomTask(WConstCreature c) {
  vector<pair<MinionTask, double>> goodTasks;
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (isTaskGood(c, t))
      goodTasks.push_back({t, c->getAttributes().getMinionTasks().getValue(t)});
  if (!goodTasks.empty())
    setMinionTask(c, Random.choose(goodTasks));
}

bool Collective::isMinionTaskPossible(WCreature c, MinionTask task) {
  return isTaskGood(c, task, true) && MinionTasks::generate(this, c, task);
}

PTask Collective::getStandardTask(WCreature c) {
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

vector<WCreature> Collective::getConsumptionTargets(WCreature consumer) const {
  vector<WCreature> ret;
  for (WCreature c : getCreatures(MinionTrait::FIGHTER))
    if (consumer->canConsume(c) && c != getLeader())
      ret.push_back(c);
  return ret;
}

void Collective::orderConsumption(WCreature consumer, WCreature who) {
  CHECK(contains(getConsumptionTargets(consumer), who));
  setTask(consumer, Task::consume(this, who));
}

PTask Collective::getEquipmentTask(WCreature c) {
  if (Random.roll(40))
    minionEquipment->autoAssign(c, getAllItems(ItemIndex::MINION_EQUIPMENT, false));
  vector<PTask> tasks;
  for (WItem it : c->getEquipment().getItems())
    if (!c->getEquipment().isEquipped(it) && c->getEquipment().canEquip(it))
      tasks.push_back(Task::equipItem(it));
  for (Position v : zones->getPositions(ZoneId::STORAGE_EQUIPMENT)) {
    vector<WItem> allItems = filter(v.getItems(ItemIndex::MINION_EQUIPMENT),
        [this, c] (const WItem it) { return minionEquipment->isOwner(it, c);});
    vector<WItem> consumables;
    for (auto item : allItems)
      if (item->canEquip())
        tasks.push_back(Task::pickAndEquipItem(this, v, item));
      else
        consumables.push_back(item);
    if (!consumables.empty())
      tasks.push_back(Task::pickItem(this, v, consumables));
  }
  if (!tasks.empty())
    return Task::chain(std::move(tasks));
  return nullptr;
}

void Collective::considerHealingTask(WCreature c) {
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

MoveInfo Collective::getTeamMemberMove(WCreature c) {
  for (auto team : teams->getContaining(c))
    if (teams->isActive(team)) {
      WConstCreature leader = teams->getLeader(team);
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

void Collective::setTask(WCreature c, PTask task) {
  returnResource(taskMap->freeFromTask(c));
  taskMap->addTaskFor(std::move(task), c);
}

bool Collective::hasTask(WConstCreature c) const {
  return taskMap->hasTask(c);
}

void Collective::cancelTask(WConstCreature c) {
  if (WTask task = taskMap->getTask(c))
    taskMap->removeTask(task);
}

MoveInfo Collective::getMove(WCreature c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  CHECK(!c->isDead());
/*  CHECK(contains(c->getPosition().getModel()->getLevels(), c->getPosition().getLevel())) <<
      c->getPosition().getLevel()->getName() << " " << c->getName().bare();*/
  if (WTask task = taskMap->getTask(c))
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
  if (WTask task = taskMap->getTask(c))
    return task->getMove(c);
  if (WTask closest = taskMap->getClosestTask(c, MinionTrait::FIGHTER)) {
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
    if (c->getPosition().getModel() == getModel())
      return c->moveTowards(Random.choose(territory->getAll()));
    else
      if (PTask t = Task::transferTo(getModel()))
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
    if (WConstCreature c = pos.getCreature())
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

void Collective::addNewCreatureMessage(const vector<WCreature>& immigrants) {
  if (immigrants.size() == 1)
    control->addMessage(PlayerMessage(immigrants[0]->getName().a() + " joins your forces.")
        .setCreature(immigrants[0]->getUniqueId()));
  else {
    control->addMessage(PlayerMessage("A " + immigrants[0]->getName().multiple(immigrants.size()) +
          " joins your forces.").setCreature(immigrants[0]->getUniqueId()));
  }
}

void Collective::decayMorale() {
  for (WCreature c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(-c->getMorale() * 0.0008);
}

void Collective::update(bool currentlyActive) {
  control->update(currentlyActive);
  if (config->hasImmigrantion(currentlyActive) && hasLeader())
    immigration->update();
}

void Collective::tick() {
  dangerLevelCache = none;
  control->tick();
  zones->tick();
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
    vector<WCreature> surrenderingVec;
    for (Position v : enemyPos)
      if (!surrendering.contains(NOTNULL(v.getCreature()))) {
        allSurrender = false;
        break;
      } else
        surrenderingVec.push_back(v.getCreature());
    if (allSurrender) {
      for (WCreature c : surrenderingVec) {
        if (!c->isDead() && territory->contains(c->getPosition())) {
          Position pos = c->getPosition();
          PCreature prisoner = CreatureFactory::fromId(CreatureId::PRISONER, getTribeId(),
              MonsterAIFactory::collective(this));
          if (pos.canEnterEmpty(prisoner.get())) {
            pos.globalMessage(c->getName().the() + " surrenders.");
            control->addMessage(PlayerMessage(c->getName().a() + " surrenders.").setPosition(c->getPosition()));
            c->die(true, false);
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
  if (config->getManageEquipment() && Random.roll(40)) {
    minionEquipment->updateOwners(getCreatures());
    minionEquipment->updateItems(getAllItems(ItemIndex::MINION_EQUIPMENT, true));
  }
  workshops->scheduleItems(this);
}

const vector<WCreature>& Collective::getCreatures(MinionTrait trait) const {
  return byTrait[trait];
}

const vector<WCreature>& Collective::getCreatures(SpawnType type) const {
  return bySpawnType[type];
}

bool Collective::hasTrait(WConstCreature c, MinionTrait t) const {
  return contains(byTrait[t], c);
}

bool Collective::hasAnyTrait(WConstCreature c, EnumSet<MinionTrait> traits) const {
  for (MinionTrait t : traits)
    if (hasTrait(c, t))
      return true;
  return false;
}

void Collective::setTrait(WCreature c, MinionTrait t) {
  if (!hasTrait(c, t))
    byTrait[t].push_back(c);
}

vector<WCreature> Collective::getCreaturesAnyOf(EnumSet<MinionTrait> trait) const {
  set<WCreature> ret;
  for (MinionTrait t : trait)
    for (WCreature c : byTrait[t])
      ret.insert(c);
  return vector<WCreature>(ret.begin(), ret.end());
}

vector<WCreature> Collective::getCreatures(EnumSet<MinionTrait> with, EnumSet<MinionTrait> without) const {
  vector<WCreature> ret;
  for (WCreature c : creatures) {
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

double Collective::getKillManaScore(WConstCreature victim) const {
  return 0;
/*  int ret = victim->getDifficultyPoints() / 3;
  if (victim->isAffected(LastingEffect::SLEEP))
    ret *= 2;
  return ret;*/
}

void Collective::addMoraleForKill(WConstCreature killer, WConstCreature victim) {
  for (WCreature c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(c == killer ? 0.25 : 0.015);
}

void Collective::decreaseMoraleForKill(WConstCreature killer, WConstCreature victim) {
  for (WCreature c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(victim == getLeader() ? -2 : -0.015);
}

void Collective::decreaseMoraleForBanishing(WConstCreature) {
  for (WCreature c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(-0.05);
}

void Collective::onKillCancelled(WCreature c) {
}

void Collective::onEvent(const GameEvent& event) {
  switch (event.getId()) {
    case EventId::ALARM: {
        Position pos = event.get<Position>();
        static const int alarmTime = 100;
        if (getTerritory().contains(pos)) {
          control->addMessage(PlayerMessage("An alarm goes off.", MessagePriority::HIGH).setPosition(pos));
          alarmInfo = {getGlobalTime() + alarmTime, pos };
          for (WCreature c : byTrait[MinionTrait::FIGHTER])
            if (c->isAffected(LastingEffect::SLEEP))
              c->removeEffect(LastingEffect::SLEEP);
        }
      }
      break;
    case EventId::KILLED: {
        WCreature victim = event.get<EventInfo::Attacked>().victim;
        WCreature killer = event.get<EventInfo::Attacked>().attacker;
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
        WCreature victim = event.get<EventInfo::Attacked>().victim;
        WCreature attacker = event.get<EventInfo::Attacked>().attacker;
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
        WCreature who = event.get<EventInfo::TrapDisarmed>().creature;
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
    /*case EventId::EQUIPED:
      minionEquipment->own(event.get<EventInfo::ItemsHandled>().creature,
          getOnlyElement(event.get<EventInfo::ItemsHandled>().items));
      break;*/
    case EventId::CONQUERED_ENEMY: {
      WCollective col = event.get<WCollective>();
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

void Collective::onMinionKilled(WCreature victim, WCreature killer) {
  control->onMemberKilled(victim, killer);
  if (hasTrait(victim, MinionTrait::PRISONER) && killer && contains(getCreatures(), killer))
    returnResource({ResourceId::PRISONER_HEAD, 1});
  if (victim == leader)
    for (WCreature c : getCreatures(MinionTrait::SUMMONED)) // shortcut to get rid of summons when summonner dies
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

void Collective::onKilledSomeone(WCreature killer, WCreature victim) {
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

double Collective::getEfficiency(WConstCreature c) const {
  return pow(2.0, c->getMorale());
}

const Territory& Collective::getTerritory() const {
  return *territory;
}

Territory& Collective::getTerritory() {
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
  for (auto furniture : pos.modFurniture())
    if (!furniture->isWall()) {
      if (!constructions->containsFurniture(pos, furniture->getLayer()))
        constructions->addFurniture(pos, ConstructionMap::FurnitureInfo::getBuilt(furniture->getType()));
      furniture->setTribe(getTribeId());
    }
  control->onClaimedSquare(pos);
}

const KnownTiles& Collective::getKnownTiles() const {
  return *knownTiles;
}

const TileEfficiency& Collective::getTileEfficiency() const {
  return *tileEfficiency;
}

MoveInfo Collective::getAlarmMove(WCreature c) {
  if (alarmInfo && alarmInfo->finishTime > getGlobalTime())
    if (auto action = c->moveTowards(alarmInfo->position))
      return {1.0, action};
  return NoMove;
}

double Collective::getLocalTime() const {
  return getModel()->getLocalTime();
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
  int ret = constructions->getDebt(id);
  for (auto& elem : taskMap->getCompletionCosts())
    if (elem.second.id == id && !taskMap->getTask(elem.first)->isDone())
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
        vector<WItem> goldHere = pos.getItems(*itemIndex);
        for (WItem it : goldHere) {
          pos.removeItem(it);
          if (--num == 0)
            return;
        }
      }
  FATAL << "Not enough " << config->getResourceInfo(cost.id).name << " missing " << num << " of " << cost.value;
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

vector<pair<WItem, Position>> Collective::getTrapItems(TrapType type, const vector<Position>& squares) const {
  vector<pair<WItem, Position>> ret;
  for (Position pos : squares) {
    vector<WItem> v = filter(pos.getItems(ItemIndex::TRAP),
        [type, this](WItem it) { return it->getTrapType() == type && !isItemMarked(it); });
    for (WItem it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}

bool Collective::usesEquipment(WConstCreature c) const {
  return config->getManageEquipment()
    && c->getBody().isHumanoid() && !hasTrait(c, MinionTrait::NO_EQUIPMENT)
    && !hasTrait(c, MinionTrait::PRISONER);
}

vector<WItem> Collective::getAllItems(bool includeMinions) const {
  vector<WItem> allItems;
  for (Position v : territory->getAll())
    append(allItems, v.getItems());
  if (includeMinions)
    for (WCreature c : getCreatures())
      append(allItems, c->getEquipment().getItems());
  return allItems;
}

vector<WItem> Collective::getAllItems(ItemPredicate predicate, bool includeMinions) const {
  vector<WItem> allItems;
  for (Position v : territory->getAll())
    append(allItems, v.getItems(predicate));
  if (includeMinions)
    for (WCreature c : getCreatures())
      append(allItems, c->getEquipment().getItems(predicate));
  return allItems;
}

vector<WItem> Collective::getAllItems(ItemIndex index, bool includeMinions) const {
  vector<WItem> allItems;
  for (Position v : territory->getAll())
    append(allItems, v.getItems(index));
  if (includeMinions)
    for (WCreature c : getCreatures())
      append(allItems, c->getEquipment().getItems(index));
  return allItems;
}

bool Collective::canPillage() const {
  for (auto pos : territory->getAll())
    if (!pos.getItems().empty())
      return true;
  return false;
}

int Collective::getNumItems(ItemIndex index, bool includeMinions) const {
  int ret = 0;
  for (Position v : territory->getAll())
    ret += v.getItems(index).size();
  if (includeMinions)
    for (WCreature c : getCreatures())
      ret += c->getEquipment().getItems(index).size();
  return ret;
}

optional<set<Position>> Collective::getStorageFor(const WItem item) const {
  for (auto& info : config->getFetchInfo())
    if (Inventory::getIndexPredicate(info.index)(item))
      return info.destinationFun(this);
  return none;
}

void Collective::addKnownVillain(WConstCollective col) {
  knownVillains.insert(col);
}

bool Collective::isKnownVillain(WConstCollective col) const {
  return getModel() != col->getModel() || knownVillains.contains(col);
}

void Collective::addKnownVillainLocation(WConstCollective col) {
  knownVillainLocations.insert(col);
}

bool Collective::isKnownVillainLocation(WConstCollective col) const {
  return knownVillainLocations.contains(col);
}

void Collective::orderExecution(WCreature c) {
  taskMap->addTask(Task::kill(this, c), c->getPosition(), MinionTrait::FIGHTER);
  setTask(c, Task::goToAndWait(c->getPosition(), 100));
}

bool Collective::isItemMarked(const WItem it) const {
  return markedItems.contains(it);
}

void Collective::markItem(const WItem it) {
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

void Collective::onAppliedItem(Position pos, WItem item) {
  CHECK(item->getTrapType());
  if (constructions->containsTrap(pos))
    constructions->getTrap(pos).setArmed();
}

void Collective::onAppliedItemCancel(Position pos) {
  if (constructions->containsTrap(pos))
    constructions->getTrap(pos).reset();
}

void Collective::onTorchBuilt(Position pos, WTrigger t) {
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
  if (WTask task = taskMap->getMarked(pos))
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
  EnumMap<TrapType, vector<pair<WItem, Position>>> trapItems(
      [this] (TrapType type) { return getTrapItems(type, territory->getAll());});
  EnumMap<TrapType, int> missingTraps;
  for (auto elem : constructions->getTraps())
    if (!elem.second.isArmed() && !elem.second.isMarked() && !isDelayed(elem.first)) {
      vector<pair<WItem, Position>>& items = trapItems[elem.second.getType()];
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
    scheduleAutoProduction([type](const WItem it) { return it->getTrapType() == type;}, missingTraps[type]);
}

void Collective::scheduleAutoProduction(function<bool(const WItem)> itemPredicate, int count) {
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
        scheduleAutoProduction([resourceId] (const WItem it) { return it->getResourceId() == resourceId; }, needed);
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
      constructions->setTask(pos.first, pos.second,
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
  vector<Vec2> enemyPos = transform2(filter(enemyPos1,
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
  vector<WItem> equipment = filter(pos.getItems(elem.index),
      [this, &elem] (const WItem item) { return elem.predicate(this, item); });
  if (!equipment.empty()) {
    const set<Position>& destination = elem.destinationFun(this);
    if (!destination.empty()) {
      warnings->setWarning(elem.warning, false);
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      taskMap->addTask(Task::bringItem(this, pos, equipment, destination), pos);
      for (WItem it : equipment)
        markItem(it);
    } else
      warnings->setWarning(elem.warning, true);
  }
}

void Collective::handleSurprise(Position pos) {
  Vec2 rad(8, 8);
  bool wasMsg = false;
  WCreature c = pos.getCreature();
  for (Position v : Random.permutation(pos.getRectangle(Rectangle(-rad, rad + Vec2(1, 1)))))
    if (WCreature other = v.getCreature())
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

void Collective::retire() {
  knownTiles->limitToModel(getModel());
  knownVillainLocations.clear();
  knownVillains.clear();
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
      if (WTask task = taskMap->getMarked(pos))
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

void Collective::addProducesMessage(WConstCreature c, const vector<PItem>& items) {
  if (items.size() > 1)
    control->addMessage(c->getName().a() + " produces " + toString(items.size())
        + " " + items[0]->getName(true));
  else
    control->addMessage(c->getName().a() + " produces " + items[0]->getAName());
}

void Collective::onAppliedSquare(WCreature c, Position pos) {
  if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE)) {
    // Furniture have variable apply time, so just multiply by it to be independent of changes.
    double efficiency = tileEfficiency->getEfficiency(pos) * furniture->getUsageTime() * getEfficiency(c);
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

optional<FurnitureType> Collective::getMissingTrainingDummy(WConstCreature c) const {
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
  if (!dangerLevelCache) {
    double ret = 0;
    for (WConstCreature c : getCreatures(MinionTrait::FIGHTER))
      ret += c->getDifficultyPoints();
    ret += constructions->getBuiltCount(FurnitureType::IMPALED_HEAD) * 150;
    dangerLevelCache = ret;
  }
  return *dangerLevelCache;
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
  return transform2(technologies, [] (const TechId t) { return Technology::get(t); });
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

void Collective::onCopulated(WCreature who, WCreature with) {
  if (with->getName().bare() == "vampire")
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a()
        + " with a monkey on " + who->getAttributes().getGender().his() + " knee");
  else
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a());
  if (contains(getCreatures(), with))
    with->addMorale(1);
  if (!who->isAffected(LastingEffect::PREGNANT) && Random.roll(2)) {
    who->addEffect(LastingEffect::PREGNANT, getConfig().getImmigrantTimeout());
    control->addMessage(who->getName().a() + " becomes pregnant.");
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

Immigration& Collective::getImmigration() {
  return *immigration;
}

const Immigration& Collective::getImmigration() const {
  return *immigration;
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
  for (WCreature c : teams->getMembers(id)) {
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
