#include "stdafx.h"
#include "collective_builder.h"
#include "collective.h"

CollectiveBuilder::CollectiveBuilder(CollectiveConfigId id, Tribe* t) : config(id), tribe(t) {
}

void CollectiveBuilder::setLevel(Level* l) {
  level = l;
}

void CollectiveBuilder::addCreature(Creature* c, EnumSet<MinionTrait> trait) {
  creatures.push_back({c, trait});
}

PCollective CollectiveBuilder::build() {
  Collective* c = new Collective(level, config, tribe);
  for (auto& elem : creatures)
    c->addCreature(elem.creature, elem.traits);
  return PCollective(c);
}

bool CollectiveBuilder::hasCreatures() const {
  return !creatures.empty();
}
