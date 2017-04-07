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
  ar(diplomatic, standing, friendlyTribes, id);
}

SERIALIZABLE(Tribe);

SERIALIZATION_CONSTRUCTOR_IMPL(Tribe);

Tribe::Tribe(TribeId d, bool p) : diplomatic(p), friendlyTribes(TribeSet::getFull()), id(d) {
}

double Tribe::getStanding(WConstCreature c) const {
  if (!friendlyTribes.contains(c->getTribeId()))
    return -1;
  if (c->getTribe() == this)
    return 1;
  if (auto res = standing.getMaybe(c)) 
    return *res;
  return 0;
}

void Tribe::initStanding(WConstCreature c) {
  standing.set(c, getStanding(c));
}

void Tribe::addEnemy(Tribe* t) {
  friendlyTribes.erase(t->id);
  if (t != this)
    t->friendlyTribes.erase(id);
}

static const double killPenalty = 0.5;
static const double attackPenalty = 0.2;
static const double thiefPenalty = 0.5;

double Tribe::getMultiplier(WConstCreature member) {
  return 1;
}

void Tribe::onMemberKilled(WCreature member, WCreature attacker) {
  CHECK(member->getTribe() == this);
  if (attacker == nullptr)
    return;
  initStanding(attacker);
  standing.getOrFail(attacker) -= killPenalty * getMultiplier(member);
}

bool Tribe::isEnemy(WConstCreature c) const {
  return getStanding(c) < 0;
}

bool Tribe::isEnemy(const Tribe* t) const {
  return !friendlyTribes.contains(t->id);
}

const TribeSet& Tribe::getFriendlyTribes() const {
  return friendlyTribes;
}

void Tribe::onItemsStolen(WConstCreature attacker) {
  if (diplomatic) {
    initStanding(attacker);
    standing.getOrFail(attacker) -= thiefPenalty;
  }
}

static void addEnemies(Tribe::Map& map, TribeId tribe, vector<TribeId> ids) {
  for (TribeId id : ids)
    map[tribe]->addEnemy(map[id].get());
}

void Tribe::init(Tribe::Map& map, TribeId id, bool diplomatic) {
  map[id].reset(new Tribe(id, diplomatic));
}

Tribe::Map Tribe::generateTribes() {
  Map ret;
  init(ret, TribeId::getMonster(), false);
  init(ret, TribeId::getLizard(), true);
  init(ret, TribeId::getPest(), false);
  init(ret, TribeId::getWildlife(), false);
  init(ret, TribeId::getElf(), true);
  init(ret, TribeId::getDarkElf(), true);
  init(ret, TribeId::getHuman(), true);
  init(ret, TribeId::getDwarf(), true);
  init(ret, TribeId::getGnome(), true);
  init(ret, TribeId::getAdventurer(), false);
  init(ret, TribeId::getKeeper(), false);
  init(ret, TribeId::getRetiredKeeper(), false);
  init(ret, TribeId::getGreenskin(), true);
  init(ret, TribeId::getAnt(), true);
  init(ret, TribeId::getBandit(), true);
  init(ret, TribeId::getHostile(), false);
  init(ret, TribeId::getPeaceful(), false);
  init(ret, TribeId::getShelob(), false);
  addEnemies(ret, TribeId::getDwarf(),
      {TribeId::getBandit()});
  addEnemies(ret, TribeId::getGnome(),
      {TribeId::getBandit()});
  addEnemies(ret, TribeId::getKeeper(), {
      TribeId::getAdventurer(), TribeId::getElf(), TribeId::getDwarf(), TribeId::getHuman(), TribeId::getLizard(),
      TribeId::getPest(), TribeId::getMonster(), TribeId::getBandit(), TribeId::getAnt(),
      TribeId::getRetiredKeeper()});
  addEnemies(ret, TribeId::getElf(), {
      TribeId::getDwarf(), TribeId::getBandit() });
  addEnemies(ret, TribeId::getDarkElf(), {
      TribeId::getDwarf(), TribeId::getBandit() });
  addEnemies(ret, TribeId::getHuman(), {
      TribeId::getLizard(), TribeId::getBandit(), TribeId::getAnt(), TribeId::getDarkElf() });
  addEnemies(ret, TribeId::getAdventurer(), {
      TribeId::getMonster(), TribeId::getPest(), TribeId::getBandit(), TribeId::getDarkElf(),
      TribeId::getGreenskin(), TribeId::getAnt(), TribeId::getRetiredKeeper() });
  addEnemies(ret, TribeId::getMonster(), {
      TribeId::getWildlife()});
  addEnemies(ret, TribeId::getShelob(), {
      TribeId::getGreenskin(), TribeId::getMonster(), TribeId::getLizard(), TribeId::getPest(),
      TribeId::getWildlife(), TribeId::getElf(), TribeId::getHuman(), TribeId::getDwarf(), TribeId::getGnome(),
      TribeId::getAdventurer(), TribeId::getKeeper(), TribeId::getBandit(),
      TribeId::getPeaceful(), TribeId::getAnt(), TribeId::getDarkElf(), TribeId::getRetiredKeeper()});
  addEnemies(ret, TribeId::getHostile(), {
      TribeId::getGreenskin(), TribeId::getMonster(), TribeId::getLizard(), TribeId::getPest(),
      TribeId::getWildlife(), TribeId::getElf(), TribeId::getHuman(), TribeId::getDwarf(), TribeId::getGnome(),
      TribeId::getAdventurer(), TribeId::getKeeper(), TribeId::getBandit(), TribeId::getHostile(),
      TribeId::getPeaceful(), TribeId::getAnt(), TribeId::getDarkElf(), TribeId::getShelob(),
      TribeId::getRetiredKeeper()});
  return ret;
}

TribeId::TribeId(KeyType k) : key(k) {}

TribeId TribeId::getMonster() {
  return TribeId(0);
}

TribeId TribeId::getPest() {
  return TribeId(1);
}

TribeId TribeId::getWildlife() {
  return TribeId(2);
}

TribeId TribeId::getHuman() {
  return TribeId(3);
}

TribeId TribeId::getElf() {
  return TribeId(4);
}

TribeId TribeId::getDarkElf() {
  return TribeId(5);
}

TribeId TribeId::getDwarf() {
  return TribeId(6);
}

TribeId TribeId::getGnome() {
  return TribeId(7);
}

TribeId TribeId::getAdventurer() {
  return TribeId(8);
}

TribeId TribeId::getBandit() {
  return TribeId(9);
}

TribeId TribeId::getHostile() {
  return TribeId(10);
}

TribeId TribeId::getPeaceful() {
  return TribeId(11);
}

TribeId TribeId::getKeeper() {
  return TribeId(12);
}

TribeId TribeId::getRetiredKeeper() {
  return TribeId(13);
}

TribeId TribeId::getLizard() {
  return TribeId(14);
}

TribeId TribeId::getGreenskin() {
  return TribeId(15);
}

TribeId TribeId::getAnt() {
  return TribeId(16);
}

TribeId TribeId::getShelob() {
  return TribeId(17);
}

optional<pair<TribeId, TribeId>> TribeId::serialSwitch;

void TribeId::switchForSerialization(TribeId from, TribeId to) {
  serialSwitch = make_pair(from, to);
}

void TribeId::clearSwitch() {
  serialSwitch = none;
}

template <class Archive> 
void TribeId::serialize(Archive& ar, const unsigned int version) {
  if (serialSwitch && *this == serialSwitch->first)
    *this = serialSwitch->second;
  ar(key);
  if (serialSwitch && *this == serialSwitch->second)
    *this = serialSwitch->first;
}

SERIALIZABLE(TribeId);

SERIALIZATION_CONSTRUCTOR_IMPL(TribeId);

bool TribeId::operator == (const TribeId& o) const {
  return key == o.key;
}

bool TribeId::operator != (const TribeId& o) const {
  return !(*this == o);
}

int TribeId::getHash() const {
  return key;
}

TribeSet TribeSet::getFull() {
  TribeSet ret;
  ret.elems.set();
  return ret;
}

void TribeSet::clear() {
  elems.reset();
}

TribeSet& TribeSet::insert(TribeId id) {
  CHECK(id.key >= 0 && id.key < elems.size());
  elems.set(id.key);
  return *this;
}

TribeSet& TribeSet::erase(TribeId id) {
  CHECK(id.key >= 0 && id.key < elems.size());
  elems.reset(id.key);
  return *this;
}

bool TribeSet::contains(TribeId id) const {
  CHECK(id.key >= 0 && id.key < elems.size());
  return elems.test(id.key);
}


bool TribeSet::operator==(const TribeSet& o) const {
  return elems == o.elems;
}

SERIALIZE_DEF(TribeSet, elems);
