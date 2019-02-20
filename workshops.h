#pragma once

#include "resource_id.h"
#include "workshop_type.h"

class WorkshopItem;
class WorkshopQueuedItem;
struct WorkshopItemCfg;
class Collective;
class CostInfo;

class Workshops {
  public:
  typedef WorkshopItem Item;
  typedef WorkshopQueuedItem QueuedItem;

  static double getLegendarySkillThreshold();

  class Type {
    public:
    Type(const vector<Item>& options);
    const vector<Item>& getOptions() const;
    const vector<QueuedItem>& getQueued() const;
    struct WorkshopResult {
      vector<PItem> items;
      bool wasUpgraded;
    };
    WorkshopResult addWork(WCollective, double workAmount, double skillAmount, double morale);
    void queue(int index, int count = 1);
    vector<PItem> unqueue(int index);
    void changeNumber(int index, int number);
    bool isIdle(WConstCollective, double skillAmount, double morale) const;
    void addUpgrade(int index, PItem);
    PItem removeUpgrade(int itemIndex, int runeIndex);

    SERIALIZATION_DECL(Type);

    private:
    friend class Workshops;
    void stackQueue();
    void addDebt(CostInfo);
    vector<Item> SERIAL(options);
    vector<QueuedItem> SERIAL(queued);
    EnumMap<CollectiveResourceId, int> SERIAL(debt);
  };

  SERIALIZATION_DECL(Workshops)
  Workshops(std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size>);
  Workshops(const Workshops&) = delete;
  Type& get(WorkshopType);
  const Type& get(WorkshopType) const;
  int getDebt(CollectiveResourceId) const;

  private:
  EnumMap<WorkshopType, Type> SERIAL(types);
};
