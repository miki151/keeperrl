#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"
#include "pantheon.h"
#include "effect.h"
#include "level.h"
#include "square.h"
#include "item.h"
#include "item_factory.h"
#include "statistics.h"
#include "technology.h"
#include "monster.h"
#include "options.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Task::Callback)
    & SVAR(creatures)
    & SVAR(taskMap)
    & SVAR(tribe)
    & SVAR(control)
    & SVAR(byTrait)
    & SVAR(bySpawnType)
    & SVAR(deityStanding)
    & SVAR(mySquares)
    & SVAR(allSquares)
    & SVAR(traps)
    & SVAR(squareEfficiency)
    & SVAR(alarmInfo)
    & SVAR(guardPosts)
    & SVAR(markedItems)
    & SVAR(constructions)
    & SVAR(minionEquipment)
    & SVAR(prisonerInfo)
    & SVAR(minionTaskStrings)
    & SVAR(delayedPos)
    & SVAR(knownTiles)
    & SVAR(technologies)
    & SVAR(numFreeTech)
    & SVAR(borderTiles)
    & SVAR(lastCombat)
    & SVAR(kills)
    & SVAR(points)
    & SVAR(currentTasks)
    & SVAR(configId)
    & SVAR(credit)
    & SVAR(level)
    & SVAR(minionPayment)
    & SVAR(flyingSectors)
    & SVAR(sectors)
    & SVAR(pregnancies)
    & SVAR(nextPayoutTime);
  CHECK_SERIAL;
}

SERIALIZABLE(Collective);

SERIALIZATION_CONSTRUCTOR_IMPL(Collective);

void Collective::setWarning(Warning w, bool state) {
  warnings[w] = state;
}

bool Collective::isWarning(Warning w) const {
  return warnings[w];
}

Collective::MinionTaskInfo::MinionTaskInfo(vector<SquareType> s, const string& desc, Optional<Warning> w,
    double _cost, bool center) 
    : type(APPLY_SQUARE), squares(s), description(desc), warning(w), cost(_cost), centerOnly(center) {
}

Collective::MinionTaskInfo::MinionTaskInfo(Type t, const string& desc) : type(t), description(desc) {
  CHECK(type != APPLY_SQUARE);
}

ItemPredicate Collective::unMarkedItems(ItemClass type) const {
  return [this, type](const Item* it) {
      return it->getClass() == type && !isItemMarked(it); };
}

const static vector<SquareType> resourceStorage {SquareId::STOCKPILE, SquareId::STOCKPILE_RES};
const static vector<SquareType> equipmentStorage {SquareId::STOCKPILE, SquareId::STOCKPILE_EQUIP};

vector<SquareType> Collective::getEquipmentStorageSquares() {
  return equipmentStorage;
}

vector<Collective::ItemFetchInfo>& Collective::getFetchInfo() const {
  static vector<ItemFetchInfo> fetchInfo {
    {unMarkedItems(ItemClass::CORPSE), {SquareId::CEMETERY}, true, {}, Warning::GRAVES},
    {unMarkedItems(ItemClass::GOLD), {SquareId::TREASURE_CHEST}, false, {}, Warning::CHESTS},
    {[this](const Item* it) {
        return minionEquipment.isItemUseful(it) && it->getClass() != ItemClass::GOLD && !isItemMarked(it);
      }, equipmentStorage, false, {}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getResourceId() == ResourceId::WOOD && !isItemMarked(it); },
    resourceStorage, false, {SquareId::TREE_TRUNK}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getResourceId() == ResourceId::IRON && !isItemMarked(it); },
    resourceStorage, false, {}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getResourceId() == ResourceId::STONE && !isItemMarked(it); },
    resourceStorage, false, {}, Warning::STORAGE},
  };
  return fetchInfo;
}


const map<Collective::ResourceId, Collective::ResourceInfo> Collective::resourceInfo {
  {ResourceId::MANA, { {}, nullptr, ItemId::GOLD_PIECE, "mana", Collective::Warning::MANA}},
  {ResourceId::PRISONER_HEAD, { {}, nullptr, ItemId::GOLD_PIECE, "", Nothing(), true}},
  {ResourceId::GOLD, { {SquareId::TREASURE_CHEST}, Item::classPredicate(ItemClass::GOLD), ItemId::GOLD_PIECE, "gold",
                       Collective::Warning::GOLD}},
  {ResourceId::WOOD, { resourceStorage, [](const Item* it) {
        return it->getResourceId() == ResourceId::WOOD; },
                       ItemId::WOOD_PLANK, "wood", Collective::Warning::WOOD}},
  {ResourceId::IRON, { resourceStorage, [](const Item* it) {
        return it->getResourceId() == ResourceId::IRON; },
                       ItemId::IRON_ORE, "iron", Collective::Warning::IRON}},
  {ResourceId::STONE, { resourceStorage, [](const Item* it) {
        return it->getResourceId() == ResourceId::STONE; }, ItemId::ROCK, "stone",
                       Collective::Warning::STONE}},
  {ResourceId::CORPSE, {
      {SquareId::CEMETERY},
      [](const Item* it) {
          return it->getClass() == ItemClass::CORPSE && it->getCorpseInfo()->canBeRevived; },
      ItemId::GOLD_PIECE,
      "corpses", Nothing(), true}},
};

map<MinionTask, Collective::MinionTaskInfo> Collective::getTaskInfo() const {
  map<MinionTask, MinionTaskInfo> ret {
    {MinionTask::LABORATORY, {{SquareId::LABORATORY}, "lab", Collective::Warning::LABORATORY, 1}},
    {MinionTask::TRAIN, {{SquareId::TRAINING_ROOM}, "training", Collective::Warning::TRAINING, 1}},
    {MinionTask::WORKSHOP, {{SquareId::WORKSHOP}, "crafting", Collective::Warning::WORKSHOP, 1}},
    {MinionTask::SLEEP, {{SquareId::BED}, "sleeping", Collective::Warning::BEDS}},
    {MinionTask::GRAVE, {{SquareId::GRAVE}, "sleeping", Collective::Warning::GRAVES}},
    {MinionTask::LAIR, {{SquareId::BEAST_CAGE}, "sleeping"}},
    {MinionTask::STUDY, {{SquareId::LIBRARY}, "studying", Collective::Warning::LIBRARY, 1}},
    {MinionTask::PRISON, {{SquareId::PRISON}, "prison", Collective::Warning::NO_PRISON}},
    {MinionTask::TORTURE, {{SquareId::TORTURE_TABLE}, "torture ordered", Collective::Warning::TORTURE_ROOM, 0, true}},
    {MinionTask::RITUAL, {{SquareId::RITUAL_ROOM}, "rituals"}},
    {MinionTask::COPULATE, {MinionTaskInfo::COPULATE, "copulation"}},
    {MinionTask::CONSUME, {MinionTaskInfo::CONSUME, "consumption"}},
    {MinionTask::EXPLORE, {MinionTaskInfo::EXPLORE, "spying"}},
    {MinionTask::SACRIFICE, {{}, "sacrifice ordered", Collective::Warning::ALTAR}},
    {MinionTask::EXECUTE, {{SquareId::PRISON}, "execution ordered", Collective::Warning::NO_PRISON}},
    {MinionTask::WORSHIP, {{}, "worship", Nothing(), 1}}};
  for (SquareType t : getSquareTypes())
    if (contains({SquareId::ALTAR, SquareId::CREATURE_ALTAR}, t.getId()) && !getSquares(t).empty()) {
      ret.at(MinionTask::WORSHIP).squares.push_back(t);
      ret.at(MinionTask::SACRIFICE).squares.push_back(t);
    }
  return ret;
};

struct Collective::AttractionInfo {
  MinionAttraction attraction;
  double amountClaimed;
  double minAmount;
};

struct Collective::ImmigrantInfo {
  CreatureId id;
  double frequency;
  vector<AttractionInfo> attractions;
  EnumSet<MinionTrait> traits;
  bool spawnAtDorm;
  int salary;
  Optional<CostInfo> cost;
  Optional<TechId> techId;
};

struct CollectiveConfig {
  bool manageEquipment;
  bool workerFollowLeader;
  double immigrantFrequency;
  int payoutTime;
  double payoutMultiplier;
  bool stripSpawns;
  bool keepSectors;
  vector<Collective::ImmigrantInfo> immigrantInfo;
};

Collective::Collective(Level* l, CollectiveConfigId cfg, Tribe* t) : configId(cfg),
  knownTiles(l->getBounds(), false), control(CollectiveControl::idle(this)),
  tribe(t), level(l), nextPayoutTime(getConfig().payoutTime), sectors(new Sectors(level->getBounds())),
    flyingSectors(new Sectors(level->getBounds())) {
  credit = {
    {ResourceId::MANA, 200},
  };
  if (Options::getValue(OptionId::STARTING_RESOURCE)) {
    for (auto elem : ENUM_ALL(ResourceId))
      credit[elem] = 10000;
  }
  if (getConfig().keepSectors)
    for (Vec2 v : level->getBounds()) {
      if (getLevel()->getSquare(v)->canEnterEmpty({MovementTrait::WALK}))
        sectors->add(v);
      if (getLevel()->getSquare(v)->canEnterEmpty({{MovementTrait::WALK, MovementTrait::FLY}}))
        flyingSectors->add(v);
    }
}

const CollectiveConfig& Collective::getConfig() const {

  static EnumMap<CollectiveConfigId, CollectiveConfig> collectiveConfigs {
    {CollectiveConfigId::KEEPER,
      CONSTRUCT(CollectiveConfig,
        c.manageEquipment = true;
        c.workerFollowLeader = true;
        c.immigrantFrequency = 0.011;
        c.payoutTime = 500;
        c.payoutMultiplier = 3;
        c.stripSpawns = true;
        c.keepSectors = true;
        c.immigrantInfo = LIST(
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::GNOME;
            c.frequency = 1;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::WORKSHOP}, 1.0, 12.0},
            );
            c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
            c.salary = 10;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::GOBLIN;
            c.frequency = 0.7;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 1.0, 12.0},
            );
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 20;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::GOBLIN_SHAMAN;
            c.frequency = 0.3;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::LIBRARY}, 1.0, 16.0},
              {{AttractionId::SQUARE, SquareId::LABORATORY}, 1.0, 9.0},
            );
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 20;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::OGRE;
            c.frequency = 0.3;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0}
            );
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 40;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::HARPY;
            c.frequency = 0.3;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
              {{AttractionId::ITEM_CLASS, ItemClass::RANGED_WEAPON}, 1.0, 3.0}
            );
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 40;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::SPECIAL_HUMANOID;
            c.frequency = 0.2;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            );
            c.traits = {MinionTrait::FIGHTER};
            c.spawnAtDorm = true;
            c.techId = TechId::HUMANOID_MUT;
            c.salary = 40;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ZOMBIE;
            c.frequency = 0.5;
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 10;
            c.spawnAtDorm = true;
            c.cost = CostInfo(ResourceId::CORPSE, 1);),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::VAMPIRE;
            c.frequency = 0.3;
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 40;
            c.spawnAtDorm = true;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 2.0, 12.0}
            );
            c.cost = CostInfo(ResourceId::CORPSE, 1);),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::LOST_SOUL;
            c.frequency = 0.3;
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 0;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 1.0, 9.0}
            );
            c.spawnAtDorm = true;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::SUCCUBUS;
            c.frequency = 0.3;
            c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
            c.salary = 0;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 2.0, 12.0}
            );
            c.spawnAtDorm = true;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::DOPPLEGANGER;
            c.frequency = 0.2;
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 0;
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 4.0, 12.0}
            );
            c.spawnAtDorm = true;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::RAVEN;
            c.frequency = 2.0;
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 0;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::WOLF;
            c.frequency = 0.3;
            c.traits = {MinionTrait::FIGHTER};
            c.salary = 0;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::WEREWOLF;
            c.frequency = 0.1;
            c.traits = {MinionTrait::FIGHTER};
            c.attractions = LIST(
              {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 4.0, 12.0}
            );
            c.salary = 0;),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::SPECIAL_MONSTER_KEEPER;
            c.frequency = 0.2;
            c.traits = {MinionTrait::FIGHTER};
            c.spawnAtDorm = true;
            c.techId = TechId::BEAST_MUT;
            c.salary = 0;),
        );)},
    {CollectiveConfigId::VILLAGE, {}},
  };
  return collectiveConfigs[configId];
}

Collective::~Collective() {
}

double Collective::getAttractionOccupation(MinionAttraction attraction) {
  double res = 0;
  for (auto& elem : minionAttraction)
    for (auto& info : elem.second)
      if (info.attraction == attraction)
        res += info.amountClaimed;
  return res;
}

double Collective::getAttractionValue(MinionAttraction attraction) {
  switch (attraction.getId()) {
    case AttractionId::SQUARE: 
      return getSquares(attraction.get<SquareType>()).size();
    case AttractionId::ITEM_CLASS: 
      return getAllItems([&](const Item* it) {
          return it->getClass() == attraction.get<ItemClass>(); }, true).size();
  }
  FAIL << "wefok";
  return 0;
}

void Collective::addCreature(PCreature creature, Vec2 pos, EnumSet<MinionTrait> traits) {
  if (getConfig().stripSpawns)
    creature->getEquipment().removeAllItems();
  Creature* c = creature.get();
  getLevel()->addCreature(pos, std::move(creature));
  addCreature(c, traits);
}

void Collective::addCreature(Creature* c, EnumSet<MinionTrait> traits) {
  if (traits[MinionTrait::FIGHTER] || traits[MinionTrait::WORKER])
    c->setController(PController(new Monster(c, MonsterAIFactory::collective(this))));
  if (creatures.empty())
    traits.insert(MinionTrait::LEADER);
  CHECK(c->getTribe() == tribe);
  creatures.push_back(c);
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
  if (auto spawnType = c->getSpawnType())
    bySpawnType[*spawnType].push_back(c);
  for (const Item* item : c->getEquipment().getItems())
    minionEquipment.own(c, item);
  for (Technology* t : technologies)
    if (Skill* skill = t->getSkill())
      c->addSkill(skill);
  if (traits[MinionTrait::PRISONER])
    prisonerInfo[c] = {PrisonerState::PRISON, 0};
  if (getConfig().keepSectors) {
    if (!c->isAffected(LastingEffect::FLYING))
      c->addSectors(sectors.get());
    else
      c->addSectors(flyingSectors.get());
  }
}

vector<Creature*>& Collective::getCreatures() {
  return creatures;
}

const Creature* Collective::getLeader() const {
  if (byTrait[MinionTrait::LEADER].empty()) {
    return nullptr;
  } else
    return getOnlyElement(byTrait[MinionTrait::LEADER]);
}

Creature* Collective::getLeader() {
  if (byTrait[MinionTrait::LEADER].empty())
    return nullptr;
  else
    return getOnlyElement(byTrait[MinionTrait::LEADER]);
}

Level* Collective::getLevel() {
  return NOTNULL(level);
}

const Level* Collective::getLevel() const {
  return NOTNULL(level);
}

const Tribe* Collective::getTribe() const {
  return NOTNULL(tribe);
}

Tribe* Collective::getTribe() {
  return NOTNULL(tribe);
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

double Collective::getWarLevel() const {
  return control->getWarLevel();
}

MoveInfo Collective::getDropItems(Creature *c) {
  if (containsSquare(c->getPosition())) {
    vector<Item*> items = c->getEquipment().getItems([this, c](const Item* item) {
        return minionEquipment.isItemUseful(item) && minionEquipment.getOwner(item) != c; });
    if (!items.empty())
      return c->drop(items);
  }
  return NoMove;
}

PTask Collective::getPrisonerTask(Creature* c) {
  if (hasTrait(c, MinionTrait::FIGHTER))
    for (auto& elem : prisonerInfo)
      if (!elem.second.task()) {
        Creature* prisoner = elem.first;
        PTask t;
        switch (elem.second.state()) {
          case PrisonerState::EXECUTE: t = Task::kill(this, prisoner); break;
          case PrisonerState::TORTURE: t = Task::torture(this, prisoner); break;
          case PrisonerState::SACRIFICE: t = Task::sacrifice(this, prisoner); break;
          default: return nullptr;
        }
        if (t)
          elem.second.task() = t->getUniqueId();
        return t;
      }
  return nullptr;
}

void Collective::onKillCancelled(Creature* c) {
  if (prisonerInfo.count(c))
    prisonerInfo.at(c).task() = 0;
}

void Collective::onBedCreated(Vec2 pos, SquareType fromType, SquareType toType) {
  changeSquareType(pos, fromType, toType);
}

MoveInfo Collective::getWorkerMove(Creature* c) {
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  if (PTask t = control->getNewTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (Task* closest = taskMap.getTaskForWorker(c)) {
    taskMap.takeTask(c, closest);
    return closest->getMove(c);
  } else {
    if (getConfig().workerFollowLeader && getLeader() && !containsSquare(c->getPosition())
        && getLeader()->getLevel() == c->getLevel()) {
      Vec2 leaderPos = getLeader()->getPosition();
      if (leaderPos.dist8(c->getPosition()) < 3)
        return NoMove;
      if (auto action = c->moveTowards(leaderPos))
        return {1.0, action};
      else
        return NoMove;
    } else
      return NoMove;
  }
}

int Collective::getTaskDuration(Creature* c, MinionTask task) const {
  switch (task) {
    case MinionTask::CONSUME:
    case MinionTask::COPULATE:
    case MinionTask::SLEEP: return 1;
    default: return 500 + 250 * c->getMorale();
  }
}

void Collective::setMinionTask(Creature* c, MinionTask task) {
  currentTasks[c->getUniqueId()] = {task, c->getTime() + getTaskDuration(c, task)};
}

static double getSum(EnumMap<MinionTask, double> m) {
  double ret = 0;
  for (MinionTask t : ENUM_ALL(MinionTask))
    ret += m[t];
  return ret;
}

MinionTask Collective::chooseRandomFreeTask(const Creature* c) {
  vector<MinionTask> freeTasks;
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (c->getMinionTasks()[t] > 0 && getTaskInfo().at(t).cost == 0)
      freeTasks.push_back(t);
  return chooseRandom(freeTasks);
}

PTask Collective::getStandardTask(Creature* c) {
  if (getSum(c->getMinionTasks()) == 0)
    return nullptr;
  if (minionPayment.count(c) && minionPayment.at(c).debt() > 0) {
    setMinionTask(c, chooseRandomFreeTask(c));
  } else
  if (!currentTasks.count(c->getUniqueId()) || currentTasks.at(c->getUniqueId()).finishTime() < c->getTime())
    setMinionTask(c, chooseRandom(c->getMinionTasks()));
  MinionTaskInfo info = getTaskInfo().at(currentTasks[c->getUniqueId()].task());
  PTask ret;
  switch (info.type) {
    case MinionTaskInfo::APPLY_SQUARE:
      if (getAllSquares(info.squares, info.centerOnly).empty()) {
        if (info.warning)
          setWarning(*info.warning, true);
        currentTasks.erase(c->getUniqueId());
        return nullptr;
      }
      ret = Task::applySquare(this, getAllSquares(info.squares, info.centerOnly));
      break;
    case MinionTaskInfo::EXPLORE:
      ret = Task::explore(chooseRandom(borderTiles));
      break;
    case MinionTaskInfo::COPULATE:
      if (Creature* target = getCopulationTarget(c))
        ret = Task::copulate(this, target, 20);
      else
        currentTasks.erase(c->getUniqueId());
      break;
    case MinionTaskInfo::CONSUME:
      if (Creature* target = getConsumptionTarget(c))
        ret = Task::consume(this, target);
      else
        currentTasks.erase(c->getUniqueId());
      break;
  }
  if (ret && info.warning)
    setWarning(*info.warning, false);
  minionTaskStrings[c->getUniqueId()] = info.description;
  return ret;
}

Creature* Collective::getCopulationTarget(Creature* succubus) {
  for (Creature* c : randomPermutation(getCreatures(MinionTrait::FIGHTER)))
    if (succubus->canCopulateWith(c))
      return c;
  return nullptr;
}

Creature* Collective::getConsumptionTarget(Creature* consumer) {
  vector<Creature*> v = getConsumptionTargets(consumer);
  if (!v.empty())
    return chooseRandom(v);
  else
    return nullptr;
}

vector<Creature*> Collective::getConsumptionTargets(Creature* consumer) {
  vector<Creature*> ret;
  for (Creature* c : randomPermutation(getCreatures(MinionTrait::FIGHTER)))
    if (consumer->canConsume(c) && c != getLeader())
      ret.push_back(c);
  return ret;
}

void Collective::orderConsumption(Creature* consumer, Creature* who) {
  CHECK(consumer->getMinionTasks()[MinionTask::CONSUME] > 0);
  setMinionTask(who, MinionTask::CONSUME);
  MinionTaskInfo info = getTaskInfo().at(currentTasks[consumer->getUniqueId()].task());
  minionTaskStrings[consumer->getUniqueId()] = info.description;
  taskMap.freeFromTask(consumer);
  taskMap.addTask(Task::consume(this, who), consumer);
}

PTask Collective::getEquipmentTask(Creature* c) {
  if (!getConfig().manageEquipment)
    return nullptr;
  if (usesEquipment(c))
    autoEquipment(c, Random.roll(10));
  if (c != getLeader() || !underAttack())
    for (Vec2 v : getAllSquares(equipmentStorage))
      for (Item* it : getLevel()->getSquare(v)->getItems([this, c] (const Item* it) {
            return minionEquipment.getOwner(it) == c; })) {
        PTask t;
        if (c->equip(it))
          return Task::equipItem(this, v, it);
        else
          return Task::pickItem(this, v, {it});
      }
  return nullptr;
}

PTask Collective::getHealingTask(Creature* c) {
  if (c->getHealth() < 1 && c->canSleep() && !c->isAffected(LastingEffect::POISON))
    for (MinionTask t : {MinionTask::SLEEP, MinionTask::GRAVE, MinionTask::LAIR})
      if (c->getMinionTasks()[t] > 0) {
        vector<Vec2> positions = getAllSquares(getTaskInfo().at(t).squares);
        if (!positions.empty())
          return Task::applySquare(nullptr, positions);
      }
  return nullptr;
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  if (c->getLevel() != getLevel())
    return NoMove;
  if (MoveInfo move = control.get()->getMove(c))
    return move;
  if (hasTrait(c, MinionTrait::WORKER))
    return getWorkerMove(c);
  if (MoveInfo move = getDropItems(c))
    return move;
  if (hasTrait(c, MinionTrait::FIGHTER)) {
    if (MoveInfo move = getAlarmMove(c))
      return move;
    if (MoveInfo move = getGuardPostMove(c))
      return move;
  }
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  if (PTask t = getHealingTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (PTask t = getEquipmentTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (PTask t = getPrisonerTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (PTask t = control->getNewTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (PTask t = getStandardTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  return NoMove;
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

void Collective::considerHealingLeader() {
  if (Deity* deity = Deity::getDeity(EpithetId::HEALTH))
    if (getStanding(deity) > 0)
      if (Random.rollD(5 / getStanding(deity))) {
        if (Creature* leader = getLeader())
          if (leader->getHealth() < 1) {
            leader->you(MsgType::ARE, "healed by " + deity->getName());
            leader->heal(1, true);
          }
      }
}

void Collective::onAttackEvent(Creature* victim, Creature* attacker) {
  Creature* leader = getLeader();
  if (victim == leader) {
    if (Deity* deity = Deity::getDeity(EpithetId::LIGHTNING))
      if (getStanding(deity) > 0)
        if (Random.rollD(5 / getStanding(deity))) {
          attacker->you(MsgType::ARE, " struck by lightning!");
          attacker->bleed(Random.getDouble(0.2, 0.5));
        }
    if (Deity* deity = Deity::getDeity(EpithetId::DEFENSE))
      if (getStanding(deity) > 0)
        if (Random.rollD(10 / getStanding(deity))) {
          victim->globalMessage(
              deity->getName() + " sends " + deity->getGender().his() + " servant with help.");
          Effect::summon(getLevel(), deity->getServant(getTribe()), leader->getPosition(), 1, 30);
        }
  }
}

vector<Vec2> Collective::getExtendedTiles(int maxRadius, int minRadius) const {
  map<Vec2, int> extendedTiles;
  vector<Vec2> extendedQueue;
  for (Vec2 pos : getAllSquares()) {
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !containsSquare(v) && !extendedTiles.count(v) 
          && getLevel()->getSquare(v)->canEnterEmpty({MovementTrait::WALK})) {
        extendedTiles[v] = 1;
        extendedQueue.push_back(v);
      }
  }
  for (int i = 0; i < extendedQueue.size(); ++i) {
    Vec2 pos = extendedQueue[i];
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !containsSquare(v) && !extendedTiles.count(v) 
          && getLevel()->getSquare(v)->canEnterEmpty({MovementTrait::WALK})) {
        int a = extendedTiles[v] = extendedTiles[pos] + 1;
        if (a < maxRadius)
          extendedQueue.push_back(v);
      }
  }
  if (minRadius > 0)
    return filter(extendedQueue, [&] (const Vec2& v) { return extendedTiles.at(v) >= minRadius; });
  return extendedQueue;
}

vector<Vec2> Collective::getEnemyPositions() const {
  vector<Vec2> enemyPos;
  for (Vec2 pos : getExtendedTiles(10))
    if (const Creature* c = getLevel()->getSquare(pos)->getCreature())
      if (c->getTribe() != getTribe())
        enemyPos.push_back(pos);
  return enemyPos;
}

static int countNeighbor(Vec2 pos, const set<Vec2>& squares) {
  int num = 0;
  for (Vec2 v : pos.neighbors8())
    num += squares.count(v);
  return num;
}

Optional<Vec2> Collective::chooseBedPos(const set<Vec2>& lair, const set<Vec2>& beds) {
  vector<Vec2> res;
  for (Vec2 v : lair) {
    if (countNeighbor(v, beds) > 2)
      continue;
    bool bad = false;
    for (Vec2 n : v.neighbors8())
      if (beds.count(n) && countNeighbor(n, beds) >= 2) {
        bad = true;
        break;
      }
    if (!bad)
      res.push_back(v);
  }
  if (!res.empty())
    return chooseRandom(res);
  else
    return Nothing();
}

Optional<SquareType> Collective::getSecondarySquare(SquareType type) {
  switch (type.getId()) {
    case SquareId::DORM: return SquareType(SquareId::BED);
    case SquareId::BEAST_LAIR: return SquareType(SquareId::BEAST_CAGE);
    case SquareId::CEMETERY: return SquareType(SquareId::GRAVE);
    default: return Nothing();
  }
}

struct Collective::DormInfo {
  SquareType dormType;
  Optional<SquareType> getBedType() const {
    return getSecondarySquare(dormType);
  }
  Optional<Collective::Warning> warning;
};

const EnumMap<SpawnType, Collective::DormInfo>& Collective::getDormInfo() {
  static EnumMap<SpawnType, DormInfo> dormInfo {
    {SpawnType::HUMANOID, {SquareId::DORM, Warning::BEDS}},
    {SpawnType::UNDEAD, {SquareId::CEMETERY}},
    {SpawnType::BEAST, {SquareId::BEAST_LAIR}},
    {SpawnType::DEMON, {SquareId::RITUAL_ROOM}},
  };
  return dormInfo;
}

bool Collective::considerImmigrant(const ImmigrantInfo& info) {
  if (info.techId && !hasTech(*info.techId))
    return false;
  PCreature creature = CreatureFactory::fromId(info.id, getTribe(), MonsterAIFactory::collective(this));
  SpawnType spawnType = *creature->getSpawnType();
  SquareType dormType = getDormInfo()[spawnType].dormType;
  if (getSquares(dormType).empty())
    return false;
  Optional<SquareType> bedType = getDormInfo()[spawnType].getBedType();
  bool good = false;
  Optional<Vec2> bedPos;
  for (int i : Range(10)) {
    if (!bedType)
      bedPos = chooseRandom(getSquares(dormType));
    else if (getCreatures(spawnType).size() < getSquares(*bedType).size()) {
      bedPos = chooseRandom(getSquares(*bedType));
    } else
      bedPos = chooseBedPos(getSquares(dormType), getSquares(*bedType));
    if (!info.spawnAtDorm || (bedPos && level->getSquare(*bedPos)->canEnter(creature.get()))) {
      good = true;
      break;
    }
  }
  if (!bedPos || !good)
    return false;
  Vec2 spawnPos;
  if (info.spawnAtDorm)
    spawnPos = *bedPos;
  else {
    vector<Vec2> extendedTiles = getExtendedTiles(20, 10);
    if (extendedTiles.empty())
      return false;
    int cnt = 100;
    do {
      spawnPos = chooseRandom(extendedTiles);
    } while (!getLevel()->getSquare(spawnPos)->canEnter(creature.get()) && --cnt > 0);
    if (cnt == 0) {
      Debug() << "Couldn't spawn immigrant " << creature->getName();
      return false;
    }
  }
  Creature* c = creature.get();
  addCreature(std::move(creature), spawnPos, info.traits);
  if (bedType)
    taskMap.addTask(Task::createBed(this, *bedPos, dormType, *bedType), c);
  minionPayment[c] = {info.salary, 0.0, 0};
  minionAttraction[c] = info.attractions;
  if (info.cost)
    takeResource(*info.cost);
  return true;
}

double Collective::getImmigrantChance(const ImmigrantInfo& info) {
  double result = 0;
  if (info.cost && !hasResource(*info.cost))
    return 0;
  if (info.attractions.empty())
    result = 1;
  for (auto& attraction : info.attractions) {
    double value = getAttractionValue(attraction.attraction);
    if (value < 0.001)
      return 0;
    result += max(0.0, value - getAttractionOccupation(attraction.attraction) - attraction.minAmount);
  }
  return result * info.frequency;
}

void Collective::considerImmigration() {
  vector<double> weights;
  bool ok = false;
  for (auto& elem : getConfig().immigrantInfo) {
    weights.push_back(getImmigrantChance(elem));
    if (weights.back() > 0)
      ok = true;
  }
  if (!ok)
    return;
  for (int i : Range(10))
    if (considerImmigrant(chooseRandom(getConfig().immigrantInfo, weights)))
      break;
}

const Collective::ResourceId paymentResource = Collective::ResourceId::GOLD;

int Collective::getPaymentAmount(const Creature* c) const {
  if (!minionPayment.count(c))
    return 0;
  else
    return getConfig().payoutMultiplier *
      minionPayment.at(c).salary() * minionPayment.at(c).workAmount() / getConfig().payoutTime;
}

int Collective::getNextSalaries() const {
  int ret = 0;
  for (Creature* c : creatures)
    if (minionPayment.count(c))
      ret += getPaymentAmount(c) + minionPayment.at(c).debt();
  return ret;
}

bool Collective::hasMinionDebt() const {
  for (Creature* c : creatures)
    if (minionPayment.count(c) && minionPayment.at(c).debt())
      return true;
  return false;
}

void Collective::makePayouts() {
  for (Creature* c : creatures)
    if (minionPayment.count(c)) {
      minionPayment.at(c).debt() += getPaymentAmount(c);
      minionPayment.at(c).workAmount() = 0;
    }
}

void Collective::cashPayouts() {
  int numGold = numResource(paymentResource);
  bool isDebt = false;
  for (Creature* c : creatures)
    if (minionPayment.count(c))
      if (int payment = minionPayment.at(c).debt()) {
        if (payment < numGold) {
          takeResource({paymentResource, payment});
          numGold -= payment;
          minionPayment.at(c).debt() = 0;
        } else 
          isDebt = true;
      }
  setWarning(Warning::GOLD, isDebt);
 /*     vector<PTask> tasks;
      for (SquareType type : resourceInfo.at(paymentResource).storageType) {
        for (Vec2 pos : getSquares(type)) {
          vector<Item*> items = getLevel()->getSquare(pos)->getItems(resourceInfo.at(paymentResource).predicate);
          if (items.size() >= payment)
            items.resize(payment);
          tasks.push_back(Task::chain(Task::pickItem(this, pos, items), Task::consumeItem(this, items)));
          payment -= items.size();
          if (payment == 0)
            break;
        }
        if (payment == 0)
          break;
      }
      taskMap.addTask(Task::chain(std::move(tasks)), c);*/
}

struct BirthSpawn {
  CreatureId id;
  double frequency;
  Optional<TechId> tech;
};

static vector<BirthSpawn> birthSpawns {
  { CreatureId::GNOME, 1 },
  { CreatureId::GOBLIN, 1 },
  { CreatureId::GOBLIN_SHAMAN, 0.5 },
  { CreatureId::HARPY, 0.5 },
  { CreatureId::OGRE, 0.5 },
  { CreatureId::WEREWOLF, 0.5 },
  { CreatureId::SPECIAL_HUMANOID, 2, TechId::HUMANOID_MUT},
  { CreatureId::SPECIAL_MONSTER_KEEPER, 2, TechId::BEAST_MUT },
};

void Collective::considerBirths() {
  if (!pregnancies.empty() && Random.roll(50)) {
    Creature* c = pregnancies.front();
    pregnancies.pop_front();
    vector<pair<CreatureId, double>> candidates;
    for (auto& elem : birthSpawns)
      if (!elem.tech || hasTech(*elem.tech)) 
        candidates.emplace_back(elem.id, elem.frequency);
    if (candidates.empty())
      return;
    PCreature spawn = CreatureFactory::fromId(chooseRandom(candidates), getTribe());
    for (Vec2 v : c->getPosition().neighbors8(true))
      if (getLevel()->getSquare(v)->canEnter(spawn.get())) {
        Debug() << c->getAName() << " gives birth to " << spawn->getAName();
        addCreature(std::move(spawn), v, {MinionTrait::FIGHTER});
        return;
      }
  }
}

static vector<SquareType> roomsNeedingLight {
  SquareId::WORKSHOP,
  SquareId::TRAINING_ROOM,
  SquareId::LIBRARY,
  SquareId::LABORATORY,
};

void Collective::tick(double time) {
  control->tick(time);
  considerHealingLeader();
  considerBirths();
  if (Random.rollD(1.0 / getConfig().immigrantFrequency))
    considerImmigration();
  if (time > nextPayoutTime) {
    nextPayoutTime += getConfig().payoutTime;
    makePayouts();
  }
  cashPayouts();
  setWarning(Warning::MANA, numResource(ResourceId::MANA) < 100);
  setWarning(Warning::WOOD, numResource(ResourceId::WOOD) == 0);
  setWarning(Warning::DIGGING, getSquares(SquareId::FLOOR).empty());
  setWarning(Warning::MORE_LIGHTS, getSquares(SquareId::TORCH).size() * 25 < getAllSquares(roomsNeedingLight).size());
  for (SpawnType spawnType : ENUM_ALL(SpawnType)) {
    DormInfo info = getDormInfo()[spawnType];
    if (info.warning && info.getBedType())
      setWarning(*info.warning, chooseBedPos(getSquares(info.dormType), getSquares(*info.getBedType())));
  }
  for (auto elem : getTaskInfo())
    if (!getAllSquares(elem.second.squares).empty() && elem.second.warning)
      setWarning(*elem.second.warning, false);
  setWarning(Warning::NO_WEAPONS, false);
  PItem genWeapon = ItemFactory::fromId(ItemId::SWORD);
  vector<Item*> freeWeapons = getAllItems([&](const Item* it) {
      return it->getClass() == ItemClass::WEAPON && !minionEquipment.getOwner(it); }, false);
  for (Creature* c : getCreatures({MinionTrait::FIGHTER}, {MinionTrait::NO_EQUIPMENT})) {
    if (usesEquipment(c) && c->equip(genWeapon.get()) && filter(freeWeapons,
          [&] (const Item* it) { return minionEquipment.needs(c, it); }).empty()) {
      setWarning(Warning::NO_WEAPONS, true);
      Debug() << "Can't get weapon for " << c->getName();
      break;
    }
  }
  vector<Vec2> enemyPos = getEnemyPositions();
  if (!enemyPos.empty())
    delayDangerousTasks(enemyPos, getTime() + 20);
  else
    alarmInfo.finishTime() = -1000;
  bool allSurrender = true;
  for (Vec2 v : enemyPos)
    if (!prisonerInfo.count(NOTNULL(getLevel()->getSquare(v)->getCreature()))) {
      allSurrender = false;
      break;
    }
  if (allSurrender) {
    for (auto elem : copyOf(prisonerInfo))
      if (elem.second.state() == PrisonerState::SURRENDER) {
        Creature* c = elem.first;
        Vec2 pos = c->getPosition();
        if (containsSquare(pos) && !c->isDead()) {
          getLevel()->globalMessage(pos, c->getTheName() + " surrenders.");
          c->die(nullptr, true, false);
          addCreature(CreatureFactory::fromId(CreatureId::PRISONER, getTribe(),
                MonsterAIFactory::collective(this)), pos, {MinionTrait::PRISONER});
        }
        prisonerInfo.erase(c);
      }
  }
  updateConstructions();
  for (ItemFetchInfo& elem : getFetchInfo()) {
    for (Vec2 pos : getAllSquares())
      fetchItems(pos, elem);
    for (SquareType type : elem.additionalPos)
      for (Vec2 pos : getSquares(type))
        fetchItems(pos, elem);
  }
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

bool Collective::underAttack() const {
  for (Vec2 v : getAllSquares())
    if (const Creature* c = getLevel()->getSquare(v)->getCreature())
      if (c->getTribe() != getTribe())
        return true;
  return false;
}
 
void Collective::onKillEvent(const Creature* victim1, const Creature* killer) {
  if (contains(creatures, victim1)) {
    Creature* victim = const_cast<Creature*>(victim1);
    if (hasTrait(victim, MinionTrait::PRISONER) && killer && contains(getCreatures(), killer)
      && prisonerInfo.at(victim).state() == PrisonerState::EXECUTE)
      returnResource({ResourceId::PRISONER_HEAD, 1});
    prisonerInfo.erase(victim);
    freeFromGuardPost(victim);
    removeElement(creatures, victim);
    minionAttraction.erase(victim);
    if (Task* task = taskMap.getTask(victim)) {
      if (!task->canTransfer()) {
        task->cancel();
        returnResource(taskMap.removeTask(task));
      } else
        taskMap.freeTaskDelay(task, getTime() + 50);
    }
    for (MinionTrait t : ENUM_ALL(MinionTrait))
      if (contains(byTrait[t], victim))
        removeElement(byTrait[t], victim);
    if (auto spawnType = victim->getSpawnType())
      removeElement(bySpawnType[*spawnType], victim);
    for (Creature* c : creatures)
      c->addMorale(-0.03);
    control->onCreatureKilled(victim, killer);
  }
  if (victim1->getTribe() != getTribe() && (!killer || contains(creatures, killer))) {
    for (Creature* c : creatures)
      c->addMorale(c == killer ? 0.25 : 0.03);
    addMana(victim1->getDifficultyPoints() / 3);
    kills.push_back(victim1);
    points += victim1->getDifficultyPoints();
    if (Creature* leader = getLeader())
      leader->increaseExpLevel(double(victim1->getDifficultyPoints()) / 200);
  }
}

double Collective::getStanding(const Deity* d) const {
  if (deityStanding.count(d))
    return deityStanding.at(d);
  else
    return 0;
}

double Collective::getStanding(EpithetId id) const {
  if (Deity* d = Deity::getDeity(id))
    return getStanding(d);
  else
    return 0;
}

static double standingFun(double standing) {
  const double maxMult = 2.0;
  return pow(maxMult, standing);
}

double Collective::getTechCostMultiplier() const {
  return 1.0 / standingFun(getStanding(EpithetId::WISDOM));
}

double Collective::getCraftingMultiplier() const {
  return standingFun(getStanding(EpithetId::CRAFTS));
}

double Collective::getWarMultiplier() const {
  return standingFun(getStanding(EpithetId::WAR)) / standingFun(getStanding(EpithetId::LOVE));
}

double Collective::getBeastMultiplier() const {
  return 1.0 / standingFun(getStanding(EpithetId::NATURE));
}

double Collective::getUndeadMultiplier() const {
  return 1.0 / standingFun(getStanding(EpithetId::DEATH));
}

void Collective::onEpithetWorship(Creature* who, WorshipType type, EpithetId id) {
  if (type == WorshipType::DESTROY_ALTAR)
    return;
  double increase = 0;
  switch (type) {
    case WorshipType::PRAYER: increase = 1.0 / 400; break;
    case WorshipType::SACRIFICE: increase = 1.0 / 2; break;
    default: break;
  }
  switch (id) {
    case EpithetId::COURAGE: who->addMorale(increase); break;
    case EpithetId::FEAR: who->addMorale(-increase); break;
    default: break;
  }
}

void Collective::onWorshipEvent(Creature* who, const Deity* to, WorshipType type) {
  if (contains(creatures, who)) {
    double increase = 0;
    switch (type) {
      case WorshipType::PRAYER: increase = 1.0 / 2000; break;
      case WorshipType::SACRIFICE: increase = 1.0 / 5; break;
      case WorshipType::DESTROY_ALTAR: deityStanding[to] = -1; return;
    }
    deityStanding[to] = min(1.0, deityStanding[to] + increase);
    for (EpithetId id : to->getEpithets())
      onEpithetWorship(who, type, id);
  }
}

double Collective::getEfficiency(const Creature* c) const {
  return pow(2.0, c->getMorale());
}

vector<Vec2> Collective::getAllSquares(const vector<SquareType>& types, bool centerOnly) const {
  vector<Vec2> ret;
  for (SquareType type : types)
    append(ret, getSquares(type));
  if (centerOnly)
    ret = filter(ret, [this] (Vec2 pos) { return squareEfficiency.count(pos) && squareEfficiency.at(pos) == 8;});
  return ret;
}

const set<Vec2>& Collective::getSquares(SquareType type) const {
  static set<Vec2> empty;
  if (mySquares.count(type))
    return mySquares.at(type);
  else
    return empty;
}

vector<SquareType> Collective::getSquareTypes() const {
  return getKeys(mySquares);
}

const set<Vec2>& Collective::getAllSquares() const {
  return allSquares;
}

void Collective::claimSquare(Vec2 pos) {
  allSquares.insert(pos);
}

void Collective::changeSquareType(Vec2 pos, SquareType from, SquareType to) {
  mySquares[from].erase(pos);
  mySquares[to].insert(pos);
}

bool Collective::containsSquare(Vec2 pos) const {
  return allSquares.count(pos);
}

bool Collective::isKnownSquare(Vec2 pos) const {
  return knownTiles[pos];
}

void Collective::update(Creature* c) {
  control->update(c);
}

const static unordered_set<SquareType> efficiencySquares {
  SquareId::TRAINING_ROOM,
  SquareId::TORTURE_TABLE,
  SquareId::WORKSHOP,
  SquareId::LIBRARY,
  SquareId::LABORATORY,
};

bool Collective::hasEfficiency(Vec2 pos) const {
  return squareEfficiency.count(pos);
}

const double lightBase = 0.5;
const double flattenVal = 0.9;

double Collective::getEfficiency(Vec2 pos) const {
  double base = squareEfficiency.at(pos) == 8 ? 1 : 0.5;
  return base * min(1.0, (lightBase + getLevel()->getLight(pos) * (1 - lightBase)) / flattenVal);
}

const double sizeBase = 0.5;

void Collective::updateEfficiency(Vec2 pos, SquareType type) {
  if (getSquares(type).count(pos)) {
    squareEfficiency[pos] = 0;
    for (Vec2 v : pos.neighbors8())
      if (getSquares(type).count(v)) {
        ++squareEfficiency[pos];
        ++squareEfficiency[v];
        CHECK(squareEfficiency[v] <= 8);
        CHECK(squareEfficiency[pos] <= 8);
      }
  } else {
    squareEfficiency.erase(pos);
    for (Vec2 v : pos.neighbors8())
      if (getSquares(type).count(v)) {
        --squareEfficiency[v];
        CHECK(squareEfficiency[v] >=0);
      }
  }
}

static const int alarmTime = 100;

void Collective::onAlarmEvent(const Level* l, Vec2 pos) {
  if (l == getLevel() && containsSquare(pos)) {
    alarmInfo.finishTime() = getTime() + alarmTime;
    alarmInfo.position() = pos;
    for (Creature* c : byTrait[MinionTrait::FIGHTER])
      if (c->isAffected(LastingEffect::SLEEP))
        c->removeEffect(LastingEffect::SLEEP);
  }
}

MoveInfo Collective::getAlarmMove(Creature* c) {
  if (alarmInfo.finishTime() > c->getTime())
    if (auto action = c->moveTowards(alarmInfo.position()))
      return {1.0, action};
  return NoMove;
}

MoveInfo Collective::getGuardPostMove(Creature* c) {
  vector<Vec2> pos = getKeys(guardPosts);
  for (Vec2 v : pos)
    if (guardPosts.at(v).attender() == c) {
      pos = {v};
      break;
    }
  for (Vec2 v : pos) {
    GuardPostInfo& info = guardPosts.at(v);
    if (!info.attender() || info.attender() == c) {
      info.attender() = c;
      minionTaskStrings[c->getUniqueId()] = "guarding";
      if (c->getPosition().dist8(v) > 1)
        if (auto action = c->moveTowards(v))
          return {1.0, action};
      break;
    }
  }
  return NoMove;
}

void Collective::freeFromGuardPost(const Creature* c) {
  for (auto& elem : guardPosts)
    if (elem.second.attender() == c)
      elem.second.attender() = nullptr;
}

double Collective::getTime() const {
  return creatures.front()->getTime();
}

void Collective::addGuardPost(Vec2 pos) {
  guardPosts[pos] = {};
}

void Collective::removeGuardPost(Vec2 pos) {
  guardPosts.erase(pos);
}

bool Collective::isGuardPost(Vec2 pos) const {
  return guardPosts.count(pos);
}

int Collective::numResource(ResourceId id) const {
  int ret = credit[id];
  for (SquareType type : resourceInfo.at(id).storageType)
    for (Vec2 pos : getSquares(type))
      ret += getLevel()->getSquare(pos)->getItems(resourceInfo.at(id).predicate).size();
  return ret;
}

bool Collective::hasResource(CostInfo cost) const {
  return numResource(cost.id()) >= cost.value();
}

void Collective::takeResource(CostInfo cost) {
  int num = cost.value();
  if (num == 0)
    return;
  CHECK(num > 0);
  if (credit[cost.id()]) {
    if (credit[cost.id()] >= num) {
      credit[cost.id()] -= num;
      return;
    } else {
      num -= credit[cost.id()];
      credit[cost.id()] = 0;
    }
  }
  for (Vec2 pos : randomPermutation(getAllSquares(resourceInfo.at(cost.id()).storageType))) {
    vector<Item*> goldHere = getLevel()->getSquare(pos)->getItems(resourceInfo.at(cost.id()).predicate);
    for (Item* it : goldHere) {
      getLevel()->getSquare(pos)->removeItem(it);
      if (--num == 0)
        return;
    }
  }
  FAIL << "Not enough " << resourceInfo.at(cost.id()).name;
}

void Collective::returnResource(CostInfo amount) {
  if (amount.value() == 0)
    return;
  CHECK(amount.value() > 0);
  vector<Vec2> destination = getAllSquares(resourceInfo.at(amount.id()).storageType);
  if (!destination.empty()) {
    getLevel()->getSquare(chooseRandom(destination))->
      dropItems(ItemFactory::fromId(resourceInfo.at(amount.id()).itemId, amount.value()));
  } else
    credit[amount.id()] += amount.value();
}

vector<pair<Item*, Vec2>> Collective::getTrapItems(TrapType type, set<Vec2> squares) const {
  vector<pair<Item*, Vec2>> ret;
  if (squares.empty())
    squares = getSquares(SquareId::WORKSHOP);
  for (Vec2 pos : squares) {
    vector<Item*> v = getLevel()->getSquare(pos)->getItems([type, this](Item* it) {
        return it->getTrapType() == type && !isItemMarked(it); });
    for (Item* it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}

bool Collective::usesEquipment(const Creature* c) const {
  return c->isHumanoid() && !hasTrait(c, MinionTrait::NO_EQUIPMENT) && !hasTrait(c, MinionTrait::PRISONER);
}

Item* Collective::getWorstItem(vector<Item*> items) const {
  Item* ret = nullptr;
  for (Item* it : items)
    if (ret == nullptr || minionEquipment.getItemValue(it) < minionEquipment.getItemValue(ret))
      ret = it;
  return ret;
}

void Collective::autoEquipment(Creature* creature, bool replace) {
  map<EquipmentSlot, vector<Item*>> slots;
  vector<Item*> myItems = getAllItems([&](const Item* it) {
      return minionEquipment.getOwner(it) == creature && it->canEquip(); });
  for (Item* it : myItems) {
    EquipmentSlot slot = it->getEquipmentSlot();
    if (slots[slot].size() < creature->getEquipment().getMaxItems(slot)) {
      slots[slot].push_back(it);
    } else  // a rare occurence that minion owns too many items of the same slot,
          //should happen only when an item leaves the fortress and then is braught back
      minionEquipment.discard(it);
  }
  for (Item* it : getAllItems([&](const Item* it) {
      return minionEquipment.needs(creature, it, false, replace) && !minionEquipment.getOwner(it); }, false)) {
    if (!it->canEquip()
        || slots[it->getEquipmentSlot()].size() < creature->getEquipment().getMaxItems(it->getEquipmentSlot())
        || minionEquipment.getItemValue(getWorstItem(slots[it->getEquipmentSlot()]))
            < minionEquipment.getItemValue(it)) {
      minionEquipment.own(creature, it);
      if (it->canEquip()
        && slots[it->getEquipmentSlot()].size() == creature->getEquipment().getMaxItems(it->getEquipmentSlot())) {
        Item* removed = getWorstItem(slots[it->getEquipmentSlot()]);
        minionEquipment.discard(removed);
        removeElement(slots[it->getEquipmentSlot()], removed); 
      }
      if (it->canEquip())
        slots[it->getEquipmentSlot()].push_back(it);
      if (it->getClass() != ItemClass::AMMO)
        break;
    }
  }
}

vector<Item*> Collective::getAllItems(ItemPredicate predicate, bool includeMinions) const {
  vector<Item*> allItems;
  for (Vec2 v : getAllSquares())
    append(allItems, getLevel()->getSquare(v)->getItems(predicate));
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems(predicate));
  sort(allItems.begin(), allItems.end(), [this](const Item* it1, const Item* it2) {
      int diff = minionEquipment.getItemValue(it1) - minionEquipment.getItemValue(it2);
      if (diff == 0)
        return it1->getUniqueId() < it2->getUniqueId();
      else
        return diff > 0;
    });
  return allItems;
}

void Collective::clearPrisonerTask(Creature* prisoner) {
  if (prisonerInfo.at(prisoner).task()) {
    taskMap.removeTask(prisonerInfo.at(prisoner).task());
    prisonerInfo.at(prisoner).task() = 0;
  }
}

void Collective::orderExecution(Creature* c) {
  if (prisonerInfo.at(c).state() == PrisonerState::EXECUTE)
    return;
  clearPrisonerTask(c);
  prisonerInfo.at(c) = {PrisonerState::EXECUTE, 0};
  setMinionTask(c, MinionTask::EXECUTE);
}

void Collective::orderTorture(Creature* c) {
  if (prisonerInfo.at(c).state() == PrisonerState::TORTURE)
    return;
  clearPrisonerTask(c);
  prisonerInfo.at(c) = {PrisonerState::TORTURE, 0};
  setMinionTask(c, MinionTask::TORTURE);
}

void Collective::orderSacrifice(Creature* c) {
  if (prisonerInfo.at(c).state() == PrisonerState::SACRIFICE)
    return;
  clearPrisonerTask(c);
  prisonerInfo.at(c) = {PrisonerState::SACRIFICE, 0};
  setMinionTask(c, MinionTask::SACRIFICE);
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

const map<Vec2, Collective::TrapInfo>& Collective::getTraps() const {
  return traps;
}

void Collective::removeTrap(Vec2 pos) {
  traps.erase(pos);
}

void Collective::removeConstruction(Vec2 pos) {
  returnResource(taskMap.removeTask(constructions.at(pos).task()));
  constructions.erase(pos);
}

void Collective::addConstruction(Vec2 pos, SquareType type, CostInfo cost, bool immediately, bool noCredit) {
  if (immediately && hasResource(cost)) {
    while (!getLevel()->getSquare(pos)->construct(type)) {}
    onConstructed(pos, type);
  } else if (!noCredit || hasResource(cost)) {
    constructions[pos] = {cost, false, 0, type, -1};
    updateConstructions();
  }
}

const map<Vec2, Collective::ConstructionInfo>& Collective::getConstructions() const {
  return constructions;
}

void Collective::dig(Vec2 pos) {
  taskMap.markSquare(pos, Task::construction(this, pos, SquareId::FLOOR));
}

void Collective::dontDig(Vec2 pos) {
  taskMap.unmarkSquare(pos);
}

bool Collective::isMarkedToDig(Vec2 pos) const {
  return taskMap.getMarked(pos);
}

void Collective::setPriorityTasks(Vec2 pos) {
  taskMap.setPriorityTasks(pos);
}

void Collective::cutTree(Vec2 pos) {
  taskMap.markSquare(pos, Task::construction(this, pos, SquareId::TREE_TRUNK));
}

set<TrapType> Collective::getNeededTraps() const {
  set<TrapType> ret;
  for (auto elem : traps)
    if (!elem.second.marked() && !elem.second.armed())
      ret.insert(elem.second.type());
  return ret;
}

void Collective::addTrap(Vec2 pos, TrapType type) {
  traps[pos] = {type, false, 0};
  updateConstructions();
}

void Collective::onAppliedItem(Vec2 pos, Item* item) {
  CHECK(item->getTrapType());
  if (traps.count(pos)) {
    traps[pos].marked() = 0;
    traps[pos].armed() = true;
  }
}

void Collective::onAppliedItemCancel(Vec2 pos) {
  if (traps.count(pos))
    traps.at(pos).marked() = 0;
}

void Collective::onConstructed(Vec2 pos, SquareType type) {
  if (!contains({SquareId::TREE_TRUNK}, type.getId()))
    allSquares.insert(pos);
  CHECK(!getSquares(type).count(pos));
  mySquares[type].insert(pos);
  if (efficiencySquares.count(type))
    updateEfficiency(pos, type);
  if (contains({SquareId::FLOOR, SquareId::BRIDGE, SquareId::BARRICADE}, type.getId()))
    updateSectors(pos);
  if (taskMap.getMarked(pos))
    taskMap.unmarkSquare(pos);
  if (constructions.count(pos)) {
    constructions.at(pos).built() = true;
    constructions.at(pos).marked() = 0;
    constructions.at(pos).task() = -1;
  }
}

void Collective::updateSectors(Vec2 pos) {
  if (getLevel()->getSquare(pos)->canEnterEmpty(MovementType(getTribe(), {MovementTrait::WALK})))
    sectors->add(pos);
  else
    sectors->remove(pos);
  if (getLevel()->getSquare(pos)->canEnterEmpty(MovementType(getTribe(), {MovementTrait::WALK, MovementTrait::FLY})))
    flyingSectors->add(pos);
  else
    flyingSectors->remove(pos);
  taskMap.clearAllLocked();
}

// after this time applying trap or building door is rescheduled (imp death, etc).
const static int timeToBuild = 50;

void Collective::updateConstructions() {
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems;
  for (TrapType type : ENUM_ALL(TrapType))
    trapItems[type] = getTrapItems(type, getAllSquares());
  for (auto elem : traps)
    if (!isDelayed(elem.first)) {
      vector<pair<Item*, Vec2>>& items = trapItems.at(elem.second.type());
      if (!items.empty()) {
        if (!elem.second.armed() && elem.second.marked() <= getTime()) {
          Vec2 pos = items.back().second;
          taskMap.addTask(Task::applyItem(this, pos, items.back().first, elem.first), pos);
          markItem(items.back().first);
          items.pop_back();
          traps[elem.first].marked() = getTime() + timeToBuild;
        }
      }
    }
  for (auto& elem : constructions)
    if (!isDelayed(elem.first) && elem.second.marked() <= getTime() && !elem.second.built()) {
      if (auto warning = resourceInfo.at(elem.second.cost().id()).warning)
        setWarning(*warning, !hasResource(elem.second.cost()));
      if (!hasResource(elem.second.cost()))
        continue;
      elem.second.task() = taskMap.addTaskCost(Task::construction(this, elem.first, elem.second.type()), elem.first,
          elem.second.cost())->getUniqueId();
      elem.second.marked() = getTime() + timeToBuild;
      takeResource(elem.second.cost());
    }
}

void Collective::delayDangerousTasks(const vector<Vec2>& enemyPos, double delayTime) {
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
    delayedPos[pos] = delayTime;
    if (dist[pos] >= radius)
      continue;
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(dist.getBounds()) && dist[v] == infinity && containsSquare(v)) {
        dist[v] = dist[pos] + 1;
        q.push(v);
      }
  }
}

bool Collective::isDelayed(Vec2 pos) {
  return delayedPos.count(pos) && delayedPos.at(pos) > getTime();
}

void Collective::fetchAllItems(Vec2 pos) {
  for (ItemFetchInfo& elem : getFetchInfo())
    fetchItems(pos, elem, true);
}

void Collective::fetchItems(Vec2 pos, ItemFetchInfo& elem, bool ignoreDelayed) {
  if ((isDelayed(pos) && !ignoreDelayed) 
      || (traps.count(pos) && traps.at(pos).type() == TrapType::BOULDER && traps.at(pos).armed() == true))
    return;
  for (SquareType type : elem.destination)
    if (getSquares(type).count(pos))
      return;
  vector<Item*> equipment = getLevel()->getSquare(pos)->getItems(elem.predicate);
  if (!equipment.empty()) {
    vector<Vec2> destination = getAllSquares(elem.destination);
    if (!destination.empty()) {
      setWarning(elem.warning, false);
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      taskMap.addTask(Task::bringItem(this, pos, equipment, destination), pos);
      for (Item* it : equipment)
        markItem(it);
    } else
      setWarning(elem.warning, true);
  }
}

void Collective::onSurrenderEvent(Creature* who, const Creature* to) {
  if (contains(getCreatures(), to) && !contains(getCreatures(), who) && !prisonerInfo.count(who) && who->isHumanoid())
    prisonerInfo[who] = {PrisonerState::SURRENDER, 0};
}

// actually only called when square is destroyed
void Collective::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == getLevel()) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
        if (efficiencySquares.count(elem.first))
          updateEfficiency(pos, elem.first);
      }
    if (constructions.count(pos)) {
      ConstructionInfo& info = constructions.at(pos);
      info.marked() = getTime() + 10; // wait a little before considering rebuilding
      info.built() = false;
      info.task() = -1;
    }
    if (getConfig().keepSectors) {
      sectors->add(pos);
      flyingSectors->add(pos);
    }
  }
}

void Collective::onTrapTriggerEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == getLevel()) {
    traps.at(pos).armed() = false;
    if (traps.at(pos).type() == TrapType::SURPRISE)
      handleSurprise(pos);
  }
}

void Collective::onTrapDisarmEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == getLevel())
    traps.at(pos).armed() = false;
}

void Collective::handleSurprise(Vec2 pos) {
  Vec2 rad(8, 8);
  bool wasMsg = false;
  Creature* c = NOTNULL(getLevel()->getSquare(pos)->getCreature());
  for (Vec2 v : randomPermutation(Rectangle(pos - rad, pos + rad).getAllSquares()))
    if (getLevel()->inBounds(v))
      if (Creature* other = getLevel()->getSquare(v)->getCreature())
        if (hasTrait(other, MinionTrait::FIGHTER) && v.dist8(pos) > 1) {
          for (Vec2 dest : pos.neighbors8(true))
            if (getLevel()->canMoveCreature(other, dest - v)) {
              getLevel()->moveCreature(other, dest - v);
              other->playerMessage("Surprise!");
              if (!wasMsg) {
                c->playerMessage("Surprise!");
                wasMsg = true;
              }
              break;
            }
        }
}

void Collective::onPickedUp(Vec2 pos, EntitySet<Item> items) {
  for (auto id : items)
    unmarkItem(id);
}
  
void Collective::onCantPickItem(EntitySet<Item> items) {
  for (auto id : items)
    unmarkItem(id);
}

void Collective::addKnownTile(Vec2 pos) {
  if (!knownTiles[pos]) {
    borderTiles.erase(pos);
    knownTiles[pos] = true;
    for (Vec2 v : pos.neighbors4())
      if (getLevel()->inBounds(v) && !knownTiles[v])
        borderTiles.insert(v);
    if (Task* task = taskMap.getMarked(pos))
      if (task->isImpossible(getLevel()))
        taskMap.removeTask(task);
  }
}

void Collective::addMana(double value) {
  if ((manaRemainder += value) >= 1) {
    returnResource({ResourceId::MANA, int(manaRemainder)});
    manaRemainder -= int(manaRemainder);
  }
}

bool Collective::isItemNeeded(const Item* item) const {
  if (isWarning(Warning::NO_WEAPONS) && item->getClass() == ItemClass::WEAPON)
    return true;
  set<TrapType> neededTraps = getNeededTraps();
  if (!neededTraps.empty() && item->getTrapType() && neededTraps.count(*item->getTrapType()))
    return true;
  return false;
}

void Collective::onAppliedSquare(Vec2 pos) {
  Creature* c = NOTNULL(getLevel()->getSquare(pos)->getCreature());
  MinionTask currentTask = currentTasks.at(c->getUniqueId()).task();
  minionPayment[c].workAmount() += getTaskInfo().at(currentTask).cost;
  if (getSquares(SquareId::LIBRARY).count(pos)) {
    addMana(0.2);
    if (Random.rollD(60.0 / (getEfficiency(pos))) && !getAvailableSpells().empty())
      c->addSpell(chooseRandom(getAvailableSpells()));
  }
  if (getSquares(SquareId::TRAINING_ROOM).count(pos)) {
    double lev1 = 0.03;
    double lev10 = 0.01;
    c->increaseExpLevel(getEfficiency(pos) * (lev1 - double(c->getExpLevel() - 1) * (lev1 - lev10) / 9.0  ));
  }
  if (getSquares(SquareId::LABORATORY).count(pos))
    if (Random.rollD(30.0 / (getCraftingMultiplier() * getEfficiency(pos)))) {
      getLevel()->getSquare(pos)->dropItems(ItemFactory::laboratory(technologies).random());
      Statistics::add(StatId::POTION_PRODUCED);
    }
  if (getSquares(SquareId::WORKSHOP).count(pos))
    if (Random.rollD(40.0 / (getCraftingMultiplier() * getEfficiency(pos)))) {
      vector<PItem> items;
      for (int i : Range(20)) {
        items = ItemFactory::workshop(technologies).random();
        if (isItemNeeded(items[0].get()))
          break;
      }
      if (items[0]->getClass() == ItemClass::WEAPON)
        Statistics::add(StatId::WEAPON_PRODUCED);
      if (items[0]->getClass() == ItemClass::ARMOR)
        Statistics::add(StatId::ARMOR_PRODUCED);
      getLevel()->getSquare(pos)->dropItems(std::move(items));
    }
}

struct SpellLearningInfo {
  SpellId id;
  TechId techId;
};

static vector<SpellLearningInfo> spellLearning {
    { SpellId::HEALING, TechId::SPELLS },
    { SpellId::SUMMON_INSECTS, TechId::SPELLS},
    { SpellId::DECEPTION, TechId::SPELLS},
    { SpellId::SPEED_SELF, TechId::SPELLS},
    { SpellId::STR_BONUS, TechId::SPELLS_ADV},
    { SpellId::DEX_BONUS, TechId::SPELLS_ADV},
    { SpellId::FIRE_SPHERE_PET, TechId::SPELLS_ADV},
    { SpellId::TELEPORT, TechId::SPELLS_ADV},
    { SpellId::CURE_POISON, TechId::SPELLS_ADV},
    { SpellId::INVISIBILITY, TechId::SPELLS_MAS},
    { SpellId::WORD_OF_POWER, TechId::SPELLS_MAS},
    { SpellId::PORTAL, TechId::SPELLS_MAS},
};

vector<SpellInfo> Collective::getSpellLearning(const Technology* tech) {
  vector<SpellInfo> ret;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      ret.push_back(Creature::getSpell(elem.id));
  return ret;
}

vector<SpellId> Collective::getAvailableSpells() const {
  vector<SpellId> ret;
  for (auto elem : spellLearning)
    if (hasTech(elem.techId))
      ret.push_back(elem.id);
  return ret;
}

vector<SpellId> Collective::getAllSpells() const {
  vector<SpellId> ret;
  for (auto elem : spellLearning)
    ret.push_back(elem.id);
  return ret;
}

TechId Collective::getNeededTech(SpellId id) const {
  for (auto elem : spellLearning)
    if (elem.id == id)
      return elem.techId;
  FAIL << "Spell not found";
  return TechId(0);
}

double Collective::getDangerLevel(bool includeExecutions) const {
  double ret = 0;
  for (const Creature* c : getCreatures({MinionTrait::FIGHTER}))
    ret += c->getDifficultyPoints();
  if (includeExecutions)
    ret += getSquares(SquareId::IMPALED_HEAD).size() * 150;
  return ret;
}

bool Collective::hasTech(TechId id) const {
  return contains(technologies, Technology::get(id));
}

double Collective::getTechCost(Technology* t) {
  return t->getCost();
 /* int numTech = technologies.size() - numFreeTech;
  int numDouble = 4;
  return getTechCostMultiplier() * pow(2, double(numTech) / numDouble);*/
}

void Collective::acquireTech(Technology* tech, bool free) {
  technologies.push_back(tech);
  if (free)
    ++numFreeTech;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      if (Creature* leader = getLeader())
        leader->addSpell(elem.id);
  if (Skill* skill = tech->getSkill())
    for (Creature* c : getCreatures())
      c->addSkill(skill);
}

const vector<Technology*>& Collective::getTechnologies() const {
  return technologies;
}

void Collective::onCombatEvent(const Creature* c) {
  CHECK(c != nullptr);
  if (contains(creatures, c))
    lastCombat[c] = c->getTime();
}

bool Collective::isInCombat(const Creature* c) const {
  double timeout = 5;
  return lastCombat.count(c) && lastCombat.at(c) > c->getTime() -5;
}

vector<const Creature*> Collective::getKills() const {
  return kills;
}

int Collective::getPoints() const {
  return points;
}

const map<UniqueEntity<Creature>::Id, string>& Collective::getMinionTaskStrings() const {
  return minionTaskStrings;
}

void Collective::onEquipEvent(const Creature* c, const Item* it) {
  if (contains(creatures, c))
    minionEquipment.own(c, it);
}

void Collective::onPickupEvent(const Creature* c, const vector<Item*>& items) {
  if (contains(creatures, c) && !hasTrait(c, MinionTrait::WORKER))
    for (Item* it : items)
      if (minionEquipment.isItemUseful(it))
        minionEquipment.own(c, it);
}

void Collective::onTortureEvent(Creature* who, const Creature* torturer) {
  if (contains(creatures, torturer))
    returnResource({ResourceId::MANA, 1});
}

void Collective::onCopulated(Creature* who, Creature* with) {
  if (contains(getCreatures(), who)) {
    if (contains(getCreatures(), with))
      with->addMorale(1);
    if (Random.roll(5) && !contains(pregnancies, who)) 
      pregnancies.push_back(who);
  }
}

MinionEquipment& Collective::getMinionEquipment() {
  return minionEquipment;
}

const MinionEquipment& Collective::getMinionEquipment() const {
  return minionEquipment;
}

int Collective::getSalary(const Creature* c) const {
  if (minionPayment.count(c))
    return minionPayment.at(c).salary();
  else
    return 0;
}

int Collective::getNextPayoutTime() const {
  return nextPayoutTime;
}
