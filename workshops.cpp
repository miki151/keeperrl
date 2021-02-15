#include "stdafx.h"
#include "workshops.h"
#include "collective.h"
#include "item_factory.h"
#include "item.h"
#include "workshop_item.h"
#include "game.h"

Workshops::Workshops(WorkshopArray options, const ContentFactory* factory) {
  for (auto& elem : options)
    types.insert(make_pair(elem.first, Type(elem.second.transform(
        [&](const auto& elem){ return elem.get(factory); }))));
}

SERIALIZATION_CONSTRUCTOR_IMPL(Workshops)
SERIALIZE_DEF(Workshops, types)

Workshops::Type::Type(const vector<Item>& o) : options(o) {}

SERIALIZATION_CONSTRUCTOR_IMPL2(Workshops::Type, Type)
SERIALIZE_DEF(Workshops::Type, options, queued, debt)


const vector<Workshops::Item>& Workshops::Type::getOptions() const {
  return options;
}

void Workshops::Type::addDebt(CostInfo cost) {
  debt[cost.id] += cost.value;
  CHECK(debt[cost.id] >= 0);
}

void Workshops::Type::checkDebtConsistency() const {
  unordered_map<CollectiveResourceId, int, CustomHash<CollectiveResourceId>> nowDebt;
  for (auto& elem : queued) {
    if (!elem.paid)
      nowDebt[options[elem.indexInWorkshop].cost.id] += options[elem.indexInWorkshop].cost.value;
  }
  for (auto& elem : nowDebt)
    CHECK(getValueMaybe(debt, elem.first).value_or(0) == elem.second);
  for (auto& elem : debt)
    CHECK(getValueMaybe(nowDebt, elem.first).value_or(0) == elem.second);
}

void Workshops::Type::queue(Collective* collective, int index, optional<int> queueIndex) {
  addDebt(options[index].cost);
  queued.insert(queueIndex.value_or(queued.size()), QueuedItem { options[index], index, false});
  updateState(collective);
}

void Workshops::Type::updateState(Collective* collective) {
  for (auto& elem : queued)
    if (!elem.paid && collective->hasResource(elem.item.cost)) {
      elem.paid = true;
      collective->takeResource(elem.item.cost);
      addDebt(-elem.item.cost);
    }
}

vector<PItem> Workshops::Type::unqueue(Collective* collective, int index) {
  if (index >= 0 && index < queued.size()) {
    auto& elem = queued[index];
    if (elem.paid) {
      if (elem.state == 0)
        collective->returnResource(elem.item.cost);
    } else
      addDebt(-elem.item.cost);
    auto ret = queued.removeIndexPreserveOrder(index).runes;
    updateState(collective);
    return ret;
  }
  return {};
}

static const double prodMult = 0.1;

double Workshops::getLegendarySkillThreshold() {
  return 0.90;
}

static bool allowUpgrades(const WorkshopQueuedItem& item, double skillAmount, double morale) {
  return item.runes.empty() || item.item.notArtifact ||
      (skillAmount >= Workshops::getLegendarySkillThreshold() - 0.01 && morale >= 0);
}

bool Workshops::Type::isIdle(const Collective* collective, double skillAmount, double morale) const {
  for (auto& product : queued)
    if ((product.paid || collective->hasResource(product.item.cost)) && allowUpgrades(product, skillAmount, morale))
      return false;
  return true;
}

void Workshops::Type::addUpgrade(int index, PItem rune) {
  queued[index].runes.push_back(std::move(rune));
  checkDebtConsistency();
}

PItem Workshops::Type::removeUpgrade(int itemIndex, int runeIndex) {
  auto ret = queued[itemIndex].runes.removeIndexPreserveOrder(runeIndex);
  return ret;
}

auto Workshops::Type::addWork(Collective* collective, double amount, double skillAmount, double morale) -> WorkshopResult {
  for (int productIndex : All(queued)) {
    auto& product = queued[productIndex];
    if ((product.paid || collective->hasResource(product.item.cost)) && allowUpgrades(product, skillAmount, morale)) {
      if (!product.paid) {
        collective->takeResource(product.item.cost);
        addDebt(-product.item.cost);
        product.paid = true;
      }
      product.state += amount * prodMult / product.item.workNeeded;
      if (product.state >= 1) {
        auto ret = product.item.type.get(collective->getGame()->getContentFactory());
        bool wasUpgraded = false;
        for (auto& rune : product.runes) {
          if (auto& upgradeInfo = rune->getUpgradeInfo())
            ret->applyPrefix(*upgradeInfo->prefix, collective->getGame()->getContentFactory());
          wasUpgraded = !product.item.notArtifact;
        }
        bool applyImmediately = product.item.applyImmediately;
        queued.removeIndexPreserveOrder(productIndex);
        checkDebtConsistency();
        return {std::move(ret), wasUpgraded, applyImmediately};
      }
      break;
    }
  }
  checkDebtConsistency();
  return WorkshopResult{{}, false, false};
}

const vector<Workshops::QueuedItem>& Workshops::Type::getQueued() const {
  return queued;
}

int Workshops::getDebt(CollectiveResourceId resource) const {
  int ret = 0;
  for (auto& elem : types)
    ret += getValueMaybe(elem.second.debt, resource).value_or(0);
  return ret;
}

vector<WorkshopType> Workshops::getWorkshopsTypes() const {
  return getKeys(types);
}

