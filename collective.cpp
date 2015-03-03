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
#include "trigger.h"
#include "model.h"
#include "spell.h"

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
    & SVAR(lastCombat)
    & SVAR(kills)
    & SVAR(points)
    & SVAR(currentTasks)
    & SVAR(credit)
    & SVAR(level)
    & SVAR(minionPayment)
    & SVAR(flyingSectors)
    & SVAR(sectors)
    & SVAR(pregnancies)
    & SVAR(nextPayoutTime)
    & SVAR(minionAttraction)
    & SVAR(teams)
    & SVAR(knownLocations)
    & SVAR(torches)
    & SVAR(name)
    & SVAR(config);
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

Collective::MinionTaskInfo::MinionTaskInfo(vector<SquareType> s, const string& desc, optional<Warning> w,
    double _cost, bool center) 
    : type(APPLY_SQUARE), squares(s), description(desc), warning(w), cost(_cost), centerOnly(center) {
}

Collective::MinionTaskInfo::MinionTaskInfo(Type t, const string& desc) : type(t), description(desc) {
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
          equipmentStorage, false, {}, Warning::STORAGE},
      {ItemIndex::WOOD, unMarkedItems(), resourceStorage, false, {SquareId::TREE_TRUNK}, Warning::STORAGE},
      {ItemIndex::IRON, unMarkedItems(), resourceStorage, false, {}, Warning::STORAGE},
      {ItemIndex::STONE, unMarkedItems(), resourceStorage, false, {}, Warning::STORAGE},
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
    {MinionTask::GRAVE, {{SquareId::GRAVE}, "sleeping", Collective::Warning::GRAVES}},
    {MinionTask::LAIR, {{SquareId::BEAST_CAGE}, "sleeping"}},
    {MinionTask::STUDY, {{SquareId::LIBRARY}, "studying", Collective::Warning::LIBRARY, 1}},
    {MinionTask::PRISON, {{SquareId::PRISON}, "prison", Collective::Warning::NO_PRISON}},
    {MinionTask::TORTURE, {{SquareId::TORTURE_TABLE}, "torture ordered", Collective::Warning::TORTURE_ROOM, 0, true}},
    {MinionTask::RITUAL, {{SquareId::RITUAL_ROOM}, "rituals"}},
    {MinionTask::COPULATE, {MinionTaskInfo::COPULATE, "copulation"}},
    {MinionTask::CONSUME, {MinionTaskInfo::CONSUME, "consumption"}},
    {MinionTask::EXPLORE, {MinionTaskInfo::EXPLORE, "spying"}},
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

Collective::Collective(Level* l, CollectiveConfig cfg, Tribe* t, EnumMap<ResourceId, int> _credit, const string& n) 
  : credit(_credit), taskMap(l->getBounds()), knownTiles(l->getBounds()), control(CollectiveControl::idle(this)),
  tribe(NOTNULL(t)), level(NOTNULL(l)), nextPayoutTime(-1), name(n), config(cfg) {
}

const string& Collective::getName() const {
  return name;
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
    case AttractionId::ITEM_INDEX: 
      return getAllItems(attraction.get<ItemIndex>(), true).size();
  }
  FAIL << "wefok";
  return 0;
}

namespace {

class LeaderControlOverride : public Creature::MoraleOverride {
  public:
  LeaderControlOverride(Collective* col, Creature* c) : collective(col), creature(c) {}

  virtual optional<double> getMorale() override {
    for (auto team : collective->getTeams().getContaining(collective->getLeader()))
      if (collective->getTeams().isActive(team) && collective->getTeams().contains(team, creature))
        return 1;
    return none;
  }

  SERIALIZATION_CONSTRUCTOR(LeaderControlOverride);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Creature::MoraleOverride) & SVAR(collective) & SVAR(creature);
    CHECK_SERIAL;
  }

  private:
  Collective* SERIAL(collective);
  Creature* SERIAL(creature);
};

}

void Collective::addCreature(PCreature creature, Vec2 pos, EnumSet<MinionTrait> traits) {
  if (config.getStripSpawns())
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
  if (traits[MinionTrait::PRISONER])
    prisonerInfo[c] = {PrisonerState::PRISON, 0};
  if (traits[MinionTrait::FIGHTER]) {
    c->addMoraleOverride(Creature::PMoraleOverride(new LeaderControlOverride(this, c)));
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
  double ret = 0;
  for (const Creature* c : getCreatures({MinionTrait::FIGHTER}))
    ret += c->getDifficultyPoints();
  ret += getSquares(SquareId::IMPALED_HEAD).size() * 150;
  return ret * getWarMultiplier();
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
  if (hasTrait(c, MinionTrait::FIGHTER) && c->getSpawnType() != SpawnType::DEMON && !c->hasSuicidalAttack())
    for (auto& elem : prisonerInfo)
      if (!elem.second.task()) {
        Creature* prisoner = elem.first;
        PTask t;
        switch (elem.second.state()) {
          case PrisonerState::EXECUTE: t = Task::kill(this, prisoner); break;
          case PrisonerState::TORTURE: t = Task::torture(this, prisoner); break;
 //         case PrisonerState::SACRIFICE: t = Task::sacrifice(this, prisoner); break;
          default: return nullptr;
        }
        if (t && t->getMove(c)) {
          elem.second.task() = t->getUniqueId();
          return t;
        } else
          return nullptr;
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
  if (Task* task = taskMap.getTask(c))
    return task->getMove(c);
  if (Task* closest = taskMap.getTaskForWorker(c)) {
    taskMap.takeTask(c, closest);
    return closest->getMove(c);
  } else {
    if (config.getWorkerFollowLeader() && getLeader() && !containsSquare(c->getPosition())
        && getLeader()->getLevel() == c->getLevel()) {
      Vec2 leaderPos = getLeader()->getPosition();
      if (leaderPos.dist8(c->getPosition()) < 3)
        return NoMove;
      if (auto action = c->moveTowards(leaderPos))
        return {1.0, action};
      else
        return NoMove;
    } else if (!hasTrait(c, MinionTrait::NO_RETURNING) &&  !getAllSquares().empty() &&
               !getAllSquares().count(c->getPosition()))
        return c->moveTowards(chooseRandom(getAllSquares()));
      return NoMove;
  }
}

int Collective::getTaskDuration(const Creature* c, MinionTask task) const {
  switch (task) {
    case MinionTask::CONSUME:
    case MinionTask::COPULATE:
    case MinionTask::GRAVE:
    case MinionTask::LAIR:
    case MinionTask::SLEEP: return 1;
    default: return 500 + 250 * c->getMorale();
  }
}

void Collective::setMinionTask(const Creature* c, MinionTask task) {
  currentTasks[c->getUniqueId()] = {task, c->getTime() + getTaskDuration(c, task)};
}

bool Collective::isTaskGood(const Creature* c, MinionTask task) const {
  if (c->getMinionTasks().getValue(task) == 0)
    return false;
  if (minionPayment.count(c) && minionPayment.at(c).debt() > 0 && getTaskInfo().at(task).cost > 0)
    return false;
  switch (task) {
    case MinionTask::EXPLORE:
        return getLevel()->getModel()->getSunlightInfo().state == Model::SunlightInfo::DAY;
    case MinionTask::EXPLORE_NOCTURNAL:
        return getLevel()->getModel()->getSunlightInfo().state == Model::SunlightInfo::NIGHT;
    default: return true;
  }
}

void Collective::setRandomTask(const Creature* c) {
  vector<pair<MinionTask, double>> goodTasks;
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (isTaskGood(c, t))
      goodTasks.push_back({t, c->getMinionTasks().getValue(t)});
  if (!goodTasks.empty())
    setMinionTask(c, chooseRandom(goodTasks));
}

static bool betterPos(Vec2 from, Vec2 current, Vec2 candidate) {
  double maxDiff = 0.3;
  double curDist = from.dist8(current);
  double newDist = from.dist8(candidate);
  return Random.getDouble() <= 1.0 - (newDist - curDist) / (curDist * maxDiff);
}

static optional<Vec2> getRandomCloseTile(Vec2 from, const vector<Vec2>& tiles, function<bool(Vec2)> predicate) {
  optional<Vec2> ret;
  for (Vec2 pos : tiles)
    if (predicate(pos) && (!ret || betterPos(from, *ret, pos)))
      ret = pos;
  return ret;
}

optional<Vec2> Collective::getTileToExplore(const Creature* c, MinionTask task) const {
  vector<Vec2> border = randomPermutation(knownTiles.getBorderTiles());
  switch (task) {
    case MinionTask::EXPLORE_CAVES:
        if (auto pos = getRandomCloseTile(c->getPosition(), border,
              [this, c](Vec2 pos) { return getLevel()->getCoverInfo(pos).sunlight() < 1 && c->isSameSector(pos);}))
          return *pos;
    case MinionTask::EXPLORE:
    case MinionTask::EXPLORE_NOCTURNAL:
        return getRandomCloseTile(c->getPosition(), border,
            [this, c](Vec2 pos) { return !getLevel()->getCoverInfo(pos).covered() && c->isSameSector(pos);});
    default: FAIL << "Unrecognized explore task: " << int(task);
  }
  return none;
}

PTask Collective::getStandardTask(Creature* c) {
  if (!c->getMinionTasks().hasAnyTask())
    return nullptr;
  if (!currentTasks.count(c->getUniqueId()) ||
      currentTasks.at(c->getUniqueId()).finishTime() < c->getTime() ||
      !isTaskGood(c, currentTasks.at(c->getUniqueId()).task()))
    currentTasks.erase(c->getUniqueId());
  if (!currentTasks.count(c->getUniqueId()))
    setRandomTask(c);
  MinionTask task = currentTasks[c->getUniqueId()].task();
  MinionTaskInfo info = getTaskInfo().at(task);
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
      if (auto pos = getTileToExplore(c, task))
        ret = Task::explore(*pos);
      else
        return nullptr;
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

bool Collective::isConquered() const {
  return getCreaturesAnyOf({MinionTrait::FIGHTER, MinionTrait::LEADER}).empty();
}

void Collective::orderConsumption(Creature* consumer, Creature* who) {
  CHECK(consumer->getMinionTasks().getValue(MinionTask::CONSUME) > 0);
  setMinionTask(who, MinionTask::CONSUME);
  MinionTaskInfo info = getTaskInfo().at(currentTasks[consumer->getUniqueId()].task());
  minionTaskStrings[consumer->getUniqueId()] = info.description;
  taskMap.freeFromTask(consumer);
  taskMap.addTask(Task::consume(this, who), consumer);
}

PTask Collective::getEquipmentTask(Creature* c) {
  autoEquipment(c, Random.roll(10));
  for (Item* it : c->getEquipment().getItems())
    if (!c->getEquipment().isEquiped(it) && c->getEquipment().canEquip(it))
      return Task::equipItem(it);
  for (Vec2 v : getAllSquares(equipmentStorage)) {
    vector<Item*> it = getLevel()->getSafeSquare(v)->getItems([this, c] (const Item* it) {
        return minionEquipment.getOwner(it) == c && it->canEquip(); });
    if (!it.empty())
      return Task::pickAndEquipItem(this, v, it[0]);
    it = getLevel()->getSafeSquare(v)->getItems([this, c] (const Item* it) {
      return minionEquipment.getOwner(it) == c; });
    if (!it.empty())
      return Task::pickItem(this, v, it);
  }
  return nullptr;
}

PTask Collective::getHealingTask(Creature* c) {
  if (c->getHealth() < 1 && c->canSleep() && !c->isAffected(LastingEffect::POISON))
    for (MinionTask t : {MinionTask::SLEEP, MinionTask::GRAVE, MinionTask::LAIR})
      if (c->getMinionTasks().getValue(t) > 0) {
        vector<Vec2> positions = getAllSquares(getTaskInfo().at(t).squares);
        if (!positions.empty())
          return Task::applySquare(nullptr, positions);
      }
  return nullptr;
}

MoveInfo Collective::getTeamMemberMove(Creature* c) {
  for (auto team : teams.getContaining(c))
    if (teams.isActive(team)) {
        const Creature* leader = teams.getLeader(team);
        if (c != leader && leader->getLevel() == c->getLevel()) {
            if (leader->getPosition().dist8(c->getPosition()) > 1)
              return c->moveTowards(teams.getLeader(team)->getPosition());
            else
              return c->wait();
        }
    }
  return NoMove;
}

void Collective::setTask(const Creature *c, PTask task) {
  if (Task* task = taskMap.getTask(c)) {
    if (!task->canTransfer()) {
      task->cancel();
      returnResource(taskMap.removeTask(task));
    } else
      taskMap.freeTaskDelay(task, getTime() + 50);
  }
  taskMap.addTask(std::move(task), c);
}

bool Collective::hasTask(const Creature* c) const {
  return taskMap.hasTask(c);
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  if (c->getLevel() != getLevel())
    return NoMove;
  if (Task* task = taskMap.getTask(c))
    if (taskMap.isPriorityTask(task))
      return task->getMove(c);
  if (MoveInfo move = getTeamMemberMove(c))
    return move;
  if (MoveInfo move = control->getMove(c))
    return move;
  if (hasTrait(c, MinionTrait::WORKER))
    return getWorkerMove(c);
  if (config.getFetchItems())
    if (MoveInfo move = getDropItems(c))
      return move;
  if (hasTrait(c, MinionTrait::FIGHTER)) {
    if (MoveInfo move = getAlarmMove(c))
      return move;
    if (MoveInfo move = getGuardPostMove(c))
      return move;
  }
  if (Task* task = taskMap.getTask(c))
    return task->getMove(c);
  if (PTask t = getHealingTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (usesEquipment(c))
    if (PTask t = getEquipmentTask(c))
      if (t->getMove(c))
        return taskMap.addTask(std::move(t), c)->getMove(c);
  if (PTask t = getPrisonerTask(c))
    return taskMap.addTask(std::move(t), c)->getMove(c);
  if (PTask t = getStandardTask(c))
    if (t->getMove(c))
      return taskMap.addTask(std::move(t), c)->getMove(c);
  if (!hasTrait(c, MinionTrait::NO_RETURNING) && !getAllSquares().empty() &&
      !getAllSquares().count(c->getPosition()))
    return c->moveTowards(chooseRandom(getAllSquares()));
  else
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
    if (!extendedTiles.count(pos))  {
      extendedTiles[pos] = 1;
      extendedQueue.push_back(pos);
    }
  }
  for (int i = 0; i < extendedQueue.size(); ++i) {
    Vec2 pos = extendedQueue[i];
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !containsSquare(v) && !extendedTiles.count(v) 
          && getLevel()->getSafeSquare(v)->canEnterEmpty({MovementTrait::WALK})) {
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
    if (const Creature* c = getLevel()->getSafeSquare(pos)->getCreature())
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

optional<Vec2> Collective::chooseBedPos(const set<Vec2>& lair, const set<Vec2>& beds) {
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

vector<Vec2> Collective::getSpawnPos(const vector<Creature*>& creatures) {
  vector<Vec2> extendedTiles = getExtendedTiles(20, 10);
  if (extendedTiles.empty())
    return {};
  vector<Vec2> spawnPos;
  for (auto c : creatures) {
    Vec2 pos;
    int cnt = 100;
    do {
      pos = chooseRandom(extendedTiles);
    } while ((!getLevel()->getSafeSquare(pos)->canEnter(c) || contains(spawnPos, pos)) && --cnt > 0);
    if (cnt == 0) {
      Debug() << "Couldn't spawn immigrant " << c->getName().bare();
      return {};
    } else
      spawnPos.push_back(pos);
  }
  return spawnPos;
}

template <typename T>
static set<T> addVector(const set<T>& s, const vector<T>& v) {
  set<T> ret(s);
  ret.insert(v.begin(), v.end());
  return ret;
}

template <typename T>
static set<T> removeVector(const set<T>& s, const vector<T>& v) {
  set<T> ret(s);
  for (auto elem : v)
    ret.erase(elem);
  return ret;
}

vector<Vec2> Collective::getBedPositions(const vector<PCreature>& creatures, const ImmigrantInfo& info) {
  SpawnType spawnType = *creatures[0]->getSpawnType();
  SquareType dormType = getDormInfo()[spawnType].dormType;
  optional<SquareType> bedType = getDormInfo()[spawnType].getBedType();
  vector<Vec2> bedPos;
  for (int i : All(creatures))
    for (int j : Range(30)) {
      optional<Vec2> newPos;
      if (!bedType)
        newPos = chooseRandom(removeVector(getSquares(dormType), bedPos));
      else if (getCreatures(spawnType).size() + bedPos.size() < getSquares(*bedType).size()) {
        newPos = chooseRandom(removeVector(getSquares(*bedType), bedPos));
      } else
        newPos = chooseBedPos(removeVector(getSquares(dormType), bedPos), addVector(getSquares(*bedType), bedPos));
      if (newPos && !contains(bedPos, *newPos) &&
          (!info.spawnAtDorm || level->getSafeSquare(*newPos)->canEnter(creatures[i].get()))) {
        bedPos.push_back(*newPos);
        break;
      }
    }
  return bedPos;
}

bool Collective::considerNonSpawnImmigrant(const ImmigrantInfo& info, vector<PCreature> creatures) {
  CHECK(!info.spawnAtDorm);
  auto creatureRefs = extractRefs(creatures);
  vector<Vec2> spawnPos;
  spawnPos = getSpawnPos(creatureRefs);
  if (spawnPos.size() < creatures.size())
    return false;
  if (info.autoTeam)
    teams.activate(teams.createHidden(extractRefs(creatures)));
  for (int i : All(creatures)) {
    Creature* c = creatures[i].get();
    addCreature(std::move(creatures[i]), spawnPos[i], info.traits);
    minionPayment[c] = {info.salary, 0.0, 0};
    minionAttraction[c] = info.attractions;
  }
  addNewCreatureMessage(creatureRefs);
  return true;
}

static Collective::CostInfo getSpawnCost(SpawnType type, int howMany) {
  switch (type) {
    case SpawnType::UNDEAD: return {CollectiveResourceId::CORPSE, howMany};
    default: return {CollectiveResourceId::GOLD, 0};
  }
}

bool Collective::considerImmigrant(const ImmigrantInfo& info) {
  if (info.techId && !hasTech(*info.techId))
    return false;
  vector<PCreature> creatures;
  for (int i : (info.groupSize ? Range(Random.get(*info.groupSize)) : Range(1)))
    creatures.push_back(CreatureFactory::fromId(info.id, getTribe(), MonsterAIFactory::collective(this)));
  if (!creatures[0]->getSpawnType())
    return considerNonSpawnImmigrant(info, std::move(creatures));
  CHECK(creatures[0]->getSpawnType()) << "No spawn type for immigrant " << creatures[0]->getName().bare();
  SpawnType spawnType = *creatures[0]->getSpawnType();
  SquareType dormType = getDormInfo()[spawnType].dormType;
  if (!hasResource(getSpawnCost(spawnType, creatures.size())))
    return false;
  takeResource(getSpawnCost(spawnType, creatures.size()));
  if (getSquares(dormType).empty())
    return false;
  optional<SquareType> bedType = getDormInfo()[spawnType].getBedType();
  vector<Vec2> bedPos = getBedPositions(creatures, info);
  if (info.groupSize) {
    if (bedPos.size() < info.groupSize->getStart())
      return false;
    if (bedPos.size() < creatures.size())
      creatures.resize(bedPos.size());
  }
  else if (bedPos.empty())
    return false;
  auto creatureRefs = extractRefs(creatures);
  vector<Vec2> spawnPos;
  if (info.spawnAtDorm)
    spawnPos = bedPos;
  else {
    spawnPos = getSpawnPos(creatureRefs);
    if (spawnPos.size() < creatures.size())
      return false;
  }
  if (info.autoTeam)
    teams.activate(teams.createHidden(extractRefs(creatures)));
  for (int i : All(creatures)) {
    Creature* c = creatures[i].get();
    if (i == 0)
      c->increaseExpLevel(2);
    addCreature(std::move(creatures[i]), spawnPos[i], info.traits);
    if (bedType) {
      taskMap.addPriorityTask(Task::createBed(this, bedPos[i], dormType, *bedType), c);
      Debug() << c->getName().bare() << " creating bed " << bedPos[i];
    }
    minionPayment[c] = {info.salary, 0.0, 0};
    minionAttraction[c] = info.attractions;
  }
  addNewCreatureMessage(creatureRefs);
  return true;
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
    result = 1;
  for (auto& attraction : info.attractions) {
    double value = getAttractionValue(attraction.attraction);
    if (value < 0.001 && attraction.mandatory)
      return 0;
    result += max(0.0, value - getAttractionOccupation(attraction.attraction) - attraction.minAmount);
  }
  return result * info.frequency;
}

void Collective::considerImmigration() {
  if (getCreatures().size() >= config.getMaxPopulation() || !getLeader())
    return;
  vector<double> weights;
  bool ok = false;
  for (auto& elem : config.getImmigrantInfo()) {
    weights.push_back(getImmigrantChance(elem));
    if (weights.back() > 0)
      ok = true;
  }
  if (!ok)
    return;
  for (int i : Range(10))
    if (considerImmigrant(chooseRandom(config.getImmigrantInfo(), weights)))
      break;
}

const Collective::ResourceId paymentResource = Collective::ResourceId::GOLD;

int Collective::getPaymentAmount(const Creature* c) const {
  if (!minionPayment.count(c))
    return 0;
  else
    return config.getPayoutMultiplier() *
      minionPayment.at(c).salary() * minionPayment.at(c).workAmount() / config.getPayoutTime();
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
  if (int numPay = getNextSalaries())
    control->addMessage(PlayerMessage("Payout time: " + toString(numPay) + " gold",
          PlayerMessage::HIGH));
  for (Creature* c : creatures)
    if (minionPayment.count(c)) {
      minionPayment.at(c).debt() += getPaymentAmount(c);
      minionPayment.at(c).workAmount() = 0;
    }
}

void Collective::cashPayouts() {
  int numGold = numResource(paymentResource);
  for (Creature* c : creatures)
    if (minionPayment.count(c))
      if (int payment = minionPayment.at(c).debt()) {
        if (payment < numGold) {
          takeResource({paymentResource, payment});
          numGold -= payment;
          minionPayment.at(c).debt() = 0;
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
    for (Square* square : c->getSquares(Vec2::directions8(true)))
      if (square->canEnter(spawn.get())) {
        control->addMessage(c->getName().a() + " gives birth to " + spawn->getName().a());
        addCreature(std::move(spawn), square->getPosition(), {MinionTrait::FIGHTER});
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
    if (usesEquipment(c) && minionEquipment.needs(c, genWeapon.get(), true, true))
      ++numNeededWeapons;
  setWarning(Warning::NO_WEAPONS, numNeededWeapons > numWeapons);
}

void Collective::tick(double time) {
  control->tick(time);
  considerHealingLeader();
  considerBirths();
  if (Random.rollD(1.0 / config.getImmigrantFrequency()))
    considerImmigration();
  if (nextPayoutTime > -1 && time > nextPayoutTime) {
    nextPayoutTime += config.getPayoutTime();
    makePayouts();
  }
  cashPayouts();
  if (config.getWarnings() && Random.roll(5)) {
    considerWeaponWarning();
    setWarning(Warning::MANA, numResource(ResourceId::MANA) < 100);
    setWarning(Warning::DIGGING, getSquares(SquareId::FLOOR).empty());
    setWarning(Warning::MORE_LIGHTS, torches.size() * 25 < getAllSquares(roomsNeedingLight).size());
    for (SpawnType spawnType : ENUM_ALL(SpawnType)) {
      DormInfo info = getDormInfo()[spawnType];
      if (info.warning && info.getBedType())
        setWarning(*info.warning, !chooseBedPos(getSquares(info.dormType), getSquares(*info.getBedType())));
    }
    for (auto elem : getTaskInfo())
      if (!getAllSquares(elem.second.squares).empty() && elem.second.warning)
        setWarning(*elem.second.warning, false);
  }
  if (config.getEnemyPositions() && Random.roll(5)) {
    vector<Vec2> enemyPos = getEnemyPositions();
    if (!enemyPos.empty())
      delayDangerousTasks(enemyPos, getTime() + 20);
    else
      alarmInfo.finishTime() = -1000;
    bool allSurrender = true;
    for (Vec2 v : enemyPos)
      if (!prisonerInfo.count(NOTNULL(getLevel()->getSafeSquare(v)->getCreature()))) {
        allSurrender = false;
        break;
      }
    if (allSurrender) {
      for (auto elem : copyOf(prisonerInfo))
        if (elem.second.state() == PrisonerState::SURRENDER) {
          Creature* c = elem.first;
          Vec2 pos = c->getPosition();
          if (containsSquare(pos) && !c->isDead()) {
            getLevel()->globalMessage(pos, c->getName().the() + " surrenders.");
            control->addMessage(PlayerMessage(c->getName().a() + " surrenders.").setPosition(c->getPosition()));
            c->die(nullptr, true, false);
            addCreature(CreatureFactory::fromId(CreatureId::PRISONER, getTribe(),
                  MonsterAIFactory::collective(this)), pos, {MinionTrait::PRISONER});
          }
          prisonerInfo.erase(c);
        }
    }
  }
  if (config.getConstructions())
    updateConstructions();
  if (config.getFetchItems() && Random.roll(5))
    for (const ItemFetchInfo& elem : getFetchInfo()) {
      for (Vec2 pos : getAllSquares())
        fetchItems(pos, elem);
      for (SquareType type : elem.additionalPos)
        for (Vec2 pos : getSquares(type))
          fetchItems(pos, elem);
    }
  if (config.getManageEquipment() && Random.roll(10))
    minionEquipment.updateOwners(getAllItems(true));
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
    if (const Creature* c = getLevel()->getSafeSquare(v)->getCreature())
      if (c->getTribe() != getTribe())
        return true;
  return false;
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

void Collective::onKillEvent(const Creature* victim1, const Creature* killer) {
  if (contains(creatures, victim1)) {
    Creature* victim = const_cast<Creature*>(victim1);
    if (hasTrait(victim, MinionTrait::PRISONER) && killer && contains(getCreatures(), killer)
      && prisonerInfo.at(victim).state() == PrisonerState::EXECUTE)
      returnResource({ResourceId::PRISONER_HEAD, 1});
    if (hasTrait(victim, MinionTrait::LEADER))
      GlobalEvents.addKilledLeaderEvent(this, victim);
    prisonerInfo.erase(victim);
    freeFromGuardPost(victim);
    decreaseMoraleForKill(killer, victim);
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
    for (auto team : teams.getContaining(victim))
      teams.remove(team, victim);
    control->onCreatureKilled(victim, killer);
    if (killer)
      control->addMessage(PlayerMessage(victim->getName().a() + " is killed by " + killer->getName().a(),
            PlayerMessage::HIGH).setPosition(victim->getPosition()));
    else
      control->addMessage(PlayerMessage(victim->getName().a() + " is killed.", PlayerMessage::HIGH)
          .setPosition(victim->getPosition()));
  }
  if (victim1->getTribe() != getTribe() && (!killer || contains(creatures, killer))) {
    addMana(getKillManaScore(victim1));
    addMoraleForKill(killer, victim1);
    kills.push_back(victim1);
    points += victim1->getDifficultyPoints();
    if (killer)
      control->addMessage(PlayerMessage(victim1->getName().a() + " is killed by " + killer->getName().a())
          .setPosition(victim1->getPosition()));
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
  return knownTiles.isKnown(pos);
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
    control->addMessage(PlayerMessage("An alarm goes off.", PlayerMessage::HIGH).setPosition(pos));
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
  if (resourceInfo.at(id).itemIndex)
    for (SquareType type : resourceInfo.at(id).storageType)
      for (Vec2 pos : getSquares(type))
        ret += getLevel()->getSafeSquare(pos)->getItems(*resourceInfo.at(id).itemIndex).size();
  return ret;
}

int Collective::numResourcePlusDebt(ResourceId id) const {
  int ret = numResource(id);
  for (auto& elem : constructions)
    if (!elem.second.built() && elem.second.cost().id() == id)
      ret -= elem.second.cost().value();
  for (auto& elem : taskMap.getCompletionCosts())
    if (elem.second.id() == id && !elem.first->isDone())
      ret += elem.second.value();
  if (id == ResourceId::GOLD)
    for (auto& elem : minionPayment)
      ret -= elem.second.debt();
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
  if (resourceInfo.at(cost.id()).itemIndex)
    for (Vec2 pos : randomPermutation(getAllSquares(resourceInfo.at(cost.id()).storageType))) {
      vector<Item*> goldHere = getLevel()->getSafeSquare(pos)->getItems(*resourceInfo.at(cost.id()).itemIndex);
      for (Item* it : goldHere) {
        getLevel()->getSafeSquare(pos)->removeItem(it);
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
    getLevel()->getSafeSquare(chooseRandom(destination))->
      dropItems(ItemFactory::fromId(resourceInfo.at(amount.id()).itemId, amount.value()));
  } else
    credit[amount.id()] += amount.value();
}

vector<pair<Item*, Vec2>> Collective::getTrapItems(TrapType type, set<Vec2> squares) const {
  vector<pair<Item*, Vec2>> ret;
  if (squares.empty())
    squares = getSquares(SquareId::WORKSHOP);
  for (Vec2 pos : squares) {
    vector<Item*> v = filter(getLevel()->getSafeSquare(pos)->getItems(ItemIndex::TRAP),
        [type, this](Item* it) { return it->getTrapType() == type && !isItemMarked(it); });
    for (Item* it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}

bool Collective::usesEquipment(const Creature* c) const {
  return config.getManageEquipment()
    && c->isHumanoid() 
    && !hasTrait(c, MinionTrait::NO_EQUIPMENT)
    && !hasTrait(c, MinionTrait::PRISONER);
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
  vector<Item*> myItems = filter(getAllItems(ItemIndex::CAN_EQUIP), [&](const Item* it) {
      return minionEquipment.getOwner(it) == creature;});
  for (Item* it : myItems) {
    EquipmentSlot slot = it->getEquipmentSlot();
    if (slots[slot].size() < creature->getEquipment().getMaxItems(slot)) {
      slots[slot].push_back(it);
    } else  // a rare occurence that minion owns too many items of the same slot,
          //should happen only when an item leaves the fortress and then is braught back
      minionEquipment.discard(it);
  }
  vector<Item*> possibleItems = getAllItems([&](const Item* it) {
      return minionEquipment.needs(creature, it, false, replace) && !minionEquipment.getOwner(it); }, false);
  sortByEquipmentValue(possibleItems);
  for (Item* it : possibleItems) {
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

vector<Item*> Collective::getAllItems(bool includeMinions) const {
  vector<Item*> allItems;
  for (Vec2 v : getAllSquares())
    append(allItems, getLevel()->getSafeSquare(v)->getItems());
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
  for (Vec2 v : getAllSquares())
    append(allItems, getLevel()->getSafeSquare(v)->getItems(predicate));
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems(predicate));
  return allItems;
}

vector<Item*> Collective::getAllItems(ItemIndex index, bool includeMinions) const {
  vector<Item*> allItems;
  for (Vec2 v : getAllSquares())
    append(allItems, getLevel()->getSafeSquare(v)->getItems(index));
  if (includeMinions)
    for (Creature* c : getCreatures())
      append(allItems, c->getEquipment().getItems(index));
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
/*  if (prisonerInfo.at(c).state() == PrisonerState::SACRIFICE)
    return;
  clearPrisonerTask(c);
  prisonerInfo.at(c) = {PrisonerState::SACRIFICE, 0};
  setMinionTask(c, MinionTask::SACRIFICE);*/
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

void Collective::destroySquare(Vec2 pos) {
  if (level->getSafeSquare(pos)->canDestroy() && containsSquare(pos))
    level->getSafeSquare(pos)->destroy();
  if (Creature* c = level->getSafeSquare(pos)->getCreature())
    if (c->isStationary())
      c->die(nullptr, false);
  if (traps.count(pos)) {
    removeTrap(pos);
  }
  if (constructions.count(pos))
    removeConstruction(pos);
  if (torches.count(pos))
    removeTorch(pos);
  level->getSafeSquare(pos)->removeTriggers();
}

void Collective::addConstruction(Vec2 pos, SquareType type, CostInfo cost, bool immediately, bool noCredit) {
  if (immediately && hasResource(cost)) {
    while (!getLevel()->getSafeSquare(pos)->construct(type)) {}
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
  taskMap.markSquare(pos, HighlightType::DIG, Task::construction(this, pos, SquareId::FLOOR));
}

void Collective::dontDig(Vec2 pos) {
  taskMap.unmarkSquare(pos);
}

bool Collective::isMarked(Vec2 pos) const {
  return taskMap.getMarked(pos);
}

HighlightType Collective::getMarkHighlight(Vec2 pos) const {
  return taskMap.getHighlightType(pos);
}

void Collective::setPriorityTasks(Vec2 pos) {
  taskMap.setPriorityTasks(pos);
}

bool Collective::hasPriorityTasks(Vec2 pos) const {
  return taskMap.hasPriorityTasks(pos);
}

void Collective::cutTree(Vec2 pos) {
  taskMap.markSquare(pos, HighlightType::CUT_TREE, Task::construction(this, pos, SquareId::TREE_TRUNK));
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
    traps[pos].marked() = false;
    traps[pos].armed() = true;
  }
}

void Collective::onAppliedItemCancel(Vec2 pos) {
  if (traps.count(pos))
    traps.at(pos).marked() = false;
}

void Collective::onTorchBuilt(Vec2 pos, Trigger* t) {
  if (!torches.count(pos)) {
    if (canPlaceTorch(pos))
      addTorch(pos);
    else
      return;
  }
  torches.at(pos).built() = true;
  torches.at(pos).marked() = false;
  torches.at(pos).task() = -1;
  torches.at(pos).trigger() = t;
}

void Collective::onConstructionCancelled(Vec2 pos) {
  if (constructions.count(pos)) {
    constructions.at(pos).built() = false;
    constructions.at(pos).marked() = false;
    constructions.at(pos).task() = -1;
  }
}

void Collective::onConstructed(Vec2 pos, SquareType type) {
  if (!contains({SquareId::TREE_TRUNK}, type.getId()))
    allSquares.insert(pos);
  CHECK(!getSquares(type).count(pos));
  for (auto& elem : mySquares)
      elem.second.erase(pos);
  mySquares[type].insert(pos);
  if (efficiencySquares.count(type))
    updateEfficiency(pos, type);
  if (contains({SquareId::FLOOR, SquareId::BRIDGE, SquareId::BARRICADE}, type.getId()))
    taskMap.clearAllLocked();
  if (taskMap.getMarked(pos))
    taskMap.unmarkSquare(pos);
  if (constructions.count(pos)) {
    constructions.at(pos).built() = true;
    constructions.at(pos).marked() = false;
    constructions.at(pos).task() = -1;
  }
  if (type == SquareId::FLOOR) {
    for (Vec2 v : pos.neighbors4())
      if (torches.count(v) && torches.at(v).attachmentDir() == (pos - v).getCardinalDir())
        removeTorch(v);
  }
  control->onConstructed(pos, type);
}

bool Collective::tryLockingDoor(Vec2 pos) {
  if (getConstructions().count(pos)) {
    Square* square = getLevel()->getSafeSquare(pos);
    if (square->canLock()) {
      square->lock();
      taskMap.clearAllLocked();
      return true;
    }
  }
  return false;
}

void Collective::updateConstructions() {
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems;
  for (TrapType type : ENUM_ALL(TrapType))
    trapItems[type] = getTrapItems(type, getAllSquares());
  for (auto elem : traps)
    if (!isDelayed(elem.first)) {
      vector<pair<Item*, Vec2>>& items = trapItems.at(elem.second.type());
      if (!items.empty()) {
        if (!elem.second.armed() && !elem.second.marked()) {
          Vec2 pos = items.back().second;
          taskMap.addTask(Task::applyItem(this, pos, items.back().first, elem.first), pos);
          markItem(items.back().first);
          items.pop_back();
          traps[elem.first].marked() = true;
        }
      }
    }
  for (auto& elem : constructions)
    if (!isDelayed(elem.first) && !elem.second.marked() && !elem.second.built()) {
      if (!hasResource(elem.second.cost()))
        continue;
      elem.second.task() = taskMap.addTaskCost(Task::construction(this, elem.first, elem.second.type()), elem.first,
          elem.second.cost())->getUniqueId();
      elem.second.marked() = true;
      takeResource(elem.second.cost());
    }
  for (auto& elem : torches)
    if (!isDelayed(elem.first) && !elem.second.marked() && !elem.second.built()) {
      elem.second.task() = taskMap.addTask(
          Task::buildTorch(this, elem.first, elem.second.attachmentDir()), elem.first)->getUniqueId();
      elem.second.marked() = true;
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
  if (isKnownSquare(pos)) {
    vector<PTask> tasks;
    for (const ItemFetchInfo& elem : getFetchInfo()) {
      vector<Item*> equipment = filter(getLevel()->getSafeSquare(pos)->getItems(elem.index), elem.predicate);
      if (!equipment.empty()) {
        vector<Vec2> destination = getAllSquares(elem.destination);
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
      taskMap.markSquare(pos, HighlightType::FETCH_ITEMS, Task::chain(std::move(tasks)));
  }
}

void Collective::fetchItems(Vec2 pos, const ItemFetchInfo& elem) {
  if (isDelayed(pos) || (traps.count(pos) && traps.at(pos).type() == TrapType::BOULDER && traps.at(pos).armed() == true))
    return;
  for (SquareType type : elem.destination)
    if (getSquares(type).count(pos))
      return;
  vector<Item*> equipment = filter(getLevel()->getSafeSquare(pos)->getItems(elem.index), elem.predicate);
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

void Collective::onSquareDestroyedEvent(const Level* l, Vec2 pos) {
  if (l == getLevel()) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
        if (efficiencySquares.count(elem.first))
          updateEfficiency(pos, elem.first);
      }
    mySquares[SquareId::FLOOR].insert(pos);
    if (constructions.count(pos)) {
      ConstructionInfo& info = constructions.at(pos);
      info.marked() = false;
      info.built() = false;
      info.task() = -1;
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

void Collective::onTrapDisarmEvent(const Level* l, const Creature* who, Vec2 pos) {
  if (traps.count(pos) && l == getLevel()) {
    control->addMessage(PlayerMessage(who->getName().a() + " disarms a " 
          + Item::getTrapName(traps.at(pos).type()) + " trap.", PlayerMessage::HIGH).setPosition(pos));
    traps.at(pos).armed() = false;
  }
}

void Collective::handleSurprise(Vec2 pos) {
  Vec2 rad(8, 8);
  bool wasMsg = false;
  Creature* c = getLevel()->getSafeSquare(pos)->getCreature();
  for (Square* square : getLevel()->getSquares(randomPermutation(Rectangle(pos - rad, pos + rad).getAllSquares())))
    if (Creature* other = square->getCreature())
      if (hasTrait(other, MinionTrait::FIGHTER) && other->getPosition().dist8(pos) > 1) {
        for (Vec2 dest : pos.neighbors8(true))
          if (getLevel()->canMoveCreature(other, dest - other->getPosition())) {
            getLevel()->moveCreature(other, dest - other->getPosition());
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
  if (!knownTiles.isKnown(pos)) {
    if (const Location* loc = getLevel()->getLocation(pos))
      if (!knownLocations.count(loc)) {
        knownLocations.insert(loc);
        control->onDiscoveredLocation(loc);
      }
    knownTiles.addTile(pos);
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
  Collective::CostInfo itemCost;
};

// The production cost is actually not applied ATM.
static WorkshopInfo getWorkshopInfo(Collective* c, Vec2 pos) {
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
        default: FAIL << "Bad workshop position " << pos;
      }
  return { ItemFactory::workshop(c->getTechnologies()),{CollectiveResourceId::GOLD, 5}};
}

void Collective::onAppliedSquare(Vec2 pos) {
  Creature* c = NOTNULL(getLevel()->getSafeSquare(pos)->getCreature());
  MinionTask currentTask = currentTasks.at(c->getUniqueId()).task();
  if (getTaskInfo().at(currentTask).cost > 0) {
    if (nextPayoutTime == -1 && minionPayment.count(c) && minionPayment.at(c).salary() > 0)
      nextPayoutTime = getTime() + config.getPayoutTime();
    minionPayment[c].workAmount() += getTaskInfo().at(currentTask).cost;
  }
  if (getSquares(SquareId::LIBRARY).count(pos)) {
    addMana(0.2);
    if (Random.rollD(60.0 / (getEfficiency(pos))) && !getAvailableSpells().empty())
      c->addSpell(chooseRandom(getAvailableSpells()));
  }
  if (getSquares(SquareId::TRAINING_ROOM).count(pos))
    c->exerciseAttr(chooseRandom<AttrType>(), getEfficiency(pos));
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
      getLevel()->getSafeSquare(pos)->dropItems(std::move(items));
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

double Collective::getDangerLevel(bool includeExecutions) const {
  double ret = 0;
  for (const Creature* c : getCreatures({MinionTrait::FIGHTER}))
    ret += c->getDifficultyPoints();
  if (includeExecutions)
    ret += getSquares(SquareId::IMPALED_HEAD).size() * 150;
  return ret;
}

bool Collective::hasTech(TechId id) const {
  return contains(technologies, id);
}

double Collective::getTechCost(Technology* t) {
  return t->getCost();
 /* int numTech = technologies.size() - numFreeTech;
  int numDouble = 4;
  return getTechCostMultiplier() * pow(2, double(numTech) / numDouble);*/
}

void Collective::acquireTech(Technology* tech, bool free) {
  technologies.push_back(tech->getId());
  if (free)
    ++numFreeTech;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      if (Creature* leader = getLeader())
        leader->addSpell(Spell::get(elem.id));
}

vector<Technology*> Collective::getTechnologies() const {
  return transform2<Technology*>(technologies, [] (const TechId t) { return Technology::get(t); });
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

void Collective::ownItems(const Creature* who, const vector<Item*> items) {
  for (const Item* it : items)
    if (minionEquipment.isItemUseful(it))
      minionEquipment.own(who, it);
}

void Collective::onTortureEvent(Creature* who, const Creature* torturer) {
  if (contains(creatures, torturer))
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

void Collective::addAssaultNotification(const Collective* col, const vector<Creature*>& c, const string& message) {
  control->addAssaultNotification(col, c, message);
}

void Collective::removeAssaultNotification(const Collective* col) {
  control->removeAssaultNotification(col);
}

CollectiveTeams& Collective::getTeams() {
  return teams;
}

const CollectiveTeams& Collective::getTeams() const {
  return teams;
}

void Collective::freeTeamMembers(TeamId id) {
  for (Creature* c : teams.getMembers(id)) {
    freeFromGuardPost(c);
    if (c->isAffected(LastingEffect::SLEEP))
      c->removeEffect(LastingEffect::SLEEP);
  }
}

static optional<Vec2> getAdjacentWall(const Level* l, Vec2 pos) {
  for (const Square* square : l->getSquares(pos.neighbors4(true)))
    if (square->canConstruct(SquareId::FLOOR))
      return square->getPosition();
  return none;
}

const map<Vec2, Collective::TorchInfo>& Collective::getTorches() const {
  return torches;
}

bool Collective::isPlannedTorch(Vec2 pos) const {
  return torches.count(pos) && !torches.at(pos).built();
}

void Collective::removeTorch(Vec2 pos) {
  if (torches.at(pos).task() > -1)
    taskMap.removeTask(torches.at(pos).task());
  if (auto trigger = torches.at(pos).trigger())
    getLevel()->getSafeSquare(pos)->removeTrigger(trigger);
  torches.erase(pos);
}

void Collective::addTorch(Vec2 pos) {
  CHECK(canPlaceTorch(pos));
  torches[pos] = {false, 0.0, -1, (*getAdjacentWall(getLevel(), pos) - pos).getCardinalDir(), nullptr};
}

bool Collective::canPlaceTorch(Vec2 pos) const {
  return getAdjacentWall(getLevel(), pos) && !torches.count(pos) &&
    isKnownSquare(pos) && getLevel()->getSafeSquare(pos)->canEnterEmpty({MovementTrait::WALK});
}

GameInfo::VillageInfo::Village Collective::getVillageInfo() const {
  GameInfo::VillageInfo::Village info;
  info.name = getName();
  info.tribeName = getTribe()->getName();
  if (isConquered())
    info.state = "conquered";
  return info;
}

const TaskMap<Collective::CostInfo>& Collective::getTaskMap() const {
  return taskMap;
}

template <class Archive>
void Collective::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, LeaderControlOverride);
}

REGISTER_TYPES(Collective::registerTypes);
