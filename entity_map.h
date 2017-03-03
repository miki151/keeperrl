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

#pragma once

#include "unique_entity.h"
#include "util.h"

template <typename Key, typename Value>
class EntityMap {
  public:
  using EntityId = typename UniqueEntity<Key>::Id;
  EntityMap();
  EntityMap& operator = (const EntityMap&) = default;
  void set(const Key*, const Value&);
  void erase(const Key*);
  const Value& getOrFail(const Key*) const;
  Value& getOrFail(const Key*);
  Value& getOrInit(const Key*);
  optional<Value> getMaybe(const Key*) const;
  const Value& getOrElse(const Key*, const Value&) const;
  bool empty() const;
  void clear();
  int getSize() const;
  vector<EntityId> getKeys() const;

  void set(EntityId, const Value&);
  void erase(EntityId);
  const Value& getOrFail(EntityId) const;
  Value& getOrFail(EntityId);
  Value& getOrInit(EntityId);
  optional<Value> getMaybe(EntityId) const;
  const Value& getOrElse(EntityId, const Value&) const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  typedef typename map<EntityId, Value>::const_iterator Iter;

  Iter begin() const;
  Iter end() const;

  private:
  map<EntityId, Value> SERIAL(elems);
};

