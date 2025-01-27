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

#include "stdafx.h"
#include "technology.h"
#include "util.h"
#include "collective.h"
#include "level.h"
#include "square.h"
#include "cost_info.h"
#include "spell.h"
#include "creature.h"
#include "creature_attributes.h"
#include "spell_map.h"
#include "construction_map.h"
#include "furniture.h"
#include "furniture_factory.h"
#include "game.h"
#include "tutorial_highlight.h"
#include "collective_config.h"
#include "creature.h"
#include "attr_type.h"


vector<TechId> Technology::getNextTechs() const {
  PROFILE;
  return getNextTechs(researched);
}

Technology::Technology(map<TechId, Technology::TechDefinition> m) : techs(std::move(m)) {}

TString Technology::getName(TechId id) const {
  return techs.at(id).name.value_or(string(id.data()));
}

vector<TechId> Technology::getNextTechs(set<TechId> from) const {
  vector<TechId> ret;
  for (auto& tech : techs)
    if (!from.count(tech.first)) {
      bool good = true;
      for (auto& pre : tech.second.prerequisites)
        if (!from.count(pre)) {
          good = false;
          break;
        }
      if (good)
        ret.push_back(tech.first);
    }
  return ret;
}

vector<TechId> Technology::getSorted() const {
  vector<TechId> ret;
  while (ret.size() < techs.size()) {
    append(ret, getNextTechs(set<TechId>(ret.begin(), ret.end())));
  }
  return ret;
}

const vector<TechId> Technology::getAllowed(const TechId& tech) const {
  vector<TechId> ret;
  for (auto& other : techs)
    if (other.second.prerequisites.contains(tech))
      ret.push_back(other.first);
  return ret;
}

#include "pretty_archive.h"

template <class Archive>
void Technology::TechDefinition::serialize(Archive& ar, const unsigned int version) {
  ar(NAMED(description), OPTION(prerequisites));
  if (version >= 1)
    ar(OPTION(name));
}

SERIALIZABLE(Technology::TechDefinition)
template void Technology::TechDefinition::serialize(PrettyInputArchive&, unsigned);
