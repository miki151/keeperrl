#include "stdafx.h"
#include "stair_key.h"

int StairKey::numKeys = 2;

StairKey StairKey::getNew() {
  return numKeys++;
}

StairKey StairKey::heroSpawn() {
  return StairKey(0);
}

StairKey StairKey::keeperSpawn() {
  return StairKey(1);
}

bool StairKey::operator == (const StairKey& o) const {
  return key == o.key;
}

int StairKey::getInternalKey() const {
  return key;
}

StairKey::StairKey(int k) : key(k) {
}

template <class Archive>
void StairKey::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(key);
}

SERIALIZABLE(StairKey);

SERIALIZATION_CONSTRUCTOR_IMPL(StairKey);
