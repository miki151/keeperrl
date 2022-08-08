#pragma once

#include "util.h"
#include "creature_predicate.h"
#include "attr_type.h"

struct SpecialAttr {
  AttrType SERIAL(attr);
  int SERIAL(value);
  CreaturePredicate SERIAL(predicate);
  SERIALIZE_ALL(attr, value, predicate)
};
