#include "furnace.h"
#include "item.h"
#include "serialization.h"

SERIALIZE_DEF(Furnace, queued)
SERIALIZATION_CONSTRUCTOR_IMPL(Furnace)

const vector<Furnace::QueuedItem>& Furnace::getQueued() const {
  return queued;
}

CostInfo Furnace::getRecycledAmount(const Item* item) const {
  return item->getCraftingCost() / 2;
}

optional<CostInfo> Furnace::addWork(double workAmount) {
  if (!queued.empty()) {
    queued[0].state += workAmount / 20.0;
    if (queued[0].state >= 1.0) {
      auto cost = getRecycledAmount(queued[0].item.get());
      queued.removeIndexPreserveOrder(0);
      return cost;
    }
  }
  return none;
}

void Furnace::queue(PItem item) {
  queued.push_back(QueuedItem{std::move(item), 0.0});
}

PItem Furnace::unqueue(int index) {
  return queued.removeIndexPreserveOrder(index).item;
}

bool Furnace::isIdle() const {
  return queued.empty();
}