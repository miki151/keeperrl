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

#ifndef _VIEW_INDEX
#define _VIEW_INDEX

#include "enums.h"
#include "util.h"

class ViewObject;

RICH_ENUM(HighlightType,
  BUILD,
  RECT_SELECTION,
  RECT_DESELECTION,
  POISON_GAS,
  FOG,
  MEMORY,
  NIGHT,
  EFFICIENCY,
  PRIORITY_TASK
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
  ~ViewIndex();
  // If the tile is not visible, we still need the id of the floor tile to render connections properly.
  optional<ViewId> getHiddenId() const;
  void setHiddenId(ViewId);

  void setHighlight(HighlightType, double amount = 1);

  double getHighlight(HighlightType) const;
  const vector<HighlightType>& getHighlights() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;

  private:
  vector<int> SERIAL(objIndex);
  vector<ViewObject> SERIAL(objects);
  EnumMap<HighlightType, double> SERIAL(highlight);
  bool SERIAL2(anyHighlight, false);
  optional<ViewId> hiddenId;
};

#endif
