#include "stdafx.h"
#include "util.h"
#include "square_type.h"
#include "creature.h"

bool isWall(SquareType t) {
  switch (t.getId()) {
    case SquareId::HELL_WALL:
    case SquareId::LOW_ROCK_WALL:
    case SquareId::ROCK_WALL:
    case SquareId::BLACK_WALL:
    case SquareId::YELLOW_WALL:
    case SquareId::WOOD_WALL:
    case SquareId::MUD_WALL:
    case SquareId::CASTLE_WALL: return true;
    default: return false;
  }
}

