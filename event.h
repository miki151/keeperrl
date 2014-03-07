#ifndef _EVENT_H
#define _EVENT_H

#include "util.h"

class Level;
class Creature;
class Item;
class Quest;

class EventListener {
  public:
  virtual void onPickupEvent(const Creature*, const vector<Item*>& items) {}
  virtual void onDropEvent(const Creature*, const vector<Item*>& items) {}
  virtual void onItemsAppearedEvent(Vec2 position, const vector<Item*>& items) {}
  virtual void onKillEvent(const Creature* victim, const Creature* killer) {}
  virtual void onAttackEvent(const Creature* victim, const Creature* attacker) {}
  // triggered when the monster AI is either attacking, chasing or fleeing
  virtual void onCombatEvent(const Creature*) {}
  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) {}
  virtual void onExplosionEvent(const Level* level, Vec2 pos) {}
  virtual void onTriggerEvent(const Level*, Vec2 pos) {}
  virtual void onSquareReplacedEvent(const Level*, Vec2 pos) {}
  virtual void onChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {}
  virtual void onAlarmEvent(const Level*, Vec2 pos) {}

  static void addPickupEvent(const Creature*, const vector<Item*>& items);
  static void addDropEvent(const Creature*, const vector<Item*>& items);
  static void addItemsAppearedEvent(const Level*, Vec2 position, const vector<Item*>& items);
  static void addKillEvent(const Creature* victim, const Creature* killer);
  static void addAttackEvent(const Creature* victim, const Creature* attacker);
  static void addCombatEvent(const Creature*);
  static void addThrowEvent(const Level*, const Creature* thrower, const Item* item, const vector<Vec2>& trajectory);
  static void addExplosionEvent(const Level* level, Vec2 pos);
  static void addTriggerEvent(const Level*, Vec2 pos);
  static void addSquareReplacedEvent(const Level*, Vec2 pos);
  static void addChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos);
  static void addAlarmEvent(const Level*, Vec2 pos);

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

#endif
