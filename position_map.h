#pragma once

#include "util.h"
#include "position.h"

class Level;

template <class T>
class PositionMap {
  public:
  optional<const T&> getReferenceMaybe(Position) const;
  optional<T&> getReferenceMaybe(Position);
  optional<T> getValueMaybe(Position) const;
  bool contains(Position) const;
  T& getOrInit(Position);
  T& getOrFail(Position);
  const T& getOrFail(Position) const;
  void set(Position, const T&);
  void erase(Position);
  void limitToModel(const Model*);
  bool containsLevel(const Level*) const;

  SERIALIZATION_DECL(PositionMap)

  private:
  Table<heap_optional<T> >& getTable(Position);
  map<LevelId, Table<heap_optional<T>>> SERIAL(tables);
  map<LevelId, map<Vec2, T>> SERIAL(outliers);
};

