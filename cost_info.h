#ifndef _COST_INFO_H
#define _COST_INFO_H

#include "enums.h"
#include "util.h"

struct CostInfo {
  CollectiveResourceId SERIAL(id);
  int SERIAL(value);
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(id) & SVAR(value);
  }
  static CostInfo noCost() {
    return CostInfo{CollectiveResourceId(0), 0};
  }
};


#endif
