#include "stdafx.h"
#include "workshops.h"
#include "collective.h"
#include "item_factory.h"
#include "item.h"

Workshops::Workshops(const EnumMap<WorkshopType, vector<Item>>& options)
    : types([&options] (WorkshopType t) { return Type(options[t]);}) {
}

Workshops::Type& Workshops::get(WorkshopType type) {
  return types[type];
}

const Workshops::Type& Workshops::get(WorkshopType type) const {
  return types[type];
}

SERIALIZATION_CONSTRUCTOR_IMPL(Workshops);
SERIALIZE_DEF(Workshops, types);

Workshops::Type::Type(const vector<Item>& o) : options(o) {}

SERIALIZATION_CONSTRUCTOR_IMPL2(Workshops::Type, Type);
SERIALIZE_DEF(Workshops::Type, options, queued, debt);


const vector<Workshops::Item>& Workshops::Type::getOptions() const {
  return options;
}

void Workshops::Type::stackQueue() {
  vector<Item> tmp;
  for (auto& elem : queued)
    if (!tmp.empty() && elem == tmp.back())
      tmp.back().number += elem.number;
    else
      tmp.push_back(elem);
  queued = tmp;
}

void Workshops::Type::addDebt(CostInfo cost) {
  debt[cost.id] += cost.value;
}

void Workshops::Type::queue(int index, int count) {
  CHECK(count > 0);
  const Item& newElem = options[index];
  addDebt(newElem.cost * count);
  if (!queued.empty() && queued.back() == newElem)
    queued.back().number += count;
  else {
    queued.push_back(newElem);
    queued.back().number = count;
  }
  stackQueue();
}

void Workshops::Type::unqueue(int index) {
  if (index >= 0 && index < queued.size()) {
    if (queued[index].state.value_or(0) == 0)
      addDebt(-queued[index].cost * queued[index].number);
    else
      addDebt(-queued[index].cost * (queued[index].number - 1));
    queued.erase(queued.begin() + index);
  }
  stackQueue();
}

void Workshops::Type::changeNumber(int index, int number) {
  CHECK(number > 0);
  if (index >= 0 && index < queued.size()) {
    auto& elem = queued[index];
    addDebt(CostInfo(elem.cost.id, elem.cost.value) * (number - elem.number));
    elem.number = number;
  }
}

static const double prodMult = 0.1;

bool Workshops::Type::isIdle() const {
  return queued.empty() || !queued[0].state;
}

void Workshops::scheduleItems(WCollective collective) {
  for (auto type : ENUM_ALL(WorkshopType))
    types[type].scheduleItems(collective);
}

void Workshops::Type::scheduleItems(WCollective collective) {
  if (queued.empty() || queued[0].state)
    return;
  for (int i : All(queued))
    if (collective->hasResource(queued[i].cost)) {
      if (i > 0)
        swap(queued[0], queued[i]);
      collective->takeResource(queued[0].cost);
      addDebt(-queued[0].cost);
      queued[0].state = 0;
      return;
    }
}

vector<PItem> Workshops::Type::addWork(double amount) {
  if (!queued.empty() && queued[0].state) {
    auto& product = queued[0];
    *product.state += amount * prodMult / product.workNeeded;
    if (*product.state >= 1) {
      vector<PItem> ret = ItemFactory::fromId(product.type, product.batchSize);
      product.state = none;
      if (!--product.number)
        queued.erase(queued.begin());
      return ret;
    }
  }
  return {};
}

const vector<Workshops::Item>& Workshops::Type::getQueued() const {
  return queued;
}

int Workshops::getDebt(CollectiveResourceId resource) const {
  int ret = 0;
  for (auto type : ENUM_ALL(WorkshopType))
    ret += types[type].debt[resource];
  return ret;
}

