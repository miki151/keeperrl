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
#include "entity_set.h"
#include "item.h"
#include "task.h"
#include "creature.h"
#include "level.h"
#include "collective.h"

template<class T>
template<class Container>
EntitySet<T>::EntitySet(const Container& v) {
  for (auto&& e : v)
    insert(e);
}

template
EntitySet<Item>::EntitySet(const vector<WItem>&);

template
EntitySet<Creature>::EntitySet(const vector<WCreature>&);

template <class T>
bool EntitySet<T>::empty() const {
  return elems.empty();
}

template <class T>
int EntitySet<T>::getSize() const {
  return elems.size();
}

template <class T>
void EntitySet<T>::insert(const T* e) {
  elems.insert(e->getUniqueId());
}

template <class T>
void EntitySet<T>::erase(const T* e) {
  elems.erase(e->getUniqueId());
}

template <class T>
bool EntitySet<T>::contains(const T* e) const {
  return elems.count(e->getUniqueId());
}

template <class T>
void EntitySet<T>::insert(WeakPointer<const T> e) {
  elems.insert(e->getUniqueId());
}

template <class T>
void EntitySet<T>::erase(WeakPointer<const T> e) {
  elems.erase(e->getUniqueId());
}

template <class T>
bool EntitySet<T>::contains(WeakPointer<const T> e) const {
  return elems.count(e->getUniqueId());
}

template <class T>
void EntitySet<T>::insert(typename UniqueEntity<T>::Id e) {
  elems.insert(e);
}

template <class T>
void EntitySet<T>::clear() {
  elems.clear();
}

template <class T>
void EntitySet<T>::erase(typename UniqueEntity<T>::Id e) {
  elems.erase(e);
}

template <class T>
bool EntitySet<T>::contains(typename UniqueEntity<T>::Id e) const {
  return elems.count(e);
}

template <class T>
typename EntitySet<T>::Iter EntitySet<T>::begin() const {
  return elems.begin();
}

template <class T>
typename EntitySet<T>::Iter EntitySet<T>::end() const {
  return elems.end();
}

template <>
ItemPredicate EntitySet<Item>::containsPredicate() const {
  return [this](WConstItem it) { return contains(it); };
}

template <class T>
SERIALIZE_TMPL(EntitySet<T>, elems)

SERIALIZABLE_TMPL(EntitySet, Item);
SERIALIZABLE_TMPL(EntitySet, Task);
SERIALIZABLE_TMPL(EntitySet, Creature);
SERIALIZABLE_TMPL(EntitySet, Collective);
