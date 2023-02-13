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
#include "game_event.h"
#include "collective.h"
#include "phylactery_info.h"
#include "content_factory.h"
#include "monster_ai.h"
#include "furniture_layer.h"
#include "known_tiles.h"
#include "territory.h"
#include "player_control.h"

template <class Archive>
void Level::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Level>);
  ar(squares, landingSquares, tickingSquares, creatures, model, fieldOfView);
  ar(sunlight, bucketMap, lightAmount, unavailable, swarmMaps, territory);
  ar(levelId, noDiagonalPassing, lightCapAmount, creatureIds, memoryUpdates, above, below, mountainLevel);
  ar(furniture, tickingFurniture, covered, name, depth, wildlife, addedWildlife, mainDungeon);
  vector<pair<TribeId, unique_ptr<EffectsTable>>> SERIAL(tmp);
  for (auto t : ENUM_ALL(TribeId::KeyType))
    if (!!furnitureEffects[t])
      tmp.push_back(make_pair(TribeId(t), std::move(furnitureEffects[t])));
  ar(tmp);
  for (auto& elem : tmp)
    if (elem.second) {
      auto& table = furnitureEffects[elem.first.getKey()];
      CHECK(!table);
      table = std::move(elem.second);
    }
  // ar(furnitureEffects)
  if (Archive::is_loading::value) // some code requires these Sectors to be always initialized
    getSectors({MovementTrait::WALK});
}

SERIALIZABLE(Level);

SERIALIZATION_CONSTRUCTOR_IMPL(Level);

Level::~Level() {}

static vector<pair<int, CreatureBucketMap>> getSwarmMaps(Vec2 size) {
  constexpr int bucketSize = 10;
  return makeVec(
    make_pair(0, CreatureBucketMap(size, bucketSize)),
    make_pair(bucketSize/ 2, CreatureBucketMap(size + Vec2(bucketSize / 2, bucketSize/ 2), bucketSize))
  );
}

Level::Level(Private, SquareArray s, FurnitureArray f, WModel m, Table<double> sun, LevelId id)
    : territory(s.getBounds(), nullptr), squares(std::move(s)), furniture(std::move(f)),
      memoryUpdates(squares->getBounds(), true), model(m),
      sunlight(sun),
      bucketMap(squares->getBounds().getSize(), FieldOfView::sightRange),
      swarmMaps(getSwarmMaps(squares->getBounds().getSize())),
      lightAmount(squares->getBounds(), 0), lightCapAmount(squares->getBounds(), 1),
      levelId(id) {
}

PLevel Level::create(SquareArray s, FurnitureArray f, WModel m,
    Table<double> sun, LevelId id, Table<bool> covered, Table<bool> unavailable, const ContentFactory* factory) {
  auto ret = makeOwner<Level>(Private{}, std::move(s), std::move(f), m, sun, id);
  for (Vec2 pos : ret->squares->getBounds()) {
    auto square = ret->squares->getReadonly(pos);
    square->onAddedToLevel(Position(pos, ret.get()));
    if (optional<StairKey> link = square->getLandingLink())
      ret->landingSquares[*link].push_back(Position(pos, ret.get()));
    for (auto layer : ENUM_ALL(FurnitureLayer)) {
      ret->furniture->getBuilt(layer).shrinkToFit();
      if (auto f = ret->furniture->getBuilt(layer).getReadonly(pos))
        if (f->isTicking())
          ret->addTickingFurniture(pos);
    }
  }
  for (VisionId vision : ENUM_ALL(VisionId))
    (*ret->fieldOfView)[vision] = FieldOfView(ret.get(), vision, factory);
  for (auto pos : ret->getAllPositions()) {
    ret->addLightSource(pos.getCoord(), pos.getLightEmission(), 1);
    for (auto layer : ENUM_ALL(FurnitureLayer))
      if (auto f = pos.getFurniture(layer)) {
        if (auto& effect = f->getLastingEffectInfo())
          pos.addFurnitureEffect(f->getTribe(), *effect);
        if (auto viewId = f->getSupportViewId(pos))
          pos.modFurniture(layer)->getViewObject()->setId(*viewId);
      }
  }
  ret->unavailable = std::move(unavailable);
  ret->covered = std::move(covered);
  ret->getSectors({MovementTrait::WALK});
  return ret;
}

LevelId Level::getUniqueId() const {
  return levelId;
}

Rectangle Level::getMaxBounds() {
  return Rectangle(360, 360);
}

double Level::getCreatureLightRadius() {
  return 5.5;
}

void Level::putCreature(Vec2 position, Creature* c) {
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
  PROFILE;
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

void Level::updateCreatureLight(Vec2 pos, int diff) {
  auto square = squares->getReadonly(pos);
  CHECK(square) << pos << " " << getBounds();
  if (Creature* c = square->getCreature()) {
    if (c->isAffected(LastingEffect::DARKNESS_SOURCE))
      addDarknessSource(pos, getCreatureLightRadius(), diff);
    if (c->isAffected(LastingEffect::LIGHT_SOURCE) || c->isAffected(LastingEffect::ON_FIRE))
      addLightSource(pos, getCreatureLightRadius(), diff);
  }
}

void Level::updateVisibility(Vec2 changedSquare) {
  auto allVisible = getVisibleTilesNoDarkness(changedSquare, VisionId::NORMAL);
  for (Vec2 pos : allVisible) {
    addLightSource(pos, Position(pos, this).getLightEmission(), -1);
    updateCreatureLight(pos, -1);
  }
  for (VisionId vision : ENUM_ALL(VisionId))
    getFieldOfView(vision).squareChanged(changedSquare);
  for (Vec2 pos : allVisible) {
    addLightSource(pos, Position(pos, this).getLightEmission(), 1);
    updateCreatureLight(pos, 1);
  }
  for (Vec2 pos : allVisible)
    getModel()->addEvent(EventInfo::VisibilityChanged{Position(pos, this)});
}

vector<Creature*> Level::getPlayers() const {
  if (auto game = model->getGame())
    return game->getPlayerCreatures().filter([this](const Creature* c) { return c->getLevel() == this; });
  return {};
}

WModel Level::getModel() const {
  return model;
}

WGame Level::getGame() const {
  return model->getGame();
}

double Level::getLight(Vec2 pos) const {
  return min(1.0, max(0.0, min(covered[pos] ? 1.0 : lightCapAmount[pos], lightAmount[pos] +
      sunlight[pos] * getGame()->getSunlightInfo().getLightAmount())));
}

double Level::getLevelGenSunlight(Vec2 pos) const {
  return sunlight[pos];
}

const vector<Position>& Level::getLandingSquares(StairKey key) const {
  if (landingSquares.count(key))
    return landingSquares.at(key);
  else {
    static vector<Position> empty;
    return empty;
  }
}

vector<StairKey> Level::getAllStairKeys() const {
  return getKeys(landingSquares);
}

bool Level::hasStairKey(StairKey key) const {
  return landingSquares.count(key);
}

bool Level::landCreature(StairKey key, Creature* creature) {
  vector<Position> landing = landingSquares.at(key);
  return landCreature(landing, creature);
}

bool Level::landCreature(StairKey key, PCreature creature) {
  // Creature may trigger traps when being placed, which can cause a crash
  // if it has no global time.
  creature->setGlobalTime(model->getGame()->getGlobalTime());
  if (landCreature(key, creature.get())) {
    model->addCreature(std::move(creature));
    return true;
  } else
    return false;
}

bool Level::landCreature(StairKey key, PCreature creature, Vec2 travelDir) {
  creature->setGlobalTime(model->getGame()->getGlobalTime());
  if (landCreature(key, creature.get(), travelDir)) {
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
  Position target = landing[0];
  for (Position p : landing)
    if (p.getCoord().distD(entryPos) < target.getCoord().distD(entryPos))
      target = p;
  return target;
}

bool Level::landCreature(StairKey key, Creature* creature, Vec2 travelDir) {
  Position bestLanding = getLandingSquare(key, travelDir);
  return landCreature({bestLanding}, creature) ||
      landCreature(bestLanding.getRectangle(Rectangle::centered(Vec2(0, 0), 10)), creature) ||
      landCreature(landingSquares.at(key), creature) ||
      landCreature(getAllLandingPositions(), creature);
}

optional<Position> Level::getClosestLanding(vector<Position> landing, Creature* creature) const {
  PROFILE;
  CHECK(creature);
  queue<Position> q;
  PositionSet marked;
  for (Position pos : Random.permutation(landing)) {
    q.push(pos);
    marked.insert(pos);
  }
  auto movement = creature->getMovementType().setForced();
  while (!q.empty()) {
    Position v = q.front();
    q.pop();
    if (v.canEnter(creature))
      return v;
    else
      for (Position next : v.neighbors8(Random))
        if (!marked.count(next) && next.canEnterEmpty(movement)) {
          q.push(next);
          marked.insert(next);
        }
  }
  return none;
}

bool Level::landCreature(vector<Position> landing, Creature* creature) {
  PROFILE;
  if (auto pos = getClosestLanding(std::move(landing), creature)) {
    pos->putCreature(creature);
    return true;
  } else
    return false;
}

void Level::killCreature(Creature* creature) {
  eraseCreature(creature, creature->getPosition().getCoord());
  getModel()->killCreature(creature);
}

void Level::changeLevel(StairKey key, Creature* c) {
  Vec2 oldPos = c->getPosition().getCoord();
  Level* otherLevel = model->getLinkedLevel(this, key);
  if (otherLevel->landCreature(key, c))
    eraseCreature(c, oldPos);
  else {
    Position otherPos = Random.choose(otherLevel->landingSquares.at(key));
    if (Creature* other = otherPos.getCreature()) {
      if (!other->isPlayer() && c->getPosition().canEnterEmpty(other) && otherPos.canEnterEmpty(c)) {
        otherLevel->eraseCreature(other, otherPos.getCoord());
        eraseCreature(c, oldPos);
        putCreature(oldPos, other);
        otherLevel->putCreature(otherPos.getCoord(), c);
        c->secondPerson("You switch levels with " + other->getName().a());
      }
    }
  }
}

void Level::changeLevel(Position destination, Creature* c) {
  Vec2 oldPos = c->getPosition().getCoord();
  if (destination.isValid() && destination.getLevel()->landCreature({destination}, c))
    eraseCreature(c, oldPos);
}

void Level::eraseCreature(Creature* c, Vec2 coord) {
  creatures.removeElement(c);
  unplaceCreature(c, coord);
  creatureIds.erase(c);
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

bool Level::containsCreature(UniqueEntity<Creature>::Id id) const {
  return creatureIds.contains(id);
}

bool Level::isWithinVision(Vec2 from, Vec2 to, const Vision& v) const {
  //PROFILE;
  auto dist = from.distD(to);
  return dist <= sightRange && v.canSeeAt(getLight(to), dist);
}

FieldOfView& Level::getFieldOfView(VisionId vision) const {
  //PROFILE;
  return (*fieldOfView)[vision];
}

bool Level::canSee(Vec2 from, Vec2 to, const Vision& vision) const {
  //PROFILE_BLOCK("Level::canSee");
  return isWithinVision(from, to, vision) && getFieldOfView(vision.getId()).canSee(from, to);
}

void Level::moveCreature(Creature* creature, Vec2 direction) {
  Vec2 position = creature->getPosition().getCoord();
  unplaceCreature(creature, position);
  placeCreature(creature, position + direction);
}

template <typename Fun>
void Level::forEachEffect(Vec2 pos, TribeId tribe, Fun f) {
  if (auto& effects = furnitureEffects[tribe.getKey()])
    for (auto effect : effects->operator[](pos).friendly)
      f(effect);
  for (auto tribeKey : ENUM_ALL(TribeId::KeyType))
    if (tribe != TribeId(tribeKey))
      if (auto& effects = furnitureEffects[tribeKey])
        for (auto effect : effects->operator[](pos).hostile)
          f(effect);
}

void Level::unplaceSwarmer(Vec2 pos, Creature* c) {
  for (auto& map : swarmMaps)
    map.second.removeElement(pos + Vec2(map.first, map.first), c);
}

void Level::placeSwarmer(Vec2 pos, Creature* c) {
  for (auto& map : swarmMaps)
    map.second.addElement(pos + Vec2(map.first, map.first), c);
}

void Level::unplaceCreature(Creature* creature, Vec2 pos) {
  bucketMap->removeElement(pos, creature);
  if (creature->isAffected(LastingEffect::SWARMER))
    unplaceSwarmer(pos, creature);
  updateCreatureLight(pos, -1);
  modSafeSquare(pos)->removeCreature(Position(pos, this));
  model->increaseMoveCounter();
  forEachEffect(pos, creature->getTribeId(),
      [&] (LastingOrBuff effect) {removePermanentEffect(effect, creature, false);});
  for (auto v : Position(pos, this).neighbors8())
    if (auto c = v.getCreature())
      if (c->isEnemy(creature))
        c->updateViewObjectFlanking();
}

void Level::placeCreature(Creature* creature, Vec2 pos) {
  auto prevPos = creature->getPosition();
  Position position(pos, this);
  creature->setPosition(position);
  bucketMap->addElement(pos, creature);
  if (creature->isAffected(LastingEffect::SWARMER))
    placeSwarmer(pos, creature);
  modSafeSquare(pos)->putCreature(creature);
  updateCreatureLight(pos, 1);
  position.onEnter(creature);
  model->increaseMoveCounter();
  int numEffects = 0;
  forEachEffect(pos, creature->getTribeId(),
      [&] (LastingOrBuff effect) {addPermanentEffect(effect, creature, false); ++numEffects;});
  int numEffectsPrev = 0;
  if (prevPos.getLevel() == this)
    forEachEffect(prevPos.getCoord(), creature->getTribeId(), [&] (LastingOrBuff) {++numEffectsPrev;});
  if (numEffects > numEffectsPrev)
    creature->verb("feel", "feels", "the presence of a magical field");
  creature->updateViewObjectFlanking();
  for (auto v : position.neighbors8())
    if (auto c = v.getCreature())
      if (c->isEnemy(creature))
        c->updateViewObjectFlanking();
}

void Level::swapCreatures(Creature* c1, Creature* c2) {
  Vec2 pos1 = c1->getPosition().getCoord();
  Vec2 pos2 = c2->getPosition().getCoord();
  unplaceCreature(c1, pos1);
  unplaceCreature(c2, pos2);
  placeCreature(c1, pos2);
  auto otherDebug = getSafeSquare(pos1)->getCreature();
  CHECK(!otherDebug) << "Can't swap " << c2->identify() << " with " << c1->identify() << " "
      << otherDebug->identify() << " present";
  placeCreature(c2, pos1);
}

const vector<SVec2>& Level::getVisibleTilesNoDarkness(Vec2 pos, VisionId vision) const {
  PROFILE;
  return getFieldOfView(vision).getVisibleTiles(pos);
}

vector<Vec2> Level::getVisibleTiles(Vec2 pos, const Vision& vision) const {
  return getFieldOfView(vision.getId()).getVisibleTiles(pos)
      .transform([](auto v) { return Vec2(v);})
      .filter([&](Vec2 v) { return isWithinVision(pos, v, vision); });
}

const Square* Level::getSafeSquare(Vec2 pos) const {
  CHECK(inBounds(pos)) << pos << " " << getBounds();
  return squares->getReadonly(pos);
}

Square* Level::modSafeSquare(Vec2 pos) {
  CHECK(inBounds(pos));
  return squares->getWritable(pos);
}

vector<Position> Level::getAllPositions() const {
  vector<Position> ret;
  for (Vec2 v : getBounds())
    ret.emplace_back(v, getThis().removeConst().get());
  return ret;
}

vector<Position> Level::getAllLandingPositions() const {
  auto largestSector = getSectors({MovementTrait::WALK}).getLargest();
  return getAllPositions().filter([&](Position pos) {
    return getSectors({MovementTrait::WALK}).isSector(pos.getCoord(), largestSector);
  });
}

void Level::addTickingSquare(Vec2 pos) {
  tickingSquares.insert(pos);
}

void Level::addTickingFurniture(Vec2 pos) {
  tickingFurniture.insert(pos);
}

void Level::tick() {
  PROFILE_BLOCK("Level::tick");
  for (Vec2 pos : tickingSquares)
    squares->getWritable(pos)->tick(Position(pos, this));
  for (Vec2 pos : tickingFurniture)
    for (auto layer : ENUM_ALL(FurnitureLayer))
      if (auto f = furniture->getBuilt(layer).getWritable(pos))
        f->tick(Position(pos, this), layer);
  addedWildlife = addedWildlife.filter([this, col = getGame()->getPlayerCollective()](Creature* c) {
    return c->getPosition().getLevel() == this && (!col || !col->getCreatures().contains(c)); });
  if (Random.roll(50) && addedWildlife.size() < wildlife.count.getStart()) {
    auto gen = wildlife.generate(Random, &getGame()->getContentFactory()->getCreatures(), TribeId::getWildlife(),
        MonsterAIFactory::wildlifeNonPredator());
    if (!gen.empty()) {
      auto c = std::move(gen[0]);
      c->getStatus().insert(CreatureStatus::CIVILIAN);
      auto ref = c.get();
      auto positions = getAllPositions().filter([](auto pos) { return !pos.isCovered(); });
      if (!positions.empty() && getModel()->landCreature(positions, std::move(c)))
        addedWildlife.push_back(ref);
    }
  }
  if (above) {
    PROFILE_BLOCK("Z level tick");
    for (auto v : getBounds()) {
      if (!unavailable[v] && above->unavailable[v]) {
        Position pos(v, this);
        if (pos.isCovered()) {
          Position abovePos(v, above);
          auto col = getGame()->getPlayerCollective();
          above->unavailable[v] = false;
          abovePos.addFurniture(getGame()->getContentFactory()->furniture.getFurniture(FurnitureType("ROOF"),
              TribeId::getMonster()));
          if (!!col && col->getKnownTiles().isKnown(pos)) {
            col->addKnownTile(abovePos);
            getGame()->getPlayerControl()->addToMemory(abovePos);
          }
          if (!!col && col->getTerritory().contains(pos))
            col->claimSquare(abovePos);
        }
      }
    }
  }
}

bool Level::inBounds(Vec2 pos) const {
  //PROFILE;
  return pos.inRectangle(getBounds());
}

const Rectangle& Level::getBounds() const {
  return squares->getBounds();
}

Sectors& Level::getSectorsDontCreate(const MovementType& movement) const {
  return sectors.at(movement);
}

static Sectors::ExtraConnections getOrCreateExtraConnections(Rectangle bounds,
    const unordered_map<MovementType, Sectors, CustomHash<MovementType>>& sectors) {
  if (sectors.empty())
    return Sectors::ExtraConnections(bounds);
  else
    return sectors.begin()->second.getExtraConnections();
}

Sectors& Level::getSectors(const MovementType& movement) const {
  if (auto res = getReferenceMaybe(sectors, movement))
    return *res;
  else {
    PROFILE_BLOCK("Gen sectors");
    sectors.insert(make_pair(movement, Sectors(getBounds(), getOrCreateExtraConnections(getBounds(), sectors))));
    Sectors& newSectors = sectors.at(movement);
    for (Position pos : getAllPositions())
      if (pos.canNavigateCalc(movement))
        newSectors.add(pos.getCoord());
    return newSectors;
  }
}

void Level::updateSunlightMovement() {
  for (auto movement : getKeys(sectors))
    if (movement.isSunlightVulnerable())
      sectors.erase(movement);
}

int Level::getNumGeneratedSquares() const {
  int ret = 0;
  for (auto l : ENUM_ALL(FurnitureLayer))
    ret += furniture->getBuilt(l).getNumGenerated();
  return ret;
}

int Level::getNumTotalSquares() const {
  return squares->getNumGenerated();
}

void Level::setNeedsMemoryUpdate(Vec2 pos, bool s) {
  memoryUpdates[pos] = s;
  if (s)
    for (auto l = above; !!l && l->unavailable[pos]; l = l->above)
      l->memoryUpdates[pos] = s;
}

bool Level::needsRenderUpdate(Vec2 pos) const {
  return renderUpdates[pos];
}

void Level::setNeedsRenderUpdate(Vec2 pos, bool s) {
  renderUpdates[pos] = s;
}

bool Level::needsMemoryUpdate(Vec2 pos) const {
  return memoryUpdates[pos];
}

void Level::setFurniture(Vec2 pos, PFurniture f) {
  auto layer = f->getLayer();
  furniture->eraseConstruction(pos, layer);
  if (f->isTicking())
    addTickingFurniture(pos);
  furniture->getBuilt(layer).putElem(pos, std::move(f));
}

vector<PhylacteryInfo> Level::getPhylacteries() {
  vector<PhylacteryInfo> ret;
  /*auto model = getModel();
  // Figure out if it's worth doing the phylactery traces and what they achieve
  for (auto c : model->getAllCreatures())
    if (auto& f = c->getPhylactery()) {
      if (!f->pos.isSameModel(c->getPosition()))
        continue;
      auto cIndex = model->getMainLevelDepth(c->getPosition().getLevel());
      auto pIndex = model->getMainLevelDepth(f->pos.getLevel());
      auto lIndex = model->getMainLevelDepth(this);
      if (!lIndex || !cIndex || !pIndex || (*cIndex < *lIndex && *pIndex < *lIndex) || (*cIndex > *lIndex && *pIndex > *lIndex))
        continue;
      auto cPos = c->getPosition().getCoord();
      optional<ViewObject> object;
      if (*cIndex != *lIndex) {
        if (auto stairs = getStairsTo(c->getPosition().getLevel()))
          cPos = stairs->getCoord();
        else
          continue;
      } else
        object = c->getViewObject();
      auto pPos = f->pos.getCoord();
      if (*pIndex != *lIndex) {
        if (auto stairs = getStairsTo(f->pos.getLevel()))
          pPos = stairs->getCoord();
        else
          continue;
      }
      ret.push_back(PhylacteryInfo{cPos, pPos, object});
    }*/
  return ret;
}
