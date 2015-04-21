#ifndef _COST_INFO_H
#define _COST_INFO_H

#include "item.h"

struct CostInfo : public NamedTupleBase<CollectiveResourceId, int> {
  NAMED_TUPLE_STUFF(CostInfo);
  NAME_ELEM(0, id);
  NAME_ELEM(1, value);
};


#endif
