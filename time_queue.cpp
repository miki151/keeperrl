#include "stdafx.h"

using namespace std;

TimeQueue::TimeQueue() : queue([](QElem e1, QElem e2) {
    return e1.time > e2.time || (e1.time == e2.time && e1.creature->getUniqueId() > e2.creature->getUniqueId());
}) {}

void TimeQueue::addCreature(PCreature c) {
  queue.push({c.get(), c->getTime()});
  creatures.push_back(std::move(c));
}
  
PCreature TimeQueue::removeCreature(Creature* cRef) {
  int ind = -1;
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      ind = i;
      break;
    }
  CHECK(ind > -1) << "Creature not found";
  PCreature ret = std::move(creatures[ind]);
  creatures.erase(creatures.begin() + ind);
  dead.insert(ret.get());
  return ret;
}

vector<Creature*> TimeQueue::getAllCreatures() const {
  vector<Creature*> ret;
  for (const PCreature& c : creatures)
    ret.push_back(c.get());
  return ret;
}

void TimeQueue::removeDead() {
  while (!queue.empty() && dead.count(queue.top().creature))
    queue.pop();
}

Creature* TimeQueue::getMinCreature() {
  CHECK(creatures.size() > 0);
  removeDead();
  QElem elem = queue.top();
  if (elem.time == elem.creature->getTime())
    return elem.creature;
  else {
    queue.pop();
    removeDead();
    queue.push({elem.creature, elem.creature->getTime()});
    CHECK(queue.top().creature->getTime() == queue.top().time);
    return queue.top().creature;
  }
  /*for (PCreature& c : creatures)
    if (c->getTime() < min || (c->getTime() == min && c->getUniqueId() < res->getUniqueId())) {
      res = c.get();
      min = c->getTime();
    }*/
}

Creature* TimeQueue::getNextCreature() {
  Creature* c = getMinCreature();
  return c;
}

double TimeQueue::getCurrentTime() {
  if (creatures.size() > 0) 
    return getMinCreature()->getTime();
  else
    return 0;
}
