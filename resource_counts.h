#pragma once

#include "util.h"

struct ResourceCounts {
  struct Elem {
    FurnitureType SERIAL(type);
    int SERIAL(countStartingPos);
    int SERIAL(countFurther);
    SERIALIZE_ALL(type, countStartingPos, countFurther)
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

