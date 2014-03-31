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

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;

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
