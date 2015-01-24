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

#ifndef _TECHNOLOGY_H
#define _TECHNOLOGY_H

#include "singleton.h"
#include "enums.h"

RICH_ENUM(TechId,
  ALCHEMY,
  ALCHEMY_ADV,
  HUMANOID_MUT,
  BEAST_MUT,
  CRAFTING,
  IRON_WORKING,
  TWO_H_WEAP,
  JEWELLERY,
  TRAPS,
  ARCHERY,
  SPELLS,
  SPELLS_ADV,
  SPELLS_MAS
);

class Technology : public Singleton<Technology, TechId> {
  public:
  Technology(const string& name, int cost, const vector<TechId>& prerequisites, bool canResearch = true);
  const string& getName() const;
  int getCost() const;
  bool canResearch() const;
  static vector<Technology*> getSorted();
  const vector<Technology*> getPrerequisites() const;
  const vector<Technology*> getAllowed() const;

  static vector<Technology*> getNextTechs(const vector<Technology*>& current);

  static void init();

  SERIALIZATION_DECL(Technology);

  private:
  bool canLearnFrom(const vector<Technology*>&) const;
  string SERIAL(name);
  int SERIAL(cost);
  vector<Technology*> SERIAL(prerequisites);
  bool SERIAL(research);
};

#endif
