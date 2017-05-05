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
#include "model.h"
#include "item.h"
#include "creature.h"
#include "square.h"
#include "collective_builder.h"
#include "progress_meter.h"
#include "level_maker.h"
#include "movement_type.h"
#include "attack.h"
#include "player_message.h"
#include "vision.h"
#include "bucket_map.h"
#include "creature_name.h"
#include "sunlight_info.h"
#include "game.h"
#include "creature_attributes.h"
#include "square_array.h"
#include "view_object.h"
#include "field_of_view.h"
#include "furniture.h"
#include "furniture_array.h"

template <class Archive> 
void Level::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Level>);
  ar(squares, oldSquares, landingSquares, tickingSquares, creatures, model, fieldOfView);
  ar(name, backgroundOffset, sunlight, bucketMap, sectors, lightAmount, unavailable);
  ar(levelId, noDiagonalPassing, lightCapAmount, creatureIds, background, memoryUpdates);
  ar(furniture, tickingFurniture, covered);
}  

SERIALIZABLE(Level);

SERIALIZATION_CONSTRUCTOR_IMPL(Level);

Level::~Level() {}

Level::Level(Private, SquareArray s, FurnitureArray f, WModel m, const string& n,
    Table<double> sun, LevelId id, Table<bool> cover)
    : squares(std::move(s)), oldSquares(squares->getBounds()), furniture(std::move(f)),
      memoryUpdates(squares->getBounds(), true), model(m),
      name(n), sunlight(sun), covered(cover), bucketMap(squares->getBounds().width(), squares->getBounds().height(),
      FieldOfView::sightRange), lightAmount(squares->getBounds(), 0), lightCapAmount(squares->getBounds(), 1),
      levelId(id) {
}

PLevel Level::create(SquareArray s, FurnitureArray f, WModel m, const string& n,
    Table<double> sun, LevelId id, Table<bool> cover) {
  auto ret = makeOwner<Level>(Private{}, std::move(s), std::move(f), m, n, sun, id, cover);
  for (Vec2 pos : ret->squares->getBounds()) {
    auto square = ret->squares->getReadonly(pos);
    square->onAddedToLevel(Position(pos, ret.get()));
    if (optional<StairKey> link = square->getLandingLink())
      ret->landingSquares[*link].push_back(Position(pos, ret.get()));
  }
  for (VisionId vision : ENUM_ALL(VisionId))
    (*ret->fieldOfView)[vision] = FieldOfView(ret.get(), vision);
  for (auto pos : ret->getAllPositions())
    ret->addLightSource(pos.getCoord(), pos.getLightEmission(), 1);
  return ret;
}

LevelId Level::getUniqueId() const {
  return levelId;
}

Rectangle Level::getMaxBounds() {
  return Rectangle(360, 360);
}

Rectangle Level::getSplashBounds() {
  return Rectangle(80, 40);
}

Rectangle Level::getSplashVisibleBounds() {
  Vec2 sz(40, 20);
  return Rectangle(getSplashBounds().middle() - sz / 2, getSplashBounds().middle() + sz / 2);
}

const static double darknessRadius = 3.5;

void Level::putCreature(Vec2 position, WCreature c) {
  CHECK(inBounds(position));
  creatures.push_back(c);
  creatureIds.insert(c);
  CHECK(getSafeSquare(position)->getCreature() == nullptr)
      << "Square occupied by " << getSafeSquare(position)->getCreature()->getName().bare();
  placeCreature(c, position);
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
      if (dist <= radius) {
        lightAmount[v] += min(1.0, 1 - (dist) / radius) * numLight;
        setNeedsRenderUpdate(v, true);
      }
    }
  }
}

void Level::addDarknessSource(Vec2 pos, double radius, int numDarkness) {
  if (radius > 0) {
    for (Vec2 v : getVisibleTilesNoDarkness(pos, VisionId::NORMAL)) {
      double dist = (v - pos).lengthD();
      if (dist <= radius) {
        lightCapAmount[v] -= min(1.0, 1 - (dist) / radius) * numDarkness;
        setNeedsRenderUpdate(v, true);
      }
//      updateConnectivity(v);
    }
  }
}

void Level::updateVisibility(Vec2 changedSquare) {
  for (Vec2 pos : getVisibleTilesNoDarkness(changedSquare, VisionId::NORMAL)) {
    addLightSource(pos, Position(pos, this).getLightEmission(), -1);
    auto square = squares->getReadonly(pos);
    CHECK(square) << pos << " " << getBounds();
    if (WCreature c = square->getCreature())
      if (c->isDarknessSource())
        addDarknessSource(pos, darknessRadius, -1);
  }
  for (VisionId vision : ENUM_ALL(VisionId))
    getFieldOfView(vision).squareChanged(changedSquare);
  for (Vec2 pos : getVisibleTilesNoDarkness(changedSquare, VisionId::NORMAL)) {
    addLightSource(pos, Position(pos, this).getLightEmission(), 1);
    auto square = squares->getReadonly(pos);
    CHECK(square) << pos << " " << getBounds();
    if (WCreature c = square->getCreature())
      if (c->isDarknessSource())
        addDarknessSource(pos, darknessRadius, 1);
  }
}

WCreature Level::getPlayer() const {
  if (auto game = model->getGame())
    if (auto player = game->getPlayer())
      if (player->getLevel() == this)
        return player;
  return nullptr;
}

const WModel Level::getModel() const {
  return model;
}

WModel Level::getModel() {
  return model;
}

WGame Level::getGame() const {
  return model->getGame();
}

bool Level::isInSunlight(Vec2 pos) const {
  return !covered[pos] && lightCapAmount[pos] == 1 &&
      getGame()->getSunlightInfo().getState() == SunlightState::DAY;
}

double Level::getLight(Vec2 pos) const {
  return max(0.0, min(covered[pos] ? 1 : lightCapAmount[pos], lightAmount[pos] +
      sunlight[pos] * getGame()->getSunlightInfo().getLightAmount()));
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

optional<Position> Level::getStairsTo(WConstLevel level) {
  return model->getStairs(this, level);
}

bool Level::landCreature(StairKey key, WCreature creature) {
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

static Vec2 projectOnBorders(Rectangle area, Vec2 d) {
  Vec2 center = Vec2((area.left() + area.right()) / 2, (area.top() + area.bottom()) / 2);
  if (d.x == 0) {
    return Vec2(center.x, d.y > 0 ? area.bottom() - 1 : area.top());
  }
  int cy = d.y * area.width() / 2 / abs(d.x);
  if (center.y + cy >= area.top() && center.y + cy < area.bottom())
    return Vec2(d.x > 0 ? area.right() - 1 : area.left(), center.y + cy);
  int cx = d.x * area.height() / 2 / abs(d.y);
  return Vec2(center.x + cx, d.y > 0 ? area.bottom() - 1: area.top());
}

Position Level::getLandingSquare(StairKey key, Vec2 travelDir) const {
  vector<Position> landing = landingSquares.at(key);
  Vec2 entryPos = projectOnBorders(getBounds(), travelDir);
  Position target = landing.at(0);
  for (Position p : landing)
    if (p.getCoord().distD(entryPos) < target.getCoord().distD(entryPos))
      target = p;
  return target;
}

bool Level::landCreature(StairKey key, WCreature creature, Vec2 travelDir) {
  Position bestLanding = getLandingSquare(key, travelDir);
  return landCreature({bestLanding}, creature) ||
      landCreature(bestLanding.getRectangle(Rectangle::centered(Vec2(0, 0), 10)), creature) ||
      landCreature(landingSquares.at(key), creature) ||
      landCreature(getAllPositions(), creature);
}

bool Level::landCreature(vector<Position> landing, PCreature creature) {
  if (landCreature(landing, creature.get())) {
    model->addCreature(std::move(creature));
    return true;
  } else
    return false;
}

bool Level::landCreature(vector<Position> landing, WCreature creature) {
  CHECK(creature);
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
  for (Vec2 v = position + direction; inBounds(v); v += direction) {
    trajectory.push_back(v);
    Position pos(v, this);
    if (!pos.canSeeThru(vision)) {
      item[0]->onHitSquareMessage(Position(v, this), item.size());
      trajectory.pop_back();
      getGame()->addEvent({EventId::ITEMS_THROWN, EventInfo::ItemsThrown{this, getWeakPointers(item), trajectory}});
      if (!item[0]->isDiscarded())
        pos.minus(direction).dropItems(std::move(item));
      return;
    }
    if (++cnt > maxDist || getSafeSquare(v)->itemLands(getWeakPointers(item), attack)) {
      getGame()->addEvent({EventId::ITEMS_THROWN, EventInfo::ItemsThrown{this, getWeakPointers(item), trajectory}});
      modSafeSquare(v)->onItemLands(Position(v, this), std::move(item), attack, maxDist - cnt - 1, direction,
          vision);
      return;
    }
  }
}

void Level::killCreature(WCreature creature) {
  eraseCreature(creature, creature->getPosition().getCoord());
  getModel()->killCreature(creature);
}

void Level::removeCreature(WCreature creature) {
  eraseCreature(creature, creature->getPosition().getCoord());
}

const static int hearingRange = 30;

void Level::globalMessage(Vec2 position, const PlayerMessage& ifPlayerCanSee, const PlayerMessage& cannot) const {
  if (WCreature player = getPlayer()) {
    if (playerCanSee(position))
      player->playerMessage(ifPlayerCanSee);
    else if (player->getPosition().getCoord().dist8(position) < hearingRange)
      player->playerMessage(cannot);
  }
}

void Level::globalMessage(Vec2 position, const PlayerMessage& playerCanSee) const {
  globalMessage(position, playerCanSee, "");
}

void Level::globalMessage(WConstCreature c, const PlayerMessage& ifPlayerCanSee, const PlayerMessage& cant) const {
  if (WCreature player = getPlayer()) {
    if (player->canSee(c))
      player->playerMessage(ifPlayerCanSee);
    else if (player->getPosition().dist8(c->getPosition()) < hearingRange)
      player->playerMessage(cant);
  }
}

void Level::changeLevel(StairKey key, WCreature c) {
  Vec2 oldPos = c->getPosition().getCoord();
  WLevel otherLevel = model->getLinkedLevel(this, key);
  if (otherLevel->landCreature(key, c))
    eraseCreature(c, oldPos);
  else {
    Position otherPos = Random.choose(otherLevel->landingSquares.at(key));
    if (WCreature other = otherPos.getCreature()) {
      if (!other->isPlayer() && c->getPosition().canEnterEmpty(other) && otherPos.canEnterEmpty(c)) {
        otherLevel->eraseCreature(other, otherPos.getCoord());
        eraseCreature(c, oldPos);
        putCreature(oldPos, other);
        otherLevel->putCreature(otherPos.getCoord(), c);
        c->playerMessage("You switch levels with " + other->getName().a());
      }
    }
  }
}

void Level::changeLevel(Position destination, WCreature c) {
  Vec2 oldPos = c->getPosition().getCoord();
  if (destination.isValid() && destination.getLevel()->landCreature({destination}, c))
    eraseCreature(c, oldPos);
}

void Level::eraseCreature(WCreature c, Vec2 coord) {
  removeElement(creatures, c);
  unplaceCreature(c, coord);
  creatureIds.erase(c);
}

const vector<WCreature>& Level::getAllCreatures() const {
  return creatures;
}

vector<WCreature>& Level::getAllCreatures() {
  return creatures;
}

vector<WCreature> Level::getAllCreatures(Rectangle bounds) const {
  return bucketMap->getElements(bounds);
}

bool Level::containsCreature(UniqueEntity<Creature>::Id id) const {
  return creatureIds.contains(id);
}

const int darkViewRadius = 5;

bool Level::isWithinVision(Vec2 from, Vec2 to, VisionId v) const {
  return Vision::get(v)->isNightVision() || from.distD(to) <= darkViewRadius || getLight(to) > 0.3;
}

FieldOfView& Level::getFieldOfView(VisionId vision) const {
  return (*fieldOfView)[vision];
}

bool Level::canSee(Vec2 from, Vec2 to, VisionId vision) const {
  return isWithinVision(from, to, vision) && getFieldOfView(vision).canSee(from, to);
}

bool Level::canSee(WConstCreature c, Vec2 pos) const {
  return canSee(c->getPosition().getCoord(), pos, c->getVision());
}

bool Level::playerCanSee(Vec2 pos) const {
  if (WCreature player = getPlayer())
    return player->canSee(pos);
  else
    return false;
}

bool Level::playerCanSee(WConstCreature c) const {
  if (WCreature player = getPlayer())
    return player->canSee(c);
  else
    return false;
}

void Level::moveCreature(WCreature creature, Vec2 direction) {
//  CHECK(canMoveCreature(creature, direction));
  Vec2 position = creature->getPosition().getCoord();
  unplaceCreature(creature, position);
  placeCreature(creature, position + direction);
}

void Level::unplaceCreature(WCreature creature, Vec2 pos) {
  bucketMap->removeElement(pos, creature);
  modSafeSquare(pos)->removeCreature(Position(pos, this));
  if (creature->isDarknessSource())   
    addDarknessSource(pos, darknessRadius, -1);
}

void Level::placeCreature(WCreature creature, Vec2 pos) {
  Position position(pos, this);
  creature->setPosition(position);
  bucketMap->addElement(pos, creature);
  modSafeSquare(pos)->putCreature(creature);
  if (creature->isDarknessSource())
    addDarknessSource(pos, darknessRadius, 1);
  position.onEnter(creature);
}

void Level::swapCreatures(WCreature c1, WCreature c2) {
  Vec2 pos1 = c1->getPosition().getCoord();
  Vec2 pos2 = c2->getPosition().getCoord();
  unplaceCreature(c1, pos1);
  unplaceCreature(c2, pos2);
  placeCreature(c1, pos2);
  placeCreature(c2, pos1);
}

vector<Vec2> Level::getVisibleTilesNoDarkness(Vec2 pos, VisionId vision) const {
  return getFieldOfView(vision).getVisibleTiles(pos);
}

vector<Vec2> Level::getVisibleTiles(Vec2 pos, VisionId vision) const {
  return filter(getFieldOfView(vision).getVisibleTiles(pos),
      [&](Vec2 v) { return isWithinVision(pos, v, vision); });
}

WConstSquare Level::getSafeSquare(Vec2 pos) const {
  CHECK(inBounds(pos));
  return squares->getReadonly(pos);
}

WSquare Level::modSafeSquare(Vec2 pos) {
  CHECK(inBounds(pos));
  return squares->getWritable(pos);
}

vector<Position> Level::getAllPositions() const {
  vector<Position> ret;
  for (Vec2 v : getBounds())
    ret.emplace_back(v, getThis().removeConst());
  return ret;
}

void Level::addTickingSquare(Vec2 pos) {
  tickingSquares.insert(pos);
}

void Level::addTickingFurniture(Vec2 pos) {
  tickingFurniture.insert(pos);
}

void Level::tick() {
  for (Vec2 pos : tickingSquares)
    squares->getWritable(pos)->tick(Position(pos, this));
  for (Vec2 pos : tickingFurniture)
    for (auto layer : ENUM_ALL(FurnitureLayer))
      if (auto f = furniture->getBuilt(layer).getWritable(pos))
        f->tick(Position(pos, this));
}

bool Level::inBounds(Vec2 pos) const {
  return pos.inRectangle(getBounds());
}

const Rectangle& Level::getBounds() const {
  return squares->getBounds();
}

const string& Level::getName() const {
  return name;
}

bool Level::areConnected(Vec2 p1, Vec2 p2, const MovementType& movement) const {
  return inBounds(p1) && inBounds(p2) && getSectors(movement).same(p1, p2);
}

Sectors& Level::getSectors(const MovementType& movement) const {
  if (!sectors.count(movement)) {
    sectors[movement] = Sectors(getBounds());
    Sectors& newSectors = sectors.at(movement);
    for (Position pos : getAllPositions())
      if (pos.canNavigate(movement))
        newSectors.add(pos.getCoord());
  }
  return sectors.at(movement);
}

bool Level::isChokePoint(Vec2 pos, const MovementType& movement) const {
  return getSectors(movement).isChokePoint(pos);
}

void Level::updateSunlightMovement() {
  sectors.clear();
}

const optional<ViewObject>& Level::getBackgroundObject(Vec2 pos) const {
  return (*background)[pos];
}

int Level::getNumGeneratedSquares() const {
  return squares->getNumGenerated();
}

int Level::getNumTotalSquares() const {
  return squares->getNumTotal();
}

void Level::setNeedsMemoryUpdate(Vec2 pos, bool s) {
  if (pos.inRectangle(getBounds()))
    memoryUpdates[pos] = s;
}

bool Level::needsRenderUpdate(Vec2 pos) const {
  return renderUpdates[pos];
}

void Level::setNeedsRenderUpdate(Vec2 pos, bool s) {
  renderUpdates[pos] = s;
  setNeedsMemoryUpdate(pos, s);
}

bool Level::needsMemoryUpdate(Vec2 pos) const {
  return memoryUpdates[pos];
}

bool Level::isUnavailable(Vec2 pos) const {
  return unavailable[pos];
}

void Level::setFurniture(Vec2 pos, PFurniture f) {
  auto layer = f->getLayer();
  furniture->getConstruction(pos, layer).reset();
  if (f->isTicking())
    addTickingFurniture(pos);
  furniture->getBuilt(layer).putElem(pos, std::move(f));
}
