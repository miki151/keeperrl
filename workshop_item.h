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
  optional<CostInfo> SERIAL(cost);
  optional<TechId> SERIAL(tech);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  optional<string> SERIAL(requireIngredient);
  bool SERIAL(notArtifact) = false;
  bool SERIAL(applyImmediately) = false;
  bool SERIAL(materialTab) = false;
  SERIALIZE_ALL(NAMED(item), OPTION(work), NAMED(cost), NAMED(tech), NAMED(tutorialHighlight), OPTION(requireIngredient), OPTION(notArtifact), OPTION(applyImmediately), OPTION(materialTab))
};

static_assert(std::is_nothrow_move_constructible<WorkshopItemCfg>::value, "T should be noexcept MoveConstructible");


struct WorkshopItem {
  ItemType SERIAL(type);
  string SERIAL(name);
  string SERIAL(pluralName);
  ViewIdList SERIAL(viewId);
  CostInfo SERIAL(cost);
  vector<string> SERIAL(description);
  double SERIAL(workNeeded);
  optional<TechId> SERIAL(techId);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  optional<ItemUpgradeType> SERIAL(upgradeType);
  int SERIAL(maxUpgrades);
  optional<string> SERIAL(requireIngredient);
  bool SERIAL(notArtifact) = false;
  bool SERIAL(applyImmediately) = false;
  bool SERIAL(materialTab) = false;
  SERIALIZE_ALL(type, name, pluralName, viewId, cost, workNeeded, techId, description, tutorialHighlight, upgradeType, maxUpgrades, requireIngredient, notArtifact, applyImmediately, materialTab)
};

struct WorkshopQueuedItem {
  SERIALIZATION_CONSTRUCTOR(WorkshopQueuedItem)
  WorkshopQueuedItem(WorkshopItem item, int index, bool paid)
      : item(std::move(item)), indexInWorkshop(index), paid(paid) {}
  WorkshopQueuedItem(const WorkshopQueuedItem&) = delete;
  WorkshopQueuedItem(WorkshopQueuedItem&& o) = default;
  WorkshopQueuedItem& operator = (WorkshopQueuedItem&& o) = default;
  WorkshopItem SERIAL(item);
  int SERIAL(indexInWorkshop);
  double SERIAL(state) = 0;
  bool SERIAL(paid);
  vector<PItem> SERIAL(runes);
  SERIALIZE_ALL(item, runes, state, paid, indexInWorkshop)
};
