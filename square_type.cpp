#include "stdafx.h"
#include "util.h"
#include "square_type.h"
#include "creature.h"

bool SquareType::isWall() const {
  switch (id) {
    case HELL_WALL:
    case LOW_ROCK_WALL:
    case ROCK_WALL:
    case BLACK_WALL:
    case YELLOW_WALL:
    case WOOD_WALL:
    case MUD_WALL:
    case CASTLE_WALL: return true;
    default: return false;
  }
}

const static vector<SquareType::Id> creatureFactorySquares {
  SquareType::CHEST,
  SquareType::COFFIN,
  SquareType::HATCHERY,
  SquareType::DECID_TREE,
  SquareType::CANIF_TREE,
  SquareType::BUSH
};

const static vector<SquareType::Id> tribeSquares { SquareType::TRIBE_DOOR, SquareType::BARRICADE};

SquareType::SquareType(Id _id) : id(_id) {
  CHECK(!contains({ALTAR, CREATURE_ALTAR}, id)
      && !contains(creatureFactorySquares, id)
      && !contains(tribeSquares, id));
}
 
SquareType::SquareType(AltarInfo info) : id(ALTAR), values(info) {
}

SquareType::SquareType(CreatureAltarInfo info) : id(CREATURE_ALTAR), values(info) {
}

SquareType::SquareType(Id i, CreatureFactory info) : id(i), values(info) {
  CHECK(contains(creatureFactorySquares, id));
}

SquareType::SquareType(Id i, TribeInfo info) : id(i), values(info) {
  CHECK(contains(tribeSquares, id));
}

SquareType::SquareType() : id(SquareType::Id(0)) {}

bool SquareType::operator==(SquareType::Id id1) const {
  return id == id1;
}

bool SquareType::operator!=(SquareType::Id id1) const {
  return !(*this == id1);
}

bool SquareType::operator==(const SquareType& o) const {
  return id == o.id && values == o.values;
}

bool SquareType::operator!=(const SquareType& o) const {
  return !(*this == o);
}

bool operator==(SquareType::Id id1, const SquareType& t) {
  return t.id == id1;
}

template <class Archive>
void SquareType::serialize(Archive& ar, const unsigned int version) {
  ar & id & values;
}

SERIALIZABLE(SquareType);

