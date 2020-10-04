#include "stdafx.h"
#include "z_level_info.h"


optional<ZLevelInfo> chooseZLevel(RandomGen& random, const vector<ZLevelInfo>& levels, int depth) {
  vector<ZLevelInfo> available;
  for (auto& l : levels)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth) {
      if (l.guarantee)
        return l;
      available.push_back(l);
    }
  if (available.empty())
    return none;
  return random.choose(available);
}
