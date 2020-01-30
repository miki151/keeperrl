#pragma once

#include "item_type.h"
#include "cost_info.h"
#include "item_upgrade_info.h"
#include "view_id.h"
#include "tech_id.h"

struct WorkshopItem;

struct WorkshopItemCfg {
  WorkshopItem get(const ContentFactory*) const;
  ItemType SERIAL(item);
  double SERIAL(work) = 1;
  CostInfo SERIAL(cost);
  optional<TechId> SERIAL(tech);
  int SERIAL(batchSize) = 1;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  optional<string> SERIAL(requireIngredient);
  bool SERIAL(notArtifact) = false;
  SERIALIZE_ALL(NAMED(item), OPTION(work), OPTION(cost), NAMED(tech), OPTION(batchSize), NAMED(tutorialHighlight), OPTION(requireIngredient), OPTION(notArtifact))
};

struct WorkshopItem {
  ItemType SERIAL(type);
  string SERIAL(name);
  string SERIAL(pluralName);
  ViewId SERIAL(viewId);
  CostInfo SERIAL(cost);
  vector<string> SERIAL(description);
  int SERIAL(batchSize);
  double SERIAL(workNeeded);
  optional<TechId> SERIAL(techId);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  optional<ItemUpgradeType> SERIAL(upgradeType);
  int SERIAL(maxUpgrades);
  optional<string> SERIAL(requireIngredient);
  bool SERIAL(notArtifact) = false;
  SERIALIZE_ALL(type, name, pluralName, viewId, cost, batchSize, workNeeded, techId, description, tutorialHighlight, upgradeType, maxUpgrades, requireIngredient, notArtifact)
};

struct WorkshopQueuedItem {
  SERIALIZATION_CONSTRUCTOR(WorkshopQueuedItem)
  WorkshopQueuedItem(WorkshopItem item, int index, int number)
      : item(std::move(item)), indexInWorkshop(index), number(number) {}
  WorkshopQueuedItem(const WorkshopQueuedItem&) = delete;
  WorkshopQueuedItem(WorkshopQueuedItem&& o) = default;
  WorkshopQueuedItem& operator = (WorkshopQueuedItem&& o) = default;
  WorkshopItem SERIAL(item);
  int SERIAL(indexInWorkshop);
  int SERIAL(number);
  optional<double> SERIAL(state);
  vector<PItem> SERIAL(runes);
  SERIALIZE_ALL(item, runes, state, indexInWorkshop, number)
};
