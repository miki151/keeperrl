#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(creatures)
    & SVAR(control);
  CHECK_SERIAL;
}

SERIALIZABLE(Collective);

Collective::Collective() : control(CollectiveControl::idle(this)) {
}

void Collective::addCreature(Creature* c) {
  creatures.push_back(c);
}

vector<Creature*>& Collective::getCreatures() {
  return creatures;
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(contains(creatures, c));
  return NOTNULL(control.get())->getMove(c);
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
