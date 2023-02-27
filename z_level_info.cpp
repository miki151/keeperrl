#include "stdafx.h"
#include "z_level_info.h"


optional<ZLevelType> chooseZLevel(RandomGen& random, const vector<ZLevelInfo>& levels, int depth) {
  vector<ZLevelType> available;
  vector<double> weights;
  for (auto& l : levels)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth) {
      if (l.guarantee)
        return l.type;
      available.push_back(l.type);
      weights.push_back(l.weight);
    }
  if (available.empty())
    return none;
  return random.choose(available, weights);
}
