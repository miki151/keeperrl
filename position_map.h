#ifndef _POSITION_MAP_H
#define _POSITION_MAP_H

#include "util.h"
#include "position.h"

class Level;

template <class T>
class PositionMap {
  public:
  PositionMap(const T& defaultVal = T());

  const T& get(Position) const;
  T& getOrInit(Position);
  T& getOrFail(Position);
  void set(Position, const T&);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  Table<T>& getTable(Position);
  map<LevelId, Table<T>> SERIAL(tables);
  map<LevelId, map<Vec2, T>> SERIAL(outliers);
  T SERIAL(defaultVal);
};

#endif
