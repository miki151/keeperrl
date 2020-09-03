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

#include "enums.h"
#include "serialization.h"

using GenericId = long long;

template<typename T>
class UniqueEntity {
  public:
  class Id {
    public:
    Id();
    Id(GenericId);
    bool operator == (const Id&) const;
    bool operator != (const Id&) const;
    bool operator < (const Id&) const;
    bool operator > (const Id&) const;
    int getHash() const;
    GenericId getGenericId() const;
    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);

    private:
    GenericId SERIAL(key);
    int SERIAL(hash);
  };
  Id getUniqueId() const;
  void setUniqueId(Id);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  static void offsetForSerialization(GenericId offset);
  static void clearOffset();

  private:
  Id SERIAL(id);
  static GenericId offset;
};

