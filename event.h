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

class EventListener {
  public:
  virtual void onPickupEvent(const Creature*, const vector<Item*>& items) {}
  virtual void onDropEvent(const Creature*, const vector<Item*>& items) {}
  virtual void onItemsAppearedEvent(Vec2 position, const vector<Item*>& items) {}
  virtual void onKillEvent(const Creature* victim, const Creature* killer) {}
  virtual void onAttackEvent(Creature* victim, Creature* attacker) {}
  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) {}
  virtual void onExplosionEvent(const Level* level, Vec2 pos) {}
  virtual void onTriggerEvent(const Level*, Vec2 pos) {}
  virtual void onSquareReplacedEvent(const Level*, Vec2 pos) {}
  virtual void onChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {}
  virtual void onAlarmEvent(const Level*, Vec2 pos) {}
  virtual void onTechBookEvent(Technology*) {}
  virtual void onEquipEvent(const Creature*, const Item*) {}
  virtual void onSurrenderEvent(Creature* who, const Creature* to) {}
  virtual void onTortureEvent(Creature* who, const Creature* torturer) {}
  virtual void onWorshipCreatureEvent(Creature* who, const Creature* to, WorshipType) {}
  virtual void onWorshipEvent(Creature* who, const Deity* to, WorshipType) {}

  static void addPickupEvent(const Creature*, const vector<Item*>& items);
  static void addDropEvent(const Creature*, const vector<Item*>& items);
  static void addItemsAppearedEvent(const Level*, Vec2 position, const vector<Item*>& items);
  static void addKillEvent(const Creature* victim, const Creature* killer);
  static void addAttackEvent(Creature* victim, Creature* attacker);
  static void addThrowEvent(const Level*, const Creature* thrower, const Item* item, const vector<Vec2>& trajectory);
  static void addExplosionEvent(const Level* level, Vec2 pos);
  static void addTriggerEvent(const Level*, Vec2 pos);
  static void addSquareReplacedEvent(const Level*, Vec2 pos);
  static void addChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos);
  static void addAlarmEvent(const Level*, Vec2 pos);
  static void addTechBookEvent(Technology*);
  static void addEquipEvent(const Creature*, const Item*);
  static void addSurrenderEvent(Creature* who, const Creature* to);
  static void addTortureEvent(Creature* who, const Creature* torturer);
  static void addWorshipCreatureEvent(Creature* who, const Creature* to, WorshipType);
  static void addWorshipEvent(Creature* who, const Deity* to, WorshipType);

  virtual const Level* getListenerLevel() const { return nullptr; }
  static void initialize();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
  }

  EventListener();
  virtual ~EventListener();

  private:
  static vector<EventListener*> listeners;
};

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

class EventListener2 {
  public:
  // triggered when the monster AI is either attacking, chasing or fleeing
  EVENT(CombatEvent, const Creature*);
};

extern EventListener2 GlobalEvents;

#define REGISTER_HANDLER(Event, Class, Name)\
  ConstructorFunction __LINE__##Name##Runner123 = ConstructorFunction([=] {\
      GlobalEvents.link##Event(this, bindMethod(&Class::Name, this));\
  }, [=] {\
      GlobalEvents.unlink##Event(this);\
  })


#endif
