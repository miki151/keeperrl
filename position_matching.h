#pragma once

#include "util.h"
#include "position.h"
#include "position_map.h"

class PositionMatching : public OwnedObject<PositionMatching> {
  public:

  optional<Position> getMatch(Position) const;
  void releaseTarget(Position);
  void addTarget(Position);
  void updateMovement(Position);

  template <typename Archive>
  void serialize(Archive&, const unsigned);

  private:
  void findPath(Position);
  bool findPath(Position, PositionSet& visited, bool matchedWithFather);
  PositionMap<Position> SERIAL(matches);
  PositionMap<Position> SERIAL(reverseMatches);
  PositionSet SERIAL(targets);
  void setMatch(Position, Position);
  void removeMatch(Position);
};
