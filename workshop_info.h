#pragma once

#include "furniture_type.h"
#include "attr_type.h"
#include "item_prefix.h"

struct WorkshopInfo {
  FurnitureType SERIAL(furniture);
  string SERIAL(name);
  string SERIAL(verb) = "produces";
  AttrType SERIAL(attr);
  int SERIAL(minAttr) = 1;
  optional<ItemPrefix> SERIAL(prefix);
  bool SERIAL(hideFromTech) = false;
  SERIALIZE_ALL(NAMED(furniture), NAMED(name), OPTION(verb), NAMED(attr), OPTION(minAttr), OPTION(prefix), OPTION(hideFromTech))
};
