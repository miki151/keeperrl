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
  Tribe::set(TribeId::MONSTER, new Tribe("monsters", false));
  Tribe::set(TribeId::LIZARD, new Tribe("lizardmen", true));
  Tribe::set(TribeId::PEST, new Tribe("pests", false));
  Tribe::set(TribeId::WILDLIFE, new Tribe("wildlife", false));
  Tribe::set(TribeId::ELVEN, new Tribe("elves", true));
  Tribe::set(TribeId::HUMAN, new Tribe("humans", true));
  Tribe::set(TribeId::ORC, new Tribe("orcs", true));
  Tribe::set(TribeId::DWARVEN, new Tribe("dwarves", true));
  Tribe::set(TribeId::PLAYER, new Tribe("player", false));
  Tribe::set(TribeId::KEEPER, new Tribe("keeper", false));
  Tribe::set(TribeId::DRAGON, new Tribe("dragon", false));
  Tribe::set(TribeId::CASTLE_CELLAR, new Tribe("", false));
  Tribe::set(TribeId::BANDIT, new Tribe("bandits", false));
  Tribe::set(TribeId::KILL_EVERYONE, new Constant(-1));
  Tribe::set(TribeId::PEACEFUL, new Constant(1));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::PLAYER));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::ELVEN));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::DWARVEN));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::HUMAN));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::LIZARD));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::DRAGON));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::PEST));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::MONSTER));
  Tribe::get(TribeId::KEEPER)->addEnemy(Tribe::get(TribeId::BANDIT));
  Tribe::get(TribeId::ELVEN)->addEnemy(Tribe::get(TribeId::ORC));
  Tribe::get(TribeId::ELVEN)->addEnemy(Tribe::get(TribeId::DWARVEN));
  Tribe::get(TribeId::ELVEN)->addEnemy(Tribe::get(TribeId::BANDIT));
  Tribe::get(TribeId::ELVEN)->addEnemy(Tribe::get(TribeId::DRAGON));
  Tribe::get(TribeId::ORC)->addEnemy(Tribe::get(TribeId::DWARVEN));
  Tribe::get(TribeId::ORC)->addEnemy(Tribe::get(TribeId::WILDLIFE));
  Tribe::get(TribeId::HUMAN)->addEnemy(Tribe::get(TribeId::ORC));
  Tribe::get(TribeId::HUMAN)->addEnemy(Tribe::get(TribeId::LIZARD));
  Tribe::get(TribeId::HUMAN)->addEnemy(Tribe::get(TribeId::BANDIT));
  Tribe::get(TribeId::HUMAN)->addEnemy(Tribe::get(TribeId::CASTLE_CELLAR));
  Tribe::get(TribeId::HUMAN)->addEnemy(Tribe::get(TribeId::DRAGON));
  Tribe::get(TribeId::PLAYER)->addEnemy(Tribe::get(TribeId::MONSTER));
  Tribe::get(TribeId::PLAYER)->addEnemy(Tribe::get(TribeId::ORC));
  Tribe::get(TribeId::PLAYER)->addEnemy(Tribe::get(TribeId::PEST));
  Tribe::get(TribeId::PLAYER)->addEnemy(Tribe::get(TribeId::CASTLE_CELLAR));
  Tribe::get(TribeId::PLAYER)->addEnemy(Tribe::get(TribeId::DRAGON));
  Tribe::get(TribeId::PLAYER)->addEnemy(Tribe::get(TribeId::BANDIT));
  Tribe::get(TribeId::MONSTER)->addEnemy(Tribe::get(TribeId::WILDLIFE));
  Tribe::get(TribeId::WILDLIFE)->addEnemy(Tribe::get(TribeId::PLAYER));
  Tribe::get(TribeId::WILDLIFE)->addEnemy(Tribe::get(TribeId::ORC));
  Tribe::get(TribeId::WILDLIFE)->addEnemy(Tribe::get(TribeId::MONSTER));
}

