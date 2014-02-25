#ifndef _TIME_QUEUE_H
#define _TIME_QUEUE_H

#include "util.h"
#include "creature.h"

class TimeQueue {
  public:
  TimeQueue();
  Creature* getNextCreature();
  vector<Creature*> getAllCreatures() const;
  void addCreature(PCreature c);
  PCreature removeCreature(Creature* c);
  double getCurrentTime();

  private:
  vector<PCreature> creatures;
  struct QElem {
    Creature* creature;
    double time;
  };
  priority_queue<QElem, vector<QElem>, function<bool(QElem, QElem)>> queue;
  unordered_set<Creature*> dead;
  void removeDead();
  Creature* getMinCreature();
};

#endif
