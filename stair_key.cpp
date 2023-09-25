#include "stdafx.h"
#include "stair_key.h"

StairKey StairKey::getNew() {
  return Random.getLL();
}

StairKey StairKey::keeperSpawn() {
  return StairKey(1);
}

StairKey StairKey::transferLanding() {
  return StairKey(2);
}

bool StairKey::operator == (const StairKey& o) const {
  return key == o.key;
}

bool StairKey::operator != (const StairKey& o) const {
  return key != o.key;
}

long long StairKey::getInternalKey() const {
  return key;
}

int StairKey::getHash() const {
  return key;
}

StairKey::StairKey(long long k) : key(k) {
}

SERIALIZE_DEF(StairKey, key)
SERIALIZATION_CONSTRUCTOR_IMPL(StairKey);
