#pragma once

#include "stdafx.h"
#include "util.h"
#include "enums.h"

struct BestAttack {
  BestAttack(WConstCreature);
  AttrType HASH(attr);
  double HASH(value);
  HASH_ALL(attr, value);
};
