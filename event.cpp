#include "stdafx.h"

vector<EventListener*> EventListener::listeners;

void EventListener::addListener(EventListener* l) {
  listeners.push_back(l);
}

void EventListener::removeListener(EventListener* l) {
  removeElement(listeners, l);
}

void EventListener::addPickupEvent(const Creature* c, const vector<Item*>& items) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == c->getLevel() || l->getListenerLevel() == nullptr)
      l->onPickupEvent(c, items);
}

void EventListener::addDropEvent(const Creature* c, const vector<Item*>& items) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == c->getLevel() || l->getListenerLevel() == nullptr)
      l->onDropEvent(c, items);
}

void EventListener::addItemsAppeared(const Level* level, Vec2 position, const vector<Item*>& items) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level)
      l->onItemsAppeared(position, items);
}

void EventListener::addKillEvent(const Creature* victim, const Creature* killer) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == victim->getLevel() || l->getListenerLevel() == nullptr)
      l->onKillEvent(victim, killer);
}
  
void EventListener::addAttackEvent(const Creature* victim, const Creature* attacker) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == victim->getLevel() || l->getListenerLevel() == nullptr)
      l->onAttackEvent(victim, attacker);
}

void EventListener::addThrowEvent(const Level* level, const Creature* thrower,
    const Item* item, const vector<Vec2>& trajectory) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level || l->getListenerLevel() == nullptr)
      l->onThrowEvent(thrower, item, trajectory);
}
  
void EventListener::addTriggerEvent(const Level* level, Vec2 pos) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level || l->getListenerLevel() == nullptr)
      l->onTriggerEvent(level, pos);
}

void EventListener::addSquareReplacedEvent(const Level* level, Vec2 pos) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level || l->getListenerLevel() == nullptr)
      l->onSquareReplacedEvent(level, pos);
}
  
void EventListener::addChangeLevelEvent(const Creature* c, const Level* level, Vec2 pos,
    const Level* to, Vec2 toPos) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level || l->getListenerLevel() == nullptr)
      l->onChangeLevelEvent(c, level, pos, to, toPos);
}
