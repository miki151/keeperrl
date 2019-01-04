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

#include <string>

#include "singleton.h"
#include "enums.h"

RICH_ENUM(SkillId,
  DIGGING,
  WORKSHOP,
  FORGE,
  LABORATORY,
  JEWELER,
  FURNACE
);

class Creature;
class Skill : public Singleton<Skill, SkillId> {
  public:
  string getName() const;
  string getNameForCreature(WConstCreature) const;
  string getHelpText() const;

  int getModifier(WConstCreature, AttrType) const;

  static void init();

  private:
  string name;
  string helpText;
  Skill(string name, string helpText);
};

class Skillset {
  public:
  double getValue(SkillId) const;
  void setValue(SkillId, double);
  void increaseValue(SkillId, double);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EnumMap<SkillId, double> SERIAL(values);
};

