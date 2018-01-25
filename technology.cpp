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

void Technology::init() {
  Technology::set(TechId::ALCHEMY, new Technology(
        "alchemy", "Build a laboratory and produce basic potions.", 80));
  Technology::set(TechId::ALCHEMY_ADV, new Technology(
        "advanced alchemy", "Produce more powerful potions.", 200, {TechId::ALCHEMY}));
  Technology::set(TechId::ALCHEMY_CONV, new Technology(
        "alchemical conversion", "Convert resources to and from gold.", 100, {TechId::ALCHEMY}));
  Technology::set(TechId::HUMANOID_MUT, new Technology(
        "humanoid mutation", "Breed new, very powerful humanoid species.", 400, {TechId::ALCHEMY}));
  Technology::set(TechId::BEAST_MUT, new Technology(
        "beast mutation", "Breed new, very powerful beast species.", 400, {TechId::ALCHEMY}));
  Technology::set(TechId::PIGSTY, new Technology(
        "pig breeding", "Build a pigsty to feed your minions.", 120, {}));
  Technology::set(TechId::IRON_WORKING, new Technology(
        "iron working", "Build a forge and produce metal weapons and armor.", 180));
  Technology::set(TechId::JEWELLERY, new Technology(
        "jewellery", "Build a jeweler room and produce magical rings and amulets.", 200, {TechId::IRON_WORKING}));
  Technology::set(TechId::TWO_H_WEAP, new Technology(
        "two-handed weapons", "Produce war hammers and battle axes.", 100, {TechId::IRON_WORKING}));
  Technology::set(TechId::TRAPS, new Technology(
        "traps", "Produce traps in the workshop.", 100));
  Technology::set(TechId::ARCHERY, new Technology(
        "archery", "Produce bows and arrows.", 100));
  Technology::set(TechId::SPELLS, new Technology(
        "sorcery", "Learn basic spells.", 60, {}));
  Technology::set(TechId::SPELLS_ADV, new Technology(
        "advanced sorcery", "Learn more advanced spells.", 120, {TechId::SPELLS}));
  Technology::set(TechId::MAGICAL_WEAPONS, new Technology(
        "magical weapons", "Produce melee weapons that deal "_s + ::getName(AttrType::SPELL_DAMAGE), 120, {TechId::SPELLS_ADV}));
  Technology::set(TechId::SPELLS_MAS, new Technology(
        "master sorcery", "Learn the most powerful spells.", 350, {TechId::SPELLS_ADV}));
  Technology::set(TechId::DEMONOLOGY, new Technology(
        "demonology", "Build demon shrines to summon demons.", 350, {TechId::SPELLS_ADV}));
}

bool Technology::canResearch() const {
  return research;
}

Technology* Technology::setTutorialHighlight(TutorialHighlight h) {
  tutorial = h;
  return this;
}

constexpr auto resource = CollectiveResourceId::MANA;

CostInfo Technology::getAvailableResource(WConstCollective col) {
  return CostInfo(resource, col->numResource(resource));
}

CostInfo Technology::getCost() const {
  return CostInfo(resource, cost);
}

vector<Technology*> Technology::getNextTechs(const vector<Technology*>& current) {
  vector<Technology*> ret;
  for (Technology* t : Technology::getAll())
    if (t->canLearnFrom(current) && !current.contains(t))
      ret.push_back(t);
  return ret;
}

Technology::Technology(const string& n, const string& d, int c, const vector<TechId>& pre, bool canR)
    : name(n), description(d), cost(100), research(canR) {
  for (TechId id : pre)
    prerequisites.push_back(Technology::get(id));
}

bool Technology::canLearnFrom(const vector<Technology*>& techs) const {
  vector<Technology*> myPre = prerequisites;
  for (Technology* t : techs)
    myPre.removeElementMaybe(t);
  return myPre.empty();
}

const string& Technology::getName() const {
  return name;
}

const string& Technology::getDescription() const {
  return description;
}

const optional<TutorialHighlight> Technology::getTutorialHighlight() const {
  return tutorial;
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
    if (t->prerequisites.contains(this))
      ret.push_back(t);
  return ret;
}

void Technology::onAcquired(TechId id, WCollective col) {
  switch (id) {
    default: break;
  } 
}
