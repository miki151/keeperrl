#include "stdafx.h"
#include "cost_info.h"
#include "resource_id.h"


CostInfo::CostInfo(CollectiveResourceId i, int v) : id(i), value(v) {}

SERIALIZATION_CONSTRUCTOR_IMPL(CostInfo);
SERIALIZE_DEF(CostInfo, id, value);

CostInfo CostInfo::noCost() {
  return CostInfo{CollectiveResourceId("GOLD"), 0};
}

CostInfo CostInfo::operator - () const {
  return CostInfo(id, -value);
}

CostInfo CostInfo::operator * (int a) const {
  return CostInfo(id, a * value);
}

CostInfo CostInfo::operator / (int a) const {
  return CostInfo(id, value / a);
}

#include "pretty_archive.h"
template void CostInfo::serialize(PrettyInputArchive&, unsigned);
