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
#include "territory.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) {
  CHECK(!serializationLocked);
  ar & SUBCLASS(OwnedObject<Model>);
  ar(levels, collectives, timeQueue, deadCreatures, currentTime, woodCount, game, lastTick, biomeId);
  ar(stairNavigation, cemetery, mainLevels, upLevels, eventGenerator, externalEnemies, defaultMusic);
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
  for (auto& l : levels)
    for (auto v : l->territory.getBounds())
      l->territory[v] = nullptr;
  for (auto& col : collectives)
    for (auto& pos : col->getTerritory().getAll())
      pos.getLevel()->territory[pos.getCoord()] = col.get();
  if (externalEnemies)
    externalEnemies->update(getGroundLevel(), time);
  stairNavigation.clear();
}

void Model::addCreature(PCreature c) {
  addCreature(std::move(c), 1_visible);// + Random.getDouble());
}

void Model::addCreature(PCreature c, TimeInterval delay) {
  if (auto game = getGame())
    c->setGlobalTime(getGame()->getGlobalTime());
  timeQueue->addCreature(std::move(c), getLocalTime() + delay);
}

Level* Model::buildLevel(const ContentFactory* factory, LevelBuilder b, PLevelMaker maker, int depth, string name) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(factory, this, maker.get(), Random.getLL()));
  levels.back()->depth = depth;
  levels.back()->name = name;
  return levels.back().get();
}

Level* Model::buildUpLevel(const ContentFactory* factory, LevelBuilder b, PLevelMaker maker) {
  int depth = -upLevels.size() - 1;
  auto ret = buildLevel(factory, std::move(b), std::move(maker), depth, "Z-level " + toString(depth));
  ret->below = upLevels.empty() ? mainLevels[0] : upLevels.back();
  ret->below->above = ret;
  upLevels.push_back(ret);
  return ret;
}

Level* Model::buildMainLevel(const ContentFactory* factory, LevelBuilder b, PLevelMaker maker) {
  int depth = mainLevels.size();
  auto ret = buildLevel(factory, std::move(b), std::move(maker), depth, depth == 0 ? "Ground"_s : "Z-level " + toString(depth));
  mainLevels.push_back(ret);
  return ret;
}

Model::Model(Private) {
}

PModel Model::create(ContentFactory* contentFactory, optional<MusicType> music, BiomeId biomeId) {
  auto ret = makeOwner<Model>(Private{});
  ret->cemetery = LevelBuilder(Random, contentFactory, 100, 100, false)
      .build(contentFactory, ret.get(), LevelMaker::emptyLevel(FurnitureType("GRASS"), false).get(), Random.getLL());
  ret->eventGenerator = makeOwner<EventGenerator>();
  ret->defaultMusic = music;
  ret->biomeId = biomeId;
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

Level* Model::getLinkedLevel(Level* from, StairKey key) const {
  for (Level* target : getLevels())
    if (target != from && target->hasStairKey(key))
      return target;
  //FATAL << "Failed to find next level for " << key.getInternalKey() << " " << from->getName();
  return nullptr;
}

BiomeId Model::getBiomeId() const {
  return biomeId;
}

Model::StairConnections Model::createStairConnections(const MovementType& movement) const {
  PROFILE;
  StairConnections connections;
  auto join = [&] (StairKey k1, StairKey k2) {
    int oldKey = connections.at(k2);
    int newKey = connections.at(k1);
    if (oldKey != newKey)
      for (auto& elem : connections)
        if (elem.second == oldKey)
          elem.second = newKey;
  };
  int cnt = 0;
  for (auto level : getLevels())
    for (auto key : level->getAllStairKeys())
      if (!connections.count(key))
        connections[key] = ++cnt;
  for (auto level : getLevels())
    for (auto key1 : level->getAllStairKeys())  
      for (auto key2 : level->getAllStairKeys())
        if (key1 != key2) {
          auto pos1 = level->getLandingSquares(key1)[0];
          auto pos2 = level->getLandingSquares(key2)[0];
          if (pos1.isConnectedTo(pos2, movement))
            join(key1, key2);
        }
  return connections;
}

void Model::calculateStairNavigation() {
  stairNavigation.clear();
}

bool Model::areConnected(StairKey key1, StairKey key2, const MovementType& movement) {
  auto& connections = [&]() -> StairConnections& {
    if (auto ret = getReferenceMaybe(stairNavigation, movement))
      return *ret;
    auto ret = stairNavigation.insert(make_pair(movement, createStairConnections(movement)));
    return ret.first->second;
  }();
  return connections.at(key1) == connections.at(key2);
}

vector<Level*> Model::getLevels() const {
  return getWeakPointers(levels);
}

vector<Level*> Model::getAllMainLevels() const {
  return concat(upLevels.reverse(), mainLevels);
}

optional<int> Model::getMainLevelDepth(const Level* l) const {
  if (auto index = upLevels.findElement(l))
    return -*index - 1;
  return mainLevels.findElement(l);
}

Range Model::getMainLevelsDepth() const {
  return Range(-upLevels.size(), mainLevels.size());
}

Level* Model::getMainLevel(int depth) const {
  if (depth < 0)
    return upLevels[-depth - 1];
  return mainLevels[depth];
}

void Model::addCollective(PCollective col) {
  collectives.push_back(std::move(col));
  if (game)
    game->addCollective(collectives.back().get());
}

Level* Model::getGroundLevel() const {
  return mainLevels[0];
}

LevelId Model::getUniqueId() const {
  return mainLevels[0]->getUniqueId();
}

void Model::killCreature(Creature* c) {
  deadCreatures.push_back(timeQueue->removeCreature(c));
  cemetery->landCreature(cemetery->getAllPositions(), c);
}

void Model::killCreature(PCreature c) {
  auto ref = c.get();
  deadCreatures.push_back(std::move(c));
  cemetery->landCreature(cemetery->getAllPositions(), ref);
}

PCreature Model::extractCreature(Creature* c) {
  PCreature ret = timeQueue->removeCreature(c);
  c->getLevel()->eraseCreature(c, c->getPosition().getCoord());
  return ret;
}

void Model::transferCreature(PCreature c, Vec2 travelDir) {
  Creature* ref = c.get();
  addCreature(std::move(c));
  CHECK(getGroundLevel()->landCreature(StairKey::transferLanding(), ref, travelDir));
}

void Model::transferCreature(PCreature c, const vector<Position>& destinations) {
  Creature* ref = c.get();
  addCreature(std::move(c));
  CHECK(getGroundLevel()->landCreature(destinations, ref));
}

bool Model::canTransferCreature(Creature* c, Vec2 travelDir) {
  for (Position pos : getGroundLevel()->getLandingSquares(StairKey::transferLanding()))
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
  Level* target = getGroundLevel();
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
