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


EntitySet::EntitySet() {
}

template <class Container>
EntitySet::EntitySet(const Container& v) {
  for (UniqueEntity* e : v)
    insert(e);
}

template EntitySet::EntitySet(const vector<Item*>&);

void EntitySet::insert(const UniqueEntity* e) {
  insert(e->getUniqueId());
}

void EntitySet::erase(const UniqueEntity* e) {
  erase(e->getUniqueId());
}

bool EntitySet::contains(const UniqueEntity* e) const {
  return contains(e->getUniqueId());
}

void EntitySet::insert(UniqueId id) {
  elems.insert(id);
}

void EntitySet::erase(UniqueId id) {
  elems.erase(id);
}

bool EntitySet::contains(UniqueId id) const {
  return elems.count(id);
}

template <class Archive> 
void EntitySet::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(elems);
  CHECK_SERIAL;
}

SERIALIZABLE(EntitySet);

EntitySet::Iter EntitySet::begin() const {
  return elems.begin();
}

EntitySet::Iter EntitySet::end() const {
  return elems.end();
}

ItemPredicate EntitySet::containsPredicate() const {
  return [this](const Item* it) { return contains(it); };
}
