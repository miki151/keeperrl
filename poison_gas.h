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

#ifndef _POISON_GAS_H
#define _POISON_GAS_H

#include "util.h"

class Level;

class PoisonGas {
  public:
  void addAmount(double amount);
  void tick(Level*, Vec2 pos);
  double getAmount() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  double SERIAL2(amount, 0);
};

#endif

