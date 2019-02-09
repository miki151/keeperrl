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
#include "enums.h"

using TechId = string;

class Technology {
  public:
  vector<TechId> getNextTechs(set<TechId> from) const;
  vector<TechId> getNextTechs() const;
  const vector<TechId> getAllowed(const TechId&) const;
  vector<TechId> getSorted() const;

  struct TechDefinition {
    string SERIAL(description);
    vector<TechId> SERIAL(prerequisites);
    SERIALIZE_ALL(NAMED(description), OPTION(prerequisites));
  };
  map<TechId, TechDefinition> SERIAL(techs);
  set<TechId> SERIAL(researched);
  SERIALIZE_ALL(NAMED(techs), OPTION(researched));
};

