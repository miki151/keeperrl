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

template <typename T>
class EntitySet {
  public:
  EntitySet() {}
  template <class Container>
  EntitySet(const Container&);
  void insert(const T*);
  void erase(const T*);
  bool contains(const T*) const;
  void insert(WeakPointer<const T>);
  void erase(WeakPointer<const T>);
  bool contains(WeakPointer<const T>) const;
  bool empty() const;
  void clear();
  int getSize() const;

  void insert(typename UniqueEntity<T>::Id);
  void erase(typename UniqueEntity<T>::Id);
  bool contains(typename UniqueEntity<T>::Id) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  ItemPredicate containsPredicate() const;
  using SetType = HashSet<typename UniqueEntity<T>::Id>;
  using Iter = typename SetType::const_iterator;

  Iter begin() const;
  Iter end() const;

  size_t getHash() const {
    return combineHashIter(elems.begin(), elems.end());
  }

  private:
  SetType SERIAL(elems);
};

