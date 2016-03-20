#include "stdafx.h"
#include "collective_builder.h"
#include "collective.h"
#include "location.h"
#include "creature.h"
#include "entity_name.h"
#include "collective_name.h"

CollectiveBuilder::CollectiveBuilder(CollectiveConfig cfg, TribeId t)
    : config(cfg), tribe(t) {
}

CollectiveBuilder& CollectiveBuilder::setLevel(Level* l) {
  level = l;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::setLocationName(const string& n) {
  locationName = n;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::setRaceName(const string& n) {
  raceName = n;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::addCreature(Creature* c) {
  if (!c->isInnocent() && (!creatures.empty() || config.isLeaderFighter()))
    creatures.push_back({c, {MinionTrait::FIGHTER}});
  else
    creatures.push_back({c, {}});
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

CollectiveBuilder& CollectiveBuilder::addSquares(const vector<Position>& v) {
  for (auto& pos : v)
    squares.push_back(pos.getCoord());
  return *this;
}

PCollective CollectiveBuilder::build() {
  CHECK(!creatures.empty());
  Collective* c = new Collective(NOTNULL(level), config, tribe, credit,
      CollectiveName(raceName, locationName, creatures[0].creature));
  for (auto& elem : creatures)
    c->addCreature(elem.creature, elem.traits);
  for (Vec2 v : squares)
    c->claimSquare(Position(v, level));
  return PCollective(c);
}

bool CollectiveBuilder::hasCreatures() const {
  return !creatures.empty();
}
