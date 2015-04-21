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

#ifndef _TIME_QUEUE_H
#define _TIME_QUEUE_H

#include "util.h"

class Creature;

class TimeQueue {
  public:
  TimeQueue();
  Creature* getNextCreature();
  vector<Creature*> getAllCreatures() const;
  void addCreature(PCreature c);
  PCreature removeCreature(Creature* c);
  double getCurrentTime();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  void removeDead();
  Creature* getMinCreature();

  vector<PCreature> SERIAL(creatures);

  struct QElem {
    Creature* creature;
    double time;

    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };
  priority_queue<QElem, vector<QElem>, function<bool(QElem, QElem)>> SERIAL(queue);
  unordered_set<Creature*> SERIAL(dead);
};

#endif
