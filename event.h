#ifndef _EVENT_H
#define _EVENT_H

#include "util.h"

class Level;
class Creature;
class Item;

class EventListener {
  public:
  virtual void onPickupEvent(const Creature*, const vector<Item*>& items) {}
  virtual void onDropEvent(const Creature*, const vector<Item*>& items) {}
  virtual void onItemsAppeared(Vec2 position, const vector<Item*>& items) {}
  virtual void onKillEvent(const Creature* victim, const Creature* killer) {}
  virtual void onAttackEvent(const Creature* victim, const Creature* attacker) {}
  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) {}
  virtual void onExplosionEvent(const Level* level, Vec2 pos) {}
  virtual void onTriggerEvent(const Level*, Vec2 pos) {}
  virtual void onSquareReplacedEvent(const Level*, Vec2 pos) {}
  virtual void onChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {}

  static void addPickupEvent(const Creature*, const vector<Item*>& items);
  static void addDropEvent(const Creature*, const vector<Item*>& items);
  static void addItemsAppeared(const Level*, Vec2 position, const vector<Item*>& items);
  static void addKillEvent(const Creature* victim, const Creature* killer);
  static void addAttackEvent(const Creature* victim, const Creature* attacker);
  static void addThrowEvent(const Level*, const Creature* thrower, const Item* item, const vector<Vec2>& trajectory);
  static void addExplosionEvent(const Level* level, Vec2 pos);
  static void addTriggerEvent(const Level*, Vec2 pos);
  static void addSquareReplacedEvent(const Level*, Vec2 pos);
  static void addChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos);

  static void addListener(EventListener*);
  static void removeListener(EventListener*);

  virtual const Level* getListenerLevel() const { return nullptr; }

  private:
  static vector<EventListener*> listeners;
};

#endif
