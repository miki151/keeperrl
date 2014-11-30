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

#ifndef _EVENT_H
#define _EVENT_H

#include "util.h"

class Level;
class Creature;
class Item;
class Quest;
class Technology;
class Deity;
enum class WorshipType;
enum class AssaultType;

#define EVENT(Name, ...)\
template<typename... Ts>\
void add##Name(Ts... ts) {\
  for (auto elem : funs##Name)\
    elem.second(ts...);\
}\
typedef function<void(__VA_ARGS__)> Fun##Name;\
unordered_map<void*, Fun##Name> funs##Name;\
void link##Name(void* obj, Fun##Name fun) {\
  funs##Name[obj] = fun;\
}\
void unlink##Name(void* obj) {\
  funs##Name.erase(obj);\
}

class EventListener {
  public:
  // triggered when the monster AI is either attacking, chasing or fleeing
  EVENT(CombatEvent, const Creature*);
  EVENT(PickupEvent, const Creature*, const vector<Item*>& items);
  EVENT(DropEvent, const Creature*, const vector<Item*>& items);
  EVENT(ItemsAppearedEvent, const Level*, Vec2 position, const vector<Item*>& items);
  EVENT(KillEvent, const Creature* victim, const Creature* killer);
  EVENT(AttackEvent, Creature* victim, Creature* attacker);
  EVENT(ThrowEvent, const Level*, const Creature* thrower, const Item* item, const vector<Vec2>& trajectory);
  EVENT(ExplosionEvent, const Level* level, Vec2 pos);
  EVENT(TrapTriggerEvent, const Level*, Vec2 pos);
  EVENT(TrapDisarmEvent, const Level*, const Creature*, Vec2 pos);
  EVENT(SquareReplacedEvent, const Level*, Vec2 pos);
  EVENT(ChangeLevelEvent, const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos);
  EVENT(AlarmEvent, const Level*, Vec2 pos);
  EVENT(TechBookEvent, Technology*);
  EVENT(EquipEvent, const Creature*, const Item*);
  EVENT(SurrenderEvent, Creature* who, const Creature* to);
  EVENT(TortureEvent, Creature* who, const Creature* torturer);
  EVENT(WorshipCreatureEvent, Creature* who, const Creature* to, WorshipType);
  EVENT(WorshipEvent, Creature* who, const Deity* to, WorshipType);
  EVENT(KilledLeaderEvent, const Collective*, const Creature*);
  EVENT(SunlightChangeEvent);
};

extern EventListener GlobalEvents;

#define REGISTER_HANDLER(Event, ...)\
  ConstructorFunction __LINE__##Event##Runner123 = ConstructorFunction([=] {\
      auto A = this;\
      GlobalEvents.link##Event(this, bindMethod(&std::remove_reference<decltype(*A)>::type::on##Event, this));\
  });\
  DestructorFunction __LINE##Event##Runnder321 = DestructorFunction([=] {\
      GlobalEvents.unlink##Event(this);\
  });\
  void on##Event(__VA_ARGS__)


#endif
