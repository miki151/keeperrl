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

void Skillset::setValue(WorkshopType s, double v) {
  workshopValues[s] = v;
}

void Skillset::increaseValue(WorkshopType type, double v) {
  workshopValues[type] = max(0.0, min(1.0, workshopValues[type] + v));
}

SERIALIZE_DEF(Skillset, workshopValues)

#include "pretty_archive.h"
template<>
void Skillset::serialize(PrettyInputArchive& ar1, unsigned) {
  map<string, double> input;
  ar1(input);
  for (auto& elem : input) {
    ar1.keyVerifier.verifyContentId<WorkshopType>(elem.first);
    workshopValues[WorkshopType(elem.first.data())] = elem.second;
  }
}
