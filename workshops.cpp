#include "stdafx.h"
#include "workshops.h"
#include "collective.h"
#include "item_factory.h"
#include "item.h"
#include "workshop_item.h"

Workshops::Workshops(std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size> options)
    : types([&options] (WorkshopType t) { return Type(options[(int) t].transform(
         [](const auto& elem){ return elem.get(); }));}) {
}

Workshops::Type& Workshops::get(WorkshopType type) {
  return types[type];
}

const Workshops::Type& Workshops::get(WorkshopType type) const {
  return types[type];
}

SERIALIZATION_CONSTRUCTOR_IMPL(Workshops)
SERIALIZE_DEF(Workshops, types)

Workshops::Type::Type(const vector<Item>& o) : options(o) {}

SERIALIZATION_CONSTRUCTOR_IMPL2(Workshops::Type, Type)
SERIALIZE_DEF(Workshops::Type, options, queued, debt)


const vector<Workshops::Item>& Workshops::Type::getOptions() const {
  return options;
}

void Workshops::Type::stackQueue() {
  vector<QueuedItem> tmp;
  for (auto& elem : queued)
    if (!tmp.empty() && elem.indexInWorkshop == tmp.back().indexInWorkshop &&
        elem.runes.empty() && tmp.back().runes.empty())
      tmp.back().number += elem.number;
    else
      tmp.push_back(std::move(elem));
  queued = std::move(tmp);
}

void Workshops::Type::addDebt(CostInfo cost) {
  debt[cost.id] += cost.value;
}

void Workshops::Type::queue(int index, int count) {
  CHECK(count > 0);
  addDebt(options[index].cost * count);
  queued.push_back(QueuedItem { options[index], index, count });
  stackQueue();
}

vector<PItem> Workshops::Type::unqueue(int index) {
  if (index >= 0 && index < queued.size()) {
    if (queued[index].state.value_or(0) == 0)
      addDebt(-queued[index].item.cost * queued[index].number);
    else
      addDebt(-queued[index].item.cost * (queued[index].number - 1));
    return queued.removeIndexPreserveOrder(index).runes;
  }
  stackQueue();
  return {};
}

void Workshops::Type::changeNumber(int index, int number) {
  CHECK(number > 0);
  if (index >= 0 && index < queued.size()) {
    auto& elem = queued[index];
    addDebt(CostInfo(elem.item.cost.id, elem.item.cost.value) * (number - elem.number));
    elem.number = number;
  }
}

static const double prodMult = 0.1;

double Workshops::getLegendarySkillThreshold() {
  return 0.9;
}

static bool allowUpgrades(double skillAmount, double morale) {
  return skillAmount >= Workshops::getLegendarySkillThreshold() && morale >= 0;
}

bool Workshops::Type::isIdle(WConstCollective collective, double skillAmount, double morale) const {
  for (auto& product : queued)
    if ((product.state || collective->hasResource(product.item.cost)) &&
        (allowUpgrades(skillAmount, morale) || product.runes.empty()))
      return false;
  return true;
}

void Workshops::Type::addUpgrade(int index, PItem rune) {
  if (queued[index].number > 1) {
    CHECK(queued[index].runes.empty());
    WorkshopQueuedItem item(queued[index].item, queued[index].indexInWorkshop, 1);
    item.runes.push_back(std::move(rune));
    --queued[index].number;
    queued.insert(index, std::move(item));
  } else
    queued[index].runes.push_back(std::move(rune));
}

PItem Workshops::Type::removeUpgrade(int itemIndex, int runeIndex) {
  auto ret = queued[itemIndex].runes.removeIndexPreserveOrder(runeIndex);
  stackQueue();
  return ret;
}

auto Workshops::Type::addWork(WCollective collective, double amount, double skillAmount, double morale) -> WorkshopResult {
  for (int productIndex : All(queued)) {
    auto& product = queued[productIndex];
    if ((product.state || collective->hasResource(product.item.cost)) &&
        (allowUpgrades(skillAmount, morale) || product.runes.empty())) {
      if (!product.state) {
        collective->takeResource(product.item.cost);
        addDebt(-product.item.cost);
        product.state = 0;
      }
      *product.state += amount * prodMult / product.item.workNeeded;
      if (*product.state >= 1) {
        vector<PItem> ret = product.item.type.get(product.item.batchSize);
        product.state = none;
        bool wasUpgraded = false;
        for (auto& rune : product.runes)
          for (auto& item : ret) {
            item->applyPrefix(rune->getUpgradeInfo()->prefix);
            wasUpgraded = true;
          }
        if (!--product.number)
          queued.removeIndexPreserveOrder(productIndex);
        return {std::move(ret), wasUpgraded};
      }
      break;
    }
  }
  return {{}, false};
}

const vector<Workshops::QueuedItem>& Workshops::Type::getQueued() const {
  return queued;
}

int Workshops::getDebt(CollectiveResourceId resource) const {
  int ret = 0;
  for (auto type : ENUM_ALL(WorkshopType))
    ret += types[type].debt[resource];
  return ret;
}

