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

#ifndef _NAME_GENERATOR
#define _NAME_GENERATOR

#include "singleton.h"

RICH_ENUM(NameGeneratorId,
  FIRST,
  SCROLL,
  AZTEC,
  CREATURE,
  WEAPON,
  WORLD,
  TOWN,
  DWARF,
  DEITY,
  DEMON,
  DOG,
  INSULTS,
  DRAGON,
);

class NameGenerator : public Singleton<NameGenerator, NameGeneratorId> {
  public:
  NameGenerator() = default;
  string getNext();

  static void init();

  private:
  NameGenerator(vector<string> names, bool oneName = false);
  queue<string> names;
  bool oneName;
};

#endif
