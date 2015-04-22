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
  EVENT(PickupEvent, const Creature*, const vector<Item*>& items);
  EVENT(DropEvent, const Creature*, const vector<Item*>& items);
  EVENT(ItemsAppearedEvent, const Level*, Vec2 position, const vector<Item*>& items);
  EVENT(ThrowEvent, const Level*, const Creature* thrower, const Item* item, const vector<Vec2>& trajectory);
  EVENT(ExplosionEvent, const Level* level, Vec2 pos);
  EVENT(EquipEvent, const Creature*, const Item*);
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
