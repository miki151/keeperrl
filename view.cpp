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

#include "view.h"
#include "creature.h"


View::ListElem::ListElem(const string& t, ElemMod m, Optional<UserInput::Type> a) : text(t), mod(m), action(a) {
}

View::ListElem::ListElem(const char* s, ElemMod m, Optional<UserInput::Type> a) : text(s), mod(m), action(a) {
}

const string& View::ListElem::getText() const {
  return text;
}

View::ElemMod View::ListElem::getMod() const {
  return mod;
}

Optional<UserInput::Type> View::ListElem::getAction() const {
  return action;
}

vector<View::ListElem> View::getListElem(const vector<string>& v) {
  function<ListElem(const string&)> fun = [](const string& s) -> ListElem { return ListElem(s); };
  return transform2<ListElem>(v, fun);
}

void View::setJukebox(Jukebox* j) {
  jukebox = j;
}

Jukebox* View::getJukebox() {
  return NOTNULL(jukebox);
}

View::View() {

}

View::~View() {
}

GameInfo::CreatureInfo::CreatureInfo(const Creature* c) 
    : viewObject(c->getViewObject()),
      uniqueId(c->getUniqueId()),
      name(c->getName()),
      speciesName(c->getSpeciesName()),
      expLevel(c->getExpLevel()) {
}
