#pragma once

#include "resource_id.h"
#include "workshop_type.h"
#include "workshop_item.h"

class Collective;

class Workshops {
  public:
  typedef WorkshopItem Item;
  class Type {
    public:
    Type(Workshops*, const vector<Item>& options);
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
    void stackQueue();
    void addDebt(CostInfo);
    vector<Item> SERIAL(options);
    vector<Item> SERIAL(queued);
    Workshops* SERIAL(workshops) = nullptr;
  };

  SERIALIZATION_DECL(Workshops);
  Workshops(const EnumMap<WorkshopType, vector<Item>>&);
  Workshops(const Workshops&) = delete;
  Type& get(WorkshopType);
  const Type& get(WorkshopType) const;
  int getDebt(CollectiveResourceId) const;
  void scheduleItems(WCollective);

  private:
  EnumMap<WorkshopType, Type> SERIAL(types);
  EnumMap<CollectiveResourceId, int> SERIAL(debt);
};
