#pragma once

#include "item_type.h"
#include "cost_info.h"

struct WorkshopItem {
  static WorkshopItem fromType(ItemType, double workNeeded, CostInfo);
  WorkshopItem& setBatchSize(int);
  WorkshopItem& setTechId(TechId);
  bool operator == (const WorkshopItem&) const;
  ItemType SERIAL(type);
  string SERIAL(name);
  ViewId SERIAL(viewId);
  CostInfo SERIAL(cost);
  int SERIAL(number);
  int SERIAL(batchSize);
  double SERIAL(workNeeded);
  optional<double> SERIAL(state);
  optional<TechId> SERIAL(techId);
  SERIALIZE_ALL(type, name, viewId, cost, number, batchSize, workNeeded, state, techId);
};


