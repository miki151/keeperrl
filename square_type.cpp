#include "stdafx.h"
#include "util.h"
#include "square_type.h"

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

SquareType::SquareType(Id _id) : id(_id) {
  CHECK(id != ALTAR);
}
 
SquareType::SquareType(AltarInfo info) : id(ALTAR), altarInfo(info) {
}
 
SquareType::SquareType() : id(SquareType::Id(0)) {}

bool SquareType::operator==(const SquareType& other) const {
  return id == other.id && (id != ALTAR || altarInfo.habitat == other.altarInfo.habitat);
}

bool SquareType::operator==(SquareType::Id id1) const {
  return id == id1;
}

bool operator==(SquareType::Id id1, const SquareType& t) {
  return t.id == id1;
}


bool SquareType::operator!=(const SquareType& other) const {
  return !(*this == other);
}

template <class Archive>
void SquareType::serialize(Archive& ar, const unsigned int version) {
  ar & id & altarInfo;
}

SERIALIZABLE(SquareType);

template <class Archive>
void SquareType::AltarInfo::serialize(Archive& ar, const unsigned int version) {
  ar & habitat;
}

SERIALIZABLE(SquareType::AltarInfo);
