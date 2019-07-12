#include "stdafx.h"
#include "workshop_item.h"
#include "item_factory.h"
#include "item.h"
#include "view_object.h"

WorkshopItem WorkshopItemCfg::get(const ContentFactory* factory) const {
  // for some reason removing this line causes a linker error, probably a compiler bug
  auto t = tech;
  PItem elem = item.get(factory);
  return {
    item,
    elem->getName(),
    elem->getName(true),
    elem->getViewObject().id(),
    cost,
    elem->getDescription(),
    batchSize,
    work,
    tech,
    tutorialHighlight,
    elem->getAppliedUpgradeType(),
    elem->getMaxUpgrades(),
    requireIngredient,
  };
}
