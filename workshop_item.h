#pragma once

#include "item_type.h"
#include "cost_info.h"

struct WorkshopItem;

struct WorkshopItemCfg {
  WorkshopItem get() const;
  ItemType SERIAL(item);
  double SERIAL(work) = 1;
  CostInfo SERIAL(cost);
  optional<TechId> SERIAL(tech);
  int SERIAL(batchSize) = 1;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  SERIALIZE_ALL(NAMED(item), OPTION(work), OPTION(cost), NAMED(tech), OPTION(batchSize), NAMED(tutorialHighlight))
};

struct WorkshopItem {
  ItemType SERIAL(type);
  string SERIAL(name);
  string SERIAL(pluralName);
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
  SERIALIZE_ALL(type, name, pluralName, viewId, cost, number, batchSize, workNeeded, state, techId, description, tutorialHighlight, indexInWorkshop)
};


