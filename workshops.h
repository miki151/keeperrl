#pragma once

#include "resource_id.h"
#include "workshop_type.h"

class WorkshopItem;
struct WorkshopItemCfg;
class Collective;
class CostInfo;

class Workshops {
  public:
  typedef WorkshopItem Item;
  class Type {
    public:
    Type(const vector<Item>& options);
    const vector<Item>& getOptions() const;
    const vector<Item>& getQueued() const;
    vector<PItem> addWork(double);
    void queue(int index, int count = 1);
    void unqueue(int index);
    void changeNumber(int index, int number);
    void scheduleItems(WCollective);
    bool isIdle() const;

    SERIALIZATION_DECL(Type);

    private:
    friend class Workshops;
    void stackQueue();
    void addDebt(CostInfo);
    vector<Item> SERIAL(options);
    vector<Item> SERIAL(queued);
    EnumMap<CollectiveResourceId, int> SERIAL(debt);
  };

  SERIALIZATION_DECL(Workshops)
  Workshops(std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size>);
  Workshops(const Workshops&) = delete;
  Type& get(WorkshopType);
  const Type& get(WorkshopType) const;
  int getDebt(CollectiveResourceId) const;
  void scheduleItems(WCollective);

  private:
  EnumMap<WorkshopType, Type> SERIAL(types);
};
