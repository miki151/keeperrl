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

#include "view_object.h"

class ViewIndex {
  public:
  ViewIndex();
  void insert(const ViewObject& obj);
  bool hasObject(ViewLayer) const;
  void removeObject(ViewLayer);
  const ViewObject& getObject(ViewLayer) const;
  ViewObject& getObject(ViewLayer);
  Optional<ViewObject> getTopObject(const vector<ViewLayer>&) const;
  bool isEmpty() const;

  void addHighlight(HighlightType, double amount = 1);

  struct HighlightInfo {
    HighlightType type;
    double amount;

    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };

  vector<HighlightInfo> getHighlight() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;
  private:
  vector<int> SERIAL(objIndex);
  vector<ViewObject> SERIAL(objects);
  vector<HighlightInfo> SERIAL(highlight);
};

#endif
