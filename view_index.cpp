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

SERIALIZE_DEF(ViewIndex, objIndex, highlights, tileGas, objects, anyHighlight, itemCounts, nightAmount)

ViewIndex::ViewIndex() {
  for (auto& elem : objIndex)
    elem = 100;
}

ViewIndex::~ViewIndex() {
}

void ViewIndex::insert(ViewObject obj) {
  PROFILE;
  int ind = objIndex[int(obj.layer())];
  if (ind < 100)
    objects[ind] = std::move(obj);
  else {
    objIndex[int(obj.layer())] = objects.size();
    objects.push_back(std::move(obj));
  }
}

bool ViewIndex::hasObject(ViewLayer l) const {
  return objIndex[int(l)] < 100;
}

void ViewIndex::removeObject(ViewLayer l) {
  objIndex[int(l)] = 100;
}

bool ViewIndex::isEmpty() const {
  return objects.empty() && !anyHighlight;
}

bool ViewIndex::hasAnyHighlight() const {
  return anyHighlight || !tileGas.empty();
}

bool ViewIndex::noObjects() const {
  PROFILE;
  return objects.empty();
}

const ViewObject& ViewIndex::getObject(ViewLayer l) const {
  int ind = objIndex[int(l)];
  CHECK(ind >= 0 && ind < objects.size()) << "No object on layer " << int(l) << " " << ind;
  return objects[ind];
}

ViewObject& ViewIndex::getObject(ViewLayer l) {
  int ind = objIndex[int(l)];
  CHECK(ind < 100) << "No object on layer " << int(l);
  return objects[ind];
}

const ViewObject* ViewIndex::getTopObject(const vector<ViewLayer>& layers) const {
  for (int i = layers.size() - 1; i >= 0; --i)
    if (hasObject(layers[i]))
      return &getObject(layers[i]);
  return nullptr;
}
void ViewIndex::setNightAmount(double amount) {
  nightAmount = (std::uint8_t) trunc(amount * 255);
}

double ViewIndex::getNightAmount() const {
  return double(nightAmount) / 255.0;
}

void ViewIndex::addGasAmount(Color color) {
  tileGas.push_back(color);
}

const vector<Color>& ViewIndex::getGasAmounts() const {
  return tileGas;
}

const static ItemCounts emptyCounts;

const ItemCounts& ViewIndex::getItemCounts() const {
  if (itemCounts)
    return itemCounts->first;
  else
    return emptyCounts;
}

const ItemCounts& ViewIndex::getEquipmentCounts() const {
  if (itemCounts)
    return itemCounts->second;
  else
    return emptyCounts;
}

ItemCounts& ViewIndex::modItemCounts() {
  if (!itemCounts)
    itemCounts.reset(make_pair(ItemCounts(), ItemCounts()));
  return itemCounts->first;
}

ItemCounts& ViewIndex::modEquipmentCounts() {
  if (!itemCounts)
    itemCounts.reset(make_pair(ItemCounts(), ItemCounts()));
  return itemCounts->second;
}

void ViewIndex::setHighlight(HighlightType h, bool state) {
  if (state)
    anyHighlight = true;
  highlights.set(h, state);
}

bool ViewIndex::isHighlight(HighlightType h) const {
  return highlights.contains(h);
}

optional<ViewId> ViewIndex::getHiddenId() const {
  return hiddenId;
}

void ViewIndex::setHiddenId(ViewId id) {
  hiddenId = id;
}

vector<ViewObject>& ViewIndex::getAllObjects() {
  return objects;
}

const vector<ViewObject>& ViewIndex::getAllObjects() const {
  return objects;
}

void ViewIndex::mergeFromMemory(const ViewIndex& memory) {
  if (isEmpty())
    *this = memory;
  else if (!hasObject(ViewLayer::FLOOR) && !hasObject(ViewLayer::FLOOR_BACKGROUND) && !isEmpty()) {
    // special case when monster or item is visible but floor is only in memory
    if (memory.hasObject(ViewLayer::FLOOR))
      insert(memory.getObject(ViewLayer::FLOOR));
    if (memory.hasObject(ViewLayer::FLOOR_BACKGROUND))
      insert(memory.getObject(ViewLayer::FLOOR_BACKGROUND));
  }
}
