#include "stdafx.h"

#include "tribe.h"

template <class Archive> 
void Tribe::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(EventListener)
    & SVAR(diplomatic)
    & SVAR(standing)
    & SVAR(attacks)
    & SVAR(leader)
    & SVAR(members)
    & SVAR(enemyTribes)
    & SVAR(name)
    & SVAR(handicap);
  CHECK_SERIAL;
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
    ar & SUBCLASS(Tribe) & SVAR(standing);
    CHECK_SERIAL;
  }

  SERIAL_CHECKER;

  private:
  double SERIAL(standing);
};

template <class Archive>
void Tribe::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Constant);
}

REGISTER_TYPES(Tribe);

Tribe::Tribe(const string& n, bool d) : diplomatic(d), name(n) {
}

int Tribe::getHandicap() const {
  return handicap;
}

void Tribe::setHandicap(int h) {
  handicap = h;
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
  Tribes::set(TribeId::MONSTER, new Tribe("", false));
  Tribes::set(TribeId::LIZARD, new Tribe("lizardmen", true));
  Tribes::set(TribeId::PEST, new Tribe("", false));
  Tribes::set(TribeId::WILDLIFE, new Tribe("", false));
  Tribes::set(TribeId::ELVEN, new Tribe("elves", true));
  Tribes::set(TribeId::HUMAN, new Tribe("humans", true));
  Tribes::set(TribeId::GOBLIN, new Tribe("goblins", true));
  Tribes::set(TribeId::DWARVEN, new Tribe("dwarves", true));
  Tribes::set(TribeId::PLAYER, new Tribe("", false));
  Tribes::set(TribeId::KEEPER, new Tribe("", false));
  Tribes::set(TribeId::DRAGON, new Tribe("", false));
  Tribes::set(TribeId::CASTLE_CELLAR, new Tribe("", false));
  Tribes::set(TribeId::BANDIT, new Tribe("", false));
  Tribes::set(TribeId::KILL_EVERYONE, new Constant(-1));
  Tribes::set(TribeId::PEACEFUL, new Constant(1));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::PLAYER));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::ELVEN));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::DWARVEN));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::HUMAN));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::LIZARD));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::PEST));
  Tribes::get(TribeId::KEEPER)->addEnemy(Tribes::get(TribeId::MONSTER));
  Tribes::get(TribeId::ELVEN)->addEnemy(Tribes::get(TribeId::GOBLIN));
  Tribes::get(TribeId::ELVEN)->addEnemy(Tribes::get(TribeId::DWARVEN));
  Tribes::get(TribeId::ELVEN)->addEnemy(Tribes::get(TribeId::BANDIT));
  Tribes::get(TribeId::GOBLIN)->addEnemy(Tribes::get(TribeId::DWARVEN));
  Tribes::get(TribeId::GOBLIN)->addEnemy(Tribes::get(TribeId::WILDLIFE));
  Tribes::get(TribeId::HUMAN)->addEnemy(Tribes::get(TribeId::GOBLIN));
  Tribes::get(TribeId::HUMAN)->addEnemy(Tribes::get(TribeId::LIZARD));
  Tribes::get(TribeId::HUMAN)->addEnemy(Tribes::get(TribeId::BANDIT));
  Tribes::get(TribeId::PLAYER)->addEnemy(Tribes::get(TribeId::MONSTER));
  Tribes::get(TribeId::PLAYER)->addEnemy(Tribes::get(TribeId::GOBLIN));
  Tribes::get(TribeId::PLAYER)->addEnemy(Tribes::get(TribeId::PEST));
  Tribes::get(TribeId::PLAYER)->addEnemy(Tribes::get(TribeId::CASTLE_CELLAR));
  Tribes::get(TribeId::PLAYER)->addEnemy(Tribes::get(TribeId::DRAGON));
  Tribes::get(TribeId::PLAYER)->addEnemy(Tribes::get(TribeId::BANDIT));
  Tribes::get(TribeId::MONSTER)->addEnemy(Tribes::get(TribeId::WILDLIFE));
  Tribes::get(TribeId::WILDLIFE)->addEnemy(Tribes::get(TribeId::PLAYER));
  Tribes::get(TribeId::WILDLIFE)->addEnemy(Tribes::get(TribeId::GOBLIN));
  Tribes::get(TribeId::WILDLIFE)->addEnemy(Tribes::get(TribeId::MONSTER));
}

