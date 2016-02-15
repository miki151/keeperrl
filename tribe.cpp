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

EnumMap<TribeId, PTribe> Tribe::generateTribes() {
  EnumMap<TribeId, PTribe> ret;
  ret[TribeId::MONSTER].reset(new Tribe("monsters", false));
  ret[TribeId::LIZARD].reset(new Tribe("lizardmen", true));
  ret[TribeId::PEST].reset(new Tribe("pests", false));
  ret[TribeId::WILDLIFE].reset(new Tribe("wildlife", false));
  ret[TribeId::ELF].reset(new Tribe("elves", true));
  ret[TribeId::DARK_ELF].reset(new Tribe("dark elves", true));
  ret[TribeId::HUMAN].reset(new Tribe("humans", true));
  ret[TribeId::DWARF].reset(new Tribe("dwarves", true));
  ret[TribeId::GNOME].reset(new Tribe("gnomes", true));
  ret[TribeId::ADVENTURER].reset(new Tribe("player", false));
  ret[TribeId::KEEPER].reset(new Tribe("keeper", false));
  ret[TribeId::GREENSKIN].reset(new Tribe("greenskins", true));
  ret[TribeId::ANT].reset(new Tribe("ants", true));
  ret[TribeId::BANDIT].reset(new Tribe("bandits", true));
  ret[TribeId::HOSTILE].reset(new Tribe("hostile", false));
  ret[TribeId::PEACEFUL].reset(new Tribe("peaceful", false));
  ret[TribeId::DWARF]->addEnemy({ret[TribeId::BANDIT].get()});
  ret[TribeId::GNOME]->addEnemy({ret[TribeId::BANDIT].get()});
  ret[TribeId::KEEPER]->addEnemy({ret[TribeId::ADVENTURER].get(), ret[TribeId::ELF].get(), ret[TribeId::DWARF].get(), ret[TribeId::HUMAN].get(), ret[TribeId::LIZARD].get(), ret[TribeId::PEST].get(),
      ret[TribeId::MONSTER].get(), ret[TribeId::BANDIT].get(), ret[TribeId::ANT].get() });
  ret[TribeId::ELF]->addEnemy({ ret[TribeId::DWARF].get(), ret[TribeId::BANDIT].get() });
  ret[TribeId::DARK_ELF]->addEnemy({ ret[TribeId::DWARF].get(), ret[TribeId::BANDIT].get() });
  ret[TribeId::HUMAN]->addEnemy({ ret[TribeId::LIZARD].get(), ret[TribeId::BANDIT].get(), ret[TribeId::ANT].get(), ret[TribeId::DARK_ELF].get() });
  ret[TribeId::ADVENTURER]->addEnemy({ ret[TribeId::MONSTER].get(), ret[TribeId::PEST].get(), ret[TribeId::BANDIT].get(), ret[TribeId::WILDLIFE].get(), ret[TribeId::GREENSKIN].get(), ret[TribeId::ANT].get() });
  ret[TribeId::MONSTER]->addEnemy({ret[TribeId::WILDLIFE].get()});
  ret[TribeId::HOSTILE]->addEnemy({ret[TribeId::GREENSKIN].get(), ret[TribeId::MONSTER].get(), ret[TribeId::LIZARD].get(), ret[TribeId::PEST].get(), ret[TribeId::WILDLIFE].get(), ret[TribeId::ELF].get(),
      ret[TribeId::HUMAN].get(), ret[TribeId::DWARF].get(), ret[TribeId::GNOME].get(), ret[TribeId::ADVENTURER].get(), ret[TribeId::KEEPER].get(), ret[TribeId::BANDIT].get(), ret[TribeId::HOSTILE].get(),
      ret[TribeId::PEACEFUL].get(), ret[TribeId::ANT].get(), ret[TribeId::DARK_ELF].get()});
  return ret;
}


