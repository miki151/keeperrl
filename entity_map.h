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

#ifndef _ENTITY_MAP_H
#define _ENTITY_MAP_H

#include "unique_entity.h"
#include "util.h"

template <typename Key, typename Value>
class EntityMap {
  public:
  EntityMap();
  EntityMap& operator = (const EntityMap&) = default;
  void set(const Key*, const Value&);
  void erase(const Key*);
  const Value& get(const Key*) const;
  Value& get(const Key*);
  optional<Value> getMaybe(const Key*) const;
  bool empty() const;
  void clear();
  int getSize() const;

  void set(typename UniqueEntity<Key>::Id, const Value&);
  void erase(typename UniqueEntity<Key>::Id);
  const Value& get(typename UniqueEntity<Key>::Id) const;
  Value& get(typename UniqueEntity<Key>::Id);
  optional<Value> getMaybe(typename UniqueEntity<Key>::Id) const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  map<typename UniqueEntity<Key>::Id, Value> SERIAL(elems);
};

#endif
