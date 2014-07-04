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

#include "serialization.h"
#include "creature_factory.h"
#include "square_factory.h"
#include "monster_ai.h"
#include "effect.h"
#include "enemy_check.h"
#include "quest.h"
#include "skill.h"
#include "tribe.h"
#include "trigger.h"
#include "player.h"
#include "monster.h"
#include "ranged_weapon.h"
#include "square.h"
#include "village_control.h"
#include "task.h"
#include "player_control.h"
#include "item_factory.h"
#include "collective_control.h"

template <class Archive>
void Serialization::registerTypes(Archive& ar) {
  CreatureFactory::registerTypes(ar);
  ItemFactory::registerTypes(ar);
  SquareFactory::registerTypes(ar);
  MonsterAI::registerTypes(ar);
  Effect::registerTypes(ar);
  EnemyCheck::registerTypes(ar);
  REGISTER_TYPE(ar, Player);
  REGISTER_TYPE(ar, Monster);
  REGISTER_TYPE(ar, RangedWeapon);
  Quest::registerTypes(ar);
  REGISTER_TYPE(ar, SolidSquare);
  Tribe::registerTypes(ar);
  Trigger::registerTypes(ar);
  VillageControl::registerTypes(ar);
  Task::registerTypes(ar);
  Player::registerTypes(ar);
  PlayerControl::registerTypes(ar);
  CollectiveControl::registerTypes(ar);
}

REGISTER_TYPES(Serialization);

void SerialChecker::checkSerial() {
  for (Check* c : checks)
    c->tickOff();
}

SerialChecker::Check::Check(SerialChecker& checker) {
  checker.addCheck(this);
}

void SerialChecker::Check::tick() {
  CHECK(!ticked);
  ticked = true;
}

void SerialChecker::Check::tickOff() {
  CHECK(ticked);
  ticked = false;
}

SerialChecker::Check::Check(const Check& other) {
  CHECK(other.newCopy);
  other.newCopy->addCheck(this);
}

SerialChecker::Check& SerialChecker::Check::operator = (const Check& other) {
  CHECK(other.newCopy);
  other.newCopy->addCheck(this);
  return *this;
}

void SerialChecker::Check::setNewCopy(SerialChecker* n) {
  newCopy = n;
}

SerialChecker::SerialChecker(const SerialChecker& other) {
  for (Check* c : other.checks)
    c->setNewCopy(this);
}
  
SerialChecker& SerialChecker::operator = (const SerialChecker& other) {
  checks.clear();
  for (Check* c : other.checks)
    c->setNewCopy(this);
  return *this;
}
  
void SerialChecker::addCheck(Check* c) {
  CHECK(!contains(checks, c));
  checks.push_back(c);
}
