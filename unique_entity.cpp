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

static int idCounter = 1;

template<typename T>
UniqueEntity<T>::UniqueEntity() {
  id = ++idCounter;
}

template<typename T>
auto UniqueEntity<T>::getUniqueId() const -> Id {
  return id;
}

template<typename T>
template <class Archive> 
void UniqueEntity<T>::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(id);
  if (id > idCounter)
    idCounter = id;
}

SERIALIZABLE_TMPL(UniqueEntity, Level);
SERIALIZABLE_TMPL(UniqueEntity, Item);
SERIALIZABLE_TMPL(UniqueEntity, Creature);
SERIALIZABLE_TMPL(UniqueEntity, Task);
SERIALIZABLE_TMPL(UniqueEntity, PlayerMessage);
