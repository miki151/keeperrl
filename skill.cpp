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

template <class Archive> 
void Skill::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Singleton)
    & SVAR(name)
    & SVAR(consume)
    & SVAR(helpText);
  CHECK_SERIAL;
}

SERIALIZABLE(Skill);

SERIALIZATION_CONSTRUCTOR_IMPL(Skill);

string Skill::getName() const {
  return name;
}

string Skill::getHelpText() const {
  return helpText;
}

void Skill::init() {
  Skill::set(SkillId::AMBUSH, new Skill("ambush",
        "Hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it."));
  Skill::set(SkillId::KNIFE_THROWING, new Skill("knife throwing", "Throw knives with deadly precision."));
  Skill::set(SkillId::STEALING, new Skill("stealing", "Steal from other monsters. Not available for player ATM."));
  Skill::set(SkillId::SWIMMING, new Skill("swimming", "Cross water without drowning."));
  Skill::set(SkillId::ARCHERY, new Skill("archery", "Shoot bows."));
  Skill::set(SkillId::CONSTRUCTION, new Skill("construction", "Mine and construct rooms.", false));
  Skill::set(SkillId::ELF_VISION, new Skill("elf vision", "See and shoot arrows through trees."));
  Skill::set(SkillId::NIGHT_VISION, new Skill("night vision", "See in the dark."));
  Skill::set(SkillId::DISARM_TRAPS, new Skill("disarm traps", "Evade traps and disarm them."));
  Skill::set(SkillId::CONSUMPTION, new Skill("absorbtion",
        "Absorb other creatures and retain their attributes.", false));
}

bool Skill::canConsume() const {
  return consume;
}

Skill::Skill(string _name, string _helpText, bool _consume) : name(_name), helpText(_helpText), consume(_consume) {}

