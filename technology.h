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

#include "singleton.h"
#include "enums.h"

RICH_ENUM(TechId,
  ALCHEMY,
  ALCHEMY_ADV,
  ALCHEMY_CONV,
  HUMANOID_MUT,
  BEAST_MUT,
  PIGSTY,
  IRON_WORKING,
  TWO_H_WEAP,
  JEWELLERY,
  TRAPS,
  ARCHERY,
  SPELLS,
  SPELLS_ADV,
  SPELLS_MAS,
  DEMONOLOGY,
  MAGICAL_WEAPONS
);

class Spell;
class Collective;
class CostInfo;

class Technology : public Singleton<Technology, TechId> {
  public:
  Technology(const string& name, const string& description, int cost, const vector<TechId>& prerequisites = {},
      bool canResearch = true);
  const string& getName() const;
  bool canResearch() const;
  Technology* setTutorialHighlight(TutorialHighlight);
  const string& getDescription() const;
  const optional<TutorialHighlight> getTutorialHighlight() const;
  const vector<Technology*> getPrerequisites() const;
  const vector<Technology*> getAllowed() const;

  static CostInfo getAvailableResource(WConstCollective);

  static vector<Technology*> getSorted();
  static vector<Technology*> getNextTechs(const vector<Technology*>& current);
  static vector<Spell*> getSpellLearning(TechId tech);
  static vector<Spell*> getAvailableSpells(WConstCollective);
  static vector<Spell*> getAllKeeperSpells();
  static TechId getNeededTech(Spell*);

  static void onAcquired(TechId, WCollective);

  static void init();

  private:
  bool canLearnFrom(const vector<Technology*>&) const;
  string name;
  string description;
  int cost;
  vector<Technology*> prerequisites;
  bool research;
  optional<TutorialHighlight> tutorial;
};

