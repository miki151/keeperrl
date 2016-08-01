#include "stdafx.h"
#include "level_builder.h"
#include "square_factory.h"
#include "progress_meter.h"
#include "square.h"
#include "location.h"
#include "creature.h"
#include "level_maker.h"
#include "collective_builder.h"
#include "view_object.h"

LevelBuilder::LevelBuilder(ProgressMeter* meter, RandomGen& r, int width, int height, const string& n, bool covered,
    optional<double> defaultLight)
  : squares(Rectangle(width, height)), background(width, height), unavailable(width, height, false),
    heightMap(width, height, 0), coverOverride(width, height),
    sunlight(width, height, defaultLight ? *defaultLight : (covered ? 0.0 : 1.0)),
    allCovered(covered), attrib(width, height),
    type(width, height, SquareType(SquareId(0))), items(width, height), name(n), progressMeter(meter), random(r) {
}

LevelBuilder::LevelBuilder(RandomGen& r, int width, int height, const string& n, bool covered)
  : LevelBuilder(nullptr, r, width, height, n, covered) {
}

RandomGen& LevelBuilder::getRandom() {
  return random;
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

const Square* LevelBuilder::getSquare(Vec2 pos) {
  return squares.getReadonly(transform(pos));
}

Square* LevelBuilder::modSquare(Vec2 pos) {
  return squares.getSquare(transform(pos));
}
   
const SquareType& LevelBuilder::getType(Vec2 pos) {
  return type[transform(pos)];
}

void LevelBuilder::putSquare(Vec2 pos, SquareType t, optional<SquareAttrib> attr) {
  putSquare(pos, t, attr ? vector<SquareAttrib>({*attr}) : vector<SquareAttrib>());
}

void LevelBuilder::putSquare(Vec2 posT, SquareType t, vector<SquareAttrib> attr) {
  if (progressMeter)
    progressMeter->addProgress();
  Vec2 pos = transform(posT);
  CHECK(type[pos].getId() != SquareId::STAIRS) << "Attempted to overwrite stairs";
  bool wasCovered = false;
  if (const Square* square = squares.getReadonly(pos)) {
    wasCovered = square->isCovered();
    if (auto backgroundObj = square->extractBackground())
      background[pos] = backgroundObj;
  }
  squares.putSquare(pos, t);
  for (SquareAttrib at : attr)
    attrib[pos].insert(at);
  type[pos] = t;
  if (coverOverride[pos])
    squares.getSquare(pos)->setCovered(*coverOverride[pos]);
  else if (allCovered || wasCovered)
    squares.getSquare(pos)->setCovered(true);
}

Rectangle LevelBuilder::toGlobalCoordinates(Rectangle area) {
  return area.apply([this](Vec2 v) { return transform(v); });
}

void LevelBuilder::addLocation(Location* l, Rectangle area) {
  l->setBounds(toGlobalCoordinates(area));
  locations.push_back(l);
}

void LevelBuilder::addCollective(CollectiveBuilder* col) {
  if (!contains(collectives, col))
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
  Vec2 pos = transform(posT);
  CHECK(squares.getReadonly(pos)->canEnterEmpty(MovementType(MovementTrait::WALK)));
  append(items[pos], std::move(it));
}

bool LevelBuilder::canPutCreature(Vec2 posT, Creature* c) {
  Vec2 pos = transform(posT);
  if (!squares.getReadonly(pos)->canEnter(c))
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

PLevel LevelBuilder::build(Model* m, LevelMaker* maker, LevelId levelId) {
  CHECK(mapStack.empty());
  maker->make(this, squares.getBounds());
  for (Vec2 v : squares.getBounds())
    if (!items[v].empty())
      squares.getSquare(v)->dropItemsLevelGen(std::move(items[v]));
  PLevel l(new Level(std::move(squares), m, locations, name, sunlight, levelId));
  l->background = background;
  l->unavailable = unavailable;
  for (pair<PCreature, Vec2>& c : creatures) {
    l->addCreature(c.second, std::move(c.first));
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
  for (auto m : reverse2(mapStack)) {
    v = m(v);
  }
  return v;
}

void LevelBuilder::setCoverOverride(Vec2 posT, bool covered) {
  Vec2 pos = transform(posT);
  coverOverride[pos] = covered;
  if (Square* square = squares.getSquare(pos))
    square->setCovered(covered);
}

void LevelBuilder::setSunlight(Vec2 pos, double s) {
  sunlight[pos] = s;
}

void LevelBuilder::setUnavailable(Vec2 pos) {
  unavailable[transform(pos)] = true;
}
