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

#ifndef _ENCYCLOPEDIA_H
#define _ENCYCLOPEDIA_H

#include "collective.h"

class View;
class Technology;
class Deity;

class Encyclopedia {
  public:
  void present(View*, int lastInd = 0);

  private:
  void bestiary(View*, int lastInd = 0);
  void advances(View*, int lastInd = 0);
  void skills(View*, int lastInd = 0);
  void advance(View*, const Technology* tech);
  void rooms(View*, int lastInd = 0);
  void room(View*, Collective::RoomInfo& info);
  void deity(View*, const Deity*);
  void skill(View*, const Skill*);
  void deities(View*, int lastInd = 0);
  void tribes(View*, int lastInd = 0);
  void workshop(View*, int lastInd = 0);
};

#endif
