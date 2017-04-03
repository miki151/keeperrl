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

#include "util.h"

class Square;
class SquareArray;

class FieldOfView {
  public:
  FieldOfView(WLevel, VisionId);
  bool canSee(Vec2 from, Vec2 to);
  const vector<Vec2>& getVisibleTiles(Vec2 from);
  void squareChanged(Vec2 pos);

  SERIALIZATION_DECL(FieldOfView);

  const static int sightRange = 30;

  private:


  class Visibility {
    public:

    bool checkVisible(int x,int y) const;
    const vector<Vec2>& getVisibleTiles() const;

    Visibility(WLevel, VisionId, int x, int y);
    Visibility(Visibility&&) = default;
    Visibility& operator = (Visibility&&) = default;

    SERIALIZATION_DECL(Visibility);

    private:
    char SERIAL(visible)[sightRange * 2 + 1][sightRange * 2 + 1];
    vector<Vec2> SERIAL(visibleTiles);
    void calculate(int,int,int,int, int, int, int, int,
        function<bool (int, int)> isBlocking,
        function<void (int, int)> setVisible);
    void setVisible(WConstLevel, int, int);

    int SERIAL(px);
    int SERIAL(py);
  };
  
  WLevel SERIAL(level);
  Table<unique_ptr<Visibility>> SERIAL(visibility);
  VisionId SERIAL(vision);
};

