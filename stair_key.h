#pragma once

#include "util.h"

class StairKey {
  public:
  static StairKey getNew();
  static StairKey heroSpawn();
  static StairKey keeperSpawn();
  static StairKey transferLanding();

  bool operator == (const StairKey&) const;
  bool operator != (const StairKey&) const;

  SERIALIZATION_DECL(StairKey)

  long long getInternalKey() const;
  int getHash() const;

  private:
  StairKey(long long key);
  long long SERIAL(key);
};

namespace std {

template <> struct hash<StairKey> {
  size_t operator()(const StairKey& obj) const {
    return hash<int>()(obj.getInternalKey());
  }
};
}
