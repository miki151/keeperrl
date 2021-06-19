#pragma once

#include "util.h"
#include "position.h"

class StoragePositions {
  public:

  using MapType = unordered_map<Position, int, CustomHash<Position>>;

  void add(Position);
  void remove(Position);
  bool empty() const;
  bool contains(Position) const;
  vector<Position> asVector() const;

  template <typename Archive>
  void serialize(Archive&, unsigned int);

  struct Iter {
    MapType::const_iterator iter;

    const Position& operator* () const;
    bool operator != (const Iter& other) const;
    const Iter& operator++ ();
  };

  Iter begin() const;
  Iter end() const;

  private:
  MapType SERIAL(positions);
};


