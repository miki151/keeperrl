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

template <class Archive> 
void Level::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(squares)
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
    & SVAR(lightAmount);
  CHECK_SERIAL;
}  

SERIALIZABLE(Level);

template <class Archive>
void Level::CoverInfo::serialize(Archive& ar, const unsigned int version) {
   ar& BOOST_SERIALIZATION_NVP(covered)
     & BOOST_SERIALIZATION_NVP(sunlight);
}

SERIALIZABLE(Level::CoverInfo);

SERIALIZATION_CONSTRUCTOR_IMPL(Level);

Level::Level(Table<PSquare> s, Model* m, vector<Location*> l, const string& message, const string& n,
    Table<CoverInfo> covers) 
    : squares(std::move(s)), locations(l), model(m), entryMessage(message), name(n), coverInfo(std::move(covers)),
      bucketMap(squares.getBounds().getW(), squares.getBounds().getH(), FieldOfView::sightRange),
      lightAmount(squares.getBounds(), 0) {
  for (Vec2 pos : squares.getBounds()) {
    squares[pos]->setLevel(this);
    Optional<pair<StairDirection, StairKey>> link = squares[pos]->getLandingLink();
    if (link)
      landingSquares[*link].push_back(pos);
  }
  for (Location *l : locations)
    l->setLevel(this);
  for (Vision* vision : Vision::getAll())
    fieldOfView.emplace(vision, FieldOfView(squares, vision));
  for (Vec2 pos : squares.getBounds())
    addLightSource(pos, squares[pos]->getLightEmission(), 1);
}

Rectangle Level::getMaxBounds() {
  return Rectangle(800, 800);
}

void Level::addCreature(Vec2 position, PCreature c) {
  Creature* ref = c.get();
  model->addCreature(std::move(c));
  putCreature(position, ref);
}

void Level::putCreature(Vec2 position, Creature* c) {
  creatures.push_back(c);
  CHECK(getSquare(position)->getCreature() == nullptr);
  c->setLevel(this);
  c->setPosition(position);
  //getSquare(position)->putCreatureSilently(c);
  getSquare(position)->putCreature(c);
  bucketMap.addElement(position, c);
  notifyLocations(c);
}
  
void Level::notifyLocations(Creature* c) {
  for (Location* l : locations)
    if (c->getPosition().inRectangle(l->getBounds()))
      l->onCreature(c);
}

void Level::addLightSource(Vec2 pos, double radius, int numLight) {
  if (radius > 0) {
    for (Vec2 v : getVisibleTilesNoDarkness(pos, Vision::get(VisionId::NORMAL))) {
      double dist = (v - pos).lengthD();
      if (dist <= radius)
        lightAmount[v] += min(1.0, 1 - (dist) / radius) * numLight;
    }
  }
}

void Level::replaceSquare(Vec2 pos, PSquare square) {
  if (contains(tickingSquares, getSquare(pos)))
    removeElement(tickingSquares, getSquare(pos));
  Creature* c = squares[pos]->getCreature();
  for (Item* it : squares[pos]->getItems())
    square->dropItem(squares[pos]->removeItem(it));
  squares[pos]->onConstructNewSquare(square.get());
  addLightSource(pos, squares[pos]->getLightEmission(), -1);
  square->setBackground(squares[pos].get());
  squares[pos] = std::move(square);
  squares[pos]->setPosition(pos);
  squares[pos]->setLevel(this);
  if (c) {
    squares[pos]->putCreatureSilently(c);
  }
  addLightSource(pos, squares[pos]->getLightEmission(), 1);
  updateVisibility(pos);
}

void Level::updateVisibility(Vec2 changedSquare) {
  for (Vec2 pos : getVisibleTilesNoDarkness(changedSquare, Vision::get(VisionId::NORMAL)))
    addLightSource(pos, squares[pos]->getLightEmission(), -1);
  for (auto& elem : fieldOfView)
    elem.second.squareChanged(changedSquare);
  for (Vec2 pos : getVisibleTilesNoDarkness(changedSquare, Vision::get(VisionId::NORMAL)))
    addLightSource(pos, squares[pos]->getLightEmission(), 1);
}

const Creature* Level::getPlayer() const {
  return player;
}

const Location* Level::getLocation(Vec2 pos) const {
  for (Location* l : locations)
    if (pos.inRectangle(l->getBounds()))
      return l;
  return nullptr;
}

const vector<Location*> Level::getAllLocations() const {
  return locations;
}

Level::CoverInfo Level::getCoverInfo(Vec2 pos) const {
  return coverInfo[pos];
}

const Model* Level::getModel() const {
  return model;
}

double Level::getSunlight(Vec2 pos) const {
  return coverInfo[pos].sunlight * model->getSunlightInfo().lightAmount;
}

double Level::getLight(Vec2 pos) const {
  return max(0.0, min(1.0, lightAmount[pos] + getSunlight(pos)));
}

vector<Vec2> Level::getLandingSquares(StairDirection dir, StairKey key) const {
  if (landingSquares.count({dir, key}))
    return landingSquares.at({dir, key});
  else
    return {};
}

Vec2 Level::landCreature(StairDirection direction, StairKey key, Creature* creature) {
  vector<Vec2> landing = landingSquares.at({direction, key});
  return landCreature(landing, creature);
}

Vec2 Level::landCreature(StairDirection direction, StairKey key, PCreature creature) {
  Vec2 pos = landCreature(direction, key, creature.get());
  model->addCreature(std::move(creature));
  return pos;
}

Vec2 Level::landCreature(vector<Vec2> landing, PCreature creature) {
  Vec2 pos = landCreature(landing, creature.get());
  model->addCreature(std::move(creature));
  return pos;
}

Vec2 Level::landCreature(vector<Vec2> landing, Creature* creature) {
  CHECK(creature);
  if (creature->isPlayer())
    player = creature;
  if (entryMessage != "") {
    creature->playerMessage(entryMessage);
    entryMessage = "";
  }
  queue<pair<Vec2, Vec2>> q;
  for (Vec2 pos : randomPermutation(landing))
    q.push(make_pair(pos, pos));
  while (!q.empty()) {
    pair<Vec2, Vec2> v = q.front();
    q.pop();
    if (squares[v.first]->canEnter(creature)) {
      putCreature(v.first, creature);
      return v.second;
    } else
      for (Vec2 next : v.first.neighbors8(true))
        if (next.inRectangle(squares.getBounds()) && squares[next]->canEnterEmpty(creature))
          q.push(make_pair(next, v.second));
  }
  FAIL << "Failed to find any square to put creature";
  return Vec2(0, 0);
}

void Level::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction, Vision* vision) {
  vector<PItem> v;
  v.push_back(std::move(item));
  throwItem(std::move(v), attack, maxDist, position, direction, vision);
}

void Level::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction,
    Vision* vision) {
  CHECK(!item.empty());
  int cnt = 1;
  vector<Vec2> trajectory;
  for (Vec2 v = position + direction;; v += direction) {
    trajectory.push_back(v);
    if (getSquare(v)->itemBounces(item[0].get(), vision)) {
        item[0]->onHitSquareMessage(v, getSquare(v), item.size() > 1);
        trajectory.pop_back();
        EventListener::addThrowEvent(this, attack.getAttacker(), item[0].get(), trajectory);
        if (!item[0]->isDiscarded())
          getSquare(v - direction)->dropItems(std::move(item));
        return;
    }
    if (++cnt > maxDist || getSquare(v)->itemLands(extractRefs(item), attack)) {
      EventListener::addThrowEvent(this, attack.getAttacker(), item[0].get(), trajectory);
      getSquare(v)->onItemLands(std::move(item), attack, maxDist - cnt - 1, direction, vision);
      return;
    }
  }
}

void Level::killCreature(Creature* creature) {
  removeElement(creatures, creature);
  getSquare(creature->getPosition())->removeCreature();
  model->removeCreature(creature);
  bucketMap.removeElement(creature->getPosition(), creature);
  if (creature->isPlayer())
    updatePlayer();
}

const static int hearingRange = 30;

void Level::globalMessage(Vec2 position, const string& ifPlayerCanSee, const string& cannot) const {
  if (player) {
    if (playerCanSee(position))
      player->playerMessage(ifPlayerCanSee);
    else if (player->getPosition().dist8(position) < hearingRange)
      player->playerMessage(cannot);
  }
}

void Level::globalMessage(const Creature* c, const string& ifPlayerCanSee, const string& cannot) const {
  if (player) {
    if (player->canSee(c))
      globalMessage(c->getPosition(), ifPlayerCanSee, cannot);
    else
      globalMessage(c->getPosition(), cannot, "");
  }
}

void Level::changeLevel(StairDirection dir, StairKey key, Creature* c) {
  Vec2 fromPosition = c->getPosition();
  removeElement(creatures, c);
  getSquare(c->getPosition())->removeCreature();
  bucketMap.removeElement(c->getPosition(), c);
  Vec2 toPosition = model->changeLevel(dir, key, c);
  EventListener::addChangeLevelEvent(c, this, fromPosition, c->getLevel(), toPosition);
}

void Level::changeLevel(Level* destination, Vec2 landing, Creature* c) {
  Vec2 fromPosition = c->getPosition();
  removeElement(creatures, c);
  getSquare(c->getPosition())->removeCreature();
  bucketMap.removeElement(c->getPosition(), c);
  model->changeLevel(destination, landing, c);
  EventListener::addChangeLevelEvent(c, this, fromPosition, destination, landing);
}

void Level::updatePlayer() {
  player = nullptr;
  for (Creature* c : creatures)
    if (c->isPlayer())
      player = c;
}

const vector<Creature*>& Level::getAllCreatures() const {
  return creatures;
}

vector<Creature*>& Level::getAllCreatures() {
  return creatures;
}

vector<Creature*> Level::getAllCreatures(Rectangle bounds) const {
  return bucketMap.getElements(bounds);
}

const int darkViewRadius = 5;

bool Level::isWithinVision(Vec2 from, Vec2 to, Vision* v) const {
  return v->isNightVision() || from.distD(to) <= darkViewRadius || getLight(to) > 0.3;
}

FieldOfView& Level::getFieldOfView(Vision* vision) const {
  return fieldOfView.at(vision);
}

bool Level::canSee(const Creature* c, Vec2 pos) const {
  return isWithinVision(c->getPosition(), pos, c->getVision())
    && getFieldOfView(c->getVision()).canSee(c->getPosition(), pos);
}

bool Level::playerCanSee(Vec2 pos) const {
  return player != nullptr && player->canSee(pos);
}

bool Level::playerCanSee(const Creature* c) const {
  return player != nullptr && player->canSee(c);
}

bool Level::canMoveCreature(const Creature* creature, Vec2 direction) const {
  Vec2 position = creature->getPosition();
  Vec2 destination = position + direction;
  if (!inBounds(destination))
    return false;
  return getSquare(destination)->canEnter(creature);
}

void Level::moveCreature(Creature* creature, Vec2 direction) {
  CHECK(canMoveCreature(creature, direction));
  Vec2 position = creature->getPosition();
  Square* nextSquare = getSquare(position + direction);
  Square* thisSquare = getSquare(position);
  thisSquare->removeCreature();
  creature->setPosition(position + direction);
  nextSquare->putCreature(creature);
  notifyLocations(creature);
  bucketMap.moveElement(position, position + direction, creature);
}

void Level::swapCreatures(Creature* c1, Creature* c2) {
  Vec2 position1 = c1->getPosition();
  Vec2 position2 = c2->getPosition();
  Square* square1 = getSquare(position1);
  Square* square2 = getSquare(position2);
  square1->removeCreature();
  square2->removeCreature();
  c1->setPosition(position2);
  c2->setPosition(position1);
  square1->putCreature(c2);
  square2->putCreature(c1);
  notifyLocations(c1);
  notifyLocations(c2);
  bucketMap.moveElement(position1, position2, c1);
  bucketMap.moveElement(position2, position1, c2);
}

vector<Vec2> Level::getVisibleTilesNoDarkness(Vec2 pos, Vision* vision) const {
  return getFieldOfView(vision).getVisibleTiles(pos);
}

vector<Vec2> Level::getVisibleTiles(Vec2 pos, Vision* vision) const {
  return filter(getFieldOfView(vision).getVisibleTiles(pos),
      [&](Vec2 v) { return isWithinVision(pos, v, vision); });
}

vector<Vec2> Level::getVisibleTiles(const Creature* c) const {
  static vector<Vec2> emptyVec;
  if (!c->isBlind())
    return getVisibleTiles(c->getPosition(), c->getVision());
  else
    return emptyVec;
}

unordered_map<Vec2, const ViewObject*> objectList;

void Level::setBackgroundLevel(const Level* l, Vec2 offs) {
  backgroundLevel = l;
  backgroundOffset = offs;
}

static unordered_map<Vec2, const ViewObject*> background;


const Square* Level::getSquare(Vec2 pos) const {
  CHECK(inBounds(pos));
  return squares[pos].get();
}

Square* Level::getSquare(Vec2 pos) {
  CHECK(inBounds(pos));
  return squares[pos].get();
}

void Level::addTickingSquare(Vec2 pos) {
  Square* s = squares[pos].get();
  if (!contains(tickingSquares, s))
    tickingSquares.push_back(s);
}
  
vector<Square*> Level::getTickingSquares() const {
  return tickingSquares;
}

Level::Builder::Builder(int width, int height, const string& n, bool covered)
  : squares(width, height), heightMap(width, height, 0),
    coverInfo(width, height, {covered, covered ? 0.0 : 1.0}), attrib(width, height),
    type(width, height, SquareType()), items(width, height), name(n) {
}

bool Level::Builder::hasAttrib(Vec2 posT, SquareAttrib attr) {
  Vec2 pos = transform(posT);
  CHECK(squares[pos] != nullptr);
  return attrib[pos].count(attr);
}

void Level::Builder::addAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].insert(attr);
}

void Level::Builder::removeAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].erase(attr);
}

Square* Level::Builder::getSquare(Vec2 pos) {
  return squares[transform(pos)].get();
}
    
SquareType Level::Builder::getType(Vec2 pos) {
  return type[transform(pos)];
}

void Level::Builder::putSquare(Vec2 pos, SquareType t, Optional<SquareAttrib> at) {
  putSquare(pos, SquareFactory::get(t), t, at);
}

void Level::Builder::putSquare(Vec2 pos, SquareType t, vector<SquareAttrib> at) {
  putSquare(pos, SquareFactory::get(t), t, at);
}

void Level::Builder::putSquare(Vec2 pos, PSquare square, SquareType t, Optional<SquareAttrib> attr) {
  putSquare(pos, std::move(square), t, attr ? vector<SquareAttrib>({*attr}) : vector<SquareAttrib>());
}

void Level::Builder::putSquare(Vec2 posT, PSquare square, SquareType t, vector<SquareAttrib> attr) {
  Vec2 pos = transform(posT);
  CHECK(!contains({SquareType::UP_STAIRS, SquareType::DOWN_STAIRS}, type[pos].id)) << "Attempted to overwrite stairs";
  square->setPosition(pos);
  if (squares[pos])
    square->setBackground(squares[pos].get());
  squares[pos] = std::move(square);
  for (SquareAttrib at : attr)
    attrib[pos].insert(at);
  type[pos] = t;
}

void Level::Builder::addLocation(Location* l, Rectangle area) {
  l->setBounds(area.apply([this](Vec2 v) { return transform(v); }));
  locations.push_back(l);
}

void Level::Builder::setHeightMap(Vec2 pos, double h) {
  heightMap[transform(pos)] = h;
}

double Level::Builder::getHeightMap(Vec2 pos) {
  return heightMap[transform(pos)];
}

void Level::Builder::putCreature(Vec2 pos, PCreature creature) {
  creature->setPosition(transform(pos));
  creatures.push_back(NOTNULL(std::move(creature)));
}

void Level::Builder::putItems(Vec2 posT, vector<PItem> it) {
  Vec2 pos = transform(posT);
  CHECK(squares[pos]->canEnterEmpty(Creature::getDefault()));
  append(items[pos], std::move(it));
}

bool Level::Builder::canPutCreature(Vec2 posT, Creature* c) {
  Vec2 pos = transform(posT);
  if (!squares[pos]->canEnter(c))
    return false;
  for (PCreature& c : creatures) {
    if (c->getPosition() == pos)
      return false;
  }
  return true;
}

void Level::Builder::setMessage(const string& message) {
  entryMessage = message;
}

PLevel Level::Builder::build(Model* m, LevelMaker* maker) {
  CHECK(mapStack.empty());
  maker->make(this, squares.getBounds());
  for (Vec2 v : heightMap.getBounds()) {
    squares[v]->setHeight(heightMap[v]);
    squares[v]->dropItems(std::move(items[v]));
  }
  PLevel l(new Level(std::move(squares), m, locations, entryMessage, name, std::move(coverInfo)));
  for (PCreature& c : creatures) {
    Vec2 pos = c->getPosition();
    l->addCreature(pos, std::move(c));
  }
  return l;
}

static Vec2::LinearMap identity() {
  return [](Vec2 v) { return v; };
}

static Vec2::LinearMap deg90(Rectangle bounds) {
  return [bounds](Vec2 v) {
    v -= bounds.getTopLeft();
    return bounds.getTopLeft() + Vec2(v.y, v.x);
  };
}

static Vec2::LinearMap deg180(Rectangle bounds) {
  return [bounds](Vec2 v) {
    return bounds.getTopLeft() - v + bounds.getBottomRight() - Vec2(1, 1);
  };
}

static Vec2::LinearMap deg270(Rectangle bounds) {
  return [bounds](Vec2 v) {
    v -= bounds.getTopRight() - Vec2(1, 0);
    return bounds.getTopLeft() + Vec2(v.y, -v.x);
  };
}

void Level::Builder::pushMap(Rectangle bounds, Rot rot) {
  switch (rot) {
    case CW0: mapStack.push_back(identity()); break;
    case CW1: mapStack.push_back(deg90(bounds)); break;
    case CW2: mapStack.push_back(deg180(bounds)); break;
    case CW3: mapStack.push_back(deg270(bounds)); break;
  }
}

void Level::Builder::popMap() {
  mapStack.pop_back();
}

Vec2 Level::Builder::transform(Vec2 v) {
  for (auto m : reverse2(mapStack)) {
    v = m(v);
  }
  return v;
}

void Level::Builder::setCoverInfo(Vec2 pos, CoverInfo info) {
  coverInfo[transform(pos)] = info;
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
