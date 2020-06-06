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

CollectiveBuilder& CollectiveBuilder::setLevel(WLevel l) {
  level = l;
  return *this;
}

CollectiveBuilder& CollectiveBuilder::setModel(WModel m) {
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
    ret.viewId = leader->getViewObject().id();
    if (locationName && raceName)
      ret.full = " The " + capitalFirst(*raceName) + " of " + capitalFirst(*locationName);
      // =========================================================
      // When Localization project is on progress, perhaps consider using this format
      // after making an identifier function for these section.
      // The usage is for anything with connector (of, and, from, etc).
      // Example 1:
      // File directory: /mod/localization/Japanese/JPN_LOC.txt 
      // (resource file name can be arbitrary, used for debugging address, 
      // the real that matters is the previous folder's name)
      // ==========================
      //  #/mod/localization/Japanese/JPN_LOC.txt (Arbitrary section used for debugging)
      // { locationName&&RaceName = locationName + "の" + raceName }
      // Example 2:
      // File directory: /mod/localization/Indonesian/INA_LOC.txt 
      // (file name can be arbitrary, used for debugging address, 
      // the real that matters is the previous folder's name)
      // ==========================
      //  #/mod/localization/Japanese/INA_LOC.txt (Arbitrary section used for debugging)
      // { locationName&&RaceName = raceName locationName }
      // =========================================================
      // DeliveryService
      // =========================================================
    else if (!!leader->getName().first())
      ret.full = leader->getName().title();
    else if (raceName)
      ret.full = "The " + capitalFirst(*raceName);
    else
      ret.full = leader->getName().title();
    if (locationName)
      ret.shortened = "The " + capitalFirst(*locationName);
    else if (auto leaderName = leader->getName().first())
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
  CHECK(model || level);
  auto c = Collective::create(!!model ? model : level->getModel(), *tribe, generateName(), discoverable, contentFactory);
  c->init(std::move(*config));
  c->setControl(CollectiveControl::idle(c.get()));
  bool wasLeader = false;
  for (auto& elem : creatures) {
    c->addCreature(elem.creature, elem.traits);
    if (elem.traits.contains(MinionTrait::LEADER))
      wasLeader = true;
  }
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
