#pragma once

#include "stdafx.h"
#include "util.h"
#include "enums.h"
#include "view_id.h"

struct BestAttack {
  ViewId HASH(viewId);
  int HASH(value);
  HASH_ALL(viewId, value)
};
