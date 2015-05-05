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
#include "skill.h"

void Technology::init() {
  Technology::set(TechId::ALCHEMY, new Technology(
        "alchemy", "Build a laboratory and produce basic potions.", 80));
  Technology::set(TechId::ALCHEMY_ADV, new Technology(
        "advanced alchemy", "Produce more powerful potions.", 200, {TechId::ALCHEMY}));
  Technology::set(TechId::HUMANOID_MUT, new Technology(
        "humanoid mutation", "Breed new, very powerful humanoid species.", 400,{}));
  Technology::set(TechId::BEAST_MUT, new Technology(
        "beast mutation", "Breed new, very powerful beast species.", 400, {}));
  Technology::set(TechId::CRAFTING, new Technology(
        "crafting", "Build a workshop and produce basic equipment.", 40, {}));
  Technology::set(TechId::PIGSTY, new Technology(
        "pig breeding", "Build a pigsty to feed your minions.", 120, {}));
  Technology::set(TechId::IRON_WORKING, new Technology(
        "iron working", "Build a forge and produce metal weapons and armor.", 60, {TechId::CRAFTING}));
  Technology::set(TechId::JEWELLERY, new Technology(
        "jewellery", "Build a jeweler room and produce magical rings and amulets.", 200, {TechId::IRON_WORKING}));
  Technology::set(TechId::TWO_H_WEAP, new Technology(
        "two-handed weapons", "Produce war hammers and battle axes.", 100, {TechId::IRON_WORKING}, true));
  Technology::set(TechId::TRAPS, new Technology(
        "traps", "Produce traps in the workshop.", 100, {TechId::CRAFTING}));
  Technology::set(TechId::ARCHERY, new Technology(
        "archery", "Produce bows and arrows.", 100, {TechId::CRAFTING}, true));
  Technology::set(TechId::SPELLS, new Technology(
        "sorcery", "Learn basic spells.", 60, {}));
  Technology::set(TechId::SPELLS_ADV, new Technology(
        "advanced sorcery", "Learn more advanced spells.", 120, {TechId::SPELLS}));
  Technology::set(TechId::SPELLS_MAS, new Technology(
        "master sorcery", "Learn the most powerful spells.", 350, {TechId::SPELLS_ADV}));
}

bool Technology::canResearch() const {
  return research;
}

int Technology::getCost() const {
  return cost;
}

vector<Technology*> Technology::getNextTechs(const vector<Technology*>& current) {
  vector<Technology*> ret;
  for (Technology* t : Technology::getAll())
    if (t->canLearnFrom(current) && !contains(current, t))
      ret.push_back(t);
  return ret;
}

Technology::Technology(const string& n, const string& d, int c, const vector<TechId>& pre, bool canR)
    : name(n), description(d), cost(c), research(canR) {
  for (TechId id : pre)
    prerequisites.push_back(Technology::get(id));
}

bool Technology::canLearnFrom(const vector<Technology*>& techs) const {
  vector<Technology*> myPre = prerequisites;
  for (Technology* t : techs)
    removeElementMaybe(myPre, t);
  return myPre.empty();
}

const string& Technology::getName() const {
  return name;
}

const string& Technology::getDescription() const {
  return description;
}

vector<Technology*> Technology::getSorted() {
  vector<Technology*> ret;
  while (ret.size() < getAll().size()) {
    append(ret, getNextTechs(ret));
  }
  return ret;
}

const vector<Technology*> Technology::getPrerequisites() const {
  return prerequisites;
}
  
const vector<Technology*> Technology::getAllowed() const {
  vector<Technology*> ret;
  for (Technology* t : getAll())
    if (contains(t->prerequisites, this))
      ret.push_back(t);
  return ret;
}
