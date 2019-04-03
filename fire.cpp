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

#include "fire.h"

SERIALIZE_DEF(Fire, burnTime, burning)
SERIALIZATION_CONSTRUCTOR_IMPL(Fire);

Fire::Fire(int burnTime) : burnTime(burnTime) {}

constexpr double epsilon = 0.001;

void Fire::tick() {
  PROFILE_BLOCK("Fire::tick");
  if (burning && burnTime > 0)
    --burnTime;
}

void Fire::set() {
  if (!burning && burnTime > 0)
    burning = true;
}

bool Fire::isBurning() const {
  return burnTime > 0 && burning;
}

bool Fire::isBurntOut() const {
  return burnTime == 0 && burning;
}
