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

bool Skill::isDiscrete() const {
  return discrete;
}

void Skill::init() {
  Skill::set(SkillId::AMBUSH, new Skill("ambush",
        "Hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it.", true));
  Skill::set(SkillId::STEALING,
      new Skill("stealing", "Steal from other monsters. Not available for player ATM.", true));
  Skill::set(SkillId::SWIMMING, new Skill("swimming", "Cross water without drowning.", true));
  Skill::set(SkillId::CONSTRUCTION, new Skill("construction", "Mine and construct rooms.", true, false));
  Skill::set(SkillId::ELF_VISION, new Skill("elf vision", "See and shoot arrows through trees.", true));
  Skill::set(SkillId::NIGHT_VISION, new Skill("night vision", "See in the dark.", true));
  Skill::set(SkillId::DISARM_TRAPS, new Skill("disarm traps", "Evade traps and disarm them.", true));
  Skill::set(SkillId::SORCERY, new Skill("sorcery", "Affects the length of spell cooldowns.", false));
  Skill::set(SkillId::CONSUMPTION, new Skill("absorbtion",
        "Absorb other creatures and retain their attributes.", true, false));
  Skill::set(SkillId::HEALING, new Skill("healing", "Heal friendly creatures.", true));
  Skill::set(SkillId::STEALTH, new Skill("stealth", "Fight without waking up creatures sleeping nearby.", true));
  Skill::set(SkillId::WORKSHOP, new Skill("workshop", "Craft items in the workshop.", false));
  Skill::set(SkillId::FORGE, new Skill("forge", "Craft items in the forge.", false));
  Skill::set(SkillId::LABORATORY, new Skill("laboratory", "Craft items in the laboratory.", false));
  Skill::set(SkillId::JEWELER, new Skill("jeweler", "Craft items at the jeweler's shop.", false));
  Skill::set(SkillId::FURNACE, new Skill("furnace", "Craft items at the furnace.", false));
}

bool Skill::transferOnConsumption() const {
  return consume;
}

Skill::Skill(string _name, string _helpText, bool _discrete, bool _consume) 
  : name(_name), helpText(_helpText), consume(_consume), discrete(_discrete) {}

void Skillset::insert(SkillId s) {
  CHECK(Skill::get(s)->isDiscrete());
  discrete.insert(s);
}

void Skillset::erase(SkillId s) {
  CHECK(Skill::get(s)->isDiscrete());
  discrete.erase(s);
}

bool Skillset::hasDiscrete(SkillId s) const {
  CHECK(Skill::get(s)->isDiscrete());
  return discrete.contains(s);
}

double Skillset::getValue(SkillId s) const {
  CHECK(!Skill::get(s)->isDiscrete());
  return gradable[s];
}

void Skillset::setValue(SkillId s, double v) {
  CHECK(!Skill::get(s)->isDiscrete());
  gradable[s] = v;
}

const EnumSet<SkillId>& Skillset::getAllDiscrete() const {
  return discrete;
}

SERIALIZE_DEF(Skillset, discrete, gradable)
