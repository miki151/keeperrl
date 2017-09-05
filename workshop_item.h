#pragma once

#include "item_type.h"
#include "cost_info.h"

struct WorkshopItem {
  static WorkshopItem fromType(ItemType, double workNeeded, CostInfo);
  WorkshopItem& setBatchSize(int);
  WorkshopItem& setTechId(TechId);
  WorkshopItem& setTutorialHighlight(TutorialHighlight);
  ItemType SERIAL(type);
  string SERIAL(name);
  ViewId SERIAL(viewId);
  CostInfo SERIAL(cost);
  string SERIAL(description);
  int SERIAL(number);
  int SERIAL(batchSize);
  double SERIAL(workNeeded);
  optional<double> SERIAL(state);
  optional<TechId> SERIAL(techId);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  int SERIAL(indexInWorkshop);
  SERIALIZE_ALL(type, name, viewId, cost, number, batchSize, workNeeded, state, techId, description, tutorialHighlight, indexInWorkshop)
};


