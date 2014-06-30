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
#include "creature_view.h"
#include "level.h"
#include "creature.h"

template <class Archive> 
void CreatureView::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(visibleEnemies);
  CHECK_SERIAL;
}

SERIALIZABLE(CreatureView);

void CreatureView::updateVisibleCreatures() {
  visibleEnemies.clear();
  visibleFriends.clear();
  for (const Creature* c : getLevel()->getAllCreatures()) 
    if (canSee(c)) {
      if (isEnemy(c))
        visibleEnemies.push_back(c);
      else if (c->getTribe() == getTribe())
        visibleFriends.push_back(c);
    }
  for (const Creature* c : getUnknownAttacker())
    if (!contains(visibleEnemies, c))
      visibleEnemies.push_back(c);
}

vector<const Creature*> CreatureView::getVisibleEnemies() const {
  return visibleEnemies;
}

vector<const Creature*> CreatureView::getVisibleFriends() const {
  return visibleFriends;
}

