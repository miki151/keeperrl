#include "stdafx.h"
#include "cost_info.h"


CostInfo::CostInfo(CollectiveResourceId i, int v) : id(i), value(v) {}

SERIALIZATION_CONSTRUCTOR_IMPL(CostInfo);
SERIALIZE_DEF(CostInfo, id, value);

CostInfo CostInfo::noCost() {
  return CostInfo{CollectiveResourceId(0), 0};
}

CostInfo CostInfo::operator-() const {
  return CostInfo(id, -value);
}
