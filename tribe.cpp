#include "stdafx.h"

#include "tribe.h"

template <class Archive> 
void Tribe::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(EventListener)
    & BOOST_SERIALIZATION_NVP(diplomatic)
    & BOOST_SERIALIZATION_NVP(standing)
    & BOOST_SERIALIZATION_NVP(attacks)
    & BOOST_SERIALIZATION_NVP(leader)
    & BOOST_SERIALIZATION_NVP(members)
    & BOOST_SERIALIZATION_NVP(enemyTribes)
    & BOOST_SERIALIZATION_NVP(name);
}

SERIALIZABLE(Tribe);

class Constant : public Tribe {
  public:
  Constant() {}
  Constant(double s) : Tribe("", false), standing(s) {}
  virtual double getStanding(const Creature*) const override {
    return standing;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Tribe) & BOOST_SERIALIZATION_NVP(standing);
  }

  private:
  double standing;
};

template <class Archive>
void Tribe::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Constant);
}

REGISTER_TYPES(Tribe);

Tribe* Tribe::monster;
Tribe* Tribe::pest;
Tribe* Tribe::wildlife;
Tribe* Tribe::elven;
Tribe* Tribe::human;
Tribe* Tribe::goblin;
Tribe* Tribe::dwarven;
Tribe* Tribe::player;
Tribe* Tribe::dragon;
Tribe* Tribe::castleCellar;
Tribe* Tribe::bandit;
Tribe* Tribe::killEveryone;
Tribe* Tribe::peaceful;

Tribe::Tribe(const string& n, bool d) : diplomatic(d), name(n) {
}

void Tribe::removeMember(const Creature* c) {
  if (c == leader)
    leader = nullptr;
  removeElement(members, c);
}

const string& Tribe::getName() {
  return name;
}

double Tribe::getStanding(const Creature* c) const {
  if (c->getTribe() == this)
    return 1;
  if (standing.count(c)) 
    return standing.at(c);
  if (enemyTribes.count(c->getTribe()))
    return -1;
  return 0;
}

void Tribe::initStanding(const Creature* c) {
  standing[c] = getStanding(c);
}

void Tribe::addEnemy(Tribe* t) {
  CHECK(t != this);
  enemyTribes.insert(t);
  t->enemyTribes.insert(this);
}

void Tribe::makeSlightEnemy(const Creature* c) {
  standing[c] = -0.001;
}

static const double killBonus = 0.1;
static const double killPenalty = 0.5;
static const double attackPenalty = 0.2;
static const double thiefPenalty = 0.5;
static const double importantMemberMult = 0.5 / killBonus;

double Tribe::getMultiplier(const Creature* member) {
  if (member == leader)
    return importantMemberMult;
  else
    return 1;
}

void Tribe::onKillEvent(const Creature* member, const Creature* attacker) {
  if (contains(members, member)) {
    CHECK(member->getTribe() == this);
    if (attacker == nullptr)
      return;
    initStanding(attacker);
    standing[attacker] -= killPenalty * getMultiplier(member);
    for (Tribe* t : enemyTribes)
      if (t->diplomatic) {
        t->initStanding(attacker);
        t->standing[attacker] += killBonus * getMultiplier(member);
      }
  }
}

void Tribe::onAttackEvent(const Creature* member, const Creature* attacker) {
  if (member->getTribe() != this)
    return;
  if (contains(attacks, make_pair(member, attacker)))
    return;
  attacks.emplace_back(member, attacker);
  initStanding(attacker);
  standing[attacker] -= attackPenalty * getMultiplier(member);
}

void Tribe::addMember(const Creature* c) {
  members.push_back(c);
}

void Tribe::setLeader(const Creature* c) {
  CHECK(!leader);
  leader = c;
}

const Creature* Tribe::getLeader() {
  return leader;
}

vector<const Creature*> Tribe::getMembers(bool includeDead) {
  if (includeDead)
    return members;
  else
    return filter(members, [](const Creature* c)->bool { return !c->isDead(); });
}

void Tribe::onItemsStolen(const Creature* attacker) {
  if (diplomatic) {
    initStanding(attacker);
    standing[attacker] -= thiefPenalty;
  }
}

void Tribe::init() {
  monster = new Tribe("", false);
  pest = new Tribe("", false);
  wildlife = new Tribe("", false);
  elven = new Tribe("elves", true);
  human = new Tribe("humans", true);
  goblin = new Tribe("goblins", true);
  dwarven = new Tribe("dwarves", true);
  player = new Tribe("", false);
  dragon = new Tribe("", false);
  castleCellar = new Tribe("", false);
  bandit = new Tribe("", false);
  killEveryone = new Constant(-1);
  peaceful = new Constant(1);
  elven->addEnemy(goblin);
  elven->addEnemy(dwarven);
  elven->addEnemy(bandit);
  goblin->addEnemy(dwarven);
  goblin->addEnemy(wildlife);
  human->addEnemy(goblin);
  human->addEnemy(bandit);
  player->addEnemy(monster);
  player->addEnemy(goblin);
  player->addEnemy(pest);
  player->addEnemy(castleCellar);
  player->addEnemy(dragon);
  player->addEnemy(bandit);
  monster->addEnemy(wildlife);
  wildlife->addEnemy(player);
  wildlife->addEnemy(goblin);
  wildlife->addEnemy(monster);
}

