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

CollectiveBuilder& CollectiveBuilder::setCredit(map<CollectiveResourceId, int> c) {
  credit = c;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::addSquares(const vector<Vec2>& v) {
  append(squares, v);
  return *this;
}

PCollective CollectiveBuilder::build(const string& name) {
  Collective* c = new Collective(NOTNULL(level), config, tribe, credit, name);
  for (auto& elem : creatures)
    c->addCreature(elem.creature, elem.traits);
  for (Vec2 v : squares)
    c->claimSquare(v);
  return PCollective(c);
}

bool CollectiveBuilder::hasCreatures() const {
  return !creatures.empty();
}
