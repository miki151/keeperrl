#include "stdafx.h"
#include "resource_counts.h"

optional<ResourceCounts> chooseResourceCounts(RandomGen& random, const vector<ResourceDistribution>& resources, int depth) {
  vector<ResourceCounts> available;
  for (auto& l : resources)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      available.push_back(l.counts);
  if (available.empty())
    return none;
  return random.choose(available);
}
