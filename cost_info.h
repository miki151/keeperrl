#ifndef _COST_INFO_H
#define _COST_INFO_H

#include "enums.h"
#include "util.h"

struct CostInfo {
  CostInfo() : id(CollectiveResourceId(0)), value(0) {}
  CostInfo(CollectiveResourceId i, int v) : id(i), value(v) {}
  CollectiveResourceId SERIAL(id);
  int SERIAL(value);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(id) & SVAR(value);
  }
  static CostInfo noCost() {
    return CostInfo{CollectiveResourceId(0), 0};
  }
};


#endif
