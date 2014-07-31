#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"
#include "pantheon.h"
#include "effect.h"
#include "level.h"
#include "square.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(creatures)
    & SVAR(taskMap)
    & SVAR(tribe)
    & SVAR(control)
    & SVAR(deityStanding)
    & SVAR(mySquares)
    & SVAR(allSquares)
    & SVAR(squareEfficiency)
    & SVAR(alarmInfo)
    & SVAR(enemyPos)
    & SVAR(guardPosts)
    & SVAR(level);
  CHECK_SERIAL;
}

SERIALIZABLE(Collective);

template <class Archive>
void Collective::AlarmInfo::serialize(Archive& ar, const unsigned int version) {
   ar& BOOST_SERIALIZATION_NVP(finishTime)
     & BOOST_SERIALIZATION_NVP(position);
}

SERIALIZABLE(Collective::AlarmInfo);

template <class Archive>
void Collective::GuardPostInfo::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(attender);
}

SERIALIZABLE(Collective::GuardPostInfo);


Collective::Collective() : control(CollectiveControl::idle(this)) {
}

void Collective::setLevel(Level* l) {
  level = l;
}

Collective::~Collective() {
}

void Collective::addCreature(Creature* c, vector<MinionTrait> traits) {
  if (!tribe)
    tribe = c->getTribe();
  creatures.push_back(c);
  for (MinionTrait t : traits)
    byTrait[t].push_back(c);
}

vector<Creature*>& Collective::getCreatures() {
  return creatures;
}

const Creature* Collective::getLeader() const {
  if (byTrait[MinionTrait::LEADER].empty())
    return nullptr;
  else
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

MoveInfo Collective::getMove(Creature* c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  if (!hasTrait(c, MinionTrait::LEADER) && hasTrait(c, MinionTrait::FIGHTER)) {
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
  PTask newTask = control->getNewTask(c);
  if (newTask)
    return taskMap.addTask(std::move(newTask), c)->getMove(c);
  else
    return control.get()->getMove(c);
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

void Collective::considerHealingLeader() {
  if (Deity* deity = Deity::getDeity(EpithetId::HEALTH))
    if (getStanding(deity) > 0)
      if (Random.rollD(5 / getStanding(deity))) {
        Creature* leader = getLeader();
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
          Effect::summon(leader, deity->getServant(), 1, 30);
        }
  }
}

const vector<Vec2>& Collective::getEnemyPositions() const {
  return enemyPos;
}

void Collective::updateEnemyPos() {
  map<Vec2, int> extendedTiles;
  queue<Vec2> extendedQueue;
  enemyPos.clear();
  for (Vec2 pos : getAllSquares()) {
    if (Creature* c = getLevel()->getSquare(pos)->getCreature()) {
      if (c->getTribe() != getTribe())
        enemyPos.push_back(c->getPosition());
    }
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !containsSquare(v) && !extendedTiles.count(v) 
          && getLevel()->getSquare(v)->canEnterEmpty(Creature::getDefault())) {
        extendedTiles[v] = 1;
        extendedQueue.push(v);
      }
  }
  const int maxRadius = 10;
  while (!extendedQueue.empty()) {
    Vec2 pos = extendedQueue.front();
    extendedQueue.pop();
    if (Creature* c = getLevel()->getSquare(pos)->getCreature())
      if (c->getTribe() != getTribe())
        enemyPos.push_back(c->getPosition());
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(getLevel()->getBounds()) && !containsSquare(v) && !extendedTiles.count(v) 
          && getLevel()->getSquare(v)->canEnterEmpty(Creature::getDefault())) {
        int a = extendedTiles[v] = extendedTiles[pos] + 1;
        if (a < maxRadius)
          extendedQueue.push(v);
      }

  }
}

void Collective::tick(double time) {
  updateEnemyPos();
  if (enemyPos.empty())
    alarmInfo.finishTime = -1000;
  control->tick(time);
  if (getLeader())
    considerHealingLeader();
}

vector<Creature*>& Collective::getCreatures(MinionTrait trait) {
  return byTrait[trait];
}

bool Collective::hasTrait(const Creature* c, MinionTrait t) {
  return contains(byTrait[t], c);
}

const vector<Creature*>& Collective::getCreatures(MinionTrait trait) const {
  return byTrait[trait];
}

bool Collective::underAttack() const {
  for (Vec2 v : getAllSquares())
    if (const Creature* c = getLevel()->getSquare(v)->getCreature())
      if (c->getTribe() != getTribe())
        return true;
  return false;
}
 
void Collective::onKillEvent(const Creature* victim, const Creature* killer) {
  if (contains(creatures, victim)) {
    control->onCreatureKilled(victim, killer);
    removeElement(creatures, victim);
    if (Task* task = taskMap.getTask(victim))
      taskMap.removeTask(task);
    for (MinionTrait t : ENUM_ALL(MinionTrait))
      if (contains(byTrait[t], victim))
        removeElement(byTrait[t], victim);
    for (Creature* c : creatures)
      c->addMorale(0.03);
  }
  if (contains(creatures, killer))
    for (Creature* c : creatures)
      c->addMorale(c == killer ? 0.25 : 0.03);
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

void Collective::changeSquareType(Vec2 pos, SquareType from, SquareType to) {
  mySquares[from].erase(pos);
  mySquares[to].insert(pos);
}

bool Collective::containsSquare(Vec2 pos) const {
  return allSquares.count(pos);
}

const static unordered_set<SquareType> efficiencySquares {
  SquareType::TRAINING_ROOM,
  SquareType::WORKSHOP,
  SquareType::LIBRARY,
  SquareType::LABORATORY,
};

void Collective::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == getLevel()) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
        if (efficiencySquares.count(elem.first))
          updateEfficiency(pos, elem.first);
      }
  }
}

void Collective::onConstructed(Vec2 pos, SquareType type) {
  if (!contains({SquareType::TREE_TRUNK}, type))
    allSquares.insert(pos);
  CHECK(!getSquares(type).count(pos));
  mySquares[type].insert(pos);
  if (efficiencySquares.count(type))
    updateEfficiency(pos, type);
}

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
    alarmInfo.finishTime = getTime() + alarmTime;
    alarmInfo.position = pos;
    for (Creature* c : byTrait[MinionTrait::FIGHTER])
      if (c->isAffected(LastingEffect::SLEEP))
        c->removeEffect(LastingEffect::SLEEP);
  }
}

MoveInfo Collective::getAlarmMove(Creature* c) {
  if (alarmInfo.finishTime > c->getTime())
    if (auto action = c->moveTowards(alarmInfo.position))
      return {1.0, action};
  return NoMove;
}

MoveInfo Collective::getGuardPostMove(Creature* c) {
  vector<Vec2> pos = getKeys(guardPosts);
  for (Vec2 v : pos)
    if (guardPosts.at(v).attender == c) {
      pos = {v};
      break;
    }
  for (Vec2 v : pos) {
    GuardPostInfo& info = guardPosts.at(v);
    if (!info.attender || info.attender == c) {
      info.attender = c;
 //     minionTaskStrings[c->getUniqueId()] = "guarding";
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
    if (elem.second.attender == c)
      elem.second.attender = nullptr;
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

