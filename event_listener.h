#pragma once

#include "util.h"
#include "event_generator.h"
#include "position.h"

class Model;

class EventListener {
  public:
  typedef EventGenerator<EventListener> Generator;

  EventListener();
  EventListener(Model*);
  ~EventListener();

  void subscribeTo(Model*);
  void unsubscribe();

  virtual void onMovedEvent(Creature*) {}
  virtual void onKilledEvent(Creature* victim, Creature* attacker) {}
  virtual void onPickedUpEvent(Creature*, const vector<Item*>& items) {}
  virtual void onDroppedEvent(Creature*, const vector<Item*>& items) {}
  virtual void onItemsAppearedEvent(Position, const vector<Item*>& items) {}
  virtual void onThrownEvent(Level*, const vector<Item*>&, const vector<Vec2>& trajectory) {}
  virtual void onExplosionEvent(Position) {}
  virtual void onWonGameEvent() {}

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  Generator* SERIAL(generator) = nullptr;
};
