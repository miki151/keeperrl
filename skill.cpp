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

#include "skill.h"
#include "enums.h"
#include "creature.h"
#include "attr_type.h"
#include "creature_attributes.h"

string Skill::getName() const {
  return name;
}

string Skill::getNameForCreature(WConstCreature c) const {
  double val = c->getAttributes().getSkills().getValue(getId());
  CHECK(val >= 0 && val <= 1) << "Skill value " << val;
  return getName() + " (" + toString<int>(val * 100) + "%)";
}

string Skill::getHelpText() const {
  return helpText;
}

void Skill::init() {
  Skill::set(SkillId::DIGGING, new Skill("digging", "Dig."));
  Skill::set(SkillId::WORKSHOP, new Skill("workshop", "Craft items in the workshop."));
  Skill::set(SkillId::FORGE, new Skill("forge", "Craft items in the forge."));
  Skill::set(SkillId::LABORATORY, new Skill("laboratory", "Craft items in the laboratory."));
  Skill::set(SkillId::JEWELER, new Skill("jeweler", "Craft items at the jeweler's shop."));
  Skill::set(SkillId::FURNACE, new Skill("furnace", "Craft items at the furnace."));
}

Skill::Skill(string _name, string _helpText)
  : name(_name), helpText(_helpText) {}

double Skillset::getValue(SkillId s) const {
  return values[s];
}

void Skillset::setValue(SkillId s, double v) {
  values[s] = v;
}

void Skillset::increaseValue(SkillId s, double v) {
  values[s] = max(0.0, min(1.0, values[s] + v));
}

SERIALIZE_DEF(Skillset, values)

#include "pretty_archive.h"
template
void Skillset::serialize(PrettyInputArchive&, unsigned);
