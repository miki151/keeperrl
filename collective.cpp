#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"
#include "pantheon.h"
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
#include "entity_name.h"
#include "modifier_type.h"
#include "cost_info.h"
#include "monster_ai.h"
#include "task.h"
#include "territory.h"
#include "collective_attack.h"

struct Collective::ItemFetchInfo {
  ItemIndex index;
  ItemPredicate predicate;
  vector<SquareType> destination;
  bool oneAtATime;
  vector<SquareType> additionalPos;
  Warning warning;
};

struct Collective::MinionTaskInfo {
  enum Type { APPLY_SQUARE, EXPLORE, COPULATE, CONSUME, EAT, SPIDER } type;
  MinionTaskInfo(vector<SquareType>, const string& description, optional<Warning> = none, double cost = 0,
      bool centerOnly = false);
  MinionTaskInfo(Type, const string& description, optional<Warning> = none);
  vector<SquareType> squares;
  string description;
  optional<Warning> warning;
  double cost = 0;
  bool centerOnly = false;
};

struct Collective::MinionPaymentInfo {
  int SERIAL(salary);
  double SERIAL(workAmount);
  int SERIAL(debt);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(salary) & SVAR(workAmount) & SVAR(debt);
  }
};

struct Collective::CurrentTaskInfo {
  MinionTask SERIAL(task);
  double SERIAL(finishTime);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(task) & SVAR(finishTime);
  }
};

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(TaskCallback)
    & SVAR(creatures)
    & SVAR(leader)
    & SVAR(taskMap)
    & SVAR(tribe)
    & SVAR(control)
    & SVAR(byTrait)
    & SVAR(bySpawnType)
    & SVAR(deityStanding)
    & SVAR(mySquares)
    & SVAR(mySquares2)
    & SVAR(territory)
    & SVAR(squareEfficiency)
    & SVAR(alarmInfo)
    & SVAR(markedItems)
    & SVAR(constructions)
    & SVAR(minionEquipment)
    & SVAR(surrendering)
    & SVAR(minionTaskStrings)
    & SVAR(delayedPos)
    & SVAR(knownTiles)
    & SVAR(technologies)
    & SVAR(numFreeTech)
    & SVAR(kills)
    & SVAR(points)
    & SVAR(currentTasks)
    & SVAR(credit)
    & SVAR(level)
    & SVAR(minionPayment)
    & SVAR(pregnancies)
    & SVAR(nextPayoutTime)
    & SVAR(minionAttraction)
    & SVAR(teams)
    & SVAR(knownLocations)
    & SVAR(knownVillains)
    & SVAR(name)
    & SVAR(config)
    & SVAR(warnings)
    & SVAR(banished)
    & SVAR(squaresInUse)
    & SVAR(equipmentUpdates)
    & SVAR(deadCreatures)
    & SVAR(spawnGhosts)
    & SVAR(lastGuardian);
}

SERIALIZABLE(Collective);

SERIALIZATION_CONSTRUCTOR_IMPL(Collective);

void Collective::setWarning(Warning w, bool state) {
  warnings[w] = state;
}

bool Collective::isWarning(Warning w) const {
  return warnings[w];
}

Collective::MinionTaskInfo::MinionTaskInfo(vector<SquareType> s, const string& desc, optional<Warning> w,
    double _cost, bool center) 
    : type(APPLY_SQUARE), squares(s), description(desc), warning(w), cost(_cost), centerOnly(center) {
}

Collective::MinionTaskInfo::MinionTaskInfo(Type t, const string& desc, optional<Warning> w)
    : type(t), description(desc), warning(w) {
  CHECK(type != APPLY_SQUARE);
}

ItemPredicate Collective::unMarkedItems() const {
  return [this](const Item* it) { return !isItemMarked(it); };
}

const static vector<SquareType> resourceStorage {SquareId::STOCKPILE, SquareId::STOCKPILE_RES};
const static vector<SquareType> equipmentStorage {SquareId::STOCKPILE, SquareId::STOCKPILE_EQUIP};

vector<SquareType> Collective::getEquipmentStorageSquares() {
  return equipmentStorage;
}

const vector<Collective::ItemFetchInfo>& Collective::getFetchInfo() const {
  if (itemFetchInfo.empty())
    itemFetchInfo = {
      {ItemIndex::CORPSE, unMarkedItems(), {SquareId::CEMETERY}, true, {}, Warning::GRAVES},
      {ItemIndex::GOLD, unMarkedItems(), {SquareId::TREASURE_CHEST}, false, {}, Warning::CHESTS},
      {ItemIndex::MINION_EQUIPMENT, [this](const Item* it)
          { return it->getClass() != ItemClass::GOLD && !isItemMarked(it);},
          equipmentStorage, false, {}, Warning::EQUIPMENT_STORAGE},
      {ItemIndex::WOOD, unMarkedItems(), resourceStorage, false, {SquareId::TREE_TRUNK}, Warning::RESOURCE_STORAGE},
      {ItemIndex::IRON, unMarkedItems(), resourceStorage, false, {}, Warning::RESOURCE_STORAGE},
      {ItemIndex::STONE, unMarkedItems(), resourceStorage, false, {}, Warning::RESOURCE_STORAGE},
  };
  return itemFetchInfo;
}

const map<Collective::ResourceId, Collective::ResourceInfo> Collective::resourceInfo {
  {ResourceId::MANA, { {}, none, ItemId::GOLD_PIECE, "mana"}},
  {ResourceId::PRISONER_HEAD, { {}, none, ItemId::GOLD_PIECE, "", true}},
  {ResourceId::GOLD, {{SquareId::TREASURE_CHEST}, ItemIndex::GOLD, ItemId::GOLD_PIECE,"gold",}},
  {ResourceId::WOOD, { resourceStorage, ItemIndex::WOOD, ItemId::WOOD_PLANK, "wood"}},
  {ResourceId::IRON, { resourceStorage, ItemIndex::IRON, ItemId::IRON_ORE, "iron"}},
  {ResourceId::STONE, { resourceStorage, ItemIndex::STONE, ItemId::ROCK, "granite"}},
  {ResourceId::CORPSE, { {SquareId::CEMETERY}, ItemIndex::REVIVABLE_CORPSE, ItemId::GOLD_PIECE, "corpses", true}},
};

map<MinionTask, Collective::MinionTaskInfo> Collective::getTaskInfo() const {
  map<MinionTask, MinionTaskInfo> ret {
    {MinionTask::TRAIN, {{SquareId::TRAINING_ROOM}, "training", Collective::Warning::TRAINING, 1}},
    {MinionTask::WORKSHOP, {{SquareId::WORKSHOP}, "workshop", Collective::Warning::WORKSHOP, 1}},
    {MinionTask::FORGE, {{SquareId::FORGE}, "forge", none, 1}},
    {MinionTask::LABORATORY, {{SquareId::LABORATORY}, "lab", none, 1}},
    {MinionTask::JEWELER, {{SquareId::JEWELER}, "jewellery", none, 1}},
    {MinionTask::SLEEP, {{SquareId::BED}, "sleeping", Collective::Warning::BEDS}},
    {MinionTask::EAT, {MinionTaskInfo::EAT, "eating"}},
    {MinionTask::GRAVE, {{SquareId::GRAVE}, "sleeping", Collective::Warning::GRAVES}},
    {MinionTask::LAIR, {{SquareId::BEAST_CAGE}, "sleeping"}},
    {MinionTask::STUDY, {{SquareId::LIBRARY}, "studying", Collective::Warning::LIBRARY, 1}},
    {MinionTask::PRISON, {{SquareId::PRISON}, "prison", Collective::Warning::NO_PRISON}},
    {MinionTask::TORTURE, {{SquareId::TORTURE_TABLE}, "torture ordered",
                            Collective::Warning::TORTURE_ROOM, 0, true}},
    {MinionTask::CROPS, {{SquareId::CROPS}, "crops"}},
    {MinionTask::RITUAL, {{SquareId::RITUAL_ROOM}, "rituals"}},
    {MinionTask::COPULATE, {MinionTaskInfo::COPULATE, "copulation"}},
    {MinionTask::CONSUME, {MinionTaskInfo::CONSUME, "consumption"}},
    {MinionTask::EXPLORE, {MinionTaskInfo::EXPLORE, "spying"}},
    {MinionTask::SPIDER, {MinionTaskInfo::SPIDER, "spider"}},
    {MinionTask::EXPLORE_NOCTURNAL, {MinionTaskInfo::EXPLORE, "spying"}},
    {MinionTask::EXPLORE_CAVES, {MinionTaskInfo::EXPLORE, "spying"}},
 //   {MinionTask::SACRIFICE, {{}, "sacrifice ordered", Collective::Warning::ALTAR}},
    {MinionTask::EXECUTE, {{SquareId::PRISON}, "execution ordered", Collective::Warning::NO_PRISON}}};
/*  for (SquareType t : getSquareTypes())
    if (contains({SquareId::ALTAR, SquareId::CREATURE_ALTAR}, t.getId()) && !getSquares(t).empty()) {
      ret.at(MinionTask::WORSHIP).squares.push_back(t);
      ret.at(MinionTask::SACRIFICE).squares.push_back(t);
    }*/
  return ret;
};

Collective::Collective(Level* l, const CollectiveConfig& cfg, Tribe* t, EnumMap<ResourceId, int> _credit,
    const optional<string>& n) 
  : credit(_credit), taskMap(l->getModel()->getLevels()), knownTiles(l->getModel()->getLevels()),
    control(CollectiveControl::idle(this)),
    tribe(NOTNULL(t)), level(NOTNULL(l)), nextPayoutTime(-1), name(n), config(cfg) {
}

string Collective::getFullName() const {
  if (name)
    return capitalFirst(getTribe()->getName()) + " of " + *name;
  else if (getLeader() && getLeader()->getFirstName())
    return getLeader()->getNameAndTitle();
  else
    return capitalFirst(getTribe()->getName());
}

string Collective::getShortName() const {
  if (name)
    return *name;
  else if (getLeader())
    return getLeader()->getFirstName().get_value_or(getLeader()->getName().bare());
  else
    return "";
}

string Collective::getTribeName() const {
  if (name)
    return getTribe()->getName();
  else if (getLeader() && getLeader()->getFirstName())
    return getLeader()->getSpeciesName();
  else
    return "";
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
    case AttractionId::SQUARE: 
      return getSquares(attraction.get<SquareType>()).size();
    case AttractionId::ITEM_INDEX: 
      return getAllItems(attraction.get<ItemIndex>(), true).size();
  }
}

namespace {

class LeaderControlOverride : public Creature::MoraleOverride {
  public:
  LeaderControlOverride(Collective* col, Creature* c) : collective(col), creature(c) {}

  virtual optional<double> getMorale() override {
    for (auto team : collective->getTeams().getContaining(collective->getLeader()))
      if (collective->getTeams().isActive(team) && collective->getTeams().contains(team, creature) &&
          collective->getTeams().getLeader(team) == collective->getLeader())
        return 1;
    return none;
  }

  SERIALIZATION_CONSTRUCTOR(LeaderControlOverride);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Creature::MoraleOverride) & SVAR(collective) & SVAR(creature);
  }

  private:
  Collective* SERIAL(collective);
  Creature* SERIAL(creature);
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
  if (!traits[MinionTrait::FARM_ANIMAL])
    c->setController(PController(new Monster(c, MonsterAIFactory::collective(this))));
  if (!leader)
    leader = c;
  CHECK(c->getTribe() == tribe);
  creatures.push_back(c);
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
  if (auto spawnType = c->getSpawnType())
    bySpawnType[*spawnType].push_back(c);
  for (const Item* item : c->getEquipment().getItems())
    minionEquipment->own(c, item);
  if (traits[MinionTrait::FIGHTER]) {
    c->addMoraleOverride(Creature::PMoraleOverride(new LeaderControlOverride(this, c)));
  }
}

void Collective::removeCreature(Creature* c) {
  removeElement(creatures, c);
  deadCreatures.push_back(c);
  minionAttraction.erase(c);
  if (Task* task = taskMap->getTask(c)) {
    if (!task->canTransfer())
      returnResource(taskMap->removeTask(task));
    else
      taskMap->freeTaskDelay(task, getTime() + 50);
  }
  if (auto spawnType = c->getSpawnType())
    removeElement(bySpawnType[*spawnType], c);
  for (auto team : teams->getContaining(c))
    teams->remove(team, c);
  for (MinionTrait t : ENUM_ALL(MinionTrait))
    if (contains(byTrait[t], c))
      removeElement(byTrait[t], c);
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
    tasks.push_back(Task::goTo(Random.choose(exitTiles)));
  tasks.push_back(Task::disappear());
  c->setController(PController(new Monster(c, MonsterAIFactory::singleTask(Task::chain(std::move(tasks))))));
  banished.push_back(c);
}

bool Collective::wasBanished(const Creature* c) const {
  return contains(banished, c);
}

vector<Creature*> Collective::getRecruits() const {
  vector<Creature*> ret;
  vector<Creature*> possibleRecruits = filter(getCreatures(MinionTrait::FIGHTER),
      [] (const Creature* c) { return c->getRecruitmentCost() > 0; });
  if (auto minPop = config->getRecruitingMinPopulation())
    for (int i = *minPop; i < possibleRecruits.size(); ++i)
      ret.push_back(possibleRecruits[i]);
  return ret;
}

void Collective::recruit(Creature* c, Collective* to) {
  removeCreature(c);
  c->setTribe(to->getTribe());
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

const Tribe* Collective::getTribe() const {
  return tribe;
}

Tribe* Collective::getTribe() {
  return tribe;
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

MoveInfo Collective::getDropItems(Creature *c) {
  if (territory->contains(c->getPosition())) {
    vector<Item*> items = c->getEquipment().getItems([this, c](const Item* item) {
        return minionEquipment->isItemUseful(item) && minionEquipment->getOwner(item) != c; });
    if (!items.empty())
      return c->drop(items);
  }
  return NoMove;
}

void Collective::onBedCreated(Position pos, const SquareType& fromType, const SquareType& toType) {
  changeSquareType(pos, fromType, toType);
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

int Collective::getTaskDuration(const Creature* c, MinionTask task) const {
  switch (task) {
    case MinionTask::CONSUME:
    case MinionTask::COPULATE:
    case MinionTask::GRAVE:
    case MinionTask::LAIR:
    case MinionTask::EAT:
    case MinionTask::SLEEP: return 1;
    default: return 500 + 250 * c->getMorale();
  }
}

void Collective::setMinionTask(const Creature* c, MinionTask task) {
  currentTasks[c->getUniqueId()] = {task, c->getTime() + getTaskDuration(c, task)};
}

optional<MinionTask> Collective::getMinionTask(const Creature* c) const {
  if (currentTasks.count(c->getUniqueId()))
    return currentTasks.at(c->getUniqueId()).task;
  else
    return none;
}

bool Collective::isTaskGood(const Creature* c, MinionTask task, bool ignoreTaskLock) const {
  if (c->getMinionTasks().getValue(task, ignoreTaskLock) == 0)
    return false;
  if (minionPayment.count(c) && minionPayment.at(c).debt > 0 && getTaskInfo().at(task).cost > 0)
    return false;
  switch (task) {
    case MinionTask::CROPS:
    case MinionTask::EXPLORE:
        return getLevel()->getModel()->getSunlightInfo().state == SunlightState::DAY;
    case MinionTask::SLEEP:
        if (!config->sleepOnlyAtNight())
          return true;
        // break skipped on purpose
    case MinionTask::EXPLORE_NOCTURNAL:
        return getLevel()->getModel()->getSunlightInfo().state == SunlightState::NIGHT;
    default: return true;
  }
}

void Collective::setRandomTask(const Creature* c) {
  vector<pair<MinionTask, double>> goodTasks;
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (isTaskGood(c, t))
      goodTasks.push_back({t, c->getMinionTasks().getValue(t)});
  if (!goodTasks.empty())
    setMinionTask(c, Random.choose(goodTasks));
}

static bool betterPos(Position from, Position current, Position candidate) {
  double maxDiff = 0.3;
  double curDist = from.dist8(current);
  double newDist = from.dist8(candidate);
  return Random.getDouble() <= 1.0 - (newDist - curDist) / (curDist * maxDiff);
}

static optional<Position> getRandomCloseTile(Position from, const vector<Position>& tiles,
    function<bool(Position)> predicate) {
  optional<Position> ret;
  for (Position pos : tiles)
    if (predicate(pos) && (!ret || betterPos(from, *ret, pos)))
      ret = pos;
  return ret;
}

optional<Position> Collective::getTileToExplore(const Creature* c, MinionTask task) const {
  vector<Position> border = Random.permutation(knownTiles->getBorderTiles());
  switch (task) {
    case MinionTask::EXPLORE_CAVES:
      if (auto pos = getRandomCloseTile(c->getPosition(), border,
            [this, c](Position pos) {
                return pos.isSameLevel(level) && pos.getCoverInfo().sunlight < 1 &&
                    (!c->getPosition().isSameLevel(level) || c->isSameSector(pos));}))
        return pos;
    case MinionTask::EXPLORE:
    case MinionTask::EXPLORE_NOCTURNAL:
      return getRandomCloseTile(c->getPosition(), border,
          [this, c](Position pos) { return pos.isSameLevel(level) && !pos.getCoverInfo().covered
              && (!c->getPosition().isSameLevel(level) || c->isSameSector(pos));});
    default: FAIL << "Unrecognized explore task: " << int(task);
  }
  return none;
}

bool Collective::isMinionTaskPossible(Creature* c, MinionTask task) {
  return isTaskGood(c, task, true) && generateMinionTask(c, task);
}

PTask Collective::generateMinionTask(Creature* c, MinionTask task) {
  MinionTaskInfo info = getTaskInfo().at(task);
  switch (info.type) {
    case MinionTaskInfo::APPLY_SQUARE: {
      vector<Position> squares = getAllSquares(info.squares, info.centerOnly);
      if (!squares.empty())
        return Task::applySquare(this, squares);
      break;
      }
    case MinionTaskInfo::EXPLORE:
      if (auto pos = getTileToExplore(c, task))
        return Task::explore(*pos);
      break;
    case MinionTaskInfo::COPULATE:
      if (Creature* target = getCopulationTarget(c))
        return Task::copulate(this, target, 20);
      break;
    case MinionTaskInfo::CONSUME:
      if (Creature* target = getConsumptionTarget(c))
        return Task::consume(this, target);
      break;
    case MinionTaskInfo::EAT: {
      const set<Position>& hatchery = getSquares(getHatcheryType(tribe));
      if (!hatchery.empty())
        return Task::eat(hatchery);
      break;
      }
    case MinionTaskInfo::SPIDER: return Task::spider(territory->getExtended(5), territory->getExtended(17));
  }
  return nullptr;
}

PTask Collective::getStandardTask(Creature* c) {
  if (!c->getMinionTasks().hasAnyTask())
    return nullptr;
  if (!currentTasks.count(c->getUniqueId()) ||
      currentTasks.at(c->getUniqueId()).finishTime < c->getTime() ||
      !isTaskGood(c, currentTasks.at(c->getUniqueId()).task))
    currentTasks.erase(c->getUniqueId());
  if (!currentTasks.count(c->getUniqueId()))
    setRandomTask(c);
  if (!currentTasks.count(c->getUniqueId()))
    return nullptr;
  MinionTask task = currentTasks[c->getUniqueId()].task;
  MinionTaskInfo info = getTaskInfo().at(task);
  PTask ret = generateMinionTask(c, task);
  if (info.warning && !territory->isEmpty())
    setWarning(*info.warning, !ret);
  if (!ret)
    currentTasks.erase(c->getUniqueId());
  else
    minionTaskStrings[c->getUniqueId()] = info.description;
  return ret;
}

SquareType Collective::getHatcheryType(Tribe* tribe) {
  return {SquareId::HATCHERY, CreatureFactory::SingleCreature(tribe, CreatureId::PIG)};
}

Creature* Collective::getCopulationTarget(Creature* succubus) {
  for (Creature* c : Random.permutation(getCreatures(MinionTrait::FIGHTER)))
    if (succubus->canCopulateWith(c))
      return c;
  return nullptr;
}

Creature* Collective::getConsumptionTarget(Creature* consumer) {
  vector<Creature*> v = getConsumptionTargets(consumer);
  if (!v.empty())
    return Random.choose(v);
  else
    return nullptr;
}

vector<Creature*> Collective::getConsumptionTargets(Creature* consumer) {
  vector<Creature*> ret;
  for (Creature* c : Random.permutation(getCreatures(MinionTrait::FIGHTER)))
    if (consumer->canConsume(c) && c != getLeader())
      ret.push_back(c);
  return ret;
}

bool Collective::isConquered() const {
  return getCreatures({MinionTrait::FIGHTER}).empty() && !hasLeader();
}

void Collective::orderConsumption(Creature* consumer, Creature* who) {
  CHECK(consumer->getMinionTasks().getValue(MinionTask::CONSUME) > 0);
  setMinionTask(who, MinionTask::CONSUME);
  MinionTaskInfo info = getTaskInfo().at(currentTasks[consumer->getUniqueId()].task);
  minionTaskStrings[consumer->getUniqueId()] = info.description;
  taskMap->freeFromTask(consumer);
  taskMap->addTask(Task::consume(this, who), consumer);
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
    if (!c->getEquipment().isEquiped(it) && c->getEquipment().canEquip(it))
      tasks.push_back(Task::equipItem(it));
  for (Position v : getAllSquares(equipmentStorage)) {
    vector<Item*> it = filter(v.getItems(ItemIndex::MINION_EQUIPMENT), 
        [this, c] (const Item* it) { return minionEquipment->getOwner(it) == c && it->canEquip(); });
    if (!it.empty())
      tasks.push_back(Task::pickAndEquipItem(this, v, it[0]));
    it = filter(v.getItems(ItemIndex::MINION_EQUIPMENT), 
        [this, c] (const Item* it) { return minionEquipment->getOwner(it) == c; });
    if (!it.empty())
      tasks.push_back(Task::pickItem(this, v, it));
  }
  if (!tasks.empty())
    return Task::chain(std::move(tasks));
  return nullptr;
}

PTask Collective::getHealingTask(Creature* c) {
  if (c->getHealth() < 1 && c->canSleep() && !c->isAffected(LastingEffect::POISON))
    for (MinionTask t : {MinionTask::SLEEP, MinionTask::GRAVE, MinionTask::LAIR})
      if (c->getMinionTasks().getValue(t) > 0) {
        vector<Position> positions = getAllSquares(getTaskInfo().at(t).squares);
        if (!positions.empty())
          return Task::applySquare(nullptr, positions);
      }
  return nullptr;
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

void Collective::setTask(const Creature *c, PTask task, bool priority) {
  if (Task* task = taskMap->getTask(c)) {
    if (!task->canTransfer())
      returnResource(taskMap->removeTask(task));
    else
      taskMap->freeTaskDelay(task, getTime() + 50);
  }
  if (priority)
    taskMap->addPriorityTask(std::move(task), c);
  else
    taskMap->addTask(std::move(task), c);
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
  if (Task* task = taskMap->getTask(c))
    return task->getMove(c);
  if (PTask t = getHealingTask(c))
    if (t->getMove(c))
      return taskMap->addTask(std::move(t), c)->getMove(c);
  if (Task* closest = taskMap->getClosestTask(c, MinionTrait::FIGHTER)) {
    taskMap->takeTask(c, closest);
    return closest->getMove(c);
  }
  if (usesEquipment(c))
    if (PTask t = getEquipmentTask(c))
      if (t->getMove(c))
        return taskMap->addTask(std::move(t), c)->getMove(c);
  if (PTask t = getStandardTask(c))
    if (t->getMove(c))
      return taskMap->addTask(std::move(t), c)->getMove(c);
  if (!hasTrait(c, MinionTrait::NO_RETURNING) && !territory->isEmpty() &&
      !territory->contains(c->getPosition()) && teams->getActive(c).empty())
    return c->moveTowards(Random.choose(territory->getAll()));
  else
    return NoMove;
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

void Collective::considerHealingLeader() {
  if (Deity* deity = Deity::getDeity(EpithetId::HEALTH))
    if (getStanding(deity) > 0 && Random.rollD(5 / getStanding(deity)) && hasLeader() && leader->getHealth() < 1) {
      leader->you(MsgType::ARE, "healed by " + deity->getName());
      leader->heal(1, true);
    }
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

optional<SquareType> Collective::getSecondarySquare(SquareType type) {
  switch (type.getId()) {
    case SquareId::DORM: return SquareType(SquareId::BED);
    case SquareId::BEAST_LAIR: return SquareType(SquareId::BEAST_CAGE);
    case SquareId::CEMETERY: return SquareType(SquareId::GRAVE);
    default: return none;
  }
}

struct Collective::DormInfo {
  SquareType dormType;
  optional<SquareType> getBedType() const {
    return getSecondarySquare(dormType);
  }
  optional<Collective::Warning> warning;
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

vector<Position> Collective::getSpawnPos(const vector<Creature*>& creatures) {
  vector<Position> extendedTiles = territory->getExtended(10, 20);
  if (extendedTiles.empty())
    return {};
  vector<Position> spawnPos;
  for (auto c : creatures) {
    Position pos;
    int cnt = 100;
    do {
      pos = Random.choose(extendedTiles);
    } while ((!pos.canEnter(c) || contains(spawnPos, pos)) && --cnt > 0);
    if (cnt == 0) {
      Debug() << "Couldn't spawn immigrant " << c->getName().bare();
      return {};
    } else
      spawnPos.push_back(pos);
  }
  return spawnPos;
}

bool Collective::considerNonSpawnImmigrant(const ImmigrantInfo& info, vector<PCreature> creatures) {
  CHECK(!info.spawnAtDorm);
  auto creatureRefs = extractRefs(creatures);
  vector<Position> spawnPos;
  spawnPos = getSpawnPos(creatureRefs);
  if (spawnPos.size() < creatures.size())
    return false;
  if (info.autoTeam)
    teams->activate(teams->createPersistent(extractRefs(creatures)));
  for (int i : All(creatures)) {
    Creature* c = creatures[i].get();
    addCreature(std::move(creatures[i]), spawnPos[i], info.traits);
    minionPayment[c] = {info.salary, 0.0, 0};
    minionAttraction[c] = info.attractions;
  }
  addNewCreatureMessage(creatureRefs);
  return true;
}

static CostInfo getSpawnCost(SpawnType type, int howMany) {
  switch (type) {
    case SpawnType::UNDEAD: return {CollectiveResourceId::CORPSE, howMany};
    default: return {CollectiveResourceId::GOLD, 0};
  }
}

void Collective::considerBuildingBeds() {
  bool bedsWarning = false;
  for (auto spawnType : ENUM_ALL(SpawnType))
    if (auto bedType = getDormInfo()[spawnType].getBedType()) {
      int neededBeds = bySpawnType[spawnType].size() - constructions->getSquareCount(*bedType);
      if (neededBeds > 0)
        bedsWarning |= tryBuildingBeds(spawnType, neededBeds) < neededBeds;
    }
  warnings[Warning::BEDS] = bedsWarning;
}

bool Collective::considerImmigrant(const ImmigrantInfo& info) {
  if (info.techId && !hasTech(*info.techId))
    return false;
  vector<PCreature> creatures;
  int groupSize = info.groupSize ? Random.get(*info.groupSize) : 1;
  groupSize = min(groupSize, getMaxPopulation() - getPopulationSize());
  for (int i : Range(groupSize))
    creatures.push_back(CreatureFactory::fromId(info.id, getTribe(), MonsterAIFactory::collective(this)));
  if (!creatures[0]->getSpawnType() || info.ignoreSpawnType)
    return considerNonSpawnImmigrant(info, std::move(creatures));
  SpawnType spawnType = *creatures[0]->getSpawnType();
  SquareType dormType = getDormInfo()[spawnType].dormType;
  if (!hasResource(getSpawnCost(spawnType, groupSize)))
    return false;
  vector<Position> spawnPos;
  if (info.spawnAtDorm) {
    for (Position v : Random.permutation(getSquares(dormType)))
      if (v.canEnter(creatures[spawnPos.size()].get())) {
        spawnPos.push_back(v);
        if (spawnPos.size() >= creatures.size())
          break;
      }
  } else
    spawnPos = getSpawnPos(extractRefs(creatures));
  groupSize = min<int>(groupSize, spawnPos.size());
  if (auto bedType = getDormInfo()[spawnType].getBedType()) {
    int neededBeds = bySpawnType[spawnType].size() + groupSize - constructions->getSquareCount(*bedType);
    if (neededBeds > 0) {
      int numBuilt = tryBuildingBeds(spawnType, neededBeds);
      if (numBuilt == 0)
        return false;
      groupSize -= neededBeds - numBuilt;
    }
  }
  if (groupSize < 1)
    return false;
  if (creatures.size() > groupSize)
    creatures.resize(groupSize);
  takeResource(getSpawnCost(spawnType, creatures.size()));
  if (info.autoTeam && groupSize > 1)
    teams->activate(teams->createPersistent(extractRefs(creatures)));
  addNewCreatureMessage(extractRefs(creatures));
  for (int i : All(creatures)) {
    Creature* c = creatures[i].get();
    if (i == 0 && groupSize > 1) // group leader
      c->increaseExpLevel(2);
    addCreature(std::move(creatures[i]), spawnPos[i], info.traits);
    minionPayment[c] = {info.salary, 0.0, 0};
    minionAttraction[c] = info.attractions;
  }
  return true;
}

int Collective::tryBuildingBeds(SpawnType spawnType, int numBeds) {
  int numBuilt = 0;
  SquareType bedType = *getDormInfo()[spawnType].getBedType();
  SquareType dormType = getDormInfo()[spawnType].dormType;
  set<Position> bedPos = getSquares(bedType);
  set<Position> dormPos = getSquares(dormType);
  for (Position v : copyOf(dormPos))
    if (constructions->containsSquare(v) && !constructions->getSquare(v).isBuilt()) {
      // this means there is a bed planned here already
      dormPos.erase(v);
      bedPos.insert(v);
    }
  for (int i : Range(numBeds))
    if (auto newPos = chooseBedPos(dormPos, bedPos)) {
      ++numBuilt;
      addConstruction(*newPos, bedType, CostInfo::noCost(), false, false);
      bedPos.insert(*newPos);
      dormPos.erase(*newPos);
    } else
      break;
  return numBuilt;
}

void Collective::addNewCreatureMessage(const vector<Creature*>& creatures) {
  if (creatures.size() == 1)
    control->addMessage(PlayerMessage(creatures[0]->getName().a() + " joins your forces.")
        .setCreature(creatures[0]->getUniqueId()));
  else {
    control->addMessage(PlayerMessage("A " + creatures[0]->getGroupName(creatures.size()) + " joins your forces.")
        .setCreature(creatures[0]->getUniqueId()));
  }
}

double Collective::getImmigrantChance(const ImmigrantInfo& info) {
  if (info.limit && info.limit != getLevel()->getModel()->getSunlightInfo().state)
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

const Collective::ResourceId paymentResource = Collective::ResourceId::GOLD;

int Collective::getPaymentAmount(const Creature* c) const {
  if (!minionPayment.count(c))
    return 0;
  else
    return config->getPayoutMultiplier() *
      minionPayment.at(c).salary * minionPayment.at(c).workAmount / config->getPayoutTime();
}

int Collective::getNextSalaries() const {
  int ret = 0;
  for (Creature* c : creatures)
    if (minionPayment.count(c))
      ret += getPaymentAmount(c) + minionPayment.at(c).debt;
  return ret;
}

bool Collective::hasMinionDebt() const {
  for (Creature* c : creatures)
    if (minionPayment.count(c) && minionPayment.at(c).debt)
      return true;
  return false;
}

void Collective::makePayouts() {
  if (int numPay = getNextSalaries())
    control->addMessage(PlayerMessage("Payout time: " + toString(numPay) + " gold",
          PlayerMessage::HIGH));
  for (Creature* c : creatures)
    if (minionPayment.count(c)) {
      minionPayment.at(c).debt += getPaymentAmount(c);
      minionPayment.at(c).workAmount = 0;
    }
}

void Collective::cashPayouts() {
  int numGold = numResource(paymentResource);
  for (Creature* c : creatures)
    if (minionPayment.count(c))
      if (int payment = minionPayment.at(c).debt) {
        if (payment < numGold) {
          takeResource({paymentResource, payment});
          numGold -= payment;
          minionPayment.at(c).debt = 0;
        }
      }
}

struct BirthSpawn {
  CreatureId id;
  double frequency;
  optional<TechId> tech;
};

static vector<BirthSpawn> birthSpawns {
  { CreatureId::GOBLIN, 1 },
  { CreatureId::ORC, 1 },
  { CreatureId::ORC_SHAMAN, 0.5 },
  { CreatureId::HARPY, 0.5 },
  { CreatureId::OGRE, 0.5 },
  { CreatureId::WEREWOLF, 0.5 },
  { CreatureId::SPECIAL_HUMANOID, 1.5, TechId::HUMANOID_MUT},
  { CreatureId::SPECIAL_MONSTER_KEEPER, 1.5, TechId::BEAST_MUT },
};

void Collective::considerBirths() {
  if (getPopulationSize() < getMaxPopulation() && !pregnancies.empty() && Random.roll(300)) {
    Creature* c = pregnancies.front();
    pregnancies.pop_front();
    vector<pair<CreatureId, double>> candidates;
    for (auto& elem : birthSpawns)
      if (!elem.tech || hasTech(*elem.tech)) 
        candidates.emplace_back(elem.id, elem.frequency);
    if (candidates.empty())
      return;
    PCreature spawn = CreatureFactory::fromId(Random.choose(candidates), getTribe());
    for (Position pos : c->getPosition().neighbors8(Random))
      if (pos.canEnter(spawn.get())) {
        control->addMessage(c->getName().a() + " gives birth to " + spawn->getName().a());
        addCreature(std::move(spawn), pos, {MinionTrait::FIGHTER});
        return;
      }
  }
}

static vector<SquareType> roomsNeedingLight {
  SquareId::WORKSHOP,
  SquareId::FORGE,
  SquareId::LABORATORY,
  SquareId::JEWELER,
  SquareId::TRAINING_ROOM,
  SquareId::LIBRARY,
};

void Collective::considerWeaponWarning() {
  int numWeapons = getAllItems(ItemIndex::WEAPON).size();
  PItem genWeapon = ItemFactory::fromId(ItemId::SWORD);
  int numNeededWeapons = 0;
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    if (usesEquipment(c) && minionEquipment->needs(c, genWeapon.get(), true, true))
      ++numNeededWeapons;
  setWarning(Warning::NO_WEAPONS, numNeededWeapons > numWeapons);
}

void Collective::considerMoraleWarning() {
  vector<Creature*> minions = getCreatures(MinionTrait::FIGHTER);
  setWarning(Warning::LOW_MORALE,
      filter(minions, [] (const Creature* c) { return c->getMorale() < -0.2; }).size() > minions.size() / 2);
}

void Collective::decayMorale() {
  for (Creature* c : getCreatures(MinionTrait::FIGHTER))
    c->addMorale(-c->getMorale() * 0.0008);
}

void Collective::considerSpawningGhosts() {
  if (deadCreatures.empty() || config->getNumGhostSpawns() == 0)
    return;
  if (spawnGhosts && *spawnGhosts <= getTime()) {
    for (Creature* dead : getPrefix(Random.permutation(deadCreatures),
          min<int>(config->getNumGhostSpawns(), deadCreatures.size())))
      addCreatureInTerritory(CreatureFactory::getGhost(dead), {});
    spawnGhosts = 1000000;
  } else
  if (!spawnGhosts) {
    if (Random.roll(1.0 / config->getGhostProb()))
      spawnGhosts = getTime() + Random.get(250, 500);
    else
      spawnGhosts = 1000000;
  }
}

int Collective::getNumKilled(double time) {
  int ret = 0;
  for (Creature* c : deadCreatures)
    if (c->getDeathTime() >= time)
      ++ret;
  return ret;
}

void Collective::considerSendingGuardian() {
  if (auto info = config->getGuardianInfo())
    if (!getCreatures().empty() && (!lastGuardian || lastGuardian->isDead()) &&
        Random.roll(1.0 / info->probability)) {
      vector<Position> enemyPos = getEnemyPositions();
      if (enemyPos.size() >= info->minEnemies && getNumKilled(getTime() - 200) >= info->minVictims) {
        PCreature guardian = CreatureFactory::fromId(info->creature, getTribe(),
            MonsterAIFactory::singleTask(Task::chain(
                Task::kill(this, Random.choose(enemyPos).getCreature()),
                Task::goTo(Random.choose(territory->getExtended(10, 20))),
                Task::disappear())));
        lastGuardian = guardian.get();
        getLevel()->landCreature(territory->getExtended(10, 20), std::move(guardian));
      }
    }
}

void Collective::tick(double time) {
  control->tick(time);
  considerHealingLeader();
  considerBirths();
  decayMorale();
  considerBuildingBeds();
  if (isConquered())
    considerSpawningGhosts();
  if (Random.rollD(1.0 / config->getImmigrantFrequency()))
    considerImmigration();
  considerSendingGuardian();
/*  if (nextPayoutTime > -1 && time > nextPayoutTime) {
    nextPayoutTime += config->getPayoutTime();
    makePayouts();
  }
  cashPayouts();*/
  if (config->getWarnings() && Random.roll(5)) {
    considerWeaponWarning();
    considerMoraleWarning();
    setWarning(Warning::MANA, numResource(ResourceId::MANA) < 100);
    setWarning(Warning::DIGGING, getSquares(SquareId::FLOOR).empty());
    setWarning(Warning::MORE_LIGHTS,
        constructions->getTorches().size() * 25 < getAllSquares(roomsNeedingLight).size());
    for (SpawnType spawnType : ENUM_ALL(SpawnType)) {
      DormInfo info = getDormInfo()[spawnType];
      if (info.warning && info.getBedType())
        setWarning(*info.warning, !chooseBedPos(getSquares(info.dormType), getSquares(*info.getBedType())));
    }
    for (auto elem : getTaskInfo())
      if (!getAllSquares(elem.second.squares).empty() && elem.second.warning)
        setWarning(*elem.second.warning, false);
  }
  if (config->getEnemyPositions() && Random.roll(5)) {
    vector<Position> enemyPos = getEnemyPositions();
    if (!enemyPos.empty())
      delayDangerousTasks(enemyPos, getTime() + 20);
    else {
      alarmInfo.reset();
      control->onNoEnemies();
    }
    bool allSurrender = true;
    for (Position v : enemyPos)
      if (!surrendering.count(NOTNULL(v.getCreature()))) {
        allSurrender = false;
        break;
      }
    if (allSurrender) {
      for (Creature* c : copyOf(surrendering)) {
        if (!c->isDead() && territory->contains(c->getPosition())) {
          Position pos = c->getPosition();
          PCreature prisoner = CreatureFactory::fromId(CreatureId::PRISONER, getTribe(),
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
    for (const ItemFetchInfo& elem : getFetchInfo()) {
      for (Position pos : territory->getAll())
        fetchItems(pos, elem);
      for (SquareType type : elem.additionalPos)
        for (Position pos : getSquares(type))
          fetchItems(pos, elem);
    }
  if (config->getManageEquipment() && Random.roll(10))
    minionEquipment->updateOwners(getAllItems(true));
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
  int ret = victim->getDifficultyPoints() / 3;
  if (victim->isAffected(LastingEffect::SLEEP))
    ret *= 2;
  return ret;
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

void Collective::onKilled(Creature* victim, Creature* killer) {
  if (contains(creatures, victim)) {
    if (hasTrait(victim, MinionTrait::PRISONER) && killer && contains(getCreatures(), killer))
      returnResource({ResourceId::PRISONER_HEAD, 1});
    if (victim == leader)
      getLevel()->getModel()->onKilledLeader(this, victim);
    if (!hasTrait(victim, MinionTrait::FARM_ANIMAL)) {
      decreaseMoraleForKill(killer, victim);
      if (killer)
        control->addMessage(PlayerMessage(victim->getName().a() + " is killed by " + killer->getName().a(),
              PlayerMessage::HIGH).setPosition(victim->getPosition()));
      else
        control->addMessage(PlayerMessage(victim->getName().a() + " is killed.", PlayerMessage::HIGH)
            .setPosition(victim->getPosition()));
    }
    removeCreature(victim);
    control->onMemberKilled(victim, killer);
  } else
    control->onOtherKilled(victim, killer);
  if (victim->getTribe() != getTribe() && (/*!killer || */contains(creatures, killer))) {
    addMana(getKillManaScore(victim));
    addMoraleForKill(killer, victim);
    kills.push_back(victim);
    int difficulty = victim->getDifficultyPoints();
    CHECK(difficulty >=0 && difficulty < 100000) << difficulty << " " << victim->getName().bare();
    points += difficulty;
    if (killer)
      control->addMessage(PlayerMessage(victim->getName().a() + " is killed by " + killer->getName().a())
          .setPosition(victim->getPosition()));
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

double Collective::getEfficiency(const Creature* c) const {
  return pow(2.0, c->getMorale());
}

vector<Position> Collective::getAllSquares(const vector<SquareType>& types, bool centerOnly) const {
  vector<Position> ret;
  for (SquareType type : types)
    append(ret, getSquares(type));
  if (centerOnly)
    ret = filter(ret, [this] (Position pos) {
        return squareEfficiency.count(pos) && squareEfficiency.at(pos) == 8;});
  return ret;
}

const set<Position>& Collective::getSquares(SquareType type) const {
  static set<Position> empty;
  if (mySquares.count(type))
    return mySquares.at(type);
  else
    return empty;
}

const set<Position>& Collective::getSquares(SquareApplyType type) const {
  static set<Position> empty;
  if (mySquares2.count(type))
    return mySquares2.at(type);
  else
    return empty;
}

vector<SquareType> Collective::getSquareTypes() const {
  return getKeys(mySquares);
}

const Territory& Collective::getTerritory() const {
  return *territory;
}

void Collective::claimSquare(Position pos) {
  territory->insert(pos);
  if (pos.getApplyType() == SquareApplyType::SLEEP)
    mySquares[SquareId::BED].insert(pos);
  if (pos.getApplyType() == SquareApplyType::CROPS)
    mySquares[SquareId::CROPS].insert(pos);
  if (auto type = pos.getApplyType())
    mySquares2[*type].insert(pos);
}

void Collective::changeSquareType(Position pos, SquareType from, SquareType to) {
  mySquares[from].erase(pos);
  mySquares[to].insert(pos);
  for (auto& elem : mySquares2)
    if (elem.second.count(pos))
      elem.second.erase(pos);
  if (auto type = pos.getApplyType())
    mySquares2[*type].insert(pos);
}

bool Collective::isKnownSquare(Position pos) const {
  return knownTiles->isKnown(pos);
}

void Collective::update(Creature* c) {
  control->update(c);
}

const static unordered_set<SquareType> efficiencySquares {
  SquareId::TRAINING_ROOM,
  SquareId::TORTURE_TABLE,
  SquareId::WORKSHOP,
  SquareId::FORGE,
  SquareId::LABORATORY,
  SquareId::JEWELER,
  SquareId::LIBRARY,
};

bool Collective::hasEfficiency(Position pos) const {
  return squareEfficiency.count(pos);
}

const double lightBase = 0.5;
const double flattenVal = 0.9;

double Collective::getEfficiency(Position pos) const {
  double base = squareEfficiency.at(pos) == 8 ? 1 : 0.5;
  return base * min(1.0, (lightBase + pos.getLight() * (1 - lightBase)) / flattenVal);
}

const double sizeBase = 0.5;

void Collective::updateEfficiency(Position pos, SquareType type) {
  if (getSquares(type).count(pos)) {
    squareEfficiency[pos] = 0;
    for (Position v : pos.neighbors8())
      if (getSquares(type).count(v)) {
        ++squareEfficiency[pos];
        ++squareEfficiency[v];
        CHECK(squareEfficiency[v] <= 8);
        CHECK(squareEfficiency[pos] <= 8);
      }
  } else {
    squareEfficiency.erase(pos);
    for (Position v : pos.neighbors8())
      if (getSquares(type).count(v)) {
        --squareEfficiency[v];
        CHECK(squareEfficiency[v] >=0) << EnumInfo<SquareId>::getString(type.getId());
      }
  }
}

static const int alarmTime = 100;

void Collective::onAlarm(Position pos) {
  control->addMessage(PlayerMessage("An alarm goes off.", PlayerMessage::HIGH).setPosition(pos));
  alarmInfo = {getTime() + alarmTime, pos };
  for (Creature* c : byTrait[MinionTrait::FIGHTER])
    if (c->isAffected(LastingEffect::SLEEP))
      c->removeEffect(LastingEffect::SLEEP);
}

MoveInfo Collective::getAlarmMove(Creature* c) {
  if (alarmInfo && alarmInfo->finishTime > c->getTime())
    if (auto action = c->moveTowards(alarmInfo->position))
      return {1.0, action};
  return NoMove;
}

double Collective::getTime() const {
  return getLevel()->getModel()->getTime();
}

int Collective::numResource(ResourceId id) const {
  int ret = credit[id];
  if (resourceInfo.at(id).itemIndex)
    for (SquareType type : resourceInfo.at(id).storageType)
      for (Position pos : getSquares(type))
        ret += pos.getItems(*resourceInfo.at(id).itemIndex).size();
  return ret;
}

int Collective::numResourcePlusDebt(ResourceId id) const {
  int ret = numResource(id);
  for (Position pos : constructions->getSquares()) {
    auto& construction = constructions->getSquare(pos);
    if (!construction.isBuilt() && construction.getCost().id == id)
      ret -= construction.getCost().value;
  }
  for (auto& elem : taskMap->getCompletionCosts())
    if (elem.second.id == id && !elem.first->isDone())
      ret += elem.second.value;
  if (id == ResourceId::GOLD)
    for (auto& elem : minionPayment)
      ret -= elem.second.debt;
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
  if (resourceInfo.at(cost.id).itemIndex)
    for (Position pos : Random.permutation(getAllSquares(resourceInfo.at(cost.id).storageType))) {
      vector<Item*> goldHere = pos.getItems(*resourceInfo.at(cost.id).itemIndex);
      for (Item* it : goldHere) {
        pos.removeItem(it);
        if (--num == 0)
          return;
      }
    }
  FAIL << "Not enough " << resourceInfo.at(cost.id).name << " missing " << num << " of " << cost.value;
}

void Collective::returnResource(const CostInfo& amount) {
  if (amount.value == 0)
    return;
  CHECK(amount.value > 0);
  vector<Position> destination = getAllSquares(resourceInfo.at(amount.id).storageType);
  if (!destination.empty()) {
    Random.choose(destination).dropItems(ItemFactory::fromId(resourceInfo.at(amount.id).itemId, amount.value));
  } else
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
    && c->isHumanoid() 
    && !hasTrait(c, MinionTrait::NO_EQUIPMENT)
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
      return minionEquipment->getOwner(it) == creature;});
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

void Collective::orderExecution(Creature* c) {
  taskMap->addTask(Task::kill(this, c), c->getPosition(), MinionTrait::FIGHTER);
  setTask(c, Task::goToAndWait(c->getPosition(), getTime() + 100));
}

void Collective::orderTorture(Creature* c) {
  vector<Position> posts = getAllSquares({SquareId::TORTURE_TABLE}, true);
  for (Position p : squaresInUse)
    removeElement(posts, p);
  if (posts.empty())
    return;
  Position pos = Random.choose(posts);
  squaresInUse.insert(pos);
  taskMap->addTask(Task::torture(this, c), pos, MinionTrait::FIGHTER);
  setTask(c, Task::goToAndWait(pos, getTime() + 100));
}

void Collective::orderSacrifice(Creature* c) {
}

void Collective::onWhippingDone(Creature* whipped, Position pos) {
  cancelTask(whipped);
  squaresInUse.erase(pos);
}

bool Collective::canWhip(Creature* c) const {
  return c->affects(LastingEffect::ENTANGLED);
}

void Collective::orderWhipping(Creature* whipped) {
  if (!canWhip(whipped))
    return;
  set<Position> posts = getSquares(SquareId::WHIPPING_POST);
  for (Position p : squaresInUse)
    posts.erase(p);
  if (posts.empty())
    return;
  Position pos = Random.choose(posts);
  squaresInUse.insert(pos);
  taskMap->addTask(Task::whipping(this, pos, whipped, 100, getTime() + 100), pos, MinionTrait::FIGHTER);
  setTask(whipped, Task::goToAndWait(pos, getTime() + 100));
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

void Collective::removeConstruction(Position pos) {
  if (constructions->getSquare(pos).hasTask())
    returnResource(taskMap->removeTask(constructions->getSquare(pos).getTask()));
  constructions->removeSquare(pos);
}

void Collective::destroySquare(Position pos) {
  if (pos.isDestroyable() && territory->contains(pos))
    pos.destroy();
  if (Creature* c = pos.getCreature())
    if (c->isStationary())
      c->die(nullptr, false);
  if (constructions->containsTrap(pos)) {
    removeTrap(pos);
  }
  if (constructions->containsSquare(pos))
    removeConstruction(pos);
  if (constructions->containsTorch(pos))
    removeTorch(pos);
  pos.removeTriggers();
}

void Collective::addConstruction(Position pos, SquareType type, const CostInfo& cost, bool immediately,
    bool noCredit) {
  if (type.getId() == SquareId::MOUNTAIN2 && (pos.isChokePoint({MovementTrait::WALK}) ||
        constructions->getSquareCount(type) > 0))
    return;
  if (immediately && hasResource(cost)) {
    while (!pos.construct(type)) {}
    onConstructed(pos, type);
  } else if (!noCredit || hasResource(cost)) {
    constructions->addSquare(pos, ConstructionMap::SquareInfo(type, cost));
    updateConstructions();
  }
}

const ConstructionMap& Collective::getConstructions() const {
  return *constructions;
}

void Collective::dig(Position pos) {
  taskMap->markSquare(pos, HighlightType::DIG, Task::construction(this, pos, SquareId::FLOOR));
}

void Collective::cancelMarkedTask(Position pos) {
  taskMap->unmarkSquare(pos);
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

void Collective::cutTree(Position pos) {
  taskMap->markSquare(pos, HighlightType::CUT_TREE, Task::construction(this, pos, SquareId::TREE_TRUNK));
}

set<TrapType> Collective::getNeededTraps() const {
  set<TrapType> ret;
  for (auto elem : constructions->getTraps())
    if (!elem.second.isMarked() && !elem.second.isArmed())
      ret.insert(elem.second.getType());
  return ret;
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

void Collective::onConstructed(Position pos, const SquareType& type) {
  CHECK(!getSquares(type).count(pos));
  for (auto& elem : mySquares)
      elem.second.erase(pos);
  if (type.getId() == SquareId::MOUNTAIN2) {
    destroySquare(pos);
    if (territory->contains(pos))
      territory->remove(pos);
    return;
  }
  if (!contains({SquareId::TREE_TRUNK}, type.getId()))
    territory->insert(pos);
  mySquares[type].insert(pos);
  if (efficiencySquares.count(type))
    updateEfficiency(pos, type);
  if (constructions->containsSquare(pos) && !constructions->getSquare(pos).isBuilt())
    constructions->getSquare(pos).setBuilt();
  if (type == SquareId::FLOOR) {
    for (Position v : pos.neighbors4())
      if (constructions->containsTorch(v) &&
          constructions->getTorch(v).getAttachmentDir() == v.getDir(pos).getCardinalDir())
        removeTorch(v);
  }
  if (auto type = pos.getApplyType())
    mySquares2[*type].insert(pos);
  control->onConstructed(pos, type);
  if (taskMap->getMarked(pos))
    taskMap->unmarkSquare(pos);
}

bool Collective::tryLockingDoor(Position pos) {
  if (territory->contains(pos) && pos.canLock()) {
    pos.lock();
    return true;
  }
  return false;
}

void Collective::updateConstructions() {
  map<TrapType, vector<pair<Item*, Position>>> trapItems;
  for (TrapType type : ENUM_ALL(TrapType))
    trapItems[type] = getTrapItems(type, territory->getAll());
  for (auto elem : constructions->getTraps())
    if (!isDelayed(elem.first)) {
      vector<pair<Item*, Position>>& items = trapItems.at(elem.second.getType());
      if (!items.empty()) {
        if (!elem.second.isArmed() && !elem.second.isMarked()) {
          Position pos = items.back().second;
          taskMap->addTask(Task::applyItem(this, pos, items.back().first, elem.first), pos);
          markItem(items.back().first);
          items.pop_back();
          constructions->getTrap(elem.first).setMarked();
        }
      }
    }
  for (Position pos : constructions->getSquares()) {
    auto& construction = constructions->getSquare(pos);
    if (!isDelayed(pos) && !construction.hasTask() && !construction.isBuilt()) {
      if (!hasResource(construction.getCost()))
        continue;
      construction.setTask(
          taskMap->addTaskCost(Task::construction(this, pos, construction.getSquareType()), pos,
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
  return delayedPos.count(pos) && delayedPos.at(pos) > getTime();
}

void Collective::fetchAllItems(Position pos) {
  if (isKnownSquare(pos)) {
    vector<PTask> tasks;
    for (const ItemFetchInfo& elem : getFetchInfo()) {
      vector<Item*> equipment = filter(pos.getItems(elem.index), elem.predicate);
      if (!equipment.empty()) {
        vector<Position> destination = getAllSquares(elem.destination);
        if (!destination.empty()) {
          setWarning(elem.warning, false);
          if (elem.oneAtATime)
            for (Item* it : equipment)
              tasks.push_back(Task::bringItem(this, pos, {it}, destination));
          else
            tasks.push_back(Task::bringItem(this, pos, equipment, destination));
          for (Item* it : equipment)
            markItem(it);
        } else
          setWarning(elem.warning, true);
      }
    }
    if (!tasks.empty())
      taskMap->markSquare(pos, HighlightType::FETCH_ITEMS, Task::chain(std::move(tasks)));
  }
}

void Collective::fetchItems(Position pos, const ItemFetchInfo& elem) {
  if (isDelayed(pos) || (constructions->containsTrap(pos) &&
        constructions->getTrap(pos).getType() == TrapType::BOULDER &&
        constructions->getTrap(pos).isArmed()))
    return;
  for (SquareType type : elem.destination)
    if (getSquares(type).count(pos))
      return;
  vector<Item*> equipment = filter(pos.getItems(elem.index), elem.predicate);
  if (!equipment.empty()) {
    vector<Position> destination = getAllSquares(elem.destination);
    if (!destination.empty()) {
      setWarning(elem.warning, false);
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      taskMap->addTask(Task::bringItem(this, pos, equipment, destination), pos);
      for (Item* it : equipment)
        markItem(it);
    } else
      setWarning(elem.warning, true);
  }
}

void Collective::onSurrender(Creature* who) {
  if (!contains(getCreatures(), who) && who->isHumanoid())
    surrendering.insert(who);
}

void Collective::onSquareDestroyed(Position pos) {
  for (auto& elem : mySquares)
    if (elem.second.count(pos)) {
      elem.second.erase(pos);
      if (efficiencySquares.count(elem.first))
        updateEfficiency(pos, elem.first);
    }
  for (auto& elem : mySquares2)
    if (elem.second.count(pos))
      elem.second.erase(pos);
  mySquares[SquareId::FLOOR].insert(pos);
  if (constructions->containsSquare(pos))
    constructions->onSquareDestroyed(pos);
}

void Collective::onTrapTrigger(Position pos) {
  if (constructions->containsTrap(pos)) {
    constructions->getTrap(pos).reset();
    if (constructions->getTrap(pos).getType() == TrapType::SURPRISE)
      handleSurprise(pos);
  }
}

void Collective::onTrapDisarm(const Creature* who, Position pos) {
  if (constructions->containsTrap(pos)) {
    control->addMessage(PlayerMessage(who->getName().a() + " disarms a " 
          + Item::getTrapName(constructions->getTrap(pos).getType()) + " trap.",
          PlayerMessage::HIGH).setPosition(pos));
    constructions->getTrap(pos).reset();
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
          if (getLevel()->canMoveCreature(other, other->getPosition().getDir(dest))) {
            getLevel()->moveCreature(other, other->getPosition().getDir(dest));
            other->playerMessage("Surprise!");
            if (!wasMsg) {
              c->playerMessage("Surprise!");
              wasMsg = true;
            }
            break;
          }
      }
}

void Collective::onPickedUp(Position pos, EntitySet<Item> items) {
  for (auto id : items)
    unmarkItem(id);
}

void Collective::onCantPickItem(EntitySet<Item> items) {
  for (auto id : items)
    unmarkItem(id);
}

bool Collective::isKnownVillain(const Collective* col) {
  return knownVillains.count(col);
}

void Collective::addKnownTile(Position pos) {
  if (!knownTiles->isKnown(pos)) {
    if (const Location* loc = pos.getLocation())
      if (!knownLocations.count(loc)) {
        knownLocations.insert(loc);
        control->onDiscoveredLocation(loc);
      }
    knownTiles->addTile(pos);
    if (pos.getLevel() == level)
      if (Task* task = taskMap->getMarked(pos))
        if (task->isImpossible(getLevel()))
          taskMap->removeTask(task);
    for (const Collective* col : level->getModel()->getMainVillains())
      if (col->territory->contains(pos))
        knownVillains.insert(col);
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

void Collective::addProducesMessage(const Creature* c, const vector<PItem>& items) {
  if (items.size() > 1)
    control->addMessage(c->getName().a() + " produces " + toString(items.size())
        + " " + items[0]->getName(true));
  else
    control->addMessage(c->getName().a() + " produces " + items[0]->getAName());
}

static vector<SquareType> workshopSquares {
  SquareId::WORKSHOP,
  SquareId::FORGE,
  SquareId::JEWELER,
  SquareId::LABORATORY
};

struct WorkshopInfo {
  ItemFactory factory;
  CostInfo itemCost;
};

// The production cost is actually not applied ATM.
static WorkshopInfo getWorkshopInfo(Collective* c, Position pos) {
  for (auto elem : workshopSquares)
    if (c->getSquares(elem).count(pos))
      switch (elem.getId()) {
        case SquareId::WORKSHOP:
            return { ItemFactory::workshop(c->getTechnologies()), {CollectiveResourceId::GOLD, 0}};
        case SquareId::FORGE:
            return { ItemFactory::forge(c->getTechnologies()), {CollectiveResourceId::IRON, 10}};
        case SquareId::LABORATORY:
            return { ItemFactory::laboratory(c->getTechnologies()), {CollectiveResourceId::MANA, 10}};
        case SquareId::JEWELER:
            return { ItemFactory::jeweler(c->getTechnologies()), {CollectiveResourceId::GOLD, 5}};
        default: FAIL << "Bad workshop position ";// << pos;
      }
  return { ItemFactory::workshop(c->getTechnologies()),{CollectiveResourceId::GOLD, 5}};
}

void Collective::onAppliedSquare(Position pos) {
  Creature* c = NOTNULL(pos.getCreature());
  MinionTask currentTask = currentTasks.at(c->getUniqueId()).task;
  if (getTaskInfo().at(currentTask).cost > 0) {
    if (nextPayoutTime == -1 && minionPayment.count(c) && minionPayment.at(c).salary > 0)
      nextPayoutTime = getTime() + config->getPayoutTime();
    minionPayment[c].workAmount += getTaskInfo().at(currentTask).cost;
  }
  if (getSquares(SquareId::LIBRARY).count(pos)) {
    addMana(0.2);
    if (Random.rollD(60.0 / (getEfficiency(pos))) && !getAvailableSpells().empty())
      c->addSpell(Random.choose(getAvailableSpells()));
  }
  if (getSquares(SquareId::TRAINING_ROOM).count(pos))
    c->exerciseAttr(Random.choose<AttrType>(), getEfficiency(pos));
  if (contains(getAllSquares(workshopSquares), pos))
    if (Random.rollD(40.0 / (getCraftingMultiplier() * getEfficiency(pos)))) {
      vector<PItem> items;
      for (int i : Range(20)) {
        auto workshopInfo = getWorkshopInfo(this, pos);
        items = workshopInfo.factory.random();
        if (isItemNeeded(items[0].get()))
          break;
      }
      if (items[0]->getClass() == ItemClass::WEAPON)
        level->getModel()->getStatistics().add(StatId::WEAPON_PRODUCED);
      if (items[0]->getClass() == ItemClass::ARMOR)
        level->getModel()->getStatistics().add(StatId::ARMOR_PRODUCED);
      if (items[0]->getClass() == ItemClass::POTION)
        level->getModel()->getStatistics().add(StatId::POTION_PRODUCED);
      addProducesMessage(c, items);
      pos.dropItems(std::move(items));
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
    { SpellId::STUN_RAY, TechId::SPELLS},
    { SpellId::MAGIC_SHIELD, TechId::SPELLS_ADV},
    { SpellId::STR_BONUS, TechId::SPELLS_ADV},
    { SpellId::DEX_BONUS, TechId::SPELLS_ADV},
    { SpellId::FIRE_SPHERE_PET, TechId::SPELLS_ADV},
    { SpellId::TELEPORT, TechId::SPELLS_ADV},
    { SpellId::CURE_POISON, TechId::SPELLS_ADV},
    { SpellId::INVISIBILITY, TechId::SPELLS_MAS},
    { SpellId::BLAST, TechId::SPELLS_MAS},
    { SpellId::WORD_OF_POWER, TechId::SPELLS_MAS},
    { SpellId::PORTAL, TechId::SPELLS_MAS},
    { SpellId::METEOR_SHOWER, TechId::SPELLS_MAS},
};

vector<Spell*> Collective::getSpellLearning(const Technology* tech) {
  vector<Spell*> ret;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      ret.push_back(Spell::get(elem.id));
  return ret;
}

vector<Spell*> Collective::getAvailableSpells() const {
  vector<Spell*> ret;
  for (auto elem : spellLearning)
    if (hasTech(elem.techId))
      ret.push_back(Spell::get(elem.id));
  return ret;
}

vector<Spell*> Collective::getAllSpells() const {
  vector<Spell*> ret;
  for (auto elem : spellLearning)
    ret.push_back(Spell::get(elem.id));
  return ret;
}

TechId Collective::getNeededTech(Spell* spell) const {
  for (auto elem : spellLearning)
    if (elem.id == spell->getId())
      return elem.techId;
  FAIL << "Spell not found";
  return TechId(0);
}

double Collective::getDangerLevel() const {
  double ret = 0;
  for (const Creature* c : getCreatures({MinionTrait::FIGHTER}))
    ret += c->getDifficultyPoints();
  ret += getSquares(SquareId::IMPALED_HEAD).size() * 150;
  return ret;
}

bool Collective::hasTech(TechId id) const {
  return contains(technologies, id);
}

double Collective::getTechCost(Technology* t) {
  return t->getCost();
}

void Collective::acquireTech(Technology* tech, bool free) {
  technologies.push_back(tech->getId());
  if (free)
    ++numFreeTech;
  if (hasLeader())
    for (auto elem : spellLearning)
      if (Technology::get(elem.techId) == tech)
        getLeader()->addSpell(Spell::get(elem.id));
}

vector<Technology*> Collective::getTechnologies() const {
  return transform2<Technology*>(technologies, [] (const TechId t) { return Technology::get(t); });
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

void Collective::onEquip(const Creature* c, const Item* it) {
  minionEquipment->own(c, it);
}

void Collective::onRansomPaid() {
  control->onRansomPaid();
}

void Collective::ownItems(const Creature* who, const vector<Item*> items) {
  for (const Item* it : items)
    if (minionEquipment->isItemUseful(it))
      minionEquipment->own(who, it);
}

void Collective::onTorture(const Creature* who, const Creature* torturer) {
  returnResource({ResourceId::MANA, 1});
}

void Collective::onCopulated(Creature* who, Creature* with) {
  if (with->getName().bare() == "vampire")
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a()
        + " with a monkey on " + who->getGender().his() + " knee");
  else
    control->addMessage(who->getName().a() + " makes love to " + with->getName().a());
  if (contains(getCreatures(), with))
    with->addMorale(1);
  if (!contains(pregnancies, who)) 
    pregnancies.push_back(who);
}

void Collective::onConsumed(Creature* consumer, Creature* who) {
  control->addMessage(consumer->getName().a() + " absorbs " + who->getName().a());
  for (string s : consumer->popPersonalEvents())
    control->addMessage(s);
}

MinionEquipment& Collective::getMinionEquipment() {
  return *minionEquipment;
}

const MinionEquipment& Collective::getMinionEquipment() const {
  return *minionEquipment;
}

int Collective::getSalary(const Creature* c) const {
  if (minionPayment.count(c))
    return minionPayment.at(c).salary;
  else
    return 0;
}

int Collective::getNextPayoutTime() const {
  return nextPayoutTime;
}

void Collective::addAttack(const CollectiveAttack& attack) {
  control->addAttack(attack);
  knownVillains.insert(attack.getAttacker());
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
    if (p.canConstruct(SquareId::FLOOR))
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
  constructions->addTorch(pos,
      ConstructionMap::TorchInfo(getAdjacentWall(pos)->getCardinalDir()));
}

bool Collective::canPlaceTorch(Position pos) const {
  return getAdjacentWall(pos) && !constructions->containsTorch(pos) &&
    isKnownSquare(pos) && pos.canEnterEmpty({MovementTrait::WALK});
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
    int sz = getSquares(elem.type).size();
    ret += min<int>(elem.maxIncrease, elem.increasePerSquare * sz);
  }
  return ret;
}

template <class Archive>
void Collective::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, LeaderControlOverride);
}

REGISTER_TYPES(Collective::registerTypes);
