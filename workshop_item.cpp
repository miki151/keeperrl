#include "stdafx.h"
#include "workshop_item.h"
#include "item_factory.h"
#include "item.h"
#include "view_object.h"

static string getItemName(Item* item, bool plural) {
  auto name = item->getName(plural);
  if (name.size() > 30)
    return item->getShortName(nullptr, plural);
  return name;
}

WorkshopItem WorkshopItemCfg::get(const ContentFactory* factory) const {
  // for some reason removing this line causes a linker error, probably a compiler bug
  auto t = tech;
  PItem elem = item.get(factory);
  vector<string> description;
  if (elem->getNameAndModifiers() != elem->getName())
    description.push_back(elem->getNameAndModifiers());
  description.append(elem->getDescription(factory));
  return WorkshopItem {
    item,
    getItemName(elem.get(), false),
    getItemName(elem.get(), true),
    elem->getViewObject().getViewIdList(),
    cost.value_or(elem->getCraftingCost()),
    std::move(description),
    work,
    tech,
    tutorialHighlight,
    elem->getAppliedUpgradeType(),
    elem->getMaxUpgrades(),
    requireIngredient,
    notArtifact,
    applyImmediately,
    materialTab
  };
}
