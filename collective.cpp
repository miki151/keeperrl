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
#include "trap_type.h"
#include "creature_factory.h"
#include "resource_info.h"
#include "workshop_item.h"
#include "quarters.h"


template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar(SUBCLASS(TaskCallback), SUBCLASS(UniqueEntity<Collective>), SUBCLASS(EventListener));
  ar(creatures, leader, taskMap, tribe, control, byTrait);
  ar(territory, alarmInfo, markedItems, constructions, minionEquipment);
  ar(surrendering, delayedPos, knownTiles, technologies, kills, points, currentTasks);
  ar(credit, level, immigration, teams, name, conqueredVillains);
  ar(config, warnings, knownVillains, knownVillainLocations, banished);
  ar(villainType, enemyId, workshops, zones, tileEfficiency, discoverable, numPrisonerOrders, quarters);
  // hack to make retired villains discoverable, remove with save version change
  if (villainType == VillainType::MAIN)
    discoverable = true;
}

SERIALIZABLE(Collective)

SERIALIZATION_CONSTRUCTOR_IMPL(Collective)

Collective::Collective(Private, WLevel l, TribeId t, const optional<CollectiveName>& n)
    : tribe(t), level(NOTNULL(l)), name(n), villainType(VillainType::NONE) {
}

PCollective Collective::create(WLevel level, TribeId tribe, const optional<CollectiveName>& name, bool discoverable) {
  auto ret = makeOwner<Collective>(Private {}, level, tribe, name);
  ret->subscribeTo(level->getModel());
  if (discoverable)
    ret->setDiscoverable();
  return ret;
}

void Collective::init(CollectiveConfig&& cfg, Immigration&& im) {
  config.reset(std::move(cfg));
  immigration = makeOwner<Immigration>(std::move(im));
  credit = cfg.getStartingResource();
  workshops = config->getWorkshops();
}

void Collective::acquireInitialTech() {
  for (auto tech : config->getInitialTech())
    acquireTech(tech);
}

const optional<CollectiveName>& Collective::getName() const {
  return *name;
}

void Collective::setVillainType(VillainType t) {
  villainType = t;
}

bool Collective::isDiscoverable() const {
  return discoverable;
}

void Collective::setDiscoverable() {
  discoverable = true;
}

void Collective::setEnemyId(EnemyId id) {
  enemyId = id;
}

VillainType Collective::getVillainType() const {
  return villainType;
}

optional<EnemyId> Collective::getEnemyId() const {
  return enemyId;
}

Collective::~Collective() {
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

void Collective::updateCreatureStatus(WCreature c) {
  c->getStatus().set(CreatureStatus::CIVILIAN,
      c->getBody().isHumanoid() &&
      !hasTrait(c, MinionTrait::FIGHTER) &&
      !hasTrait(c, MinionTrait::LEADER));
  c->getStatus().set(CreatureStatus::FIGHTER, hasTrait(c, MinionTrait::FIGHTER));
  c->getStatus().set(CreatureStatus::LEADER, hasTrait(c, MinionTrait::LEADER));
}

void Collective::addCreature(WCreature c, EnumSet<MinionTrait> traits) {
  if (c->isAffected(LastingEffect::SUMMONED)) {
    traits.insert(MinionTrait::NO_LIMIT);
    traits.insert(MinionTrait::SUMMONED);
  }
  if (!traits.contains(MinionTrait::FARM_ANIMAL) && !c->getController()->isCustomController())
    c->setController(makeOwner<Monster>(c, MonsterAIFactory::collective(this)));
  if (traits.contains(MinionTrait::LEADER)) {
    CHECK(!leader);
    leader = c;
    if (config->isLeaderFighter())
      traits.insert(MinionTrait::FIGHTER);
  }
  if (c->getTribeId() != *tribe)
    c->setTribe(*tribe);
  if (WGame game = getGame())
    for (WCollective col : getGame()->getCollectives())
      if (col->getCreatures().contains(c))
        col->removeCreature(c);
  creatures.push_back(c);
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
  updateCreatureStatus(c);
  for (WItem item : c->getEquipment().getItems())
    CHECK(minionEquipment->tryToOwn(c, item));
  control->onMemberAdded(c);
}

void Collective::removeCreature(WCreature c) {
  creatures.removeElement(c);
  returnResource(taskMap->freeFromTask(c));
  for (auto team : teams->getContaining(c))
    teams->remove(team, c);
  for (MinionTrait t : ENUM_ALL(MinionTrait))
    if (byTrait[t].contains(c))
      byTrait[t].removeElement(c);
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

void Collective::setMinionTask(WConstCreature c, MinionTask task) {
  if (auto duration = MinionTasks::getDuration(c, task))
    currentTasks.set(c, {task, getLocalTime() + *duration});
  else
    currentTasks.set(c, {task, none});
}

Collective::CurrentTaskInfo Collective::getCurrentTask(WConstCreature c) const {
  if (auto current = currentTasks.getMaybe(c))
    return *current;
  else
    return CurrentTaskInfo{MinionTask::IDLE, none};
}

bool Collective::isTaskGood(WConstCreature c, MinionTask task, bool ignoreTaskLock) {
  if (!c->getAttributes().getMinionTasks().isAvailable(this, c, task, ignoreTaskLock) ||
      (!MinionTasks::generate(this, c, task) && !MinionTasks::getExisting(this, c, task)))
    return false;
  switch (task) {
    case MinionTask::BE_WHIPPED:
      return c->getMorale() < 0.95;
    case MinionTask::CROPS:
    case MinionTask::EXPLORE:
      return getGame()->getSunlightInfo().getState() == SunlightState::DAY;
    case MinionTask::SLEEP:
      if (!config->hasVillainSleepingTask())
        return true;
      FALLTHROUGH;
    case MinionTask::EXPLORE_NOCTURNAL:
      return getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
    default: return true;
  }
}

void Collective::setRandomTask(WConstCreature c) {
  vector<MinionTask> goodTasks;
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (isTaskGood(c, t) && c->getAttributes().getMinionTasks().canChooseRandomly(c, t))
      goodTasks.push_back(t);
  if (!goodTasks.empty())
    setMinionTask(c, Random.choose(goodTasks));
}

bool Collective::isMinionTaskPossible(WCreature c, MinionTask task) {
  return isTaskGood(c, task, true) && (MinionTasks::generate(this, c, task) || MinionTasks::getExisting(this, c, task));
}

WTask Collective::getStandardTask(WCreature c) {
  auto current = currentTasks.getMaybe(c);
  if (!current || (current->finishTime && *current->finishTime < getLocalTime()) || !isTaskGood(c, current->task)) {
    currentTasks.erase(c);
    setRandomTask(c);
  }
  current = getCurrentTask(c);
  CHECK(current) << "No minion task found for " << c->getName().bare();
  MinionTask task = current->task;
  if (!current->finishTime) // see comment in header
    currentTasks.getOrFail(c).finishTime = LocalTime(-1000);
  if (PTask ret = MinionTasks::generate(this, c, task))
    return taskMap->addTaskFor(std::move(ret), c);
  if (WTask ret = MinionTasks::getExisting(this, c, task)) {
    taskMap->takeTask(c, ret);
    return ret;
  }
  FATAL << "No task generated for minion task " << EnumInfo<MinionTask>::getString(task);
  return {};
}

bool Collective::isConquered() const {
  return getCreatures(MinionTrait::FIGHTER).empty() && !hasLeader();
}

vector<WCreature> Collective::getConsumptionTargets(WCreature consumer) const {
  vector<WCreature> ret;
  for (WCreature c : getCreatures())
    if (consumer->canConsume(c) && c != getLeader())
      ret.push_back(c);
  return ret;
}

void Collective::orderConsumption(WCreature consumer, WCreature who) {
  CHECK(getConsumptionTargets(consumer).contains(who));
  setTask(consumer, Task::consume(this, who));
}

PTask Collective::getEquipmentTask(WCreature c) {
  if (!usesEquipment(c))
    return nullptr;
  if (!hasTrait(c, MinionTrait::NO_AUTO_EQUIPMENT) && Random.roll(40))
    minionEquipment->autoAssign(c, getAllItems(ItemIndex::MINION_EQUIPMENT, false));
  vector<PTask> tasks;
  for (WItem it : c->getEquipment().getItems())
    if (!c->getEquipment().isEquipped(it) && c->getEquipment().canEquip(it))
      tasks.push_back(Task::equipItem(it));
  for (Position v : zones->getPositions(ZoneId::STORAGE_EQUIPMENT)) {
    vector<WItem> allItems = v.getItems(ItemIndex::MINION_EQUIPMENT).filter(
        [this, c] (WConstItem it) { return minionEquipment->isOwner(it, c);});
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

const static EnumSet<MinionTask> healingTasks {MinionTask::SLEEP};

void Collective::considerHealingTask(WCreature c) {
  if (c->getBody().canHeal() && !c->isAffected(LastingEffect::POISON))
    for (MinionTask t : healingTasks) {
      auto currentTask = getCurrentTask(c).task;
      if (c->getAttributes().getMinionTasks().isAvailable(this, c, t) && !healingTasks.contains(currentTask)) {
        cancelTask(c);
        setMinionTask(c, t);
        return;
      }
    }
}

void Collective::clearLeader() {
  leader = nullptr;
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

static MoveInfo getFirstGoodMove() {
  return NoMove;
}

template <typename MoveFun1, typename... MoveFuns>
MoveInfo getFirstGoodMove(MoveFun1&& f1, MoveFuns&&... funs) {
  if (auto move = std::forward<MoveFun1>(f1)())
    return move;
  else
    return getFirstGoodMove(std::forward<MoveFuns>(funs)...);
}

MoveInfo Collective::getMove(WCreature c) {
  CHECK(control);
  CHECK(creatures.contains(c));
  CHECK(!c->isDead());

  auto waitIfNoMove = [&] (MoveInfo move) -> MoveInfo {
    if (!move)
      return c->wait();
    else
      return move;
  };

  auto priorityTask = [&] {
    if (WTask task = taskMap->getTask(c))
      if (taskMap->isPriorityTask(task))
        return waitIfNoMove(task->getMove(c));
    return NoMove;
  };

  auto followTeamLeader = [&] () -> MoveInfo {
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
  };

  auto dropLoot = [&] () -> MoveInfo {
    if (config->getFetchItems() && territory->contains(c->getPosition())) {
      vector<WItem> items = c->getEquipment().getItems([this, c](WConstItem item) {
          return !isItemMarked(item) && !minionEquipment->isOwner(item, c); });
      if (!items.empty())
        return c->drop(items);
    }
    return NoMove;
  };

  auto goToAlarm = [&] () -> MoveInfo {
    if (hasTrait(c, MinionTrait::FIGHTER) && alarmInfo && alarmInfo->finishTime > getGlobalTime())
      if (auto action = c->moveTowards(alarmInfo->position))
        return {1.0, action};
    return NoMove;
  };

  auto normalTask = [&] () -> MoveInfo {
    if (WTask task = taskMap->getTask(c))
      return waitIfNoMove(task->getMove(c));
    return NoMove;
  };

  auto newEquipmentTask = [&] {
    if (PTask t = getEquipmentTask(c))
      if (auto move = t->getMove(c)) {
        taskMap->addTaskFor(std::move(t), c);
        return move;
      }
    return NoMove;
  };

  auto newStandardTask = [&] {
    WTask t = getStandardTask(c);
    return waitIfNoMove(t->getMove(c));
  };
  if (getConfig().allowHealingTaskOutsideTerritory() || territory->contains(c->getPosition()))
    considerHealingTask(c);
  return getFirstGoodMove(
      priorityTask,
      followTeamLeader,
      dropLoot,
      goToAlarm,
      normalTask,
      newEquipmentTask,
      newStandardTask
  );
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

void Collective::addNewCreatureMessage(const vector<WCreature>& immigrants) {
  if (immigrants.size() == 1)
    control->addMessage(PlayerMessage(immigrants[0]->getName().a() + " joins your forces.")
        .setCreature(immigrants[0]->getUniqueId()));
  else {
    control->addMessage(PlayerMessage("A " + immigrants[0]->getName().groupOf(immigrants.size()) +
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

void Collective::considerTransferingLostMinions() {
  for (auto c : getCreatures())
    if (c->getPosition().getModel() != getGame()->getCurrentModel())
      getGame()->transferCreature(c, getModel());
}

void Collective::tick() {
  considerTransferingLostMinions();
  dangerLevelCache = none;
  control->tick();
  zones->tick();
  decayMorale();
  if (config->getWarnings() && Random.roll(5))
    warnings->considerWarnings(this);
  if (config->getEnemyPositions() && Random.roll(5)) {
    vector<Position> enemyPos = getEnemyPositions();
    if (!enemyPos.empty())
      delayDangerousTasks(enemyPos, getLocalTime() + 20_visible);
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
            c->dieNoReason(Creature::DropType::ONLY_INVENTORY);
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

bool Collective::hasTrait(WConstCreature c, MinionTrait t) const {
  return byTrait[t].contains(c);
}

void Collective::setTrait(WCreature c, MinionTrait t) {
  if (!hasTrait(c, t))
    byTrait[t].push_back(c);
  updateCreatureStatus(c);
}

void Collective::removeTrait(WCreature c, MinionTrait t) {
  byTrait[t].removeElementMaybe(c);
  updateCreatureStatus(c);
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
  using namespace EventInfo;
  event.visit(
      [&](const Alarm& info) {
        static const auto alarmTime = 100_visible;
        if (getTerritory().contains(info.pos)) {
          if (!info.silent)
            control->addMessage(PlayerMessage("An alarm goes off.", MessagePriority::HIGH).setPosition(info.pos));
          alarmInfo = AlarmInfo {getGlobalTime() + alarmTime, info.pos };
          for (WCreature c : byTrait[MinionTrait::FIGHTER])
            if (c->isAffected(LastingEffect::SLEEP))
              c->removeEffect(LastingEffect::SLEEP);
        }
      },
      [&](const CreatureKilled& info) {
        if (creatures.contains(info.victim))
          onMinionKilled(info.victim, info.attacker);
        if (creatures.contains(info.attacker))
          onKilledSomeone(info.attacker, info.victim);
      },
      [&](const CreatureTortured& info) {
        if (creatures.contains(info.torturer))
          returnResource({ResourceId::MANA, 1});
      },
      [&](const CreatureSurrendered& info) {
        if (getCreatures().contains(info.attacker) && !getCreatures().contains(info.victim) &&
            info.victim->getBody().isHumanoid())
          surrendering.insert(info.victim);
      },
      [&](const TrapTriggered& info) {
        if (auto& trap = constructions->getTrap(info.pos)) {
          trap->reset();
          if (trap->getType() == TrapType::SURPRISE)
            handleSurprise(info.pos);
        }
      },
      [&](const TrapDisarmed& info) {
        if (auto& trap = constructions->getTrap(info.pos)) {
          control->addMessage(PlayerMessage(info.creature->getName().a() +
              " disarms a " + getTrapName(trap->getType()) + " trap.",
              MessagePriority::HIGH).setPosition(info.pos));
          trap->reset();
        }
      },
      [&](const FurnitureDestroyed& info) {
        constructions->onFurnitureDestroyed(info.position, info.layer);
        tileEfficiency->update(info.position);
      },
      [&](const ConqueredEnemy& info) {
        auto col = info.collective;
        if (col->isDiscoverable()) {
          if (auto& name = col->getName())
            control->addMessage(PlayerMessage("The tribe of " + name->full + " is destroyed.",
                MessagePriority::CRITICAL));
          else
            control->addMessage(PlayerMessage("An unnamed tribe is destroyed.", MessagePriority::CRITICAL));
          auto mana = config->getManaForConquering(col->getVillainType());
          addMana(mana);
          control->addMessage(PlayerMessage("You feel a surge of power (+" + toString(mana) + " mana)",
              MessagePriority::CRITICAL));
        }
        auto civilians = col->getCreatures().filter(
            [&](WConstCreature c) { return c->getBody().isHumanoid() && !col->hasTrait(c, MinionTrait::FIGHTER); });
        int numCaptured = min(civilians.size(), numPrisonerOrders);
        numPrisonerOrders -= numCaptured;
        for (int i : Range(numCaptured)) {
          auto pos = civilians[i]->getPosition();
          civilians[i]->dieNoReason(Creature::DropType::ONLY_INVENTORY);
          addCreature(CreatureFactory::fromId(CreatureId::PRISONER, getTribeId()), pos,
              {MinionTrait::PRISONER, MinionTrait::NO_LIMIT});
        }
      },
      [&](const auto&) {}
  );
}

void Collective::onPositionDiscovered(Position pos) {
  control->onPositionDiscovered(pos);
}

void Collective::onMinionKilled(WCreature victim, WCreature killer) {
  string deathDescription = victim->getAttributes().getDeathDescription();
  control->onMemberKilled(victim, killer);
  if (hasTrait(victim, MinionTrait::PRISONER) && killer && getCreatures().contains(killer))
    returnResource({ResourceId::PRISONER_HEAD, 1});
  if (!hasTrait(victim, MinionTrait::FARM_ANIMAL) && !hasTrait(victim, MinionTrait::SUMMONED)) {
    decreaseMoraleForKill(killer, victim);
    if (killer)
      control->addMessage(PlayerMessage(victim->getName().a() + " is " + deathDescription + " by " + killer->getName().a(),
            MessagePriority::HIGH).setPosition(victim->getPosition()));
    else
      control->addMessage(PlayerMessage(victim->getName().a() + " is " + deathDescription + ".", MessagePriority::HIGH)
          .setPosition(victim->getPosition()));
  }
  bool fighterKilled = hasTrait(victim, MinionTrait::FIGHTER) || victim == getLeader();
  removeCreature(victim);
  if (isConquered() && fighterKilled)
    getGame()->addEvent(EventInfo::ConqueredEnemy{this});
}

void Collective::onKilledSomeone(WCreature killer, WCreature victim) {
  string deathDescription=victim->getAttributes().getDeathDescription();
  if (victim->getTribe() != getTribe()) {
    addMana(getKillManaScore(victim));
    addMoraleForKill(killer, victim);
    kills.insert(victim);
    int difficulty = victim->getDifficultyPoints();
    CHECK(difficulty >=0 && difficulty < 100000) << difficulty << " " << victim->getName().bare();
    points += difficulty;
    control->addMessage(PlayerMessage(victim->getName().a() + " is " + deathDescription + " by " + killer->getName().a())
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
      pos.canEnter({MovementTrait::WALK}) &&
      !pos.isWall();
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

LocalTime Collective::getLocalTime() const {
  return getModel()->getLocalTime();
}

GlobalTime Collective::getGlobalTime() const {
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
      Random.choose(destination).dropItems(config->getResourceInfo(amount.id).itemId.get(amount.value));
      return;
    }
  }
  credit[amount.id] += amount.value;
}

vector<pair<WItem, Position>> Collective::getTrapItems(TrapType type, const vector<Position>& squares) const {
  vector<pair<WItem, Position>> ret;
  for (Position pos : squares)
    for (auto it : pos.getItems(ItemIndex::TRAP))
      if (it->getTrapType() == type && !isItemMarked(it))
        ret.emplace_back(it, pos);
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

optional<set<Position>> Collective::getStorageFor(WConstItem item) const {
  for (auto& info : config->getFetchInfo())
    if (getIndexPredicate(info.index)(item))
      return info.destinationFun(this);
  return none;
}

void Collective::addKnownVillain(WConstCollective col) {
  knownVillains.insert(col);
}

bool Collective::isKnownVillain(WConstCollective col) const {
  return (getModel() != col->getModel() && col->getVillainType() != VillainType::NONE) || knownVillains.contains(col);
}

void Collective::addKnownVillainLocation(WConstCollective col) {
  knownVillainLocations.insert(col);
}

bool Collective::isKnownVillainLocation(WConstCollective col) const {
  return knownVillainLocations.contains(col);
}

bool Collective::isItemMarked(WConstItem it) const {
  return !!markedItems.getOrElse(it, nullptr);
}

void Collective::markItem(WConstItem it, WConstTask task) {
  markedItems.set(it, task);
}

void Collective::removeTrap(Position pos) {
  constructions->removeTrap(pos);
}

bool Collective::canAddFurniture(Position position, FurnitureType type) const {
  return knownTiles->isKnown(position)
      && (territory->contains(position) ||
          canClaimSquare(position) ||
          CollectiveConfig::canBuildOutsideTerritory(type))
      && !getConstructions().getTrap(position)
      && !getConstructions().containsFurniture(position, Furniture::getLayer(type))
      && position.canConstruct(type);
}

void Collective::removeFurniture(Position pos, FurnitureLayer layer) {
  auto f = constructions->getFurniture(pos, layer);
  if (f->hasTask())
    returnResource(taskMap->removeTask(f->getTask()));
  constructions->removeFurniture(pos, layer);
}

void Collective::destroySquare(Position pos, FurnitureLayer layer) {
  if (constructions->containsFurniture(pos, layer)) {
    if (auto furniture = pos.modFurniture(layer))
      if (furniture->getTribe() == getTribeId()) {
        furniture->destroy(pos, DestroyAction::Type::BASH);
        tileEfficiency->update(pos);
      }
    removeFurniture(pos, layer);
  }
  if (layer != FurnitureLayer::FLOOR) {
    zones->eraseZones(pos);
    if (constructions->getTrap(pos))
      removeTrap(pos);
  }
}

void Collective::addFurniture(Position pos, FurnitureType type, const CostInfo& cost, bool noCredit) {
  /*if (type == FurnitureType::MOUNTAIN && (pos.isChokePoint({MovementTrait::WALK}) ||
        constructions->getTotalCount(type) - constructions->getBuiltCount(type) > 0))
    return;*/
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
  return !!taskMap->getMarked(pos);
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
  if (auto& trap = constructions->getTrap(pos))
    trap->setArmed();
}

void Collective::onAppliedItemCancel(Position pos) {
  if (auto& trap = constructions->getTrap(pos))
    trap->reset();
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
      break;
    default:
      break;
  }
  control->onDestructed(pos, type, action);
}

void Collective::handleTrapPlacementAndProduction() {
  EnumMap<TrapType, vector<pair<WItem, Position>>> trapItems(
      [this] (TrapType type) { return getTrapItems(type, territory->getAll());});
  EnumMap<TrapType, int> missingTraps;
  for (auto trapPos : constructions->getAllTraps()) {
    auto& trap = *constructions->getTrap(trapPos);
    if (!trap.isArmed() && !trap.isMarked() && !isDelayed(trapPos)) {
      vector<pair<WItem, Position>>& items = trapItems[trap.getType()];
      if (!items.empty()) {
        Position pos = items.back().second;
        auto task = taskMap->addTask(Task::applyItem(this, pos, items.back().first, trapPos), pos);
        markItem(items.back().first, task);
        items.pop_back();
        trap.setMarked();
      } else
        ++missingTraps[trap.getType()];
    }
  }
  for (TrapType type : ENUM_ALL(TrapType))
    scheduleAutoProduction([type](WConstItem it) { return it->getTrapType() == type;}, missingTraps[type]);
}

void Collective::scheduleAutoProduction(function<bool(WConstItem)> itemPredicate, int count) {
  if (count > 0)
    for (auto workshopType : ENUM_ALL(WorkshopType))
      for (auto& item : workshops->get(workshopType).getQueued())
        if (itemPredicate(item.type.get().get()))
          count -= item.number * item.batchSize;
  if (count > 0)
    for (auto workshopType : ENUM_ALL(WorkshopType)) {
      //Don't use alchemy to get resources automatically as it is expensive
      if (workshopType == WorkshopType::LABORATORY)
        continue;
      auto& options = workshops->get(workshopType).getOptions();
      for (int index : All(options))
        if (itemPredicate(options[index].type.get().get())) {
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
        scheduleAutoProduction([resourceId] (WConstItem it) { return it->getResourceId() == resourceId; }, needed);
  }
}

void Collective::updateConstructions() {
  handleTrapPlacementAndProduction();
  for (auto& pos : constructions->getAllFurniture()) {
    auto& construction = *constructions->getFurniture(pos.first, pos.second);
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
}

void Collective::delayDangerousTasks(const vector<Position>& enemyPos1, LocalTime delayTime) {
  vector<Vec2> enemyPos = enemyPos1
      .filter([=] (const Position& p) { return p.isSameLevel(level); })
      .transform([] (const Position& p) { return p.getCoord();});
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
  vector<WItem> equipment = pos.getItems(elem.index).filter(
      [this, &elem] (WConstItem item) { return elem.predicate(this, item); });
  if (!equipment.empty()) {
    const set<Position>& destination = elem.destinationFun(this);
    if (!destination.empty()) {
      warnings->setWarning(elem.warning, false);
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      auto task = taskMap->addTask(Task::bringItem(this, pos, equipment, destination), pos);
      for (WItem it : equipment)
        markItem(it, task);
    } else
      warnings->setWarning(elem.warning, true);
  }
}

void Collective::handleSurprise(Position pos) {
  Vec2 rad(8, 8);
  WCreature c = pos.getCreature();
  for (Position v : Random.permutation(pos.getRectangle(Rectangle(-rad, rad + Vec2(1, 1)))))
    if (WCreature other = v.getCreature())
      if (hasTrait(other, MinionTrait::FIGHTER) && other->getPosition().dist8(pos) > 1) {
        for (Position dest : pos.neighbors8(Random))
          if (other->getPosition().canMoveCreature(dest)) {
            other->getPosition().moveCreature(dest);
            break;
          }
      }
  pos.globalMessage("Surprise!");
}

void Collective::retire() {
  knownTiles->limitToModel(getModel());
  knownVillainLocations.clear();
  knownVillains.clear();
  discoverable = true;
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
    // Furniture have variable usage time, so just multiply by it to be independent of changes.
    double efficiency = tileEfficiency->getEfficiency(pos) * furniture->getUsageTime().getVisibleDouble()
        * getEfficiency(c);
    switch (furniture->getType()) {
      case FurnitureType::THRONE:
        if (config->getRegenerateMana())
          addMana(0.2 * efficiency);
        break;
      case FurnitureType::WHIPPING_POST:
        taskMap->addTask(Task::whipping(pos, c), pos);
        break;
      case FurnitureType::GALLOWS:
        taskMap->addTask(Task::kill(this, c), pos);
        break;
      case FurnitureType::TORTURE_TABLE:
        taskMap->addTask(Task::torture(this, c), pos);
        break;
      default:
        break;
    }
    if (auto usage = furniture->getUsageType()) {
      auto increaseLevel = [&] (ExperienceType exp) {
        double increase = 0.007 * efficiency;
        if (auto maxLevel = config->getTrainingMaxLevel(exp, furniture->getType()))
          increase = min(increase, *maxLevel - c->getAttributes().getExpLevel(exp));
        if (increase > 0)
          c->increaseExpLevel(exp, increase);
      };
      switch (*usage) {
        case FurnitureUsageType::TRAIN:
          increaseLevel(ExperienceType::MELEE);
          break;
        case FurnitureUsageType::STUDY:
          increaseLevel(ExperienceType::SPELL);
          if (config->getRegenerateMana())
            addMana(0.1 * efficiency);
          break;
        case FurnitureUsageType::ARCHERY_RANGE:
          increaseLevel(ExperienceType::ARCHERY);
          break;
        default:
          break;
      }
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

optional<FurnitureType> Collective::getMissingTrainingFurniture(WConstCreature c, ExperienceType expType) const {
  if (c->getAttributes().isTrainingMaxedOut(expType))
    return none;
  optional<FurnitureType> requiredDummy;
  for (auto dummyType : CollectiveConfig::getTrainingFurniture(expType)) {
    bool canTrain = *config->getTrainingMaxLevel(expType, dummyType) >
        c->getAttributes().getExpLevel(expType);
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
  return technologies.contains(id);
}

void Collective::acquireTech(Technology* tech) {
  technologies.push_back(tech->getId());
  Technology::onAcquired(tech->getId(), this);
}

vector<Technology*> Collective::getTechnologies() const {
  return technologies.transform([] (const TechId t) { return Technology::get(t); });
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

void Collective::onExternalEnemyKilled(const std::string& name) {
  control->addMessage(PlayerMessage("You resisted the attack of " + name + ".",
      MessagePriority::CRITICAL));
  int mana = 100;
  addMana(mana);
  control->addMessage(PlayerMessage("You feel a surge of power (+" + toString(mana) + " mana)",
                                    MessagePriority::CRITICAL));
}

void Collective::addPrisonerOrder() {
  ++numPrisonerOrders;
}

void Collective::removePrisonerOrder() {
  if (numPrisonerOrders > 0)
    --numPrisonerOrders;
}

int Collective::getNumPrisonerOrders() const {
  return numPrisonerOrders;
}

void Collective::onCopulated(WCreature who, WCreature with) {
  if (with->getName().bare() == "vampire")
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a()
        + " with a monkey on " + who->getAttributes().getGender().his() + " knee");
  else
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a());
  if (getCreatures().contains(with))
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

Zones& Collective::getZones() {
  return *zones;
}

const Zones& Collective::getZones() const {
  return *zones;
}

Quarters& Collective::getQuarters() {
  return *quarters;
}

const Quarters& Collective::getQuarters() const {
  return *quarters;
}

const TaskMap& Collective::getTaskMap() const {
  return *taskMap;
}

TaskMap& Collective::getTaskMap() {
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

REGISTER_TYPE(Collective)
REGISTER_TYPE(ListenerTemplate<Collective>)
