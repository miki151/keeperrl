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
#include "map_memory.h"
#include "creature.h"
#include "player_message.h"
#include "creature_name.h"
#include "gender.h"
#include "monster_ai.h"
#include "creature_attributes.h"

SERIALIZE_DEF(Monster, SUBCLASS(Controller), monsterAI)
SERIALIZATION_CONSTRUCTOR_IMPL(Monster);

Monster::Monster(WCreature c, const MonsterAIFactory& f) : Controller(c), monsterAI(f.getMonsterAI(c)) {}

ControllerFactory Monster::getFactory(MonsterAIFactory f) {
  return ControllerFactory([=](WCreature c) { return makeOwner<Monster>(c, f);});
}

void Monster::makeMove() {
  monsterAI->makeMove();
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

void Monster::onBump(WCreature c) {
  if (c->isEnemy(getCreature()))
    c->attack(getCreature(), none, false).perform(c);
  else if (auto action = c->move(getCreature()->getPosition()))
    action.perform(c);
}

void Monster::you(MsgType type, const vector<string>& param) {
  string msg;
  switch (type) {
    case MsgType::SWING_WEAPON:
      msg = getCreature()->getName().the() + " swings " + getCreature()->getAttributes().getGender().his() + " " + param.at(0); break;
    case MsgType::THRUST_WEAPON:
      msg = getCreature()->getName().the() + " thrusts " + getCreature()->getAttributes().getGender().his() + " " + param.at(0); break;
    case MsgType::KICK: msg = getCreature()->getName().the() + addName(" kicks", param.at(0)); break;
    case MsgType::PUNCH: msg = getCreature()->getName().the() + addName(" punches", param.at(0)); break;
    default: you(type, param.at(0)); return;
  }
  if (param.size() > 1)
    msg += " " + param[1];
  if (!msg.empty())
    getCreature()->monsterMessage(msg);
}

void Monster::you(MsgType type, const string& param) {
  string msg, msgNoSee;
  switch (type) {
    case MsgType::ARE: msg = getCreature()->getName().the() + " is " + param; break;
    case MsgType::YOUR: msg = getCreature()->getName().the() + "'s " + param; break;
    case MsgType::FEEL: msg = getCreature()->getName().the() + " looks " + param; break;
    case MsgType::FALL_ASLEEP: msg = (getCreature()->getName().the() + " falls asleep") +
                               (param.size() > 0 ? " on the " + param : ".");
                               msgNoSee = "You hear snoring."; break;
    case MsgType::WAKE_UP: msg = getCreature()->getName().the() + " wakes up."; break;
    case MsgType::DIE: msg = getCreature()->getName().the() + " is killed!"; break;
    case MsgType::TELE_APPEAR: msg = getCreature()->getName().the() + " appears out of nowhere!"; break;
    case MsgType::TELE_DISAPPEAR: msg = getCreature()->getName().the() + " suddenly disappears!"; break;
    case MsgType::BLEEDING_STOPS: msg = getCreature()->getName().the() + "'s bleeding stops."; break;
    case MsgType::DIE_OF: msg = getCreature()->getName().the() +
                          " dies" + (param.empty() ? string(".") : " of " + param); break;
    case MsgType::MISS_ATTACK: msg = getCreature()->getName().the() + addName(" misses", param); break;
    case MsgType::MISS_THROWN_ITEM: msg = param + " misses " + getCreature()->getName().the(); break;
    case MsgType::MISS_THROWN_ITEM_PLURAL: msg = param + " miss " + getCreature()->getName().the(); break;
    case MsgType::HIT_THROWN_ITEM: msg = param + " hits " + getCreature()->getName().the(); break;
    case MsgType::HIT_THROWN_ITEM_PLURAL: msg = param + " hit " + getCreature()->getName().the(); break;
    case MsgType::ITEM_CRASHES: msg = param + " crashes on " + getCreature()->getName().the(); break;
    case MsgType::ITEM_CRASHES_PLURAL: msg = param + " crash on " + getCreature()->getName().the(); break;
    case MsgType::GET_HIT_NODAMAGE: msg = "The " + param + " is harmless."; break;
    case MsgType::COLLAPSE: msg = getCreature()->getName().the() + " collapses."; break;
    case MsgType::FALL: msg = getCreature()->getName().the() + " falls " + param; break;
    case MsgType::TRIGGER_TRAP: msg = getCreature()->getName().the() + " triggers something."; break;
    case MsgType::DISARM_TRAP: msg = getCreature()->getName().the() + " disarms a " + param; break;
    case MsgType::PANIC: msg = getCreature()->getName().the() + " panics."; break;
    case MsgType::RAGE: msg = getCreature()->getName().the() + " is enraged."; break;
    case MsgType::BITE: msg = getCreature()->getName().the() + addName(" bites", param); break;
    case MsgType::CRAWL: msg = getCreature()->getName().the() + " is crawling"; break;
    case MsgType::STAND_UP: msg = getCreature()->getName().the() + " is back on " +
                            getCreature()->getAttributes().getGender().his() + " feet ";
                            break;
    case MsgType::TURN_INVISIBLE: msg = getCreature()->getName().the() + " disappears!"; break;
    case MsgType::TURN_VISIBLE: msg = getCreature()->getName().the() + " appears out of nowhere!"; break;
    case MsgType::DROP_WEAPON: msg = getCreature()->getName().the() + " drops " +
                               getCreature()->getAttributes().getGender().his() + " " + param;
                               break;
    case MsgType::ENTER_PORTAL: msg = getCreature()->getName().the() + " disappears in the portal."; break;
    case MsgType::HAPPENS_TO: msg = param + " " + getCreature()->getName().the(); break;
    case MsgType::BURN: msg = getCreature()->getName().the() + " burns in the " + param; msgNoSee = "You hear a horrible shriek"; break;
    case MsgType::DROWN: msg = getCreature()->getName().the() + " drowns in the " + param; msgNoSee = "You hear a loud splash" ;break;
    case MsgType::SET_UP_TRAP: msg = getCreature()->getName().the() + " sets up the trap"; break;
    case MsgType::KILLED_BY: msg = getCreature()->getName().the() + " is killed by " + param; break;
    case MsgType::TURN: msg = getCreature()->getName().the() + " turns " + param; break;
    case MsgType::BECOME: msg = getCreature()->getName().the() + " becomes " + param; break;
    case MsgType::COPULATE: msg = getCreature()->getName().the() + " copulates " + param; break;
    case MsgType::CONSUME: msg = getCreature()->getName().the() + " absorbs " + param; break;
    case MsgType::GROW: msg = getCreature()->getName().the() + " grows " + param; break;
    case MsgType::BREAK_FREE:
        if (param.empty())
          msg = getCreature()->getName().the() + " breaks free";
        else
          msg = getCreature()->getName().the() + " breaks free from " + param;
        break;
    case MsgType::PRAY: msg = getCreature()->getName().the() + " prays to " + param; break;
    case MsgType::SACRIFICE: msg = getCreature()->getName().the() + " makes a sacrifice to " + param; break;
    case MsgType::HIT: msg = getCreature()->getName().the() + addName(" hits", param); break;
    default: break;
  }
  if (!msg.empty())
    getCreature()->monsterMessage(msg, msgNoSee);
}
  
void Monster::you(const string& param) {
  getCreature()->monsterMessage(getCreature()->getName().the() + " " + param);
}

