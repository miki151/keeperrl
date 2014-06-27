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


template <class Archive> 
void Fire::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(burnt)
    & SVAR(size)
    & SVAR(weight)
    & SVAR(flamability);
  CHECK_SERIAL;
}

SERIALIZABLE(Fire);

SERIALIZATION_CONSTRUCTOR_IMPL(Fire);

Fire::Fire(double objectWeight, double objectFlamability) : weight(objectWeight), flamability(objectFlamability) {}

double epsilon = 0.001;

void Fire::tick(Level* level, Vec2 position) {
  burnt = min(1., burnt + size / weight);
  size += (burnt * weight - size) / 10;
  size *= (1 - burnt);
  if (size < epsilon && burnt > 1 - epsilon) {
    size = 0;
    burnt = 1;
  }
}

void Fire::set(double amount) {
  if (!isBurntOut() && amount > epsilon)
    size = max(size, amount * flamability);
}

bool Fire::isBurning() const {
  return size > 0;
}

double Fire::getSize() const {
  return size;
}

bool Fire::isBurntOut() const {
  return burnt > 0.999 && size == 0;
}

double Fire::getFlamability() const {
  return flamability;
}
