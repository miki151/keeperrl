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

#include "util.h"
#include "item.h"

class RangedWeapon {
  public:
  RangedWeapon(AttrType damageAttr, const string& projectileName, ViewId projectileViewId, int maxDistance);

  void fire(WCreature c, Vec2 dir) const;
  AttrType getDamageAttr() const;
  int getMaxDistance() const;

  SERIALIZATION_DECL(RangedWeapon);

  private:
  AttrType SERIAL(damageAttr);
  string SERIAL(projectileName);
  ViewId SERIAL(projectileViewId);
  int SERIAL(maxDistance);
};
