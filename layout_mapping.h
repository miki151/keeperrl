#pragma once

#include "stdafx.h"
#include "furniture_type.h"

struct LayoutMapping {
  map<string, FurnitureType> SERIAL(furniture);
  map<string, SquareAttrib> SERIAL(flags);
  optional<string> SERIAL(downStairs);
  optional<string> SERIAL(upStairs);
  SERIALIZE_ALL(NAMED(downStairs), NAMED(flags), NAMED(upStairs), NAMED(furniture))
};
