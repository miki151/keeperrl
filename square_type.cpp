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

SquareType::SquareType(Id _id) : id(_id) {
  CHECK(id != ALTAR && id != CREATURE_ALTAR);
}
 
SquareType::SquareType(AltarInfo info) : id(ALTAR), altarInfo(info) {
}

SquareType::SquareType(CreatureAltarInfo info) : id(CREATURE_ALTAR), creatureAltarInfo(info) {
}


SquareType::SquareType() : id(SquareType::Id(0)) {}

bool SquareType::operator==(const SquareType& other) const {
  if (id != other.id)
    return false;
  switch (id) {
    case ALTAR: return altarInfo.habitat == other.altarInfo.habitat;
    case CREATURE_ALTAR: return creatureAltarInfo.creature == other.creatureAltarInfo.creature;
    default: return true;
  }
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
  ar & id;
  switch (id) {
    case ALTAR: ar & altarInfo.habitat; break;
    case CREATURE_ALTAR: ar & creatureAltarInfo.creature; break;
    default: break;
  }
}

size_t SquareType::getHash() const {
  switch (id) {
    case ALTAR: return 123456 * size_t(altarInfo.habitat); break;
    case CREATURE_ALTAR: return 654321 * size_t(creatureAltarInfo.creature->getUniqueId()); break;
    default: return size_t(id);
  }
}

SERIALIZABLE(SquareType);

template <class Archive>
void SquareType::AltarInfo::serialize(Archive& ar, const unsigned int version) {
  ar & habitat;
}

SERIALIZABLE(SquareType::AltarInfo);

template <class Archive>
void SquareType::CreatureAltarInfo::serialize(Archive& ar, const unsigned int version) {
  ar & creature;
}

SERIALIZABLE(SquareType::CreatureAltarInfo);
