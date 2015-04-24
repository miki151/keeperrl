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

#include "location.h"
#include "creature.h"
#include "level.h"
#include "square.h"

template <class Archive> 
void Location::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(name)
    & SVAR(description)
    & SVAR(squares)
    & SVAR(level)
    & SVAR(table)
    & SVAR(surprise)
    & SVAR(middle)
    & SVAR(bottomRight);
}

SERIALIZABLE(Location);

Location::Location(const string& _name, const string& desc, bool sup)
    : name(_name), description(desc), surprise(sup) {
}

Location::Location(bool s) : surprise(s) {}

Location::Location(Level* l, Rectangle b) : level(l), squares(b.getAllSquares()) {
  for (Vec2 v : squares)
    table[v] = true;
}

bool Location::isMarkedAsSurprise() const {
  return surprise;
}

string Location::getName() const {
  return *name;
}

string Location::getDescription() const {
  return *description;
}

bool Location::hasName() const {
  return !!name;
}

const vector<Vec2>& Location::getAllSquares() const {
  CHECK(level) << "Location bounds not initialized";
  return squares;
}

bool Location::contains(Vec2 pos) const {
  return table[pos];
}

Vec2 Location::getMiddle() const {
  return middle;
}

Vec2 Location::getBottomRight() const {
  return bottomRight;
}

void Location::setBounds(Rectangle b) {
  squares = b.getAllSquares();
  for (Vec2 v : squares)
    table[v] = true;
  middle = b.middle();
  bottomRight = b.getBottomRight();
}

void Location::setLevel(const Level* l) {
  level = l;
}

const Level* Location::getLevel() const {
  return level;
}

