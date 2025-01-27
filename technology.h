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
#include "tech_id.h"
#include "t_string.h"

class Technology {
  public:
  struct TechDefinition {
    string SERIAL(description);
    vector<TechId> SERIAL(prerequisites);
    optional<TString> SERIAL(name);
    template <class Archive>
    void serialize(Archive&, const unsigned int);
  };
  Technology(map<TechId, TechDefinition>);
  Technology() {}
  vector<TechId> getNextTechs(set<TechId> from) const;
  vector<TechId> getNextTechs() const;
  const vector<TechId> getAllowed(const TechId&) const;
  vector<TechId> getSorted() const;
  TString getName(TechId) const;
  map<TechId, TechDefinition> SERIAL(techs);
  set<TechId> SERIAL(researched);
  SERIALIZE_ALL(NAMED(techs), OPTION(researched))
};

CEREAL_CLASS_VERSION(Technology::TechDefinition, 1)