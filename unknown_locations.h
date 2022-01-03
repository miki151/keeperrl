#pragma once

#include "util.h"
#include "position.h"

class UnknownLocations {
  public:

  void update(const vector<Position>&);
  bool contains(Position) const;
  const vector<Vec2>& getOnLevel(const Level*) const;

  template <typename Archive>
  void serialize(Archive&, const unsigned);

  private:

  map<LevelId, vector<Vec2>> SERIAL(locationsByLevel);
  PositionSet SERIAL(allLocations);
};
