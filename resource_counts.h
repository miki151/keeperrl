#pragma once

#include "util.h"
#include "furniture_type.h"

struct ResourceCounts {
  struct Elem {
    FurnitureType SERIAL(type);
    Range SERIAL(size);
    int SERIAL(countStartingPos);
    int SERIAL(countFurther);
    SERIALIZE_ALL(type, size, countStartingPos, countFurther)
  };
  vector<Elem> SERIAL(elems);
  SERIALIZE_ALL(elems)
};

struct ResourceDistribution {
  ResourceCounts SERIAL(counts);
  optional<int> SERIAL(minDepth);
  optional<int> SERIAL(maxDepth);
  SERIALIZE_ALL(NAMED(counts), NAMED(minDepth), NAMED(maxDepth))
};

optional<ResourceCounts> chooseResourceCounts(RandomGen&, const vector<ResourceDistribution>&, int depth);

