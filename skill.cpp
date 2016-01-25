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
#include "modifier_type.h"

string Skill::getName() const {
  return name;
}

string Skill::getNameForCreature(const Creature* c) const {
  double val = c->getSkillValue(this);
  CHECK(val >= 0 && val <= 1) << "Skill value " << val;
  string grade;
  if (val == 0)
    grade = "unskilled";
  else if (val < 0.3)
    grade = "basic";
  else if (val < 0.6)
    grade = "skilled";
  else if (val < 1)
    grade = "expert";
  else
    grade = "master";
  return getName() + " (" + grade + ")";
}

string Skill::getHelpText() const {
  return helpText;
}

bool Skill::isDiscrete() const {
  return discrete;
}

static int archeryBonus(const Creature* c, ModifierType t) {
  switch (t) {
    case ModifierType::FIRED_ACCURACY: return c->getSkillValue(Skill::get(SkillId::ARCHERY)) * 10;
    case ModifierType::FIRED_DAMAGE: return c->getSkillValue(Skill::get(SkillId::ARCHERY)) * 10;
    default: break;
  }
  return 0;
}

static int weaponBonus(const Creature* c, ModifierType t) {
  if (!c->getWeapon())
    return 0;
  switch (t) {
    case ModifierType::ACCURACY: return c->getSkillValue(Skill::get(SkillId::WEAPON_MELEE)) * 10;
    case ModifierType::DAMAGE: return c->getSkillValue(Skill::get(SkillId::WEAPON_MELEE)) * 10;
    default: break;
  }
  return 0;
}

static int unarmedBonus(const Creature* c, ModifierType t) {
  if (c->getWeapon())
    return 0;
  switch (t) {
    case ModifierType::ACCURACY: return c->getSkillValue(Skill::get(SkillId::UNARMED_MELEE)) * 10;
    case ModifierType::DAMAGE: return c->getSkillValue(Skill::get(SkillId::UNARMED_MELEE)) * 10;
    default: break;
  }
  return 0;
}

static int knifeBonus(const Creature* c, ModifierType t) {
  switch (t) {
    case ModifierType::THROWN_ACCURACY: return c->getSkillValue(Skill::get(SkillId::KNIFE_THROWING)) * 10;
    case ModifierType::THROWN_DAMAGE: return c->getSkillValue(Skill::get(SkillId::KNIFE_THROWING)) * 10;
    default: break;
  }
  return 0;
}

int Skill::getModifier(const Creature* c, ModifierType t) const {
  switch (getId()) {
    case SkillId::ARCHERY: return archeryBonus(c, t);
    case SkillId::WEAPON_MELEE: return weaponBonus(c, t);
    case SkillId::UNARMED_MELEE: return unarmedBonus(c, t);
    case SkillId::KNIFE_THROWING: return knifeBonus(c, t);
    default: break;
  }
  return 0;
}

void Skill::init() {
  Skill::set(SkillId::AMBUSH, new Skill("ambush",
        "Hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it.", true));
  Skill::set(SkillId::KNIFE_THROWING, new Skill("knife throwing", "Throw knives with deadly precision.", false));
  Skill::set(SkillId::STEALING,
      new Skill("stealing", "Steal from other monsters. Not available for player ATM.", true));
  Skill::set(SkillId::SWIMMING, new Skill("swimming", "Cross water without drowning.", true));
  Skill::set(SkillId::ARCHERY, new Skill("archery", "Shoot bows.", false));
  Skill::set(SkillId::WEAPON_MELEE, new Skill("weapon melee", "Fight with weapons.", false));
  Skill::set(SkillId::UNARMED_MELEE, new Skill("unarmed melee", "Fight unarmed.", false));
  Skill::set(SkillId::CONSTRUCTION, new Skill("construction", "Mine and construct rooms.", true, false));
  Skill::set(SkillId::ELF_VISION, new Skill("elf vision", "See and shoot arrows through trees.", true));
  Skill::set(SkillId::NIGHT_VISION, new Skill("night vision", "See in the dark.", true));
  Skill::set(SkillId::DISARM_TRAPS, new Skill("disarm traps", "Evade traps and disarm them.", true));
  Skill::set(SkillId::SORCERY, new Skill("sorcery", "Cast spells.", false));
  Skill::set(SkillId::CONSUMPTION, new Skill("absorbtion",
        "Absorb other creatures and retain their attributes.", true, false));
  Skill::set(SkillId::HEALING, new Skill("healing", "Heal friendly creatures.", true));
  Skill::set(SkillId::STEALTH, new Skill("stealth", "Fight without waking up creatures sleeping nearby.", true));
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
  return discrete[s];
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


template <class Archive>
void Skillset::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(discrete) & SVAR(gradable);
}

SERIALIZABLE(Skillset);
