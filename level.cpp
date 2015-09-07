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

#include "level.h"
#include "location.h"
#include "model.h"
#include "item.h"
#include "creature.h"
#include "square.h"
#include "collective_builder.h"
#include "trigger.h"
#include "progress_meter.h"
#include "level_maker.h"
#include "movement_type.h"
#include "attack.h"
#include "player_message.h"
#include "vision.h"
#include "event.h"
#include "bucket_map.h"

template <class Archive> 
void Level::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(squares)
    & SVAR(oldSquares)
    & SVAR(landingSquares)
    & SVAR(locations)
    & SVAR(tickingSquares)
    & SVAR(creatures)
    & SVAR(model)
    & SVAR(fieldOfView)
    & SVAR(entryMessage)
    & SVAR(name)
    & SVAR(player)
    & SVAR(backgroundLevel)
    & SVAR(backgroundOffset)
    & SVAR(coverInfo)
    & SVAR(bucketMap)
    & SVAR(sectors)
    & SVAR(lightAmount)
    & SVAR(lightCapAmount)
    & SVAR(levelId)
    & SVAR(noDiagonalPassing);
}  

SERIALIZABLE(Level);

SERIALIZATION_CONSTRUCTOR_IMPL(Level);

Level::~Level() {}

Level::Level(Table<PSquare> s, Model* m, vector<Location*> l, const string& message, const string& n,
    Table<CoverInfo> covers, int id) 
    : squares(std::move(s)), oldSquares(squares.getBounds()), locations(l), model(m), entryMessage(message),
      name(n), coverInfo(std::move(covers)), bucketMap(squares.getBounds().getW(), squares.getBounds().getH(),
      FieldOfView::sightRange), lightAmount(squares.getBounds(), 0), lightCapAmount(squares.getBounds(), 1),
      levelId(id) {
  for (Vec2 pos : squares.getBounds()) {
    squares[pos]->setLevel(this);
    optional<StairKey> link = squares[pos]->getLandingLink();
    if (link)
      landingSquares[*link].push_back(Position(pos, this));
  }
  for (Location *l : locations)
    l->setLevel(this);
  for (VisionId vision : ENUM_ALL(VisionId))
    fieldOfView[vision] = FieldOfView(squares, vision);
  for (Vec2 pos : squares.getBounds())
    addLightSource(pos, squares[pos]->getLightEmission(), 1);
  updateSunlightMovement();
}

int Level::getUniqueId() const {
  return levelId;
}

Rectangle Level::getMaxBounds() {
  return Rectangle(300, 300);
}

Rectangle Level::getSplashBounds() {
  return Rectangle(80, 40);
}

Rectangle Level::getSplashVisibleBounds() {
  Vec2 sz(40, 20);
  return Rectangle(getSplashBounds().middle() - sz / 2, getSplashBounds().middle() + sz / 2);
}

void Level::addCreature(Vec2 position, PCreature c) {
  Creature* ref = c.get();
  model->addCreature(std::move(c));
  putCreature(position, ref);
}

const static double darknessRadius = 6.5;

void Level::putCreature(Vec2 position, Creature* c) {
  CHECK(inBounds(position));
  insertCreature(c);
  CHECK(getSafeSquare(position)->getCreature() == nullptr);
  bucketMap->addElement(position, c);
  c->setPosition(Position(position, this));
  getSafeSquare(position)->putCreature(c);
  if (c->isDarknessSource())
    addDarknessSource(position, darknessRadius);
  if (c->isPlayer())
    player = c;
}

void Level::addLightSource(Vec2 pos, double radius) {
  addLightSource(pos, radius, 1);
}

void Level::removeLightSource(Vec2 pos, double radius) {
  addLightSource(pos, radius, -1);
}

void Level::addLightSource(Vec2 pos, double radius, int numLight) {
  if (radius > 0) {
    for (Vec2 v : getVisibleTilesNoDarkness(pos, VisionId::NORMAL)) {
      double dist = (v - pos).lengthD();
      if (dist <= radius)
        lightAmount[v] += min(1.0, 1 - (dist) / radius) * numLight;
    }
  }
}

void Level::addDarknessSource(Vec2 pos, double radius) {
  addDarknessSource(pos, radius, 1);
}

void Level::removeDarknessSource(Vec2 pos, double radius) {
  addDarknessSource(pos, radius, -1);
}

void Level::addDarknessSource(Vec2 pos, double radius, int numDarkness) {
  if (radius > 0) {
    for (Vec2 v : getVisibleTilesNoDarkness(pos, VisionId::NORMAL)) {
      double dist = (v - pos).lengthD();
      if (dist <= radius)
        lightCapAmount[v] -= min(1.0, 1 - (dist) / radius) * numDarkness;
      squares[v]->updateSunlightMovement(isInSunlight(v));
      updateConnectivity(v);
    }
  }
}

void Level::removeSquare(Vec2 pos, PSquare defaultSquare) {
  if (!oldSquares[pos])
    replaceSquare(pos, std::move(defaultSquare), false);
  else
    replaceSquare(pos, std::move(oldSquares[pos]), false);
}

void Level::replaceSquare(Vec2 pos, PSquare square, bool storePrevious) {
  squares[pos]->onConstructNewSquare(square.get());
  Creature* c = squares[pos]->getCreature();
  if (c)
    squares[pos]->removeCreature();
  for (Item* it : copyOf(squares[pos]->getItems()))
    square->dropItem(squares[pos]->removeItem(it));
  addLightSource(pos, squares[pos]->getLightEmission(), -1);
  square->setPosition(pos);
  square->setLevel(this);
  for (PTrigger& t : squares[pos]->removeTriggers())
    square->addTrigger(std::move(t));
  square->setBackground(squares[pos].get());
  if (const Tribe* tribe = squares[pos]->getForbiddenTribe())
    square->forbidMovementForTribe(tribe);
  if (storePrevious)
    oldSquares[pos] = std::move(squares[pos]);
  squares[pos] = std::move(square);
  if (c) {
    squares[pos]->setCreature(c);
  }
  addLightSource(pos, squares[pos]->getLightEmission(), 1);
  updateVisibility(pos);
  squares[pos]->updateSunlightMovement(isInSunlight(pos));
  updateConnectivity(pos);
}

void Level::updateVisibility(Vec2 changedSquare) {
  for (Vec2 pos : getVisibleTilesNoDarkness(changedSquare, VisionId::NORMAL)) {
    addLightSource(pos, squares[pos]->getLightEmission(), -1);
    if (Creature* c = squares[pos]->getCreature())
      if (c->isDarknessSource())
        addDarknessSource(pos, darknessRadius, -1);
  }
  for (VisionId vision : ENUM_ALL(VisionId))
    fieldOfView[vision].squareChanged(changedSquare);
  for (Vec2 pos : getVisibleTilesNoDarkness(changedSquare, VisionId::NORMAL)) {
    addLightSource(pos, squares[pos]->getLightEmission(), 1);
    if (Creature* c = squares[pos]->getCreature())
      if (c->isDarknessSource())
        addDarknessSource(pos, darknessRadius, 1);
  }
}

const Creature* Level::getPlayer() const {
  return player;
}

const vector<Location*> Level::getAllLocations() const {
  return locations;
}

CoverInfo Level::getCoverInfo(Vec2 pos) const {
  return coverInfo[pos];
}

const Model* Level::getModel() const {
  return model;
}

Model* Level::getModel() {
  return model;
}

bool Level::isInSunlight(Vec2 pos) const {
  return !coverInfo[pos].covered && lightCapAmount[pos] == 1 &&
      model->getSunlightInfo().state == SunlightState::DAY;
}

double Level::getLight(Vec2 pos) const {
  return max(0.0, min(coverInfo[pos].covered ? 1 : lightCapAmount[pos], lightAmount[pos] +
        coverInfo[pos].sunlight * model->getSunlightInfo().lightAmount));
}

vector<Position> Level::getLandingSquares(StairKey key) const {
  if (landingSquares.count(key))
    return landingSquares.at(key);
  else
    return vector<Position>();
}

vector<StairKey> Level::getAllStairKeys() const {
  return getKeys(landingSquares);
}

bool Level::hasStairKey(StairKey key) const {
  return landingSquares.count(key);
}

optional<Position> Level::getStairsTo(const Level* level) const {
  return model->getStairs(this, level);
}

bool Level::landCreature(StairKey key, Creature* creature) {
  vector<Position> landing = landingSquares.at(key);
  return landCreature(landing, creature);
}

bool Level::landCreature(StairKey key, PCreature creature) {
  if (landCreature(key, creature.get())) {
    model->addCreature(std::move(creature));
    return true;
  } else
    return false;
}

bool Level::landCreature(vector<Position> landing, PCreature creature) {
  if (landCreature(landing, creature.get())) {
    model->addCreature(std::move(creature));
    return true;
  } else
    return false;
}

bool Level::landCreature(vector<Position> landing, Creature* creature) {
  CHECK(creature);
  if (creature->isPlayer())
    player = creature;
  if (entryMessage != "") {
    creature->playerMessage(entryMessage);
    entryMessage = "";
  }
  queue<Position> q;
  set<Position> marked;
  for (Position pos : Random.permutation(landing)) {
    q.push(pos);
    marked.insert(pos);
  }
  while (!q.empty()) {
    Position v = q.front();
    q.pop();
    if (v.canEnter(creature)) {
      v.putCreature(creature);
      return true;
    } else
      for (Position next : v.neighbors8(Random))
        if (!marked.count(next) && next.canEnterEmpty(creature)) {
          q.push(next);
          marked.insert(next);
        }
  }
  return false;
}

void Level::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction, VisionId vision) {
  vector<PItem> v;
  v.push_back(std::move(item));
  throwItem(std::move(v), attack, maxDist, position, direction, vision);
}

void Level::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction,
    VisionId vision) {
  CHECK(!item.empty());
  CHECK(direction.length8() == 1);
  int cnt = 1;
  vector<Vec2> trajectory;
  for (Vec2 v = position + direction;; v += direction) {
    trajectory.push_back(v);
    if (getSafeSquare(v)->itemBounces(item[0].get(), vision)) {
        item[0]->onHitSquareMessage(Position(v, this), item.size());
        trajectory.pop_back();
        GlobalEvents.addThrowEvent(this, attack.getAttacker(), item[0].get(), trajectory);
        if (!item[0]->isDiscarded())
          getSafeSquare(v - direction)->dropItems(std::move(item));
        return;
    }
    if (++cnt > maxDist || getSafeSquare(v)->itemLands(extractRefs(item), attack)) {
      GlobalEvents.addThrowEvent(this, attack.getAttacker(), item[0].get(), trajectory);
      getSafeSquare(v)->onItemLands(std::move(item), attack, maxDist - cnt - 1, direction, vision);
      return;
    }
  }
}

void Level::killCreature(Creature* creature) {
  bucketMap->removeElement(creature->getPosition().getCoord(), creature);
  eraseCreature(creature);
  creature->getPosition().removeCreature();
}

const static int hearingRange = 30;

void Level::globalMessage(Vec2 position, const PlayerMessage& ifPlayerCanSee, const PlayerMessage& cannot) const {
  if (player) {
    if (playerCanSee(position))
      player->playerMessage(ifPlayerCanSee);
    else if (player->getPosition().getCoord().dist8(position) < hearingRange)
      player->playerMessage(cannot);
  }
}

void Level::globalMessage(Vec2 position, const PlayerMessage& playerCanSee) const {
  globalMessage(position, playerCanSee, "");
}

void Level::globalMessage(const Creature* c, const PlayerMessage& ifPlayerCanSee, const PlayerMessage& cannot) const {
  if (player) {
    if (player->canSee(c))
      player->playerMessage(ifPlayerCanSee);
    else if (player->getPosition().dist8(c->getPosition()) < hearingRange)
      player->playerMessage(cannot);
  }
}

void Level::changeLevel(StairKey key, Creature* c) {
  Vec2 oldPos = c->getPosition().getCoord();
  if (model->changeLevel(key, c)) {
    eraseCreature(c);
    getSafeSquare(oldPos)->removeCreature();
    bucketMap->removeElement(oldPos, c);
  }
}

void Level::changeLevel(Position destination, Creature* c) {
  Vec2 oldPos = c->getPosition().getCoord();
  if (model->changeLevel(destination, c)) {
    eraseCreature(c);
    getSafeSquare(oldPos)->removeCreature();
    bucketMap->removeElement(oldPos, c);
  }
}

void Level::updatePlayer() {
  player = nullptr;
  for (Creature* c : creatures)
    if (c->isPlayer())
      player = c;
}

void Level::insertCreature(Creature* c) {
  creatures.push_back(c);
  if (c->isPlayer())
    player = c;
}

void Level::eraseCreature(Creature* c) {
  removeElement(creatures, c);
  if (c->isPlayer())
    player = nullptr;
}

const vector<Creature*>& Level::getAllCreatures() const {
  return creatures;
}

vector<Creature*>& Level::getAllCreatures() {
  return creatures;
}

vector<Creature*> Level::getAllCreatures(Rectangle bounds) const {
  return bucketMap->getElements(bounds);
}

const int darkViewRadius = 5;

bool Level::isWithinVision(Vec2 from, Vec2 to, VisionId v) const {
  return Vision::get(v)->isNightVision() || from.distD(to) <= darkViewRadius || getLight(to) > 0.3;
}

FieldOfView& Level::getFieldOfView(VisionId vision) const {
  return fieldOfView[vision];
}

bool Level::canSee(Vec2 from, Vec2 to, VisionId vision) const {
  return isWithinVision(from, to, vision) && getFieldOfView(vision).canSee(from, to);
}

bool Level::canSee(const Creature* c, Vec2 pos) const {
  return canSee(c->getPosition().getCoord(), pos, c->getVision());
}

bool Level::playerCanSee(Vec2 pos) const {
  return player != nullptr && player->canSee(pos);
}

bool Level::playerCanSee(const Creature* c) const {
  return player != nullptr && player->canSee(c);
}

static bool canPass(const Square* square, const Creature* c) {
  return square->canEnterEmpty(c) && (!square->getCreature() || !square->getCreature()->isStationary());
}

bool Level::canMoveCreature(const Creature* creature, Vec2 direction) const {
  Vec2 position = creature->getPosition().getCoord();
  Vec2 destination = position + direction;
  if (!inBounds(destination))
    return false;
  if (noDiagonalPassing && direction.isCardinal8() && !direction.isCardinal4() &&
      !canPass(getSafeSquare(position + Vec2(direction.x, 0)), creature) &&
      !canPass(getSafeSquare(position + Vec2(0, direction.y)), creature))
    return false;
  return getSafeSquare(destination)->canEnter(creature);
}

void Level::moveCreature(Creature* creature, Vec2 direction) {
  CHECK(canMoveCreature(creature, direction));
  Vec2 position = creature->getPosition().getCoord();
  bucketMap->moveElement(position, position + direction, creature);
  Square* nextSquare = getSafeSquare(position + direction);
  Square* thisSquare = getSafeSquare(position);
  thisSquare->removeCreature();
  creature->setPosition(Position(position + direction, this));
  nextSquare->putCreature(creature);
  if (creature->isAffected(LastingEffect::DARKNESS_SOURCE)) {
    addDarknessSource(position + direction, darknessRadius);
    removeDarknessSource(position, darknessRadius);
  }
}

void Level::swapCreatures(Creature* c1, Creature* c2) {
  Vec2 position1 = c1->getPosition().getCoord();
  Vec2 position2 = c2->getPosition().getCoord();
  bucketMap->moveElement(position1, position2, c1);
  bucketMap->moveElement(position2, position1, c2);
  Square* square1 = getSafeSquare(position1);
  Square* square2 = getSafeSquare(position2);
  square1->removeCreature();
  square2->removeCreature();
  c1->setPosition(Position(position2, this));
  c2->setPosition(Position(position1, this));
  square1->putCreature(c2);
  square2->putCreature(c1);
  if (c1->isAffected(LastingEffect::DARKNESS_SOURCE)) {
    addDarknessSource(position2, darknessRadius);
    removeDarknessSource(position1, darknessRadius);
  }
  if (c2->isAffected(LastingEffect::DARKNESS_SOURCE)) {
    addDarknessSource(position1, darknessRadius);
    removeDarknessSource(position2, darknessRadius);
  }
}

vector<Vec2> Level::getVisibleTilesNoDarkness(Vec2 pos, VisionId vision) const {
  return getFieldOfView(vision).getVisibleTiles(pos);
}

vector<Vec2> Level::getVisibleTiles(Vec2 pos, VisionId vision) const {
  return filter(getFieldOfView(vision).getVisibleTiles(pos),
      [&](Vec2 v) { return isWithinVision(pos, v, vision); });
}

unordered_map<Vec2, const ViewObject*> objectList;

void Level::setBackgroundLevel(const Level* l, Vec2 offs) {
  backgroundLevel = l;
  backgroundOffset = offs;
}

static unordered_map<Vec2, const ViewObject*> background;


const Square* Level::getSafeSquare(Vec2 pos) const {
  CHECK(inBounds(pos));
  return squares[pos].get();
}

Square* Level::getSafeSquare(Vec2 pos) {
  CHECK(inBounds(pos));
  return squares[pos].get();
}

Position Level::getPosition(Vec2 pos) const {
  return Position(pos, const_cast<Level*>(this)); 
}

vector<Position> Level::getAllPositions() const {
  vector<Position> ret;
  for (Vec2 v : getBounds())
    ret.emplace_back(v, const_cast<Level*>(this)); 
  return ret;
}

void Level::addTickingSquare(Vec2 pos) {
  tickingSquares.insert(pos);
}

void Level::tick(double time) {
  for (Vec2 pos : tickingSquares)
    squares[pos]->tick(time);
}

bool Level::inBounds(Vec2 pos) const {
  return pos.inRectangle(getBounds());
}

Rectangle Level::getBounds() const {
  return Rectangle(0, 0, getWidth(), getHeight());
}

int Level::getWidth() const {
  return squares.getWidth();
}

int Level::getHeight() const {
  return squares.getHeight();
}
const string& Level::getName() const {
  return name;
}

void Level::updateConnectivity(Vec2 pos) {
  for (auto& elem : sectors)
    if (getSafeSquare(pos)->canNavigate(elem.first))
      elem.second.add(pos);
    else
      elem.second.remove(pos);
}

bool Level::areConnected(Vec2 p1, Vec2 p2, const MovementType& movement) const {
  return inBounds(p1) && inBounds(p2) && getSectors(movement).same(p1, p2);
}

Sectors& Level::getSectors(const MovementType& movement) const {
  if (!sectors.count(movement)) {
    sectors[movement] = Sectors(getBounds());
    Sectors& newSectors = sectors.at(movement);
    for (Vec2 v : getBounds())
      if (getSafeSquare(v)->canNavigate(movement))
        newSectors.add(v);
  }
  return sectors.at(movement);
}

bool Level::isChokePoint(Vec2 pos, const MovementType& movement) const {
  return getSectors(movement).isChokePoint(pos);
}

void Level::updateSunlightMovement() {
  for (Vec2 v : getBounds())
    squares[v]->updateSunlightMovement(isInSunlight(v));
  sectors.clear();
}

