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

#include "util.h"
#include "entity_set.h"
#include "entity_map.h"

class Creature;

class TimeQueue {
  public:
  TimeQueue();
  Creature* getNextCreature();
  vector<Creature*> getAllCreatures() const;
  void addCreature(PCreature, double time);
  PCreature removeCreature(Creature*);
  double getTime(const Creature*);
  void increaseTime(Creature*, double diff);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  typedef set<Creature*, function<bool(const Creature*, const Creature*)>> Queue;
  vector<PCreature> SERIAL(creatures);
  Queue SERIAL(queue);
  EntityMap<Creature, double> SERIAL(timeMap);
};

