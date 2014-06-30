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

#ifndef _QUEST_H
#define _QUEST_H

#include <map>
#include <string>

#include "util.h"
#include "singleton.h"

class Location;
class Tribe;

enum class QuestId {
  DRAGON,
  CASTLE_CELLAR,
  BANDITS,
  GOBLINS,
  DWARVES,

  ENUM_END};

class Quest : public Singleton<Quest, QuestId> {
  public:
  virtual ~Quest() {}
  virtual bool isFinished() const = 0;
  void addAdventurer(Creature* c);
  void setLocation(const Location*);
  
  static Quest* killTribeQuest(Tribe* tribe, string message, bool onlyImp = false);

  SERIALIZATION_DECL(Quest);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Quest(const string& startMessage);

  unordered_set<const Creature*> SERIAL(adventurers);
  const Location* SERIAL2(location, nullptr);
  string SERIAL(startMessage);
};

#endif
