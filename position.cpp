#include "stdafx.h"
#include "position.h"
#include "level.h"
#include "square.h"
#include "creature.h"


template <class Archive> 
void Position::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(coord) & SVAR(level);
}

SERIALIZABLE(Position);
SERIALIZATION_CONSTRUCTOR_IMPL(Position);

Vec2 Position::getCoord() const {
  return coord;
}

Level* Position::getLevel() const {
  return level;
}

Position::Position(Vec2 v, Level* l) : coord(v), level(l) {
}

const static int otherLevel = 1000000;

int Position::dist8(const Position& pos) const {
  if (pos.level != level)
    return otherLevel;
  return pos.getCoord().dist8(coord);
}

bool Position::isSameLevel(const Position& p) const {
  return level == p.level;
}

Vec2 Position::getDir(const Position& p) const {
  CHECK(isSameLevel(p));
  return p.coord - coord;
}

Square* Position::getSafeSquare() const {
  return level->getSafeSquare(coord);
}

Creature* Position::getCreature() const {
  if (level->inBounds(coord))
    return level->getSafeSquare(coord)->getCreature();
  else
    return nullptr;
}

bool Position::operator == (const Position& o) const {
  return coord == o.coord && level == o.level;
}

Position& Position::operator = (const Position& o) {
  coord = o.coord;
  level = o.level;
  return *this;
}

bool Position::operator < (const Position& p) const {
  if (level->getUniqueId() == p.level->getUniqueId())
    return coord < p.coord;
  else
    return level->getUniqueId() < p.level->getUniqueId();
}

void Position::globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const {
  level->globalMessage(coord, playerCanSee, cannot);
}

void Position::globalMessage(const PlayerMessage& playerCanSee) const {
  level->globalMessage(coord, playerCanSee);
}

vector<Position> Position::neighbors8(bool shuffle) const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors8(shuffle))
    if (level->inBounds(v))
      ret.push_back(Position(v, level));
  return ret;
}

vector<Position> Position::neighbors4(bool shuffle) const {
  vector<Position> ret;
  for (Vec2 v : coord.neighbors4(shuffle))
    if (level->inBounds(v))
      ret.push_back(Position(v, level));
  return ret;
}

void Position::addCreature(PCreature c) {
  level->addCreature(coord, std::move(c));
}

Position Position::operator + (Vec2 v) const {
  return Position(coord + v, level);
}

Position Position::operator - (Vec2 v) const {
  return Position(coord - v, level);
}
