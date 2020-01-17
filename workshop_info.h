#pragma once

#include "furniture_type.h"

struct WorkshopInfo {
  FurnitureType SERIAL(furniture);
  string SERIAL(name);
  SERIALIZE_ALL(furniture, name)
};
