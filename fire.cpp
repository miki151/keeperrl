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

SERIALIZE_DEF(Fire, burnTime, SKIP(burnState))
SERIALIZATION_CONSTRUCTOR_IMPL(Fire);

Fire::Fire(int burnTime) : burnTime(burnTime) {}

void Fire::tick() {
  PROFILE_BLOCK("Fire::tick");
  if (burnState && *burnState < burnTime)
    ++*burnState;
}

void Fire::set() {
  if (!burnState && burnTime > 0)
    burnState = 0;
}

bool Fire::isBurning() const {
  return burnState && *burnState < burnTime;
}

bool Fire::isBurntOut() const {
  return burnTime == burnState;
}

int Fire::getBurnState() const {
    return min(*burnState, burnTime - *burnState);
}

#include "pretty_archive.h"
template void Fire::serialize(PrettyInputArchive&, unsigned);
