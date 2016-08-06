#pragma once

#include "cost_info.h"
#include "item_type.h"
#include "resource_id.h"

RICH_ENUM(WorkshopType,
    WORKSHOP,
    FORGE,
    LABORATORY,
    JEWELER
);

class Collective;

class Workshops {
  public:
  struct Item {
    static Item fromType(ItemType, CostInfo, double workNeeded, int batchSize = 1);
    bool operator == (const Item&) const;
    ItemType SERIAL(type);
    string SERIAL(name);
    ViewId SERIAL(viewId);
    CostInfo SERIAL(cost);
    bool SERIAL(active);
    int SERIAL(number);
    int SERIAL(batchSize);
    double SERIAL(workNeeded);
    optional<double> SERIAL(state);
    SERIALIZE_ALL(type, name, viewId, cost, active, number, batchSize, workNeeded, state);
  };

  class Type {
    public:
    Type(Workshops*, const vector<Item>& options);
    const vector<Item>& getOptions() const;
    const vector<Item>& getQueued() const;
    vector<PItem> addWork(double);
    void queue(int);
    void unqueue(int);
    void changeNumber(int index, int number);
    void scheduleItems(Collective*);
    bool isIdle() const;

    SERIALIZATION_DECL(Type);

    private:
    void stackQueue();
    void addCost(CostInfo);
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
  void scheduleItems(Collective*);

  private:
  EnumMap<WorkshopType, Type> SERIAL(types);
  EnumMap<CollectiveResourceId, int> SERIAL(debt);
};
