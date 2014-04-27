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

#include "event.h"
#include "creature.h"

vector<EventListener*> EventListener::listeners;

EventListener::EventListener() {
  listeners.push_back(this);
}

EventListener::~EventListener() {
  removeElement(listeners, this);
}

void EventListener::initialize() {
#ifndef RELEASE // for some reason this sometimes fails on windows
  CHECK(listeners.empty());
#endif
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

void EventListener::addItemsAppearedEvent(const Level* level, Vec2 position, const vector<Item*>& items) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level)
      l->onItemsAppearedEvent(position, items);
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
  
void EventListener::addExplosionEvent(const Level* level, Vec2 pos) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level || l->getListenerLevel() == nullptr)
      l->onExplosionEvent(level, pos);
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
  
void EventListener::addCombatEvent(const Creature* c) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == c->getLevel() || l->getListenerLevel() == nullptr)
      l->onCombatEvent(c);
}

void EventListener::addAlarmEvent(const Level* level, Vec2 pos) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == level || l->getListenerLevel() == nullptr)
      l->onAlarmEvent(level, pos);
}
  
void EventListener::addTechBookEvent(Technology* t) {
  for (EventListener* l : listeners)
    l->onTechBookEvent(t);
}
  
void EventListener::addEquipEvent(const Creature* c, const Item* it) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == c->getLevel() || l->getListenerLevel() == nullptr)
      l->onEquipEvent(c, it);
}

void EventListener::addSurrenderEvent(Creature* c, const Creature* to) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == c->getLevel() || l->getListenerLevel() == nullptr)
      l->onSurrenderEvent(c, to);
}
  
void EventListener::addTortureEvent(Creature* c, const Creature* torturer) {
  for (EventListener* l : listeners)
    if (l->getListenerLevel() == c->getLevel() || l->getListenerLevel() == nullptr)
      l->onTortureEvent(c, torturer);
}

