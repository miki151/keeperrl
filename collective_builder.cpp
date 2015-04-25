#include "stdafx.h"
#include "collective_builder.h"
#include "collective.h"
#include "location.h"

CollectiveBuilder::CollectiveBuilder(CollectiveConfig cfg, Tribe* t)
    : config(cfg), tribe(t) {
}

CollectiveBuilder& CollectiveBuilder::setLevel(Level* l) {
  level = l;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::addCreature(Creature* c, EnumSet<MinionTrait> trait) {
  creatures.push_back({c, trait});
  return *this;
}

CollectiveBuilder& CollectiveBuilder::setCredit(EnumMap<CollectiveResourceId, int> c) {
  credit = c;
  return *this;
}

PCollective CollectiveBuilder::build(const Location* location) {
  Collective* c = new Collective(NOTNULL(level), config, tribe, credit,
      location->hasName() ? location->getName() : "");
  for (auto& elem : creatures)
    c->addCreature(elem.creature, elem.traits);
  for (Vec2 v : location->getAllSquares())
    c->claimSquare(v);
  return PCollective(c);
}

PCollective CollectiveBuilder::build() {
  Collective* c = new Collective(NOTNULL(level), config, tribe, credit, "");
  for (auto& elem : creatures)
    c->addCreature(elem.creature, elem.traits);
  return PCollective(c);
}

bool CollectiveBuilder::hasCreatures() const {
  return !creatures.empty();
}
