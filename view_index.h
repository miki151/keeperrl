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

#pragma once

#include "enums.h"
#include "util.h"
#include "view_layer.h"
#include "item_counts.h"

class ViewObject;

RICH_ENUM(HighlightType,
  DIG,
  CUT_TREE,
  FETCH_ITEMS,
  PERMANENT_FETCH_ITEMS,
  RECT_SELECTION,
  RECT_DESELECTION,
  POISON_GAS,
  FOG,
  MEMORY,
  NIGHT,
  EFFICIENCY,
  PRIORITY_TASK,
  FORBIDDEN_ZONE,
  UNAVAILABLE,
  CREATURE_DROP,
  CLICKABLE_FURNITURE,
  CLICKED_FURNITURE,
  STORAGE_EQUIPMENT,
  STORAGE_RESOURCES,
  QUARTERS1,
  QUARTERS2,
  QUARTERS3
);

class ViewIndex {
  public:
  ViewIndex();
  void insert(const ViewObject& obj);
  bool hasObject(ViewLayer) const;
  void removeObject(ViewLayer);
  const ViewObject& getObject(ViewLayer) const;
  ViewObject& getObject(ViewLayer);
  const ViewObject* getTopObject(const vector<ViewLayer>&) const;
  void mergeFromMemory(const ViewIndex& memory);
  bool isEmpty() const;
  bool noObjects() const;
  bool hasAnyHighlight() const;
  ~ViewIndex();
  // If the tile is not visible, we still need the id of the floor tile to render connections properly.
  optional<ViewId> getHiddenId() const;
  void setHiddenId(ViewId);
  vector<ViewObject>& getAllObjects();

  void setHighlight(HighlightType, double amount = 1);

  double getHighlight(HighlightType) const;
  const EnumMap<HighlightType, double>& getHighlightMap() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  ItemCounts SERIAL(itemCounts);
  ItemCounts SERIAL(equipmentCounts);

  private:
  std::array<char, EnumInfo<ViewLayer>::size> SERIAL(objIndex);
  vector<ViewObject> SERIAL(objects);
  EnumMap<HighlightType, double> SERIAL(highlight);
  bool SERIAL(anyHighlight) = false;
  optional<ViewId> hiddenId;
};
