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
#include "item_factory.h"
#include "enums.h"

template <class Archive> 
void Skill::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(name)
    & SVAR(helpText);
  CHECK_SERIAL;
}

SERIALIZABLE(Skill);

string Skill::getName() const {
  return name;
}

string Skill::getHelpText() const {
  return helpText;
}

class IdentifySkill : public Skill {
  public:
  IdentifySkill(string name, string helpText, ItemFactory f) : Skill(name, helpText), factory(f) {}

  virtual void onTeach(Creature* c) override {
    string message;
    for (PItem& it : factory.getAll()) {
      Item::identify(it->getName());
      message.append(it->getName() + ": " + it->getDescription() + "\n");
    }
 //   c->privateMessage(MessageBuffer::important(message));
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Skill) & SVAR(factory);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(IdentifySkill);
  private:
  ItemFactory SERIAL(factory);
};

template <class Archive>
void Skill::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, IdentifySkill);
}

REGISTER_TYPES(Skill);

void Skill::init() {
  Skill::set(SkillId::AMBUSH, new Skill("ambush",
        "Hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it."));
  Skill::set(SkillId::KNIFE_THROWING, new Skill("knife throwing", "Throw knives with deadly precision."));
  Skill::set(SkillId::STEALING, new Skill("stealing", "Steal from other monsters. Not available for player ATM."));
  Skill::set(SkillId::SWIMMING, new Skill("swimming", "Cross water without drowning."));
  Skill::set(SkillId::ARCHERY, new Skill("archery", "Shoot bows."));
  Skill::set(SkillId::CONSTRUCTION, new Skill("construction", "Mine and construct rooms."));
  Skill::set(SkillId::ELF_VISION, new Skill("elf vision", "See and shoot arrows through trees."));
  Skill::set(SkillId::NIGHT_VISION, new Skill("night vision", "See in the dark."));
}

Skill::Skill(string _name, string _helpText) : name(_name), helpText(_helpText) {}

