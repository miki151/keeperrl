#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(creatures)
    & SVAR(taskMap)
    & SVAR(tribe)
    & SVAR(control);
  CHECK_SERIAL;
}

SERIALIZABLE(Collective);

Collective::Collective() : control(CollectiveControl::idle(this)) {
}

Collective::~Collective() {
}

void Collective::addCreature(Creature* c) {
  if (!tribe)
    tribe = c->getTribe();
  creatures.push_back(c);
}

vector<Creature*>& Collective::getCreatures() {
  return creatures;
}

const Tribe* Collective::getTribe() const {
  return NOTNULL(tribe);
}

Tribe* Collective::getTribe() {
  return NOTNULL(tribe);
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  PTask newTask = control->getNewTask(c);
  if (newTask)
    return taskMap.addTask(std::move(newTask), c)->getMove(c);
  else
    return control.get()->getMove(c);
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

void Collective::tick(double time) {
  control->tick(time);
}

vector<Creature*>& Collective::getCreatures(MinionTrait trait) {
  return byTrait[trait];
}

const vector<Creature*>& Collective::getCreatures(MinionTrait trait) const {
  return byTrait[trait];
}

void Collective::setTrait(Creature* c, MinionTrait trait) {
  byTrait[trait].push_back(c);
}
  
void Collective::onKillEvent(const Creature* victim, const Creature* killer) {
  if (contains(creatures, victim)) {
    control->onCreatureKilled(victim, killer);
    removeElement(creatures, victim);
    if (Task* task = taskMap.getTask(victim))
      taskMap.removeTask(task);
    for (MinionTrait t : ENUM_ALL(MinionTrait))
      if (contains(byTrait[t], victim))
        removeElement(byTrait[t], victim);
  }
}

