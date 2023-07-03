#pragma once

#include "util.h"
#include "position.h"

class Territory {
  public:
  void insert(Position);
  void remove(Position);
  void setCentralPoint(Position);

  bool contains(Position) const;
  const vector<Position>& getAll() const;
  const PositionSet& getAllAsSet() const;
  const vector<Position>& getExtended(int min, int max) const;
  const vector<Position>& getExtended(int max) const;
  const vector<Position>& getStandardExtended() const;
  bool isEmpty() const;
  const optional<Position>& getCentralPoint() const;
  IterateVectors<Position> getPillagePositions() const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void clearCache();
  vector<Position> calculateExtended(int minRadius, int maxRadius) const;
  PositionSet SERIAL(allSquares);
  vector<Position> SERIAL(allSquaresVec);
  optional<Position> SERIAL(centralPoint);
  mutable map<pair<int, int>, vector<Position>> extendedCache;
  mutable map<int, vector<Position>> extendedCache2;
};


