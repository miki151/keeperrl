#pragma once

#include "stdafx.h"
#include "util.h"
#include "game_time.h"
#include "position.h"

RICH_ENUM(DancingPositionsType, CYCLE, FULL);

class Dancing {
  public:
  struct Positions {
    using Type = DancingPositionsType;
    Type SERIAL(type);
    int SERIAL(minCount);
    vector<vector<Vec2>> SERIAL(coord);
    Vec2 get(int iteration, int creatureIndex);
    SERIALIZE_ALL(type, minCount, coord)
  };
  Dancing(const ContentFactory*);

  void setArea(PositionSet, LocalTime);
  optional<Position> getTarget(Creature*);

  SERIALIZATION_DECL(Dancing)

  private:
  vector<Positions> SERIAL(positions);
  struct CurrentDanceInfo {
    int SERIAL(index);
    Position SERIAL(origin);
    LocalTime SERIAL(startTime);
    SERIALIZE_ALL(index, origin, startTime)
  };
  optional<CurrentDanceInfo> SERIAL(currentDanceInfo);
  void initializeCurrentDance(LocalTime);
  optional<int> assignCreatureIndex(Creature*, LocalTime);
  int getNumActive(LocalTime);
  vector<Creature*> SERIAL(assignments);
  map<Creature*, LocalTime> SERIAL(lastSeen);
  PositionSet SERIAL(area);
};
