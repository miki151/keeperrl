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
  Skill::registerTypes(ar);
  REGISTER_TYPE(ar, SolidSquare);
  Tribe::registerTypes(ar);
  Trigger::registerTypes(ar);
  VillageControl::registerTypes(ar);
  Task::registerTypes(ar);
}

REGISTER_TYPES(Serialization);

void SerialChecker::checkSerial() {
  for (Check* c : checks)
    c->tickOff();
}

SerialChecker::Check::Check(SerialChecker& checker, const string& n) : name(n) {
  checker.addCheck(this);
}

void SerialChecker::Check::tick() {
  CHECK(!ticked) << name;
  ticked = true;
}

void SerialChecker::Check::tickOff() {
  CHECK(ticked) << name;
  ticked = false;
}

SerialChecker::Check::Check(const Check& other) {
  CHECK(other.newCopy);
  other.newCopy->addCheck(this);
  name = other.name;
}

SerialChecker::Check& SerialChecker::Check::operator = (const Check& other) {
  CHECK(other.newCopy);
  other.newCopy->addCheck(this);
  name = other.name;
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
