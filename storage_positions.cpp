#include "storage_positions.h"

void StoragePositions::add(Position p) {
  ++positions[p];
}

void StoragePositions::remove(Position p) {
  auto res = --positions[p];
  CHECK(res >= 0);
  if (res == 0)
    positions.erase(p);
}

bool StoragePositions::empty() const {
  return positions.empty();
}

bool StoragePositions::contains(Position p) const {
  return positions.count(p);
}

vector<Position> StoragePositions::asVector() const {
  return getKeys(positions);
}

SERIALIZE_DEF(StoragePositions, positions)

const Position& StoragePositions::Iter::operator* () const {
  return iter->first;
}

bool StoragePositions::Iter::operator != (const Iter& other) const {
  return iter != other.iter;
}

const StoragePositions::Iter& StoragePositions::Iter::operator++ () {
  ++iter;
  return *this;
}

StoragePositions::Iter StoragePositions::begin() const {
  return Iter{positions.begin()};
}

StoragePositions::Iter StoragePositions::end() const {
  return Iter{positions.end()};
}
