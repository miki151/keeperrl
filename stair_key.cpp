#include "stdafx.h"
#include "stair_key.h"

int StairKey::numKeys = 3;

StairKey StairKey::getNew() {
  return numKeys++;
}

StairKey StairKey::heroSpawn() {
  return StairKey(0);
}

StairKey StairKey::keeperSpawn() {
  return StairKey(1);
}

StairKey StairKey::transferLanding() {
  return StairKey(2);
}

StairKey StairKey::sokoban() {
  return StairKey(3);
}

bool StairKey::operator == (const StairKey& o) const {
  return key == o.key;
}

int StairKey::getInternalKey() const {
  return key;
}

StairKey::StairKey(int k) : key(k) {
}

SERIALIZE_DEF(StairKey, key)
SERIALIZATION_CONSTRUCTOR_IMPL(StairKey);
