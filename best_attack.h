#pragma once

#include "stdafx.h"
#include "util.h"
#include "enums.h"

struct BestAttack {
  BestAttack(const Creature*);
  AttrType HASH(attr);
  double HASH(value);
  HASH_ALL(attr, value)
};
