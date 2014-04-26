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

#ifndef _ENTITY_SET_H
#define _ENTITY_SET_H

#include "unique_entity.h"
#include "item.h"

class EntitySet {
  public:
  EntitySet();
  template <class Container>
  EntitySet(const Container&);
  EntitySet& operator = (const EntitySet&) = default;
  void insert(const UniqueEntity*);
  void erase(const UniqueEntity*);
  bool contains(const UniqueEntity*) const;
  void insert(UniqueId);
  void erase(UniqueId);
  bool contains(UniqueId) const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  ItemPredicate containsPredicate() const;

  typedef set<UniqueId>::const_iterator Iter;

  Iter begin() const;
  Iter end() const;

  SERIAL_CHECKER;

  private:
  set<UniqueId> SERIAL(elems);
};

#endif
