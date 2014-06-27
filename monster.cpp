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

#include "monster.h"

template <class Archive> 
void Monster::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(Controller)
     & SVAR(creature)
     & SVAR(actor)
     & SVAR(enemies);
  CHECK_SERIAL;
}

SERIALIZABLE(Monster);

SERIALIZATION_CONSTRUCTOR_IMPL(Monster);

Monster::Monster(Creature* c, MonsterAIFactory f) : Controller(c), actor(f.getMonsterAI(c)) {}

ControllerFactory Monster::getFactory(MonsterAIFactory f) {
  return ControllerFactory([=](Creature* c) { return new Monster(c, f);});
}

void Monster::makeMove() {
  actor->makeMove();
}

static string addName(const string& s, const string& n) {
  if (n.size() > 0)
    return s + " " + n;
  else
    return s;
}

bool Monster::isPlayer() const {
  return false;
}

const MapMemory& Monster::getMemory() const {
  return MapMemory::empty();
}

void Monster::onBump(Creature* c) {
  if (c->isEnemy(creature))
    c->attack(creature, Nothing(), false).perform();
  else if (auto action = c->move(creature->getPosition() - c->getPosition()))
    action.perform();
}

void Monster::you(MsgType type, const string& param) const {
  string msg, msgNoSee;
  switch (type) {
    case MsgType::ARE: msg = creature->getTheName() + " is " + param; break;
    case MsgType::YOUR: msg = creature->getTheName() + "'s " + param; break;
    case MsgType::FEEL: msg = creature->getTheName() + " looks " + param; break;
    case MsgType::FALL_ASLEEP: msg = (creature->getTheName() + " falls asleep") + (param.size() > 0 ? " on the " + param : ".");
                               msgNoSee = "You hear snoring."; break;
    case MsgType::WAKE_UP: msg = creature->getTheName() + " wakes up."; break;
    case MsgType::DIE: msg = creature->getTheName() + " is killed!"; break;
    case MsgType::TELE_APPEAR: msg = creature->getTheName() + " appears out of nowhere!"; break;
    case MsgType::TELE_DISAPPEAR: msg = creature->getTheName() + " suddenly disappears!"; break;
    case MsgType::BLEEDING_STOPS: msg = creature->getTheName() + "'s bleeding stops."; break;
    case MsgType::DIE_OF: msg = creature->getTheName() +
                          " dies" + (param.empty() ? string(".") : " of " + param); break;
    case MsgType::FALL_APART: msg = creature->getTheName() + " falls apart."; break;
    case MsgType::MISS_ATTACK: msg = creature->getTheName() + addName(" misses", param); break;
    case MsgType::MISS_THROWN_ITEM: msg = param + " misses " + creature->getTheName(); break;
    case MsgType::MISS_THROWN_ITEM_PLURAL: msg = param + " miss " + creature->getTheName(); break;
    case MsgType::HIT_THROWN_ITEM: msg = param + " hits " + creature->getTheName(); break;
    case MsgType::HIT_THROWN_ITEM_PLURAL: msg = param + " hit " + creature->getTheName(); break;
    case MsgType::ITEM_CRASHES: msg = param + " crashes on " + creature->getTheName(); break;
    case MsgType::ITEM_CRASHES_PLURAL: msg = param + " crash on " + creature->getTheName(); break;
    case MsgType::GET_HIT_NODAMAGE: msg = "The " + param + " is harmless."; break;
    case MsgType::COLLAPSE: msg = creature->getTheName() + " collapses."; break;
    case MsgType::FALL: msg = creature->getTheName() + " falls on the " + param; break;
    case MsgType::TRIGGER_TRAP: msg = creature->getTheName() + " triggers something."; break;
    case MsgType::PANIC: msg = creature->getTheName() + " panics."; break;
    case MsgType::RAGE: msg = creature->getTheName() + " is enraged."; break;
    case MsgType::SWING_WEAPON: msg = creature->getTheName() + " swings " + creature->getGender().his() + " " +
                                    param; break;
    case MsgType::THRUST_WEAPON: msg = creature->getTheName() + " thrusts " + creature->getGender().his() + " " +
                                 param; break;
    case MsgType::KICK: msg = creature->getTheName() + addName(" kicks", param); break;
    case MsgType::BITE: msg = creature->getTheName() + addName(" bites", param); break;
    case MsgType::PUNCH: msg = creature->getTheName() + addName(" punches", param); break;
    case MsgType::CRAWL: msg = creature->getTheName() + " is crawling"; break;
    case MsgType::STAND_UP: msg = creature->getTheName() + " is back on " + creature->getGender().his() + " feet ";
                            break;
    case MsgType::TURN_INVISIBLE: msg = creature->getTheName() + " disappears!"; break;
    case MsgType::TURN_VISIBLE: msg = creature->getTheName() + " appears out of nowhere!"; break;
    case MsgType::DROP_WEAPON: msg = creature->getTheName() + " drops " + creature->getGender().his() + " " + param;
                               break;
    case MsgType::ENTER_PORTAL: msg = creature->getTheName() + " disappears in the portal."; break;
    case MsgType::HAPPENS_TO: msg = param + " " + creature->getTheName(); break;
    case MsgType::BURN: msg = creature->getTheName() + " burns in the " + param; msgNoSee = "You hear a horrible shriek"; break;
    case MsgType::DROWN: msg = creature->getTheName() + " drowns in the " + param; msgNoSee = "You hear a loud splash" ;break;
    case MsgType::SET_UP_TRAP: msg = creature->getTheName() + " sets up the trap"; break;
    case MsgType::KILLED_BY: msg = creature->getTheName() + " is killed by " + param; break;
    case MsgType::TURN: msg = creature->getTheName() + " turns into " + param; break;
    case MsgType::BREAK_FREE: msg = creature->getTheName() + " breaks free from " + param; break;
    case MsgType::HIT: msg = creature->getTheName() + addName(" hits", param); break;
    default: break;
  }
  if (!msg.empty())
    creature->monsterMessage(msg, msgNoSee);
}
  
void Monster::you(const string& param) const {
  creature->monsterMessage(creature->getTheName() + " " + param);
}

