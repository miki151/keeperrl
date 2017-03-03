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

Location::Location(const string& _name)
    : name(_name), table(Level::getMaxBounds(), false) {
}

Location::Location() : table(Level::getMaxBounds(), false) {}

Location::Location(Level* l, Rectangle b) : level(l), squares(b.getAllSquares()),
    table(Level::getMaxBounds(), false) {
  for (Vec2 v : squares)
    table[v] = true;
}

void Location::setSurprise() {
  surprise = true;
}

bool Location::isMarkedAsSurprise() const {
  return surprise;
}

const optional<string>& Location::getName() const {
  return name;
}

vector<Position> Location::getAllSquares() const {
  CHECK(level) << "Location bounds not initialized";
  return transform2(squares, [this] (Vec2 v) { return Position(v, level); });
}

bool Location::contains(Position pos) const {
  return pos.isSameLevel(level) && table[pos.getCoord()];
}

Position Location::getMiddle() const {
  return Position(middle, level);
}

Position Location::getBottomRight() const {
  return Position(bottomRight, level);
}

void Location::setBounds(Rectangle b) {
  squares = b.getAllSquares();
  for (Vec2 v : squares)
    table[v] = true;
  middle = b.middle();
  bottomRight = b.bottomRight();
}

void Location::setLevel(Level* l) {
  level = l;
}

const Level* Location::getLevel() const {
  return level;
}

