#ifndef _POSITION_MAP_H
#define _POSITION_MAP_H

#include "util.h"
#include "position.h"

class Level;

template <class T>
class PositionMap {
  public:
  PositionMap(const vector<Level*>&);
  PositionMap(const vector<Level*>&, const T& def);

  bool isValid(Position) const;

  const T& operator [] (Position) const;
  T& operator [] (Position);

  SERIALIZATION_DECL(PositionMap);

  private:
  vector<Table<T>> SERIAL(tables);
  vector<map<Vec2, T>> SERIAL(outliers);
};

#endif
