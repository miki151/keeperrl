#pragma once

#include "enums.h"
#include "util.h"
#include "resource_id.h"

class CostInfo {
  public:
  CostInfo(CollectiveResourceId, int amount);
  static CostInfo noCost();

  CostInfo operator - () const;
  CostInfo operator * (int) const;

  SERIALIZATION_DECL(CostInfo)

  CollectiveResourceId SERIAL(id) = CollectiveResourceId(0);
  short SERIAL(value) = 0;
};

