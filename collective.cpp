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
#include "minion_activity_map.h"
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
#include "position_matching.h"
#include "storage_id.h"
#include "game_config.h"
#include "conquer_condition.h"
#include "game_event.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar(SUBCLASS(TaskCallback), SUBCLASS(UniqueEntity<Collective>), SUBCLASS(EventListener));
  ar(creatures, taskMap, tribe, control, byTrait, populationGroups, hadALeader);
  ar(territory, alarmInfo, markedItems, constructions, minionEquipment);
  ar(delayedPos, knownTiles, technology, kills, points, currentActivity);
  ar(credit, model, immigration, teams, name, conqueredVillains);
  ar(config, warnings, knownVillains, knownVillainLocations, banished, positionMatching);
  ar(villainType, enemyId, workshops, zones, discoverable, quarters, populationIncrease, dungeonLevel);
}

SERIALIZABLE(Collective)

SERIALIZATION_CONSTRUCTOR_IMPL(Collective)

Collective::Collective(Private, WModel model, TribeId t, const optional<CollectiveName>& n)
    : positionMatching(makeOwner<PositionMatching>()), tribe(t), model(NOTNULL(model)), name(n),
      villainType(VillainType::NONE) {
}

PCollective Collective::create(WModel model, TribeId tribe, const optional<CollectiveName>& name, bool discoverable) {
  auto ret = makeOwner<Collective>(Private {}, model, tribe, name);
  ret->subscribeTo(model);
  ret->discoverable = discoverable;
  ret->workshops = unique<Workshops>(std::array<vector<WorkshopItemCfg>, 4>());
  ret->immigration = makeOwner<Immigration>(ret.get(), vector<ImmigrantInfo>());
  return ret;
}

void Collective::init(CollectiveConfig cfg) {
  config.reset(std::move(cfg));
  credit = cfg.getStartingResource();
}

void Collective::setImmigration(PImmigration i) {
  immigration = std::move(i);
}

void Collective::setWorkshops(unique_ptr<Workshops> w) {
  workshops = std::move(w);
}

const heap_optional<CollectiveName>& Collective::getName() const {
  return name;
}

void Collective::setVillainType(VillainType t) {
  villainType = t;
}

bool Collective::isDiscoverable() const {
  return discoverable;
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

void Collective::setPopulationGroup(const vector<Creature*>& creatures) {
  for (auto c : copyOf(populationGroups))
    if (c.size() == 1 && creatures.contains(c[0]))
      populationGroups.removeElement(c);
  populationGroups.push_back(creatures);
}

void Collective::addCreature(PCreature creature, Position pos, EnumSet<MinionTrait> traits) {
  Creature* c = creature.get();
  pos.addCreature(std::move(creature));
  addCreature(c, traits);
}

void Collective::updateCreatureStatus(Creature* c) {
  c->getStatus().set(CreatureStatus::CIVILIAN,
      getCreatures().contains(c) &&
      c->getBody().isHumanoid() &&
      !c->isAffected(LastingEffect::STUNNED) &&
      !hasTrait(c, MinionTrait::FIGHTER) &&
      !hasTrait(c, MinionTrait::LEADER));
  c->getStatus().set(CreatureStatus::FIGHTER, hasTrait(c, MinionTrait::FIGHTER));
  c->getStatus().set(CreatureStatus::LEADER, hasTrait(c, MinionTrait::LEADER));
  c->getStatus().set(CreatureStatus::PRISONER, hasTrait(c, MinionTrait::PRISONER));
}

void Collective::addCreature(Creature* c, EnumSet<MinionTrait> traits) {
  if (c->getGlobalTime()) { // only do this if creature already exists on the map
    c->addEffect(LastingEffect::RESTED, 500_visible, false);
    c->addEffect(LastingEffect::SATIATED, 500_visible, false);
  }
  if (c->isAffected(LastingEffect::SUMMONED)) {
    traits.insert(MinionTrait::NO_LIMIT);
    traits.insert(MinionTrait::SUMMONED);
  }
  if (!traits.contains(MinionTrait::FARM_ANIMAL) && !c->getController()->dontReplaceInCollective())
    c->setController(makeOwner<Monster>(c, MonsterAIFactory::collective(this)));
  if (traits.contains(MinionTrait::LEADER)) {
    hadALeader = true;
    CHECK(!getLeader());
    if (config->isLeaderFighter())
      traits.insert(MinionTrait::FIGHTER);
  }
  if (c->getTribeId() != *tribe)
    c->setTribe(*tribe);
  if (auto game = getGame())
    for (WCollective col : getGame()->getCollectives())
      if (col->getCreatures().contains(c))
        col->removeCreature(c);
  creatures.push_back(c);
  populationGroups.push_back({c});
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
  updateCreatureStatus(c);
  for (Item* item : c->getEquipment().getItems())
    CHECK(minionEquipment->tryToOwn(c, item));
  for (auto minion : getCreatures()) {
    c->removePrivateEnemy(minion);
    minion->removePrivateEnemy(c);
  }
  control->onMemberAdded(c);
}

void Collective::removeCreature(Creature* c) {
  creatures.removeElement(c);
  for (auto& group : populationGroups)
    group.removeElementMaybe(c);
  for (auto& group : copyOf(populationGroups))
    if (group.size() == 0)
      populationGroups.removeElement(group);
  returnResource(taskMap->freeFromTask(c));
  for (auto team : teams->getContaining(c))
    teams->remove(team, c);
  for (MinionTrait t : ENUM_ALL(MinionTrait))
    if (byTrait[t].contains(c))
      byTrait[t].removeElement(c);
  updateCreatureStatus(c);
}

void Collective::banishCreature(Creature* c) {
  decreaseMoraleForBanishing(c);
  removeCreature(c);
  vector<Position> exitTiles = territory->getExtended(10, 20);
  vector<PTask> tasks;
  vector<Item*> items = c->getEquipment().getItems();
  if (!items.empty())
    tasks.push_back(Task::dropItemsAnywhere(items));
  if (!exitTiles.empty())
    tasks.push_back(Task::goToTryForever(Random.choose(exitTiles)));
  tasks.push_back(Task::disappear());
  c->setController(makeOwner<Monster>(c, MonsterAIFactory::singleTask(Task::chain(std::move(tasks)))));
  banished.insert(c);
}

bool Collective::wasBanished(const Creature* c) const {
  return banished.contains(c);
}

/*vector<Creature*> Collective::getRecruits() const {
  vector<Creature*> ret;
  vector<Creature*> possibleRecruits = filter(getCreatures(MinionTrait::FIGHTER),
      [] (const Creature* c) { return c->getAttributes().getRecruitmentCost() > 0; });
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
  FATAL << "Couldn't find item";
  return nullptr;
}

vector<TriggerInfo> Collective::getTriggers(WConstCollective against) const {
  if (!isConquered())
    return control->getTriggers(against);
  else
    return {};
}

Creature* Collective::getLeader() const {
  if (!byTrait[MinionTrait::LEADER].empty())
    return byTrait[MinionTrait::LEADER].getOnlyElement();
  else
    return nullptr;
}

WGame Collective::getGame() const {
  return model->getGame();
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
  return model;
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

void Collective::setMinionActivity(Creature* c, MinionActivity activity) {
  auto current = getCurrentActivity(c);
  if (current.activity != activity) {
    freeFromTask(c);
    c->removeEffect(LastingEffect::SLEEP);
    currentActivity.set(c, {activity, getLocalTime() +
        MinionActivities::getDuration(c, activity).value_or(-1_visible)});
  }
}

Collective::CurrentActivity Collective::getCurrentActivity(const Creature* c) const {
  return currentActivity.getMaybe(c)
      .value_or(CurrentActivity{MinionActivity::IDLE, getLocalTime() - 1_visible});
}

bool Collective::isActivityGood(Creature* c, MinionActivity activity, bool ignoreTaskLock) {
  PROFILE;
  if (!c->getAttributes().getMinionActivities().isAvailable(this, c, activity, ignoreTaskLock) ||
      (!MinionActivities::generate(this, c, activity) && !MinionActivities::getExisting(this, c, activity)))
    return false;
  switch (activity) {
    case MinionActivity::BE_WHIPPED:
      return c->getMorale() < 0.95;
    case MinionActivity::CROPS:
    case MinionActivity::EXPLORE:
      return getGame()->getSunlightInfo().getState() == SunlightState::DAY;
    case MinionActivity::SLEEP:
      if (!config->hasVillainSleepingTask())
        return true;
      FALLTHROUGH;
    case MinionActivity::EXPLORE_NOCTURNAL:
      return getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
    case MinionActivity::BE_TORTURED:
      return getMaxPopulation() > getPopulationSize();
    default: return true;
  }
}

bool Collective::isConquered() const {
  return config->isConquered(this);
}

void Collective::setTask(Creature* c, PTask task) {
  returnResource(taskMap->freeFromTask(c));
  taskMap->addTaskFor(std::move(task), c);
}

bool Collective::hasTask(const Creature* c) const {
  return taskMap->hasTask(c);
}

void Collective::freeFromTask(const Creature* c) {
  taskMap->freeFromTask(c);
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

vector<Position> Collective::getEnemyPositions() const {
  PROFILE;
  vector<Position> enemyPos;
  for (Position pos : territory->getExtended(10))
    if (const Creature* c = pos.getCreature())
      if (getTribe()->isEnemy(c) && !c->isAffected(LastingEffect::STUNNED))
        enemyPos.push_back(pos);
  return enemyPos;
}

void Collective::addNewCreatureMessage(const vector<Creature*>& immigrants) {
  if (immigrants.size() == 1)
    control->addMessage(PlayerMessage(immigrants[0]->getName().a() + " joins your forces.")
        .setCreature(immigrants[0]->getUniqueId()));
  else {
    control->addMessage(PlayerMessage("A " + immigrants[0]->getName().groupOf(immigrants.size()) +
          " joins your forces.").setCreature(immigrants[0]->getUniqueId()));
  }
}

static int getKeeperUpgradeLevel(int dungeonLevel) {
  return (dungeonLevel + 1) / 5;
}

void Collective::update(bool currentlyActive) {
  auto leader = getLeader();
  if (leader)
    leader->upgradeViewId(getKeeperUpgradeLevel(dungeonLevel.level));
  control->update(currentlyActive);
  if (config->hasImmigrantion(currentlyActive) && (leader || !hadALeader) && !isConquered())
    immigration->update();
}

double Collective::getRebellionProbability() const {
  const double allowedPrisonerRatio = 0.5;
  const double maxPrisonerRatio = 1.5;
  const int numPrisoners = getCreatures(MinionTrait::PRISONER).size();
  const int numFighters = getCreatures(MinionTrait::FIGHTER).size();
  const int numFreePrisoners = 4;
  if (numPrisoners <= numFreePrisoners)
    return 0;
  if (numFighters == 0)
    return 1;
  double ratio = double(numPrisoners - numFreePrisoners) / double(numFighters);
  return min(1.0, max(0.0, (ratio - allowedPrisonerRatio) / (maxPrisonerRatio - allowedPrisonerRatio)));
}

void Collective::considerRebellion() {
  if (Random.chance(getRebellionProbability() / 1000)) {
    Position escapeTarget = model->getTopLevel()->getLandingSquare(StairKey::transferLanding(),
        Random.choose(Vec2::directions8()));
    for (auto c : copyOf(getCreatures(MinionTrait::PRISONER))) {
      removeCreature(c);
      c->setController(makeOwner<Monster>(c, MonsterAIFactory::singleTask(
          Task::chain(Task::goToTryForever(escapeTarget), Task::disappear()))));
      c->setTribe(TribeId::getMonster());
      c->removeEffect(LastingEffect::SLEEP);
      c->removeEffect(LastingEffect::ENTANGLED);
      c->removeEffect(LastingEffect::TIED_UP);
      c->toggleCaptureOrder();
    }
    control->addMessage(PlayerMessage("Prisoners escaping!", MessagePriority::CRITICAL));
  }
}

void Collective::tick() {
  PROFILE_BLOCK("Collective::tick");
  considerRebellion();
  dangerLevelCache = none;
  control->tick();
  zones->tick();
  taskMap->clearFinishedTasks();
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
  }
  if (config->getConstructions())
    updateConstructions();
  if (Random.roll(5)) {
    auto& fetchInfo = getConfig().getFetchInfo();
    if (!fetchInfo.empty()) {
      for (Position pos : territory->getAll())
        if (!isDelayed(pos) && pos.canEnterEmpty(MovementTrait::WALK) && !pos.getItems().empty())
          for (const ItemFetchInfo& elem : fetchInfo)
            fetchItems(pos, elem);
      for (Position pos : zones->getPositions(ZoneId::FETCH_ITEMS))
        if (!isDelayed(pos) && pos.canEnterEmpty(MovementTrait::WALK))
          for (const ItemFetchInfo& elem : fetchInfo)
            fetchItems(pos, elem);
      for (Position pos : zones->getPositions(ZoneId::PERMANENT_FETCH_ITEMS))
        if (!isDelayed(pos) && pos.canEnterEmpty(MovementTrait::WALK))
          for (const ItemFetchInfo& elem : fetchInfo)
            fetchItems(pos, elem);
    }
  }
  if (config->getManageEquipment() && Random.roll(40)) {
    minionEquipment->updateOwners(getCreatures());
    minionEquipment->updateItems(getAllItems(ItemIndex::MINION_EQUIPMENT, true));
  }
  for (auto c : getCreatures())
    if (!usesEquipment(c))
      for (auto it : minionEquipment->getItemsOwnedBy(c))
        minionEquipment->discard(it);
}

const vector<Creature*>& Collective::getCreatures(MinionTrait trait) const {
  return byTrait[trait];
}

bool Collective::hasTrait(const Creature* c, MinionTrait t) const {
  return byTrait[t].contains(c);
}

void Collective::setTrait(Creature* c, MinionTrait t) {
  if (!hasTrait(c, t)) {
    byTrait[t].push_back(c);
    updateCreatureStatus(c);
  }
}

void Collective::removeTrait(Creature* c, MinionTrait t) {
  if (byTrait[t].removeElementMaybe(c))
    updateCreatureStatus(c);
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

const optional<Collective::AlarmInfo>& Collective::getAlarmInfo() const {
  return alarmInfo;
}

void Collective::onEvent(const GameEvent& event) {
  PROFILE;
  using namespace EventInfo;
  event.visit(
      [&](const Alarm& info) {
        static const auto alarmTime = 100_visible;
        if (getTerritory().contains(info.pos)) {
          if (!info.silent)
            control->addMessage(PlayerMessage("An alarm goes off.", MessagePriority::HIGH).setPosition(info.pos));
          alarmInfo = AlarmInfo {getGlobalTime() + alarmTime, info.pos };
          for (Creature* c : byTrait[MinionTrait::FIGHTER])
            if (c->isAffected(LastingEffect::SLEEP))
              c->removeEffect(LastingEffect::SLEEP);
        }
      },
      [&](const CreatureKilled& info) {
        if (getCreatures().contains(info.victim))
          onMinionKilled(info.victim, info.attacker);
        if (getCreatures().contains(info.attacker))
          onKilledSomeone(info.attacker, info.victim);
      },
      [&](const CreatureTortured& info) {
        auto victim = info.victim;
        if (getCreatures().contains(victim)) {
          if (Random.roll(30)) {
            if (Random.roll(2)) {
              victim->dieWithReason("killed by torture");
            } else {
              control->addMessage("A prisoner is converted to your side");
              removeTrait(victim, MinionTrait::PRISONER);
              removeTrait(victim, MinionTrait::WORKER);
              removeTrait(victim, MinionTrait::NO_LIMIT);
              setTrait(victim, MinionTrait::FIGHTER);
              victim->removeEffect(LastingEffect::TIED_UP);
            }
          }
        }
      },
      [&](const CreatureStunned& info) {
        auto victim = info.victim;
        if (getCreatures().contains(victim)) {
          bool fighterStunned = hasTrait(victim, MinionTrait::FIGHTER) || victim == getLeader();
          removeTrait(victim, MinionTrait::FIGHTER);
          removeTrait(victim, MinionTrait::LEADER);
          control->addMessage(PlayerMessage(victim->getName().a() + " is unconsious.")
              .setPosition(victim->getPosition()));
          if (isConquered() && fighterStunned)
            getGame()->addEvent(EventInfo::ConqueredEnemy{this});
          freeFromTask(victim);
        }
      },
      [&](const TrapTriggered& info) {
        if (auto trap = constructions->getTrap(info.pos)) {
          trap->reset();
          if (trap->getType() == TrapType::SURPRISE)
            handleSurprise(info.pos);
        }
      },
      [&](const TrapDisarmed& info) {
        if (auto trap = constructions->getTrap(info.pos)) {
          control->addMessage(PlayerMessage(info.creature->getName().a() +
              " disarms a " + getTrapName(trap->getType()) + " trap.",
              MessagePriority::HIGH).setPosition(info.pos));
          trap->reset();
        }
      },
      [&](const MovementChanged& info) {
        positionMatching->updateMovement(info.pos);
      },
      [&](const FurnitureDestroyed& info) {
        if (info.position.getModel() == model) {
          populationIncrease -= Furniture::getPopulationIncrease(info.type, constructions->getBuiltCount(info.type));
          constructions->onFurnitureDestroyed(info.position, info.layer);
          populationIncrease += Furniture::getPopulationIncrease(info.type, constructions->getBuiltCount(info.type));
        }
      },
      [&](const ConqueredEnemy& info) {
        auto col = info.collective;
        if (col->isDiscoverable()) {
          if (auto& name = col->getName())
            control->addMessage(PlayerMessage("The tribe of " + name->full + " is destroyed.",
                MessagePriority::CRITICAL));
          else
            control->addMessage(PlayerMessage("An unnamed tribe is destroyed.", MessagePriority::CRITICAL));
          dungeonLevel.onKilledVillain(col->getVillainType());
        }
      },
      [&](const auto&) {}
  );
}

void Collective::onMinionKilled(Creature* victim, Creature* killer) {
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

void Collective::onKilledSomeone(Creature* killer, Creature* victim) {
  string deathDescription=victim->getAttributes().getDeathDescription();
  if (victim->getTribe() != getTribe()) {
    addMoraleForKill(killer, victim);
    kills.insert(victim);
    int difficulty = victim->getDifficultyPoints();
    CHECK(difficulty >=0 && difficulty < 100000) << difficulty << " " << victim->getName().bare();
    points += difficulty;
    control->addMessage(PlayerMessage(victim->getName().a() + " is " + deathDescription + " by " + killer->getName().a())
        .setPosition(victim->getPosition()));
  }
}

double Collective::getEfficiency(const Creature* c) const {
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
      pos.canEnterEmpty({MovementTrait::WALK}) &&
      !pos.isWall();
}

void Collective::claimSquare(Position pos) {
  //CHECK(canClaimSquare(pos));
  territory->insert(pos);
  addKnownTile(pos);
  for (auto furniture : pos.modFurniture())
    if (!furniture->forgetAfterBuilding()) {
      if (!constructions->containsFurniture(pos, furniture->getLayer()))
        constructions->addFurniture(pos, ConstructionMap::FurnitureInfo::getBuilt(furniture->getType()));
      furniture->setTribe(getTribeId());
    }
  control->onClaimedSquare(pos);
}

const KnownTiles& Collective::getKnownTiles() const {
  return *knownTiles;
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
    if (auto storage = config->getResourceInfo(id).storageId)
      for (auto& pos : getStoragePositions(*storage))
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
    if (auto storage = config->getResourceInfo(cost.id).storageId)
      for (auto& pos : getStoragePositions(*storage)) {
        vector<Item*> goldHere = pos.getItems(*itemIndex);
        for (Item* it : goldHere) {
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
  if (auto storage = config->getResourceInfo(amount.id).storageId) {
    const auto& destination = getStoragePositions(*storage);
    if (!destination.empty()) {
      Random.choose(destination).dropItems(config->getResourceInfo(amount.id).itemId.get(amount.value));
      return;
    }
  }
  credit[amount.id] += amount.value;
}

vector<pair<Item*, Position>> Collective::getTrapItems(const vector<Position>& squares) const {
  PROFILE;
  vector<pair<Item*, Position>> ret;
  for (Position pos : squares)
    for (auto it : pos.getItems(ItemIndex::TRAP))
      if (!isItemMarked(it))
        ret.emplace_back(it, pos);
  return ret;
}

bool Collective::usesEquipment(const Creature* c) const {
  return config->getManageEquipment()
    && c->getBody().isHumanoid() && !hasTrait(c, MinionTrait::NO_EQUIPMENT)
    && !hasTrait(c, MinionTrait::PRISONER);
}

vector<Item*> Collective::getAllItems(bool includeMinions) const {
  return getAllItemsImpl(none, includeMinions);
}

vector<Item*> Collective::getAllItems(ItemIndex index, bool includeMinions) const {
  return getAllItemsImpl(index, includeMinions);
}

vector<Item*> Collective::getAllItemsImpl(optional<ItemIndex> index, bool includeMinions) const {
  vector<Item*> allItems;
  for (auto& v : territory->getAll())
    append(allItems, index ? v.getItems(*index) : v.getItems());
  for (auto& v : zones->getPositions(ZoneId::STORAGE_EQUIPMENT))
    if (!territory->contains(v))
      append(allItems, v.getItems());
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, index ? c->getEquipment().getItems(*index) : c->getEquipment().getItems());
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

const PositionSet& Collective::getStorageForPillagedItem(const Item* item) const {
  for (auto& info : config->getFetchInfo())
    if (hasIndex(info.index, item))
      return getStoragePositions(info.storageId);
  return zones->getPositions(ZoneId::STORAGE_EQUIPMENT);
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

bool Collective::isItemMarked(const Item* it) const {
  return !!markedItems.getOrElse(it, nullptr);
}

void Collective::markItem(const Item* it, WConstTask task) {
  markedItems.set(it, task);
}

void Collective::removeTrap(Position pos) {
  constructions->removeTrap(pos);
}

bool Collective::canAddFurniture(Position position, FurnitureType type) const {
  auto layer = Furniture::getLayer(type);
  return knownTiles->isKnown(position)
      && (territory->contains(position) ||
          canClaimSquare(position) ||
          CollectiveConfig::canBuildOutsideTerritory(type))
      && (!getConstructions().getTrap(position) || layer != FurnitureLayer::MIDDLE)
      && !getConstructions().containsFurniture(position, layer)
      && position.canConstruct(type);
}

void Collective::removeFurniture(Position pos, FurnitureLayer layer) {
  if (auto f = constructions->getFurniture(pos, layer)) {
    if (f->hasTask())
      returnResource(taskMap->removeTask(f->getTask()));
    constructions->removeFurniture(pos, layer);
  }
}

void Collective::destroyOrder(Position pos, FurnitureLayer layer) {
  auto furniture = pos.modFurniture(layer);
  if (!furniture || furniture->canRemoveWithCreaturePresent() || !pos.getCreature()) {
    if (furniture && !furniture->isWall() && !furniture->forgetAfterBuilding() &&
        (furniture->getTribe() == getTribeId() || furniture->canRemoveNonFriendly())) {
      furniture->destroy(pos, DestroyAction::Type::BASH);
    }
    removeFurniture(pos, layer);
  }
  if (layer == FurnitureLayer::MIDDLE) {
    zones->onDestroyOrder(pos);
    if (constructions->getTrap(pos))
      removeTrap(pos);
  }
}

void Collective::addFurniture(Position pos, FurnitureType type, const CostInfo& cost, bool noCredit) {
  if (!noCredit || hasResource(cost)) {
    constructions->addFurniture(pos, ConstructionMap::FurnitureInfo(type, cost));
    updateConstructions();
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
  removeFurniture(pos, FurnitureLayer::MIDDLE);
  auto f = NOTNULL(pos.getFurniture(FurnitureLayer::MIDDLE));
  CHECK(f->canDestroy(action));
  taskMap->markSquare(pos, getHighlight(action), Task::destruction(this, pos, f, action,
      pos.canEnterEmpty({MovementTrait::WALK}) ? nullptr : positionMatching.get()),
      action.getMinionActivity());
}

void Collective::addTrap(Position pos, TrapType type) {
  constructions->addTrap(pos, ConstructionMap::TrapInfo(type));
  updateConstructions();
}

void Collective::onAppliedItem(Position pos, Item* item) {
  CHECK(item->getTrapType());
  if (auto trap = constructions->getTrap(pos))
    trap->setArmed();
}

bool Collective::isConstructionReachable(Position pos) {
  PROFILE;
  for (Position v : pos.neighbors8())
    if (knownTiles->isKnown(v))
      return true;
  return false;
}

void Collective::onConstructed(Position pos, FurnitureType type) {
  if (pos.getFurniture(type)->forgetAfterBuilding()) {
    constructions->removeFurniture(pos, Furniture::getLayer(type));
    if (territory->contains(pos))
      territory->remove(pos);
    return;
  }
  populationIncrease -= Furniture::getPopulationIncrease(type, constructions->getBuiltCount(type));
  constructions->onConstructed(pos, type);
  populationIncrease += Furniture::getPopulationIncrease(type, constructions->getBuiltCount(type));
  control->onConstructed(pos, type);
  if (WTask task = taskMap->getMarked(pos))
    taskMap->removeTask(task);
  //if (canClaimSquare(pos))
    claimSquare(pos);
}

void Collective::onDestructed(Position pos, FurnitureType type, const DestroyAction& action) {
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
  PROFILE;
  EnumMap<TrapType, vector<pair<Item*, Position>>> trapItems;
  for (auto& elem : getTrapItems(territory->getAll()))
    trapItems[*elem.first->getTrapType()].push_back(elem);
  EnumMap<TrapType, int> missingTraps;
  for (auto trapPos : constructions->getAllTraps()) {
    auto& trap = *constructions->getTrap(trapPos);
    if (!trap.isArmed() && !trap.isMarked() && !isDelayed(trapPos)) {
      vector<pair<Item*, Position>>& items = trapItems[trap.getType()];
      if (!items.empty()) {
        Position pos = items.back().second;
        auto item = items.back().first;
        auto task = taskMap->addTask(Task::chain(Task::pickUpItem(pos, {item}), Task::applyItem(this, trapPos, item)), pos,
            MinionActivity::CONSTRUCTION);
        markItem(items.back().first, task);
        items.pop_back();
        trap.setMarked();
      } else
        ++missingTraps[trap.getType()];
    }
  }
  for (TrapType type : ENUM_ALL(TrapType))
    scheduleAutoProduction([type](const Item* it) { return it->getTrapType() == type;}, missingTraps[type]);
}

void Collective::scheduleAutoProduction(function<bool(const Item*)> itemPredicate, int count) {
  if (count > 0)
    for (auto workshopType : ENUM_ALL(WorkshopType))
      for (auto& item : workshops->get(workshopType).getQueued())
        if (itemPredicate(item.item.type.get().get()))
          count -= item.number * item.item.batchSize;
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

void Collective::updateConstructions() {
  PROFILE;
  handleTrapPlacementAndProduction();
  for (auto& pos : constructions->getAllFurniture()) {
    auto& construction = *constructions->getFurniture(pos.first, pos.second);
    if (!isDelayed(pos.first) &&
        !construction.hasTask() &&
        !construction.isBuilt(pos.first) &&
        hasResource(construction.getCost())) {
      constructions->setTask(pos.first, pos.second,
          taskMap->addTaskCost(Task::construction(this, pos.first, construction.getFurnitureType()), pos.first,
              construction.getCost(), MinionActivity::CONSTRUCTION)->getUniqueId());
      takeResource(construction.getCost());
    }
  }
}

void Collective::delayDangerousTasks(const vector<Position>& enemyPos1, LocalTime delayTime) {
  PROFILE;
  unordered_set<WLevel, CustomHash<WLevel>> levels;
  for (auto& pos : enemyPos1)
    levels.insert(pos.getLevel());
  for (auto& level : levels) {
    vector<Vec2> enemyPos = enemyPos1
        .filter([=] (const Position& p) { return p.isSameLevel(level); })
        .transform([] (const Position& p) { return p.getCoord();});
    int infinity = 1000000;
    int radius = 10;
    Table<int> dist(Rectangle::boundingBox(enemyPos)
        .minusMargin(-radius)
        .intersection(level->getBounds()), infinity);
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
}

bool Collective::isDelayed(Position pos) {
  PROFILE
  return delayedPos.count(pos) && delayedPos.at(pos) > getLocalTime();
}

static Position chooseClosest(Position pos, const PositionSet& squares) {
  optional<Position> ret;
  for (auto& p : squares)
    if (!ret || pos.dist8(p).value_or(10000) < pos.dist8(*ret).value_or(10000))
      ret = p;
  return *ret;
}

const PositionSet& Collective::getStoragePositions(StorageId storage) const {
  switch (storage) {
    case StorageId::RESOURCE:
      return zones->getPositions(ZoneId::STORAGE_RESOURCES);
    case StorageId::EQUIPMENT:
      return zones->getPositions(ZoneId::STORAGE_EQUIPMENT);
    case StorageId::GOLD:
      return constructions->getBuiltPositions(FurnitureType::TREASURE_CHEST);
    case StorageId::CORPSES:
      return constructions->getBuiltPositions(FurnitureType::GRAVE);
  }
}

void Collective::fetchItems(Position pos, const ItemFetchInfo& elem) {
  PROFILE;
  const auto& destination = getStoragePositions(elem.storageId);
  if (destination.count(pos))
    return;
  vector<Item*> equipment = pos.getItems(elem.index).filter(
      [this, &elem] (const Item* item) { return elem.predicate(this, item); });
  if (!equipment.empty()) {
    if (!destination.empty()) {
      warnings->setWarning(elem.warning, false);
      auto pickUpAndDrop = Task::pickUpAndDrop(pos, equipment, elem.storageId, this);
      auto task = taskMap->addTask(std::move(pickUpAndDrop.pickUp), pos, MinionActivity::HAULING);
      taskMap->addTask(std::move(pickUpAndDrop.drop), chooseClosest(pos, destination), MinionActivity::HAULING);
      for (Item* it : equipment)
        markItem(it, task);
    } else
      warnings->setWarning(elem.warning, true);
  }
}

void Collective::handleSurprise(Position pos) {
  Vec2 rad(8, 8);
  for (Position v : Random.permutation(pos.getRectangle(Rectangle(-rad, rad + Vec2(1, 1)))))
    if (Creature* other = v.getCreature())
      if (hasTrait(other, MinionTrait::FIGHTER) && *v.dist8(pos) > 1) {
        for (Position dest : pos.neighbors8(Random))
          if (v.canMoveCreature(dest)) {
            v.moveCreature(dest, true);
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
    if (WTask task = taskMap->getMarked(pos))
      if (task->isBogus())
        taskMap->removeTask(task);
    return true;
  } else
    return false;
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
    // Furniture have variable usage time, so just multiply by it to be independent of changes.
    double efficiency = furniture->getUsageTime().getVisibleDouble() * getEfficiency(c) * pos.getLightingEfficiency();
    switch (furniture->getType()) {
      case FurnitureType::THRONE:
        if (config->getRegenerateMana())
          dungeonLevel.onLibraryWork(efficiency);
        break;
      case FurnitureType::WHIPPING_POST:
        taskMap->addTask(Task::whipping(pos, c), pos, MinionActivity::WORKING);
        break;
      case FurnitureType::GALLOWS:
        taskMap->addTask(Task::kill(this, c), pos, MinionActivity::WORKING);
        break;
      case FurnitureType::TORTURE_TABLE:
        taskMap->addTask(Task::torture(this, c), pos, MinionActivity::WORKING);
        break;
      default:
        break;
    }
    if (auto usage = furniture->getUsageType()) {
      auto increaseLevel = [&] (ExperienceType exp) {
        double increase = 0.007 * efficiency * LastingEffects::getTrainingSpeed(c);
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
            dungeonLevel.onLibraryWork(efficiency);
          break;
        case FurnitureUsageType::ARCHERY_RANGE:
          increaseLevel(ExperienceType::ARCHERY);
          break;
        default:
          break;
      }
    }
    if (auto workshopType = config->getWorkshopType(furniture->getType())) {
      auto& workshop = workshops->get(*workshopType);
      auto& info = config->getWorkshopInfo(*workshopType);
      auto craftingSkill = c->getAttributes().getSkills().getValue(info.skill);
      auto result = workshop.addWork(this, efficiency * craftingSkill * LastingEffects::getCraftingSpeed(c),
          craftingSkill, c->getMorale());
      if (!result.items.empty()) {
        if (result.items[0]->getClass() == ItemClass::WEAPON)
          getGame()->getStatistics().add(StatId::WEAPON_PRODUCED);
        if (result.items[0]->getClass() == ItemClass::ARMOR)
          getGame()->getStatistics().add(StatId::ARMOR_PRODUCED);
        if (result.items[0]->getClass() == ItemClass::POTION)
          getGame()->getStatistics().add(StatId::POTION_PRODUCED);
        addProducesMessage(c, result.items);
        if (result.wasUpgraded) {
          control->addMessage(PlayerMessage(c->getName().the() + " is depressed after crafting his masterpiece.", MessagePriority::HIGH));
          c->addMorale(-2);
        }
        c->getPosition().dropItems(std::move(result.items));
      }
    }
  }
}

optional<FurnitureType> Collective::getMissingTrainingFurniture(const Creature* c, ExperienceType expType) const {
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
    for (const Creature* c : getCreatures(MinionTrait::FIGHTER))
      ret += c->getDifficultyPoints();
    ret += constructions->getBuiltCount(FurnitureType::IMPALED_HEAD) * 150;
    dangerLevelCache = ret;
  }
  return *dangerLevelCache;
}

void Collective::acquireTech(TechId tech, bool throughLevelling) {
  technology->researched.insert(tech);
  if (throughLevelling)
    ++dungeonLevel.consumedLevels;
}

const Technology& Collective::getTechnology() const {
  return *technology;
}

void Collective::setTechnology(Technology t) {
  technology = std::move(t);
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
  dungeonLevel.onKilledWave();
}

void Collective::onCopulated(Creature* who, Creature* with) {
  PROFILE;
  if (with->getName().bare() == "vampire")
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a()
        + " with a monkey on " + his(who->getAttributes().getGender()) + " knee");
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

void Collective::freeTeamMembers(const vector<Creature*>& members) {
  PROFILE;
  for (Creature* c : members) {
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
  return populationGroups.size() - getCreatures(MinionTrait::NO_LIMIT).size();
}

int Collective::getMaxPopulation() const {
  return populationIncrease + config->getMaxPopulation() + getCreatures(MinionTrait::INCREASE_POPULATION).size();
}

const DungeonLevel& Collective::getDungeonLevel() const {
  return dungeonLevel;
}

DungeonLevel& Collective::getDungeonLevel() {
  return dungeonLevel;
}

REGISTER_TYPE(Collective)
REGISTER_TYPE(ListenerTemplate<Collective>)
