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

#ifndef _SKILL_H
#define _SKILL_H

#include <string>

#include "singleton.h"
#include "enums.h"

RICH_ENUM(SkillId,
  AMBUSH,
  KNIFE_THROWING,
  STEALING,
  SWIMMING,
  ARCHERY,
  CONSTRUCTION,
  ELF_VISION,
  NIGHT_VISION,
  DISARM_TRAPS,
  CONSUMPTION
);

class Creature;
class Skill : public Singleton<Skill, SkillId> {
  public:
  string getName() const;
  string getHelpText() const;
  bool canConsume() const;

  virtual void onTeach(Creature* c) {}

  SERIALIZATION_DECL(Skill);
  
  static void init();

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Skill(string name, string helpText, bool canConsume = true);

  private:
  string SERIAL(name);
  string SERIAL(helpText);
  bool SERIAL(consume);
};


#endif
