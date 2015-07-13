#include "stdafx.h"
#include "level_builder.h"
#include "square_factory.h"
#include "progress_meter.h"
#include "square.h"
#include "location.h"
#include "creature.h"
#include "level_maker.h"
#include "collective_builder.h"

LevelBuilder::LevelBuilder(ProgressMeter& meter, int width, int height, const string& n, bool covered)
  : squares(width, height), heightMap(width, height, 0),
    coverInfo(width, height, {covered, covered ? 0.0 : 1.0}), attrib(width, height),
    type(width, height, SquareType(SquareId(0))), items(width, height), name(n), progressMeter(meter) {
}

bool LevelBuilder::hasAttrib(Vec2 posT, SquareAttrib attr) {
  Vec2 pos = transform(posT);
  CHECK(squares[pos] != nullptr);
  return attrib[pos][attr];
}

void LevelBuilder::addAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].insert(attr);
}

void LevelBuilder::removeAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].erase(attr);
}

Square* LevelBuilder::getSquare(Vec2 pos) {
  return squares[transform(pos)].get();
}
    
const SquareType& LevelBuilder::getType(Vec2 pos) {
  return type[transform(pos)];
}

void LevelBuilder::putSquare(Vec2 pos, SquareType t, optional<SquareAttrib> at) {
  putSquare(pos, SquareFactory::get(t), t, at);
}

void LevelBuilder::putSquare(Vec2 pos, SquareType t, vector<SquareAttrib> at) {
  putSquare(pos, SquareFactory::get(t), t, at);
}

void LevelBuilder::putSquare(Vec2 pos, PSquare square, SquareType t, optional<SquareAttrib> attr) {
  putSquare(pos, std::move(square), t, attr ? vector<SquareAttrib>({*attr}) : vector<SquareAttrib>());
}

void LevelBuilder::putSquare(Vec2 posT, PSquare square, SquareType t, vector<SquareAttrib> attr) {
  progressMeter.addProgress();
  Vec2 pos = transform(posT);
  CHECK(type[pos].getId() != SquareId::STAIRS) << "Attempted to overwrite stairs";
  square->setPosition(pos);
  if (squares[pos])
    square->setBackground(squares[pos].get());
  squares[pos] = std::move(square);
  for (SquareAttrib at : attr)
    attrib[pos].insert(at);
  type[pos] = t;
  squares[pos]->updateSunlightMovement(isInSunlight(pos));
}

bool LevelBuilder::isInSunlight(Vec2 pos) {
  return !coverInfo[pos].covered();
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
  creature->setPosition(transform(pos));
  creatures.push_back(NOTNULL(std::move(creature)));
}

void LevelBuilder::putItems(Vec2 posT, vector<PItem> it) {
  Vec2 pos = transform(posT);
  CHECK(squares[pos]->canEnterEmpty(MovementType({MovementTrait::WALK})));
  append(items[pos], std::move(it));
}

bool LevelBuilder::canPutCreature(Vec2 posT, Creature* c) {
  Vec2 pos = transform(posT);
  if (!squares[pos]->canEnter(c))
    return false;
  for (PCreature& c : creatures) {
    if (c->getPosition() == pos)
      return false;
  }
  return true;
}

void LevelBuilder::setMessage(const string& message) {
  entryMessage = message;
}

PLevel LevelBuilder::build(Model* m, LevelMaker* maker, int levelId) {
  CHECK(mapStack.empty());
  maker->make(this, squares.getBounds());
  for (Vec2 v : heightMap.getBounds()) {
    squares[v]->setHeight(heightMap[v]);
    squares[v]->dropItems(std::move(items[v]));
  }
  PLevel l(new Level(std::move(squares), m, locations, entryMessage, name, std::move(coverInfo), levelId));
  for (PCreature& c : creatures) {
    Vec2 pos = c->getPosition();
    l->addCreature(pos, std::move(c));
  }
  for (CollectiveBuilder* c : collectives)
    c->setLevel(l.get());
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

void LevelBuilder::setCoverInfo(Vec2 pos, Level::CoverInfo info) {
  coverInfo[transform(pos)] = info;
  if (squares[pos])
    squares[pos]->updateSunlightMovement(isInSunlight(pos));
}

