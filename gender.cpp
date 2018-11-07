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
#include "gender.h"

const Gender Gender::male(false);
const Gender Gender::female(true);

SERIALIZATION_CONSTRUCTOR_IMPL(Gender)
SERIALIZE_DEF(Gender, fem)

Gender::Gender(bool f) : fem(f) {}

const char* Gender::his() const {
  return fem ? "her" : "his";
}

const char* Gender::him() const {
  return fem ? "her" : "him";
}

const char* Gender::he() const {
  return fem ? "she" : "he";
}

const char* Gender::god() const {
  return fem ? "goddess" : "god";
}

const char* Gender::sireOrDame() const {
  return fem ? "Dame" : "Sire";
}

const char* Gender::get(const char* male, const char* female) const {
  return fem ? female : male;
}

bool Gender::operator == (const Gender& o) const {
  return fem == o.fem;
}

bool Gender::operator != (const Gender& o) const {
  return !(*this == o);
}

