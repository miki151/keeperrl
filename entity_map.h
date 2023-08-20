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

  bool empty() const;
  void clear();
  int getSize() const;
  vector<EntityId> getKeys() const;

  void set(const Key*, const Value&);
  void erase(const Key*);
  const Value& getOrFail(const Key*) const;
  Value& getOrFail(const Key*);
  Value& getOrInit(const Key*);
  optional<Value> getMaybe(const Key*) const;
  const Value& getOrElse(const Key*, const Value&) const;
  bool hasKey(const Key*) const;

  void set(WeakPointer<const Key>, const Value&);
  void erase(WeakPointer<const Key>);
  const Value& getOrFail(WeakPointer<const Key>) const;
  Value& getOrFail(WeakPointer<const Key>);
  Value& getOrInit(WeakPointer<const Key>);
  optional<Value> getMaybe(WeakPointer<const Key>) const;
  const Value& getOrElse(WeakPointer<const Key>, const Value&) const;
  bool hasKey(WeakPointer<const Key>) const;

  void set(EntityId, const Value&);
  void erase(EntityId);
  const Value& getOrFail(EntityId) const;
  Value& getOrFail(EntityId);
  Value& getOrInit(EntityId);
  optional<Value> getMaybe(EntityId) const;
  const Value& getOrElse(EntityId, const Value&) const;
  bool hasKey(EntityId) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  using Iter = typename HashMap<EntityId, Value>::const_iterator;
  ~EntityMap();

  Iter begin() const;
  Iter end() const;

  private:
  HashMap<EntityId, Value> SERIAL(elems);
};

