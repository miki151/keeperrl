#ifndef _TERRITORY_H
#define _TERRITORY_H

#include "util.h"
#include "position.h"

class Territory {
  public:
  void insert(Position);
  bool contains(Position) const;
  void remove(Position);
  const vector<Position>& getAll() const;
  const vector<Position>& getExtended(int min, int max) const;
  const vector<Position>& getExtended(int max) const;
  const vector<Position>& getStandardExtended() const;
  bool isEmpty() const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void clearCache();
  vector<Position> calculateExtended(int minRadius, int maxRadius) const;
  set<Position> SERIAL(allSquares);
  vector<Position> SERIAL(allSquaresVec);
  mutable map<pair<int, int>, vector<Position>> extendedCache;
  mutable map<int, vector<Position>> extendedCache2;
};

#endif

