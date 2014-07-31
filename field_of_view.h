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

#ifndef _FIELD_OF_VIEW_H
#define _FIELD_OF_VIEW_H

#include "util.h"

class Square;
class Vision;

class FieldOfView {
  public:
  FieldOfView(const Table<PSquare>& squares, Vision*);
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

    Visibility(const Table<PSquare>& squares, Vision*, int x, int y);
    Visibility(Visibility&&) = default;
    Visibility& operator = (Visibility&&) = default;

    SERIALIZATION_DECL(Visibility);

    private:
    char visible[sightRange * 2 + 1][sightRange * 2 + 1];
    SERIAL3(visible);
    vector<Vec2> SERIAL(visibleTiles);
    void calculate(int,int,int,int, int, int, int, int,
        function<bool (int, int)> isBlocking,
        function<void (int, int)> setVisible);
    void setVisible(int, int);

    int SERIAL(px);
    int SERIAL(py);
  };
  
  const Table<PSquare>* SERIAL(squares);
  Table<Optional<Visibility>> SERIAL(visibility);
  Vision* SERIAL(vision);
};

#endif
