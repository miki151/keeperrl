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

template<>
template<>
EntitySet<Item>::EntitySet(const vector<Item*>& v) {
  for (const Item* e : v)
    insert(e);
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
bool EntitySet<T>::empty() const {
  return elems.empty();
}

template <class T>
void EntitySet<T>::insert(typename UniqueEntity<T>::Id e) {
  elems.insert(e);
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
template <class Archive> 
void EntitySet<T>::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(elems);
  CHECK_SERIAL;
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
  return [this](const Item* it) { return contains(it); };
}

SERIALIZABLE_TMPL(EntitySet, Item);
SERIALIZABLE_TMPL(EntitySet, Task);
SERIALIZABLE_TMPL(EntitySet, Creature);
SERIALIZABLE_TMPL(EntitySet, Level);


