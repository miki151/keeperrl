#pragma once
#include "stdafx.h"
#include "cost_info.h"

class Collective;
class CostInfo;
class ContentFactory;

class Furnace {
  public:
  struct QueuedItem {
    PItem SERIAL(item);
    double SERIAL(state);
    SERIALIZE_ALL(item, state)
  };
  const vector<QueuedItem>& getQueued() const;
  optional<CostInfo> addWork(double workAmount);
  void queue(PItem);
  PItem unqueue(int index);
  bool isIdle() const;
  CostInfo getRecycledAmount(const Item*) const;

  SERIALIZATION_DECL(Furnace)

  private:
  vector<QueuedItem> SERIAL(queued);
};
