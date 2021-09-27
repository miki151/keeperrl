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
#include "unique_entity.h"
#include "level.h"
#include "item.h"
#include "creature.h"
#include "task.h"
#include "player_message.h"
#include "furniture.h"

template<typename T>
void UniqueEntity<T>::offsetForSerialization(GenericId o) {
  CHECK(o != 0);
  offset = o;
}

template<typename T>
void UniqueEntity<T>::clearOffset() {
  offset = 0;
}

template<typename T>
long long UniqueEntity<T>::offset = 0;

template<typename T>
UniqueEntity<T>::Id::Id() {
  key = Random.getLL();
  hash = int(key);
}

template<typename T>
UniqueEntity<T>::Id::Id(GenericId id) {
  key = id;
  hash = int(key);
}

template<typename T>
bool UniqueEntity<T>::Id::operator == (const Id& id) const {
  return key == id.key;
}

template<typename T>
bool UniqueEntity<T>::Id::operator < (const Id& id) const {
  return key < id.key;
}

template<typename T>
bool UniqueEntity<T>::Id::operator > (const Id& id) const {
  return key > id.key;
}

template<typename T>
bool UniqueEntity<T>::Id::operator != (const Id& id) const {
  return !(*this == id);
}

template<typename T>
int UniqueEntity<T>::Id::getHash() const {
  return hash;
}

template<typename T>
template <class Archive> 
void UniqueEntity<T>::Id::serialize(Archive& ar, const unsigned int version) {
  if (key != 0)
    key += offset;
  ar(key, hash);
  if (key != 0)
    key -= offset;
}

template<typename T>
auto UniqueEntity<T>::getUniqueId() const -> Id {
  return id;
}

template<typename T>
void UniqueEntity<T>::setUniqueId(UniqueEntity::Id i) {
  id = i;
}

template<typename T>
GenericId UniqueEntity<T>::Id::getGenericId() const {
  if (hash == 0) // this workaround can be removed in alpha 34
    return 0;
  return key;
}

template<typename T>
template <class Archive> 
void UniqueEntity<T>::serialize(Archive& ar, const unsigned int version) {
  ar(id);
}

SERIALIZABLE_TMPL(UniqueEntity, Level);
SERIALIZABLE_TMPL(UniqueEntity, Item);
SERIALIZABLE_TMPL(UniqueEntity, Creature);
SERIALIZABLE_TMPL(UniqueEntity, Task);
SERIALIZABLE_TMPL(UniqueEntity, PlayerMessage);
SERIALIZABLE_TMPL(UniqueEntity, Furniture);
SERIALIZABLE_TMPL(UniqueEntity, Collective);

SERIALIZABLE(UniqueEntity<Creature>::Id);
SERIALIZABLE(UniqueEntity<Item>::Id);
SERIALIZABLE(UniqueEntity<PlayerMessage>::Id);
SERIALIZABLE(UniqueEntity<Task>::Id);
SERIALIZABLE(UniqueEntity<Furniture>::Id);
SERIALIZABLE(UniqueEntity<Collective>::Id);
