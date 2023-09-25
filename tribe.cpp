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

double Tribe::getStanding(const Creature* c) const {
  auto tribeId = c->getTribeId();
  if (!friendlyTribes.contains(tribeId))
    return -1;
  if (tribeId == id)
    return 1;
  if (auto res = standing.getMaybe(c)) 
    return *res;
  return 0;
}

void Tribe::initStanding(const Creature* c) {
  standing.set(c, getStanding(c));
}

void Tribe::addEnemy(Tribe* t) {
  if (t != this) {
    friendlyTribes.erase(t->id);
    t->friendlyTribes.erase(id);
  }
}

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
  if (diplomatic) {
    initStanding(attacker);
    standing.getOrFail(attacker) -= killPenalty * getMultiplier(member);
  }
}

bool Tribe::isEnemy(const Creature* c) const {
  return getStanding(c) < 0;
}

bool Tribe::isEnemy(const Tribe* t) const {
  return !friendlyTribes.contains(t->id);
}

const TribeSet& Tribe::getFriendlyTribes() const {
  return friendlyTribes;
}

void Tribe::onItemsStolen(Creature* attacker) {
  if (diplomatic) {
    initStanding(attacker);
    standing.getOrFail(attacker) -= thiefPenalty;
    addEnemy(attacker->getTribe());
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
  init(ret, TribeId::getWhiteKeeper(), false);
  init(ret, TribeId::getDarkKeeper(), false);
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
  addEnemies(ret, TribeId::getDarkKeeper(), {
      TribeId::getWhiteKeeper(), TribeId::getElf(), TribeId::getDwarf(), TribeId::getHuman(), TribeId::getLizard(),
      TribeId::getPest(), TribeId::getMonster(), TribeId::getBandit(), TribeId::getAnt(),
      TribeId::getRetiredKeeper(), TribeId::getWildlife()});
  addEnemies(ret, TribeId::getElf(), {
      TribeId::getDwarf(), TribeId::getBandit() });
  addEnemies(ret, TribeId::getDarkElf(), {
      TribeId::getDwarf(), TribeId::getBandit() });
  addEnemies(ret, TribeId::getHuman(), {
      TribeId::getLizard(), TribeId::getBandit(), TribeId::getAnt(), TribeId::getDarkElf(),
      TribeId::getGreenskin()});
  addEnemies(ret, TribeId::getWhiteKeeper(), {
      TribeId::getMonster(), TribeId::getPest(), TribeId::getBandit(), TribeId::getDarkElf(),
      TribeId::getGreenskin(), TribeId::getAnt(), TribeId::getRetiredKeeper(), TribeId::getLizard(), TribeId::getWildlife() });
  addEnemies(ret, TribeId::getMonster(), {
      TribeId::getWildlife(), TribeId::getHuman(), TribeId::getBandit(),  TribeId::getLizard()});
  addEnemies(ret, TribeId::getShelob(), {
      TribeId::getGreenskin(), TribeId::getMonster(), TribeId::getLizard(), TribeId::getPest(),
      TribeId::getWildlife(), TribeId::getElf(), TribeId::getHuman(), TribeId::getDwarf(), TribeId::getGnome(),
      TribeId::getWhiteKeeper(), TribeId::getDarkKeeper(), TribeId::getBandit(),
      TribeId::getPeaceful(), TribeId::getAnt(), TribeId::getDarkElf(), TribeId::getRetiredKeeper()});
  addEnemies(ret, TribeId::getHostile(), {
      TribeId::getGreenskin(), TribeId::getMonster(), TribeId::getLizard(), TribeId::getPest(),
      TribeId::getWildlife(), TribeId::getElf(), TribeId::getHuman(), TribeId::getDwarf(), TribeId::getGnome(),
      TribeId::getWhiteKeeper(), TribeId::getDarkKeeper(), TribeId::getBandit(), TribeId::getHostile(),
      TribeId::getPeaceful(), TribeId::getAnt(), TribeId::getDarkElf(), TribeId::getShelob(),
      TribeId::getRetiredKeeper()});
  return ret;
}

TribeId::TribeId(KeyType k) : key(k) {}

TribeId TribeId::getMonster() {
  return TribeId(KeyType::MONSTER);
}

TribeId TribeId::getPest() {
  return TribeId(KeyType::PEST);
}

TribeId TribeId::getWildlife() {
  return TribeId(KeyType::WILDLIFE);
}

TribeId TribeId::getHuman() {
  return TribeId(KeyType::HUMAN);
}

TribeId TribeId::getElf() {
  return TribeId(KeyType::ELF);
}

TribeId TribeId::getDarkElf() {
  return TribeId(KeyType::DARK_ELF);
}

TribeId TribeId::getDwarf() {
  return TribeId(KeyType::DWARF);
}

TribeId TribeId::getGnome() {
  return TribeId(KeyType::GNOME);
}

TribeId TribeId::getWhiteKeeper() {
  return TribeId(KeyType::WHITE_KEEPER);
}

TribeId TribeId::getBandit() {
  return TribeId(KeyType::BANDIT);
}

TribeId TribeId::getHostile() {
  return TribeId(KeyType::HOSTILE);
}

TribeId TribeId::getPeaceful() {
  return TribeId(KeyType::PEACEFUL);
}

TribeId TribeId::getDarkKeeper() {
  return TribeId(KeyType::DARK_KEEPER);
}

TribeId TribeId::getRetiredKeeper() {
  return TribeId(KeyType::RETIRED_KEEPER);
}

TribeId TribeId::getLizard() {
  return TribeId(KeyType::LIZARD);
}

TribeId TribeId::getGreenskin() {
  return TribeId(KeyType::GREENSKIN);
}

TribeId TribeId::getAnt() {
  return TribeId(KeyType::ANT);
}

TribeId TribeId::getShelob() {
  return TribeId(KeyType::SHELOB);
}

static HashMap<TribeId, TribeId> serialSwitch;

void TribeId::switchForSerialization(TribeId from, TribeId to) {
  serialSwitch[from] = to;
}

void TribeId::clearSwitch() {
  serialSwitch.clear();
}

template <class Archive>
void TribeId::serialize(Archive& ar, const unsigned int version) {
  ar(key);
  if (serialSwitch.count(*this))
    *this = serialSwitch.at(*this);
}

SERIALIZABLE(TribeId);

SERIALIZATION_CONSTRUCTOR_IMPL(TribeId);

bool TribeId::operator == (const TribeId& o) const {
  return key == o.key;
}

bool TribeId::operator != (const TribeId& o) const {
  return !(*this == o);
}

TribeId::KeyType TribeId::getKey() const {
  return key;
}

int TribeId::getHash() const {
  return int(key);
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
  auto key = int(id.key);
  CHECK(key >= 0 && key < elems.size());
  elems.set(key);
  return *this;
}

TribeSet& TribeSet::erase(TribeId id) {
  auto key = int(id.key);
  CHECK(key >= 0 && key < elems.size());
  elems.reset(key);
  return *this;
}

bool TribeSet::contains(TribeId id) const {
  auto key = int(id.key);
  CHECK(key >= 0 && key < elems.size());
  return elems.test(key);
}

int TribeSet::getHash() const {
  return int(elems.to_ulong());
}


bool TribeSet::operator==(const TribeSet& o) const {
  return elems == o.elems;
}

SERIALIZE_DEF(TribeSet, elems);


#include "pretty_archive.h"
template
void TribeId::serialize(PrettyInputArchive& ar1, unsigned);
