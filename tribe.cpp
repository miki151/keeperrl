/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "tribe.h"
#include "creature.h"

template <class Archive> 
void Tribe::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Singleton)
    & SVAR(diplomatic)
    & SVAR(standing)
    & SVAR(attacks)
    & SVAR(leader)
    & SVAR(members)
    & SVAR(enemyTribes)
    & SVAR(name);
  CHECK_SERIAL;
}

SERIALIZABLE(Tribe);

SERIALIZATION_CONSTRUCTOR_IMPL(Tribe);

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

void Tribe::removeMember(const Creature* c) {
  if (c == leader)
    leader = nullptr;
  removeElement(members, c);
}

const string& Tribe::getName() const {
  return name;
}

double Tribe::getStanding(const Creature* c) const {
  if (c->getTribe() == this)
    return 1;
  if (standing.count(c)) 
    return standing.at(c);
  if (enemyTribes.count(const_cast<Tribe*>(c->getTribe())))
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

void Tribe::addFriend(Tribe* t) {
  CHECK(t != this);
  enemyTribes.erase(t);
  t->enemyTribes.erase(this);
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

void Tribe::onAttackEvent(Creature* member, Creature* attacker) {
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

const Creature* Tribe::getLeader() const {
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
  set(TribeId::MONSTER, new Tribe("monsters", false));
  set(TribeId::LIZARD, new Tribe("lizardmen", true));
  set(TribeId::PEST, new Tribe("pests", false));
  set(TribeId::WILDLIFE, new Tribe("wildlife", false));
  set(TribeId::ELVEN, new Tribe("elves", true));
  set(TribeId::HUMAN, new Tribe("humans", true));
  set(TribeId::ORC, new Tribe("orcs", true));
  set(TribeId::DWARVEN, new Tribe("dwarves", true));
  set(TribeId::PLAYER, new Tribe("player", false));
  set(TribeId::KEEPER, new Tribe("keeper", false));
  set(TribeId::DRAGON, new Tribe("dragon", false));
  set(TribeId::CASTLE_CELLAR, new Tribe("", false));
  set(TribeId::BANDIT, new Tribe("bandits", false));
  set(TribeId::KILL_EVERYONE, new Constant(-1));
  set(TribeId::PEACEFUL, new Constant(1));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::PLAYER));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::ELVEN));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::DWARVEN));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::HUMAN));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::LIZARD));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::DRAGON));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::PEST));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::MONSTER));
  get(TribeId::KEEPER)->addEnemy(get(TribeId::BANDIT));
  get(TribeId::ELVEN)->addEnemy(get(TribeId::ORC));
  get(TribeId::ELVEN)->addEnemy(get(TribeId::DWARVEN));
  get(TribeId::ELVEN)->addEnemy(get(TribeId::BANDIT));
  get(TribeId::ELVEN)->addEnemy(get(TribeId::DRAGON));
  get(TribeId::ORC)->addEnemy(get(TribeId::DWARVEN));
  get(TribeId::ORC)->addEnemy(get(TribeId::WILDLIFE));
  get(TribeId::HUMAN)->addEnemy(get(TribeId::ORC));
  get(TribeId::HUMAN)->addEnemy(get(TribeId::LIZARD));
  get(TribeId::HUMAN)->addEnemy(get(TribeId::BANDIT));
  get(TribeId::HUMAN)->addEnemy(get(TribeId::CASTLE_CELLAR));
  get(TribeId::HUMAN)->addEnemy(get(TribeId::DRAGON));
  get(TribeId::PLAYER)->addEnemy(get(TribeId::MONSTER));
  get(TribeId::PLAYER)->addEnemy(get(TribeId::ORC));
  get(TribeId::PLAYER)->addEnemy(get(TribeId::PEST));
  get(TribeId::PLAYER)->addEnemy(get(TribeId::CASTLE_CELLAR));
  get(TribeId::PLAYER)->addEnemy(get(TribeId::DRAGON));
  get(TribeId::PLAYER)->addEnemy(get(TribeId::BANDIT));
  get(TribeId::MONSTER)->addEnemy(get(TribeId::WILDLIFE));
  get(TribeId::WILDLIFE)->addEnemy(get(TribeId::PLAYER));
  get(TribeId::WILDLIFE)->addEnemy(get(TribeId::ORC));
  get(TribeId::WILDLIFE)->addEnemy(get(TribeId::MONSTER));
}

