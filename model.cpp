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
#include "pantheon.h"
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
#include "entity_name.h"
#include "creature_attributes.h"
#include "view.h"
#include "view_index.h"
#include "stair_key.h"
#include "territory.h"
#include "game.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(levels)
    & SVAR(collectives)
    & SVAR(timeQueue)
    & SVAR(deadCreatures)
    & SVAR(currentTime)
    & SVAR(woodCount)
    & SVAR(game)
    & SVAR(lastTick)
    & SVAR(stairNavigation)
    & SVAR(cemetery);
}


SERIALIZABLE(Model);

void Model::addWoodCount(int cnt) {
  woodCount += cnt;
}

int Model::getWoodCount() const {
  return woodCount;
}

const Creature* Model::getPlayer() const {
  for (const PLevel& l : levels)
    if (l->getPlayer())
      return l->getPlayer();
  return nullptr;
}

vector<Collective*> Model::getCollectives() const {
  return extractRefs(collectives);
}

void Model::updateSunlightMovement() {
  for (PLevel& l : levels)
    l->updateSunlightMovement();
}

void Model::update(double totalTime) {
  Creature* creature = timeQueue->getNextCreature();
  CHECK(creature) << "No more creatures";
  //Debug() << creature->getName().the() << " moving now " << creature->getTime();
  currentTime = creature->getTime();
  if (currentTime > totalTime)
    return;
  while (totalTime > lastTick + 1) {
    lastTick += 1;
    tick(lastTick);
  }
  if (!creature->isDead()) {
#ifndef RELEASE
    CreatureAction::checkUsage(true);
#endif
    creature->makeMove();
#ifndef RELEASE
    CreatureAction::checkUsage(false);
#endif
  }
  for (PCollective& c : collectives)
    c->update(creature);
  if (!creature->isDead())
    CHECK(creature->getPosition().getCreature() == creature);
}

void Model::tick(double time) {
  for (Creature* c : timeQueue->getAllCreatures()) {
    c->tick(time);
  }
  for (PLevel& l : levels)
    l->tick(time);
  for (PCollective& col : collectives)
    col->tick(time);
}

void Model::addCreature(PCreature c, double delay) {
  c->setTime(timeQueue->getCurrentTime() + 1 + delay + Random.getDouble());
  timeQueue->addCreature(std::move(c));
}

Level* Model::buildLevel(LevelBuilder&& b, PLevelMaker maker) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(this, maker.get(), levels.size()));
  return levels.back().get();
}

Model::Model() {
  cemetery = LevelBuilder(Random, 100, 100, "Dead creatures", false)
      .build(this, LevelMaker::emptyLevel(Random).get(), 0);
}

Model::~Model() {
}

double Model::getTime() const {
  return currentTime;
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
  FAIL << "Failed to find next level for " << key.getInternalKey() << " " << from->getName();
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
          if (l2 != l1 && l2 != li && !stairNavigation.count(make_pair(l1, l2)) && stairNavigation.count(make_pair(li, l2)) &&
              stairNavigation.count(make_pair(l1, li)))
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

Position Model::getStairs(const Level* from, const Level* to) {
  CHECK(contains(getLevels(), from));
  CHECK(contains(getLevels(), to));
  CHECK(from != to);
  CHECK(stairNavigation.count({from, to})) << "No link " << from->getName() << " " << to->getName();
  return Random.choose(from->getLandingSquares(stairNavigation.at({from, to})));
}

vector<Level*> Model::getLevels() const {
  return extractRefs(levels);
}

void Model::killCreature(Creature* c, Creature* attacker) {
  if (attacker)
    attacker->onKilled(c);
  c->getTribe()->onMemberKilled(c, attacker);
  for (auto& col : collectives)
    col->onKilled(c, attacker);
  deadCreatures.push_back(timeQueue->removeCreature(c));
  cemetery->landCreature(cemetery->getAllPositions(), c);
}

