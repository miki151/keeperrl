#pragma once

#include "furniture_type.h"
#include "attr_type.h"
#include "item_prefix.h"
#include "resource_id.h"

struct WorkshopInfo {
  FurnitureType SERIAL(furniture);
  string SERIAL(name);
  string SERIAL(verb) = "produces";
  AttrType SERIAL(attr);
  HashMap<CollectiveResourceId, int> SERIAL(minAttr);
  int getMinAttrFor(CollectiveResourceId id) const {
    return getValueMaybe(minAttr, id).value_or(1);
  }
  optional<ItemPrefix> SERIAL(prefix);
  bool SERIAL(hideFromTech) = false;
  SERIALIZE_ALL(NAMED(furniture), NAMED(name), OPTION(verb), NAMED(attr), OPTION(minAttr), OPTION(prefix), OPTION(hideFromTech))
};
