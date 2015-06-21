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

#ifndef _SQUARE_FACTORY
#define _SQUARE_FACTORY

#include "util.h"
#include "square_type.h"

class Square;
class Deity;

enum class StairLook {
  NORMAL,
  HELL,
  CELLAR,
  PYRAMID,
  DUNGEON_ENTRANCE,
  DUNGEON_ENTRANCE_MUD,
};

class SquareFactory {
  public:
  static PSquare get(SquareType);
  static PSquare getStairs(StairDirection, StairKey, StairLook = StairLook::NORMAL);
  static PSquare getAltar(Deity*);
  static PSquare getAltar(Creature*);
  static PSquare getWater(double depth);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  private:
  static Square* getPtr(SquareType s);
};

#endif
