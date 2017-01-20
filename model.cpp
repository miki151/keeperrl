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
#include "trigger.h"
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

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) {
  CHECK(!serializationLocked);
  serializeAll(ar, levels, collectives, timeQueue, deadCreatures, currentTime, woodCount, game, lastTick);
  serializeAll(ar, stairNavigation, cemetery, topLevel, eventGenerator, externalEnemies);
}

void Model::lockSerialization() {
  serializationLocked = true;
}

SERIALIZABLE(Model);

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

vector<Collective*> Model::getCollectives() const {
  return extractRefs(collectives);
}

void Model::updateSunlightMovement() {
  for (PLevel& l : levels)
    l->updateSunlightMovement();
}

void Model::checkCreatureConsistency() {
  EntitySet<Creature> tmp;
  for (Creature* c : timeQueue->getAllCreatures()) {
    CHECK(!tmp.contains(c)) << c->getName().bare();
    tmp.insert(c);
  }
  for (PCreature& c : deadCreatures) {
    CHECK(!tmp.contains(c.get())) << c->getName().bare();
    tmp.insert(c.get());
  }
}

void Model::update(double totalTime) {
  if (Creature* creature = timeQueue->getNextCreature()) {
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced before processing: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (creature->isDead()) {
      checkCreatureConsistency();
      FATAL << "Dead: " << creature->getName().bare();
    }
    currentTime = creature->getLocalTime();
    if (currentTime > totalTime)
      return;
    while (totalTime > lastTick + 1) {
      lastTick += 1;
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
}

void Model::tick(double time) {
  for (Creature* c : timeQueue->getAllCreatures()) {
    c->tick();
  }
  for (PLevel& l : levels)
    l->tick();
  for (PCollective& col : collectives)
    col->tick();
  if (*externalEnemies)
    (*externalEnemies)->update(getTopLevel(), time);
}

void Model::addCreature(PCreature c) {
  addCreature(std::move(c), 1 + Random.getDouble());
}

void Model::addCreature(PCreature c, double delay) {
  if (c->isPlayer())
    game->setPlayer(c.get());
  timeQueue->addCreature(std::move(c), getLocalTime() + delay);
}

Level* Model::buildLevel(LevelBuilder&& b, PLevelMaker maker) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(this, maker.get(), Random.getLL()));
  return levels.back().get();
}

Level* Model::buildTopLevel(LevelBuilder&& b, PLevelMaker maker) {
  Level* ret = buildLevel(std::move(b), std::move(maker));
  topLevel = ret;
  return ret;
}

Model::Model() {
  cemetery = LevelBuilder(Random, 100, 100, "Dead creatures", false)
      .build(this, LevelMaker::emptyLevel(Random).get(), Random.getLL());
}

Model::~Model() {
}

double Model::getLocalTime() const {
  return currentTime;
}

void Model::increaseLocalTime(Creature* c, double diff) {
  timeQueue->increaseTime(c, diff);
}

double Model::getLocalTime(const Creature* c) {
  return timeQueue->getTime(c);
}

void Model::setGame(Game* g) {
  game = g;
}

Game* Model::getGame() const {
  return game;
}

Level* Model::getLinkedLevel(Level* from, StairKey key) const {
  for (Level* target : getLevels())
    if (target != from && target->hasStairKey(key))
      return target;
  FATAL << "Failed to find next level for " << key.getInternalKey() << " " << from->getName();
  return nullptr;
}

void Model::calculateStairNavigation() {
  // Floyd-Warshall algorithm
  for (const Level* l1 : getLevels())
    for (const Level* l2 : getLevels())
      if (l1 != l2)
        if (auto stairKey = getStairsBetween(l1, l2))
          stairNavigation[make_pair(l1, l2)] = *stairKey;
  for (const Level* li : getLevels())
    for (const Level* l1 : getLevels())
      if (li != l1)
        for (const Level* l2 : getLevels())
          if (l2 != l1 && l2 != li && !stairNavigation.count(make_pair(l1, l2)) &&
              stairNavigation.count(make_pair(li, l2)) && stairNavigation.count(make_pair(l1, li)))
            stairNavigation[make_pair(l1, l2)] = stairNavigation.at(make_pair(l1, li));
  for (const Level* l1 : getLevels())
    for (const Level* l2 : getLevels())
      if (l1 != l2)
        CHECK(stairNavigation.count(make_pair(l1, l2))) <<
            "No stair path between levels " << l1->getName() << " " << l2->getName();
}

optional<StairKey> Model::getStairsBetween(const Level* from, const Level* to) {
  for (StairKey key : from->getAllStairKeys())
    if (to->hasStairKey(key))
      return key;
  return none;
}

optional<Position> Model::getStairs(const Level* from, const Level* to) {
  CHECK(from != to);
  if (!contains(getLevels(), from) || !contains(getLevels(), to) || !stairNavigation.count({from, to}))
    return none;
  return Random.choose(from->getLandingSquares(stairNavigation.at({from, to})));
}

vector<Level*> Model::getLevels() const {
  return extractRefs(levels);
}

Level* Model::getTopLevel() const {
  return topLevel;
}

void Model::killCreature(Creature* c) {
  deadCreatures.push_back(timeQueue->removeCreature(c));
  cemetery->landCreature(cemetery->getAllPositions(), c);
}

PCreature Model::extractCreature(Creature* c) {
  PCreature ret = timeQueue->removeCreature(c);
  c->getLevel()->removeCreature(c);
  return ret;
}

void Model::transferCreature(PCreature c, Vec2 travelDir) {
  CHECK(getTopLevel()->landCreature(StairKey::transferLanding(), std::move(c), travelDir));
}

bool Model::canTransferCreature(Creature* c, Vec2 travelDir) {
  for (Position pos : getTopLevel()->getLandingSquares(StairKey::transferLanding()))
    if (pos.canEnter(c))
      return true;
  return false;
}

vector<Creature*> Model::getAllCreatures() const { 
  return timeQueue->getAllCreatures();
}

void Model::landHeroPlayer(PCreature player) {
  player->setController(make_shared<Player>(player.get(), true, new MapMemory()));
  Level* target = getTopLevel();
  vector<Position> landing = target->getLandingSquares(StairKey::heroSpawn());
  for (Position pos : landing)
    if (pos.canEnter(player.get())) {
      CHECK(target->landCreature(landing, std::move(player))) << "No place to spawn player";
      return;
    }
  CHECK(target->landCreature(target->getAllPositions(), std::move(player))) << "No place to spawn player";
}

void Model::addExternalEnemies(const ExternalEnemies& e) {
  externalEnemies = e;
}

void Model::addEvent(const GameEvent& e) {
  for (EventListener* l : eventGenerator->getListeners())
    l->onEvent(e);
}

