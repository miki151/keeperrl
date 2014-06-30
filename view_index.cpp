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
    & SVAR(objects)
    & SVAR(highlight);
  CHECK_SERIAL;
}

SERIALIZABLE(ViewIndex);


template <class Archive> 
void ViewIndex::HighlightInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(amount);
}

SERIALIZABLE(ViewIndex::HighlightInfo);

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
  return objects.empty() && highlight.empty();
}

const ViewObject& ViewIndex::getObject(ViewLayer l) const {
  int ind = objIndex[int(l)];
  CHECK(ind > -1) << "No object on layer " << int(l);
  return objects[ind];
}

ViewObject& ViewIndex::getObject(ViewLayer l) {
  int ind = objIndex[int(l)];
  CHECK(ind > -1) << "No object on layer " << int(l);
  return objects[ind];
}

Optional<ViewObject> ViewIndex::getTopObject(const vector<ViewLayer>& layers) const {
  for (int i = layers.size() - 1; i >= 0; --i)
    if (hasObject(layers[i]))
      return getObject(layers[i]);
  return Nothing();
}

void ViewIndex::addHighlight(HighlightInfo info) {
  addHighlight(info.type, info.amount);
}

void ViewIndex::addHighlight(HighlightType h, double amount) {
  CHECK(amount >= 0 && amount <= 1);
  for (auto& elem : highlight)
    if (elem.type == h) {
      elem.amount = max(elem.amount, amount);
      return;
    }
  highlight.push_back({h, amount});
}

vector<ViewIndex::HighlightInfo> ViewIndex::getHighlight() const {
  return highlight;
}

