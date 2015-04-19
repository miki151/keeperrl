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
    & SVAR(bounds)
    & SVAR(level)
    & SVAR(surprise);
}

SERIALIZABLE(Location);

Location::Location(const string& _name, const string& desc, bool sup)
    : name(_name), description(desc), bounds(-1, -1, 1, 1), surprise(sup) {
}

Location::Location(bool s) : bounds(-1, -1, 1, 1), surprise(s) {}

Location::Location(Level* l, Rectangle b) : level(l), bounds(b) {
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

Rectangle Location::getBounds() const {
  CHECK(bounds.getPX() > -1) << "Location bounds not initialized";
  return bounds;
}

void Location::setBounds(Rectangle b) {
  bounds = b;
}

void Location::setLevel(const Level* l) {
  level = l;
}

const Level* Location::getLevel() const {
  return level;
}

class TowerTopLocation : public Location {
  public:

  virtual void onCreature(Creature* c) override {
    if (!c->isPlayer())
      return;
    if (!entered.count(c) && !c->isBlind()) {
 /*     for (Vec2 v : c->getLevel()->getBounds())
        if ((v - c->getPosition()).lengthD() < 300 && !c->getLevel()->getSquare(v)->isCovered())
          c->remember(v, c->getLevel()->getSquare(v)->getViewObject());*/
      c->playerMessage("You stand at the top of a very tall stone tower.");
      c->playerMessage("You see distant land in all directions.");
      entered.insert(c);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Location) & SVAR(entered);
  }

  private:
  unordered_set<Creature*> SERIAL(entered);
};

Location* Location::towerTopLocation() {
  return new TowerTopLocation();
}

