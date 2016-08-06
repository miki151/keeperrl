#pragma once

#include "enums.h"
#include "util.h"

class CostInfo {
  public:
  CostInfo(CollectiveResourceId, int amount);
  static CostInfo noCost();

  CostInfo operator-() const;

  SERIALIZATION_DECL(CostInfo);

  CollectiveResourceId SERIAL(id) = CollectiveResourceId(0);
  int SERIAL(value) = 0;
};

