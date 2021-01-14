#pragma once

#include "util.h"
#include "creature_predicate.h"

struct SpecialAttr {
  AttrType SERIAL(attr);
  int SERIAL(value);
  CreaturePredicate SERIAL(predicate);
  SERIALIZE_ALL(attr, value, predicate)
};
