#pragma once

#include "stdafx.h"
#include "util.h"
#include "tribe.h"
#include "random_layout_id.h"

struct KeeperBaseInfo {
  optional<RandomLayoutId> SERIAL(layout);
  Vec2 SERIAL(size);
  SERIALIZE_ALL(size, layout)
};

