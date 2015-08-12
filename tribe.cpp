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
  ar& SVAR(diplomatic)
    & SVAR(standing)
    & SVAR(attacks)
    & SVAR(enemyTribes)
    & SVAR(name);
}

SERIALIZABLE(Tribe);

SERIALIZATION_CONSTRUCTOR_IMPL(Tribe);

Tribe::Tribe(const string& n, bool d) : diplomatic(d), name(n) {
}

const string& Tribe::getName() const {
  return name;
}

double Tribe::getStanding(const Creature* c) const {
  if (enemyTribes.count(const_cast<Tribe*>(c->getTribe())))
    return -1;
  if (c->getTribe() == this)
    return 1;
  if (standing.count(c)) 
    return standing.at(c);
  return 0;
}

void Tribe::initStanding(const Creature* c) {
  standing[c] = getStanding(c);
}

void Tribe::addEnemy(vector<Tribe*> tribes) {
  for (Tribe* t : tribes) {
    enemyTribes.insert(t);
    if (t != this)
      t->enemyTribes.insert(this);
  }
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

double Tribe::getMultiplier(const Creature* member) {
  return 1;
}

void Tribe::onMemberKilled(Creature* member, Creature* attacker) {
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

void Tribe::onMemberAttacked(Creature* member, Creature* attacker) {
  if (contains(attacks, make_pair(member, attacker)))
    return;
  attacks.emplace_back(member, attacker);
  initStanding(attacker);
  standing[attacker] -= attackPenalty * getMultiplier(member);
}

bool Tribe::isEnemy(const Creature* c) const {
  return getStanding(c) < 0;
}

bool Tribe::isEnemy(const Tribe* t) const {
  return enemyTribes.count(const_cast<Tribe*>(t));
}

bool Tribe::isDiplomatic() const {
  return diplomatic;
}

void Tribe::onItemsStolen(const Creature* attacker) {
  if (diplomatic) {
    initStanding(attacker);
    standing[attacker] -= thiefPenalty;
  }
}

template <class Archive> 
void TribeSet::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(monster)
    & SVAR(pest)
    & SVAR(wildlife)
    & SVAR(human)
    & SVAR(elven)
    & SVAR(dwarven)
    & SVAR(gnomish)
    & SVAR(adventurer)
    & SVAR(bandit)
    & SVAR(killEveryone)
    & SVAR(peaceful)
    & SVAR(keeper)
    & SVAR(greenskins)
    & SVAR(lizard);
}

SERIALIZABLE(TribeSet);


TribeSet::TribeSet() {
  monster.reset(new Tribe("monsters", false));
  lizard.reset(new Tribe("lizardmen", true));
  pest.reset(new Tribe("pests", false));
  wildlife.reset(new Tribe("wildlife", false));
  elven.reset(new Tribe("elves", true));
  human.reset(new Tribe("humans", true));
  dwarven.reset(new Tribe("dwarves", true));
  gnomish.reset(new Tribe("gnomes", true));
  adventurer.reset(new Tribe("player", false));
  keeper.reset(new Tribe("keeper", false));
  greenskins.reset(new Tribe("greenskins", true));
  bandit.reset(new Tribe("bandits", true));
  killEveryone.reset(new Tribe("hostile", false));
  peaceful.reset(new Tribe("peaceful", false));
  dwarven->addEnemy({bandit.get()});
  gnomish->addEnemy({bandit.get()});
  keeper->addEnemy({adventurer.get(), elven.get(), dwarven.get(), human.get(), lizard.get(), pest.get(),
      monster.get(), bandit.get() });
  elven->addEnemy({ dwarven.get(), bandit.get() });
  human->addEnemy({ lizard.get(), bandit.get() });
  adventurer->addEnemy({ monster.get(), pest.get(), bandit.get(), wildlife.get(), greenskins.get() });
  monster->addEnemy({wildlife.get()});
  killEveryone->addEnemy({greenskins.get(), monster.get(), lizard.get(), pest.get(), wildlife.get(), elven.get(),
      human.get(), dwarven.get(), gnomish.get(), adventurer.get(), keeper.get(), bandit.get(), killEveryone.get(),
      peaceful.get()});
}


