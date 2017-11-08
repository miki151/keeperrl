/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "model.h"
#include "player.h"
#include "village_control.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "level.h"
#include "name_generator.h"
#include "item_factory.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "collective.h"
#include "music.h"
#include "level_maker.h"
#include "map_memory.h"
#include "level_builder.h"
#include "tribe.h"
#include "time_queue.h"
#include "visibility_map.h"
#include "creature_name.h"
#include "creature_attributes.h"
#include "view.h"
#include "view_index.h"
#include "map_memory.h"
#include "stair_key.h"
#include "territory.h"
#include "game.h"
#include "progress_meter.h"
#include "view_object.h"
#include "item.h"
#include "external_enemies.h"
#include "tutorial.h"
#include "villain_type.h"
#include "player_control.h"
#include "tutorial.h"
#include "message_buffer.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) {
  CHECK(!serializationLocked);
  ar & SUBCLASS(OwnedObject<Model>);
  ar(portals, levels, collectives, timeQueue, deadCreatures, currentTime, woodCount, game, lastTick);
  ar(stairNavigation, cemetery, topLevel, eventGenerator, externalEnemies);
}

SERIALIZATION_CONSTRUCTOR_IMPL(Model)

SERIALIZABLE(Model)

void Model::discardForRetirement() {
  serializationLocked = true;
  deadCreatures.clear();
}

void Model::addWoodCount(int cnt) {
  woodCount += cnt;
}

int Model::getWoodCount() const {
  return woodCount;
}

int Model::getSaveProgressCount() const {
  int ret = 0;
  for (const PLevel& l : levels)
    ret += l->getNumGeneratedSquares();
  return ret;
}

vector<WCollective> Model::getCollectives() const {
  return getWeakPointers(collectives);
}

void Model::updateSunlightMovement() {
  for (PLevel& l : levels)
    l->updateSunlightMovement();
}

void Model::checkCreatureConsistency() {
  EntitySet<Creature> tmp;
  for (WCreature c : timeQueue->getAllCreatures()) {
    CHECK(!tmp.contains(c)) << c->getName().bare();
    tmp.insert(c);
  }
  for (PCreature& c : deadCreatures) {
    CHECK(!tmp.contains(c.get())) << c->getName().bare();
    tmp.insert(c.get());
  }
}

bool Model::update(LocalTime totalTime) {
  if (WCreature creature = timeQueue->getNextCreature()) {
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced before processing: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (creature->isDead()) {
      checkCreatureConsistency();
      FATAL << "Dead: " << creature->getName().bare();
    }
    currentTime = totalTime;
    if (creature->getLocalTime() > totalTime)
      return false;
    while (totalTime > lastTick) {
      lastTick += TimeInterval::fromInternal(1);
      tick(lastTick);
    }
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced before moving: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (!creature->isDead())
       creature->makeMove();
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced after moving: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (!creature->isDead() && creature->getLevel()->getModel() == this)
      CHECK(creature->getPosition().getCreature() == creature);
  } else
    currentTime = totalTime;
  return true;
}

void Model::tick(LocalTime time) {
  for (WCreature c : timeQueue->getAllCreatures()) {
    c->tick();
  }
  for (PLevel& l : levels)
    l->tick();
  for (PCollective& col : collectives)
    col->tick();
  if (externalEnemies)
    externalEnemies->update(getTopLevel(), time);
}

void Model::addCreature(PCreature c) {
  addCreature(std::move(c), 1_visible);// + Random.getDouble());
}

void Model::addCreature(PCreature c, TimeInterval delay) {
  timeQueue->addCreature(std::move(c), getLocalTime() + delay);
}

WLevel Model::buildLevel(LevelBuilder&& b, PLevelMaker maker) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(this, maker.get(), Random.getLL()));
  return levels.back().get();
}

WLevel Model::buildTopLevel(LevelBuilder&& b, PLevelMaker maker) {
  WLevel ret = buildLevel(std::move(b), std::move(maker));
  topLevel = ret;
  return ret;
}

Model::Model(Private) {
}

PModel Model::create() {
  auto ret = makeOwner<Model>(Private{});
  ret->cemetery = LevelBuilder(Random, 100, 100, "Dead creatures", false)
      .build(ret.get(), LevelMaker::emptyLevel(Random).get(), Random.getLL());
  ret->eventGenerator = makeOwner<EventGenerator>();
  return ret;
}

Model::~Model() {
}

LocalTime Model::getLocalTime() const {
  return currentTime;
}

void Model::increaseLocalTime(WCreature c, TimeInterval diff) {
  timeQueue->increaseTime(c, diff);
}

LocalTime Model::getLocalTime(WConstCreature c) {
  return timeQueue->getTime(c);
}

void Model::setGame(WGame g) {
  game = g;
}

WGame Model::getGame() const {
  return game;
}

WLevel Model::getLinkedLevel(WLevel from, StairKey key) const {
  for (WLevel target : getLevels())
    if (target != from && target->hasStairKey(key))
      return target;
  FATAL << "Failed to find next level for " << key.getInternalKey() << " " << from->getName();
  return nullptr;
}

static pair<LevelId, LevelId> getIds(WConstLevel l1, WConstLevel l2) {
  return {l1->getUniqueId(), l2->getUniqueId()};
}

void Model::calculateStairNavigation() {
  // Floyd-Warshall algorithm
  for (auto l1 : getLevels())
    for (auto l2 : getLevels())
      if (l1 != l2)
        if (auto stairKey = getStairsBetween(l1, l2))
          stairNavigation[getIds(l1, l2)] = *stairKey;
  for (auto li : getLevels())
    for (auto l1 : getLevels())
      if (li != l1)
        for (auto l2 : getLevels())
          if (l2 != l1 && l2 != li && !stairNavigation.count(getIds(l1, l2)) &&
              stairNavigation.count(getIds(li, l2)) && stairNavigation.count(getIds(l1, li)))
            stairNavigation[getIds(l1, l2)] = stairNavigation.at(getIds(l1, li));
  for (auto l1 : getLevels())
    for (auto l2 : getLevels())
      if (l1 != l2)
        CHECK(stairNavigation.count(getIds(l1, l2))) <<
            "No stair path between levels " << l1->getName() << " " << l2->getName();
}

optional<StairKey> Model::getStairsBetween(WConstLevel from, WConstLevel to) {
  for (StairKey key : from->getAllStairKeys())
    if (to->hasStairKey(key))
      return key;
  return none;
}

optional<Position> Model::getStairs(WConstLevel from, WConstLevel to) {
  CHECK(from != to);
  if (!getLevels().contains(from) || !getLevels().contains(to) || !stairNavigation.count(getIds(from, to)))
    return none;
  return Random.choose(from->getLandingSquares(stairNavigation.at(getIds(from, to))));
}

vector<WLevel> Model::getLevels() const {
  return getWeakPointers(levels);
}

WLevel Model::getTopLevel() const {
  return topLevel;
}

void Model::killCreature(WCreature c) {
  deadCreatures.push_back(timeQueue->removeCreature(c));
  cemetery->landCreature(cemetery->getAllPositions(), c);
}

PCreature Model::extractCreature(WCreature c) {
  PCreature ret = timeQueue->removeCreature(c);
  c->getLevel()->removeCreature(c);
  return ret;
}

void Model::transferCreature(PCreature c, Vec2 travelDir) {
  WCreature ref = c.get();
  addCreature(std::move(c));
  CHECK(getTopLevel()->landCreature(StairKey::transferLanding(), ref, travelDir));
}

bool Model::canTransferCreature(WCreature c, Vec2 travelDir) {
  for (Position pos : getTopLevel()->getLandingSquares(StairKey::transferLanding()))
    if (pos.canEnter(c))
      return true;
  return false;
}

vector<WCreature> Model::getAllCreatures() const { 
  return timeQueue->getAllCreatures();
}

void Model::landHeroPlayer(PCreature player) {
  WCreature ref = player.get();
  WLevel target = getTopLevel();
  vector<Position> landing = target->getLandingSquares(StairKey::heroSpawn());
  if (!target->landCreature(landing, ref)) {
    CHECK(target->landCreature(target->getAllPositions(), ref)) << "No place to spawn player";
  }
  addCreature(std::move(player));
  ref->setController(makeOwner<Player>(ref, true, make_shared<MapMemory>(), make_shared<MessageBuffer>(),
      make_shared<VisibilityMap>()));
}

void Model::addExternalEnemies(ExternalEnemies e) {
  externalEnemies = std::move(e);
}

const optional<ExternalEnemies>& Model::getExternalEnemies() const {
  return *externalEnemies;
}

void Model::clearExternalEnemies() {
  externalEnemies = none;
}

void Model::addEvent(const GameEvent& e) {
  eventGenerator->addEvent(e);
}

optional<Position> Model::getOtherPortal(Position position) const {
  if (auto index = portals.findElement(position)) {
    if (*index % 2 == 1)
      return portals[*index - 1];
    if (*index < portals.size() - 1)
      return portals[*index + 1];
  }
  return none;
}

void Model::registerPortal(Position pos) {
  if (!portals.contains(pos))
    portals.push_back(pos);
}
