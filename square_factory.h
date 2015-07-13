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
class StairKey;

class SquareFactory {
  public:
  static PSquare get(SquareType);
  static PSquare getAltar(Deity*);
  static PSquare getAltar(Creature*);
  static PSquare getWater(double depth);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);
  
  SquareType getRandom();

  static SquareFactory roomFurniture(Tribe* rats);
  static SquareFactory castleFurniture(Tribe* rats);
  static SquareFactory castleOutside();
  static SquareFactory cryptCoffins(Tribe* vampire);
  static SquareFactory single(SquareType);

  private:
  static Square* getPtr(SquareType s);

  SquareFactory(const vector<SquareType>&, const vector<double>&);
  SquareFactory(const vector<SquareType>& first, const vector<SquareType>&, const vector<double>&);

  vector<SquareType> first;
  vector<SquareType> squares;
  vector<double> weights;
};

#endif
