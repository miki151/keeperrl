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

#include "view_index.h"
#include "view_object.h"


template <class Archive> 
void ViewIndex::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(objIndex)
    & SVAR(highlight)
    & SVAR(objects)
    & SVAR(anyHighlight);
  CHECK_SERIAL;
}

SERIALIZABLE(ViewIndex);

ViewIndex::ViewIndex() {
  objIndex = vector<int>(allLayers.size(), -1);
}

ViewIndex::~ViewIndex() {
}

void ViewIndex::insert(const ViewObject& obj) {
  int ind = objIndex[int(obj.layer())];
  if (ind > -1)
    objects[ind] = obj;
  else {
    objIndex[int(obj.layer())] = objects.size();
    objects.push_back(obj);
  }
}

bool ViewIndex::hasObject(ViewLayer l) const {
  return objIndex[int(l)] > -1;
}

void ViewIndex::removeObject(ViewLayer l) {
  objIndex[int(l)] = -1;
}

bool ViewIndex::isEmpty() const {
  return objects.empty() && !anyHighlight;
}

bool ViewIndex::noObjects() const {
  return objects.empty();
}

const ViewObject& ViewIndex::getObject(ViewLayer l) const {
  int ind = objIndex[int(l)];
  CHECK(ind >= 0 && ind < objects.size()) << "No object on layer " << int(l) << " " << ind;
  return objects[ind];
}

ViewObject& ViewIndex::getObject(ViewLayer l) {
  int ind = objIndex[int(l)];
  CHECK(ind > -1) << "No object on layer " << int(l);
  return objects[ind];
}

const ViewObject* ViewIndex::getTopObject(const vector<ViewLayer>& layers) const {
  for (int i = layers.size() - 1; i >= 0; --i)
    if (hasObject(layers[i]))
      return &getObject(layers[i]);
  return nullptr;
}

void ViewIndex::setHighlight(HighlightType h, double amount) {
  CHECK(amount >= 0 && amount <= 1);
  if (amount > 0)
    anyHighlight = true;
  highlight[h] = amount;
}

double ViewIndex::getHighlight(HighlightType h) const {
  return highlight[h];
}

