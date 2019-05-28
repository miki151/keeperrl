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

#include "item_factory.h"
#include "creature_factory.h"
#include "lasting_effect.h"
#include "effect.h"
#include "item_type.h"
#include "item_list.h"
#include "game_config.h"

SERIALIZE_DEF(ItemFactory, lists)
SERIALIZATION_CONSTRUCTOR_IMPL(ItemFactory)

ItemFactory::ItemFactory(map<ItemListId, ItemList> lists) : lists(std::move(lists)) {
}

ItemList ItemFactory::get(ItemListId id) {
  return lists.at(id);
}
