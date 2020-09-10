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
#include "unknown_locations.h"
#include "avatar_info.h"
#include "collective_config.h"
#include "monster.h"
#include "monster_ai.h"
#include "warlord_controller.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) {
  CHECK(!serializationLocked);
  ar & SUBCLASS(OwnedObject<Model>);
  ar(levels, collectives, timeQueue, deadCreatures, currentTime, woodCount, game, lastTick);
  ar(stairNavigation, cemetery, mainLevels, eventGenerator, externalEnemies, defaultMusic);
}

SERIALIZATION_CONSTRUCTOR_IMPL(Model)

SERIALIZABLE(Model)

void Model::discardForRetirement() {
  serializationLocked = true;
  deadCreatures.clear();
}

void Model::prepareForRetirement() {
  for (Creature* c : timeQueue->getAllCreatures())
    c->clearInfoForRetiring();
  for (PCreature& c : deadCreatures)
    c->clearInfoForRetiring();
  externalEnemies = none;
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

vector<Collective*> Model::getCollectives() const {
  return getWeakPointers(collectives);
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

bool Model::update(double totalTime) {
  currentTime = totalTime;
  if (Creature* creature = timeQueue->getNextCreature(totalTime)) {
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced before processing: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (creature->isDead()) {
      checkCreatureConsistency();
      FATAL << "Dead: " << creature->getName().bare();
    }
    while (totalTime > lastTick.getDouble()) {
      lastTick += 1_visible;
      tick(lastTick);
    }
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced before moving: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (!creature->isDead()) {
      INFO << "Turn " << totalTime << " " << creature->getName().bare() << " moving now";
      creature->makeMove();
    }
    CHECK(creature->getLevel() != nullptr) << "Creature misplaced after moving: " << creature->getName().bare() <<
        ". Any idea why this happened?";
    if (!creature->isDead() && creature->getLevel()->getModel() == this)
      CHECK(creature->getPosition().getCreature() == creature);
    return true;
  }
  return false;
}

void Model::tick(LocalTime time) { PROFILE
  for (Creature* c : timeQueue->getAllCreatures()) {
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
  if (auto game = getGame())
    c->setGlobalTime(getGame()->getGlobalTime());
  timeQueue->addCreature(std::move(c), getLocalTime() + delay);
}

WLevel Model::buildLevel(LevelBuilder b, PLevelMaker maker, int depth, optional<string> name) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(this, maker.get(), Random.getLL()));
  levels.back()->depth = depth;
  levels.back()->name = name;
  return levels.back().get();
}

WLevel Model::buildMainLevel(LevelBuilder b, PLevelMaker maker) {
  int depth = mainLevels.size();
  auto ret = buildLevel(std::move(b), std::move(maker), depth, depth == 0 ? optional<string>("Ground") : none);
  mainLevels.push_back(ret);
  return ret;
}

Model::Model(Private) {
}

PModel Model::create(ContentFactory* contentFactory, optional<MusicType> music) {
  auto ret = makeOwner<Model>(Private{});
  ret->cemetery = LevelBuilder(Random, contentFactory, 100, 100, false)
      .build(ret.get(), LevelMaker::emptyLevel(FurnitureType("GRASS"), false).get(), Random.getLL());
  ret->eventGenerator = makeOwner<EventGenerator>();
  ret->defaultMusic = music;
  return ret;
}

Model::~Model() {
}

LocalTime Model::getLocalTime() const {
  return LocalTime((int) currentTime);
}

double Model::getLocalTimeDouble() const {
  return currentTime;
}

TimeQueue& Model::getTimeQueue() {
  return *timeQueue;
}

int Model::getMoveCounter() const {
  return moveCounter;
}

void Model::increaseMoveCounter() {
  ++moveCounter;
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
  //FATAL << "Failed to find next level for " << key.getInternalKey() << " " << from->getName();
  return nullptr;
}

static pair<LevelId, LevelId> getIds(WConstLevel l1, WConstLevel l2) {
  return {l1->getUniqueId(), l2->getUniqueId()};
}

void Model::calculateStairNavigation() {
  stairNavigation.clear();
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
            "No stair path between levels ";// << l1->getName() << " " << l2->getName();
}

optional<StairKey> Model::getStairsBetween(WConstLevel from, WConstLevel to) const {
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

const vector<WLevel>& Model::getMainLevels() const {
  return mainLevels;
}

void Model::addCollective(PCollective col) {
  collectives.push_back(std::move(col));
  if (game)
    game->addCollective(collectives.back().get());
}

WLevel Model::getTopLevel() const {
  return mainLevels[0];
}

LevelId Model::getUniqueId() const {
  return mainLevels[0]->getUniqueId();
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
  Creature* ref = c.get();
  addCreature(std::move(c));
  CHECK(getTopLevel()->landCreature(StairKey::transferLanding(), ref, travelDir));
}

void Model::transferCreature(PCreature c, const vector<Position>& destinations) {
  Creature* ref = c.get();
  addCreature(std::move(c));
  CHECK(getTopLevel()->landCreature(destinations, ref));
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

const vector<PCreature>& Model::getDeadCreatures() const {
  return deadCreatures;
}

void Model::landWarlord(vector<PCreature> player) {
  auto ref = getWeakPointers(player);
  for (auto& c : player) {
    c->setUniqueId(UniqueEntity<Creature>::Id());
    transferCreature(std::move(c), Vec2(1, 0));
  }
  auto team = make_shared<vector<Creature*>>(ref);
  auto teamOrders = make_shared<EnumSet<TeamOrder>>();
  for (int i : All(ref))
    ref[i]->setController(Monster::getFactory(MonsterAIFactory::warlord(team, teamOrders)).get(ref[i]));
  ref[0]->pushController(getWarlordController(team, teamOrders));
}

void Model::landHeroPlayer(PCreature player) {
  Creature* ref = player.get();
  WLevel target = getTopLevel();
  vector<Position> landing = target->getLandingSquares(StairKey::heroSpawn());
  if (!target->landCreature(landing, ref))
    CHECK(target->landCreature(target->getAllLandingPositions(), ref)) << "No place to spawn player";
  addCreature(std::move(player));
  ref->setController(makeOwner<Player>(ref, true, make_shared<MapMemory>(), make_shared<MessageBuffer>(),
      make_shared<VisibilityMap>(), make_shared<UnknownLocations>()));
}

void Model::addExternalEnemies(ExternalEnemies e) {
  externalEnemies = std::move(e);
}

const heap_optional<ExternalEnemies>& Model::getExternalEnemies() const {
  return externalEnemies;
}

void Model::addEvent(const GameEvent& e) {
  PROFILE;
  eventGenerator->addEvent(e);
}

optional<MusicType> Model::getDefaultMusic() const {
  return defaultMusic;
}
