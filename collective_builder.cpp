#include "stdafx.h"
#include "collective_builder.h"
#include "collective.h"
#include "creature.h"
#include "creature_name.h"
#include "collective_name.h"
#include "creature_attributes.h"
#include "collective_config.h"
#include "tribe.h"
#include "collective_control.h"
#include "immigration.h"
#include "territory.h"
#include "view_object.h"
#include "level.h"

CollectiveBuilder::CollectiveBuilder(const CollectiveConfig& cfg, TribeId t)
    : config(cfg), tribe(t) {
}

CollectiveBuilder& CollectiveBuilder::setLevel(Level* l) {
  level = l;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::setModel(Model* m) {
  model = m;
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

CollectiveBuilder& CollectiveBuilder::setDiscoverable() {
  discoverable = true;
  return *this;
}

TribeId CollectiveBuilder::getTribe() {
  return *tribe;
}

CollectiveBuilder& CollectiveBuilder::addCreature(Creature* c, EnumSet<MinionTrait> traits) {
  creatures.push_back({c, traits});
  return *this;
}

bool CollectiveBuilder::hasCentralPoint() {
  return !!centralPoint;
}

void CollectiveBuilder::setCentralPoint(Vec2 pos) {
  centralPoint = pos;
}

CollectiveBuilder& CollectiveBuilder::addArea(const vector<Vec2>& v) {
  append(squares, v);
  return *this;
}

optional<CollectiveName> CollectiveBuilder::generateName() const {
  if (!creatures.empty()) {
    CollectiveName ret;
    auto leader = creatures[0].creature;
    ret.viewId = leader->getViewIdWithWeapon();
    if (locationName && raceName)
      ret.full = capitalFirst(*raceName) + " of " + *locationName;
    else if (!!leader->getName().first())
      ret.full = leader->getName().title();
    else if (raceName)
      ret.full = capitalFirst(*raceName);
    else
      ret.full = leader->getName().title();
    if (locationName) {
      ret.shortened = *locationName;
      ret.location = *locationName;
    } else if (auto leaderName = leader->getName().first())
      ret.shortened = *leaderName;
    if (raceName)
      ret.race = *raceName;
    else
      ret.race = leader->getName().stack();
    return ret;
  } else
    return none;
}

PCollective CollectiveBuilder::build(const ContentFactory* contentFactory) const {
  auto name = generateName();
  CHECK(model || level) << EnumInfo<TribeId::KeyType>::getString(tribe->getKey()) << " " << (name ? name->full : "no name");
  auto c = Collective::create(!!model ? model : level->getModel(), *tribe, std::move(name), discoverable, contentFactory);
  c->init(std::move(*config));
  c->setControl(CollectiveControl::idle(c.get()));
  for (auto& elem : creatures)
    c->addCreature(elem.creature, elem.traits);
  //CHECK(wasLeader || creatures.empty()) << "No leader added to collective " << c->getName()->full;
  for (Vec2 v : squares) {
    CHECK(level);
    Position pos(v, level);
    c->addKnownTile(pos);
    //if (c->canClaimSquare(pos))
      c->claimSquare(pos);
  }
  if (centralPoint)
    c->getTerritory().setCentralPoint(Position(*centralPoint, level));
  return c;
}

bool CollectiveBuilder::hasCreatures() const {
  return !creatures.empty();
}
