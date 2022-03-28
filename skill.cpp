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
#include "content_factory.h"

string getName(SkillId skillid) {
  return Skill::get(skillid)->getName();
}

string Skill::getName() const {
  return name;
}

string Skill::getNameForCreature(const Creature* c) const {
  double val = c->getAttributes().getSkills().getValue(getId());
  CHECK(val >= 0 && val <= 1) << "Skill value " << val;
  return getName() + " (" + toPercentage(val) + ")";
}

string Skill::getHelpText() const {
  return helpText;
}

void Skill::init() {
  Skill::set(SkillId::FURNACE, new Skill("furnace", "Smelt and recycle unneeded items."));
}

Skill::Skill(string _name, string _helpText)
  : name(_name), helpText(_helpText) {}

double Skillset::getValue(SkillId s) const {
  return values[s];
}

double Skillset::getValue(WorkshopType t) const {
  return getValueMaybe(workshopValues, t).value_or(0);
}

const map<WorkshopType, double>&Skillset::getWorkshopValues() const {
  return workshopValues;
}

string Skillset::getNameForCreature(const ContentFactory* factory, WorkshopType type) const {
  return factory->workshopInfo.at(type).name + " ("_s + toPercentage(getValue(type)) + ")";
}

string Skillset::getHelpText(const ContentFactory* factory, WorkshopType type) const {
  return "Craft items in the "_s + factory->workshopInfo.at(type).name + ".";
}

void Skillset::setValue(SkillId s, double v) {
  values[s] = v;
}

void Skillset::setValue(WorkshopType s, double v) {
  workshopValues[s] = v;
}

void Skillset::increaseValue(SkillId s, double v) {
  values[s] = max(0.0, min(1.0, values[s] + v));
}

void Skillset::increaseValue(WorkshopType type, double v) {
  workshopValues[type] = max(0.0, min(1.0, workshopValues[type] + v));
}

SERIALIZE_DEF(Skillset, values, workshopValues)

#include "pretty_archive.h"
template<>
void Skillset::serialize(PrettyInputArchive& ar1, unsigned) {
  map<string, double> input;
  ar1(input);
  for (auto& elem : input) {
    if (elem.second < 0 || elem.second > 1)
      ar1.error("Skill value for "_s + elem.first + " must be between 0 and one, inclusive.");
    if (auto id = EnumInfo<SkillId>::fromStringSafe(elem.first))
      values[*id] = elem.second;
    else {
      ar1.keyVerifier.verifyContentId<WorkshopType>(elem.first);
      workshopValues[WorkshopType(elem.first.data())] = elem.second;
    }
  }
}
