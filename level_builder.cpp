#include "stdafx.h"
#include "level_builder.h"
#include "progress_meter.h"
#include "square.h"
#include "creature.h"
#include "level_maker.h"
#include "collective_builder.h"
#include "view_object.h"
#include "item.h"
#include "furniture.h"
#include "furniture_factory.h"
#include "position.h"
#include "movement_set.h"
#include "content_factory.h"
#include "creature_name.h"

LevelBuilder::LevelBuilder(ProgressMeter* meter, RandomGen& r, ContentFactory* contentFactory, int width, int height,
    bool allCovered, optional<double> defaultLight)
  : squares(Rectangle(width, height)), unavailable(width, height, false),
    heightMap(width, height, 0), covered(width, height, allCovered), building(width, height, false),
    sunlight(width, height, defaultLight ? *defaultLight : (allCovered ? 0.0 : 1.0)),
    attrib(width, height), items(width, height), furniture(Rectangle(width, height)),
    progressMeter(meter), random(r), contentFactory(contentFactory) {
}

LevelBuilder::LevelBuilder(RandomGen& r, ContentFactory* contentFactory, int width, int height, bool covered)
  : LevelBuilder(nullptr, r, contentFactory, width, height, covered) {
}

LevelBuilder::~LevelBuilder() {}

LevelBuilder::LevelBuilder(LevelBuilder&&) = default;

RandomGen& LevelBuilder::getRandom() {
  return random;
}

ContentFactory* LevelBuilder::getContentFactory() const {
  return contentFactory;
}

bool LevelBuilder::hasAttrib(Vec2 posT, SquareAttrib attr) {
  Vec2 pos = transform(posT);
  return attrib[pos].contains(attr);
}

void LevelBuilder::addAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].insert(attr);
}

void LevelBuilder::removeAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].erase(attr);
}

Square* LevelBuilder::modSquare(Vec2 pos) {
  return squares.getWritable(transform(pos));
}

Rectangle LevelBuilder::toGlobalCoordinates(Rectangle area) {
  return area.apply([this](Vec2 v) { return transform(v); });
}

vector<Vec2> LevelBuilder::toGlobalCoordinates(const vector<Vec2>& v) {
  return v.transform([this](Vec2 v) { return transform(v); });
}

void LevelBuilder::addCollective(CollectiveBuilder* col) {
  if (!collectives.contains(col))
    collectives.push_back(col);
}

void LevelBuilder::setHeightMap(Vec2 pos, double h) {
  heightMap[transform(pos)] = h;
}

double LevelBuilder::getHeightMap(Vec2 pos) {
  return heightMap[transform(pos)];
}

void LevelBuilder::putCreature(Vec2 pos, PCreature creature) {
  creatures.emplace_back(std::move(creature), transform(pos));
}

void LevelBuilder::putItems(Vec2 posT, vector<PItem> it) {
  CHECK(canPutItems(posT));
  Vec2 pos = transform(posT);
  append(items[pos], std::move(it));
}

bool LevelBuilder::canPutItems(Vec2 posT) {
  return canNavigate(posT, {MovementTrait::WALK});
}

void LevelBuilder::putFurniture(Vec2 posT, FurnitureList& f, TribeId tribe, optional<SquareAttrib> attrib) {
  if (auto res = f.getRandom(getRandom(), tribe))
    putFurniture(posT, *res, attrib);
}

void LevelBuilder::putFurniture(Vec2 posT, FurnitureParams f, optional<SquareAttrib> attrib) {
  auto layer = contentFactory->furniture.getData(f.type).getLayer();
  if (getFurniture(posT, layer))
    removeFurniture(posT, layer);
  furniture.getBuilt(layer).putElem(transform(posT), f, [&](const FurnitureParams& t) {
    return contentFactory->furniture.getFurniture(t.type, t.tribe); });
  if (attrib)
    addAttrib(posT, *attrib);
}

void LevelBuilder::putFurniture(Vec2 pos, FurnitureType type, optional<SquareAttrib> attrib) {
  putFurniture(pos, {type, TribeId::getHostile()}, attrib);
}

void LevelBuilder::putFurniture(Vec2 pos, FurnitureType type, TribeId tribe, optional<SquareAttrib> attrib) {
  putFurniture(pos, {type, tribe}, attrib);
}

void LevelBuilder::resetFurniture(Vec2 posT, FurnitureType type, optional<SquareAttrib> attrib) {
  USER_CHECK(contentFactory->furniture.getData(type).getLayer() == FurnitureLayer::GROUND)
      << type.data() << " must have layer set to GROUND ";
  removeAllFurniture(posT);
  putFurniture(posT, type, attrib);
}

void LevelBuilder::resetFurniture(Vec2 posT, FurnitureParams params, optional<SquareAttrib> attrib) {
  USER_CHECK(contentFactory->furniture.getData(params.type).getLayer() == FurnitureLayer::GROUND)
      << params.type.data() << " must have layer set to GROUND ";  removeAllFurniture(posT);
  putFurniture(posT, params, attrib);
}

bool LevelBuilder::canPutFurniture(Vec2 posT, FurnitureLayer layer) {
  return !getFurniture(posT, layer);
}

void LevelBuilder::removeFurniture(Vec2 pos, FurnitureLayer layer) {
  CHECK(getFurnitureType(pos, layer) != FurnitureType("DOWN_STAIRS"));
  furniture.getBuilt(layer).clearElem(transform(pos));
}

void LevelBuilder::removeAllFurniture(Vec2 pos) {
  for (auto layer : ENUM_ALL(FurnitureLayer))
    removeFurniture(pos, layer);
}

optional<FurnitureType> LevelBuilder::getFurnitureType(Vec2 posT, FurnitureLayer layer) {
  if (auto f = getFurniture(posT, layer))
    return f->getType();
  else
    return none;
}

bool LevelBuilder::isFurnitureType(Vec2 pos, FurnitureType type) {
  return getFurnitureType(pos, contentFactory->furniture.getData(type).getLayer()) == type;
}

const Furniture* LevelBuilder::getFurniture(Vec2 posT, FurnitureLayer layer) {
  return furniture.getBuilt(layer).getReadonly(transform(posT));
}

void LevelBuilder::setLandingLink(Vec2 posT, StairKey key) {
  Vec2 pos = transform(posT);
  squares.getWritable(pos)->setLandingLink(key);
}

bool LevelBuilder::canPutCreature(Vec2 posT, Creature* c) {
  Vec2 pos = transform(posT);
  if (!canNavigate(posT, c->getMovementType().setForced(false)) || hasAttrib(posT, SquareAttrib::NO_CREATURES))
    return false;
  for (pair<PCreature, Vec2>& c : creatures) {
    if (c.second == pos)
      return false;
  }
  return true;
}

void LevelBuilder::setNoDiagonalPassing() {
  noDiagonalPassing = true;
}

PLevel LevelBuilder::build(const ContentFactory* factory, WModel m, LevelMaker* maker, LevelId levelId) {
  PROFILE;
  CHECK(!!m);
  CHECK(mapStack.empty());
  maker->make(this, squares.getBounds());
  for (Vec2 v : squares.getBounds())
    if (!items[v].empty())
      squares.getWritable(v)->dropItemsLevelGen(std::move(items[v]));
  for (auto& elem : permanentGas)
    squares.getWritable(elem.second)->addPermanentGas(elem.first, 1);
  auto l = Level::create(std::move(squares), std::move(furniture), m, sunlight, levelId, covered, unavailable, factory);
  for (pair<PCreature, Vec2>& c : creatures) {
    Position pos(c.second, l.get());
    /*CHECK(pos.canEnter(c.first.get())) << c.first->getName().bare();
    pos.addCreature(std::move(c.first));*/
    // Use landCreature instead, because it will try to put it in adjacent positions
    CHECK(l->landCreature({pos}, c.first.get()));
    m->addCreature(std::move(c.first));
  }
  for (CollectiveBuilder* c : collectives)
    c->setLevel(l.get());
  l->noDiagonalPassing = noDiagonalPassing;
  return l;
}

static Vec2::LinearMap identity() {
  return [](Vec2 v) { return v; };
}

static Vec2::LinearMap deg90(Rectangle bounds) {
  return [bounds](Vec2 v) {
    v -= bounds.topLeft();
    return bounds.topLeft() + Vec2(v.y, v.x);
  };
}

static Vec2::LinearMap deg180(Rectangle bounds) {
  return [bounds](Vec2 v) {
    return bounds.topLeft() - v + bounds.bottomRight() - Vec2(1, 1);
  };
}

static Vec2::LinearMap deg270(Rectangle bounds) {
  return [bounds](Vec2 v) {
    v -= bounds.topRight() - Vec2(1, 0);
    return bounds.topLeft() + Vec2(v.y, -v.x);
  };
}

void LevelBuilder::pushMap(Rectangle bounds, Rot rot) {
  switch (rot) {
    case CW0: mapStack.push_back(identity()); break;
    case CW1: mapStack.push_back(deg90(bounds)); break;
    case CW2: mapStack.push_back(deg180(bounds)); break;
    case CW3: mapStack.push_back(deg270(bounds)); break;
  }
}

void LevelBuilder::popMap() {
  mapStack.pop_back();
}

Vec2 LevelBuilder::transform(Vec2 v) {
  for (auto m : mapStack.reverse()) {
    v = m(v);
  }
  return v;
}

void LevelBuilder::setCovered(Vec2 posT, bool state) {
  covered[transform(posT)] = state;
}

void LevelBuilder::setBuilding(Vec2 posT, bool state) {
  building[transform(posT)] = state;
}

void LevelBuilder::setSunlight(Vec2 pos, double s) {
  sunlight[pos] = s;
}

void LevelBuilder::addPermanentGas(TileGasType type, Vec2 posT) {
  permanentGas.push_back({type, transform(posT)});
}

void LevelBuilder::setUnavailable(Vec2 pos) {
  unavailable[transform(pos)] = true;
}

bool LevelBuilder::canNavigate(Vec2 posT, const MovementType& movement) {
  Vec2 pos = transform(posT);
  if (unavailable[pos])
    return false;
  bool result = true;
  for (auto layer : ENUM_ALL(FurnitureLayer))
    if (auto f = furniture.getBuilt(layer).getReadonly(pos)) {
      bool canEnter = f->getMovementSet().canEnter(movement, covered[pos] || building[pos], false, none);
      if (f->overridesMovement())
        return canEnter;
      else
        result &= canEnter;
    }
  return result;

}
