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
#include "encyclopedia.h"
#include "view.h"
#include "player_control.h"
#include "collective.h"
#include "technology.h"
#include "creature_factory.h"
#include "creature.h"
#include "spell.h"
#include "build_info.h"
#include "dungeon_level.h"
#include "spell_school.h"
#include "content_factory.h"
#include "creature_factory.h"
#include "game_info.h"
#include "creature_name.h"
#include "item_attributes.h"
#include "workshops.h"

static vector<PlayerInfo> getBestiary(ContentFactory* f) {
  vector<PlayerInfo> ret;
  for (auto& id : f->getCreatures().getAllCreatures()) {
    auto c = f->getCreatures().fromId(id, TribeId::getMonster());
    ret.push_back(PlayerInfo(c.get(), f));
    ret.back().name = c->getName().groupOf(1);
  }
  return ret;
}

static vector<SpellSchoolInfo> getSpellSchools(ContentFactory* f) {
  vector<SpellSchoolInfo> ret;
  for (auto& id : f->getCreatures().getSpellSchools())
    ret.push_back(fillSpellSchool(nullptr, id.first, f));
  return ret;
}

static vector<ItemInfo> getItems(const ContentFactory *f, const Workshops* workshops) {
  vector<ItemInfo> ret;
  auto getItemInfo = [&] (const ItemType type) {
    return ItemInfo::get(nullptr, {type.get(f).get()}, f);
  };
  for (auto& elem : f->items)
    ret.push_back(getItemInfo(ItemType(elem.first)));
  for (auto type : workshops->getWorkshopsTypes())
    for (auto& option : workshops->types.at(type).getOptions())
      if (!option.type.type->contains<CustomItemId>())
        ret.push_back(getItemInfo(option.type));
  for (auto& elem : f->itemFactory.lists)
    for (auto& item : elem.second.getAllItems())
      if (!item.type->contains<CustomItemId>())
        ret.push_back(getItemInfo(item));
  return ret;
}

Encyclopedia::Encyclopedia(ContentFactory* f)
    : spellSchools(getSpellSchools(f)), bestiary(getBestiary(f)) {
}

Encyclopedia::~Encyclopedia() {
}

void Encyclopedia::setKeeperThings(ContentFactory* f, const Technology* t, const Workshops* workshops) {
  PROFILE;
  technology = t;
  if (items.empty())
    items = getItems(f, workshops);
}


