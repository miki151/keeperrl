#pragma once

#include "furniture_type.h"

struct WorkshopInfo {
  FurnitureType SERIAL(furniture);
  string SERIAL(name);
  string SERIAL(verb) = "produces";
  SERIALIZE_ALL(NAMED(furniture), NAMED(name), OPTION(verb))
};
