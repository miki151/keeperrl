#include "stdafx.h"
#include "workshops.h"
#include "item_factory.h"
#include "view_object.h"
#include "item.h"

Workshops::Workshops(const EnumMap<WorkshopType, vector<Item>>& o)
    : types(o.mapValues<Type>([] (const vector<Item>& v) { return Type(v);})) {
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
SERIALIZE_DEF(Workshops::Type, options, queued);


const vector<Workshops::Item>& Workshops::Type::getOptions() const {
  return options;
}

bool Workshops::Item::operator == (const Item& item) const {
  return type == item.type;
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

void Workshops::Type::queue(int index) {
  const Item& newElem = options[index];
  if (!queued.empty() && queued.back() == newElem)
    queued.back().number += newElem.number;
  else
    queued.push_back(newElem);
  stackQueue();
}

void Workshops::Type::unqueue(int index) {
  if (index >= 0 && index < queued.size())
    queued.erase(queued.begin() + index);
  stackQueue();
}

void Workshops::Type::changeNumber(int index, int number) {
  if (number <= 0)
    unqueue(index);
  else {
    auto& elems = queued;
    if (index >= 0 && index < elems.size())
      elems[index].number = number;
  }
}

static const double prodMult = 0.1;

vector<PItem> Workshops::Type::addWork(double amount) {
  if (!queued.empty()) {
    auto& product = queued[0];
    product.state += amount * prodMult / product.workNeeded;
    if (product.state >= 1) {
      vector<PItem> ret = ItemFactory::fromId(product.type, product.batchSize);
      product.state = 0;
      changeNumber(0, product.number - 1);
      return ret;
    }
  }
  return {};
}

const vector<Workshops::Item>& Workshops::Type::getQueued() const {
  return queued;
}

Workshops::Item Workshops::Item::fromType(ItemType type, CostInfo cost, double workNeeded, int batchSize) {
  PItem item = ItemFactory::fromId(type);
  return {
    type,
    item->getPluralName(batchSize),
    item->getViewObject().id(),
    cost,
    true,
    1,
    batchSize,
    workNeeded,
    0
  };
}

