#pragma once

#include "stdafx.h"
#include "furniture_type.h"

struct LayoutMapping {
  map<string, FurnitureType> SERIAL(furniture);
  SERIALIZE_ALL(NAMED(furniture))
};
