#pragma once

#include "cost_info.h"
#include "util.h"
#include "unique_entity.h"
#include "position.h"
#include "furniture_type.h"
#include "furniture_layer.h"
#include "resource_id.h"
#include "position_map.h"

class ConstructionMap {
  public:

  class FurnitureInfo {
    public:
    FurnitureInfo(FurnitureType, CostInfo);
    static FurnitureInfo getBuilt(FurnitureType);
    void clearTask();
    void reset();
    void setTask(UniqueEntity<Task>::Id);
    CostInfo getCost() const;
    bool isBuilt(Position, FurnitureLayer) const;
    UniqueEntity<Task>::Id getTask() const;
    bool hasTask() const;
    FurnitureType getFurnitureType() const;
    const Furniture* getBuilt(Position) const;

    SERIALIZATION_DECL(FurnitureInfo)

    private:
    GenericId SERIAL(task);
    CostInfo SERIAL(cost);
    FurnitureType SERIAL(type);
  };

  class TrapInfo {
    public:
    TrapInfo(FurnitureType);
    bool isMarked() const;
    bool isArmed() const;
    FurnitureType getType() const;
    void setArmed();
    void setMarked();
    void reset();

    SERIALIZATION_DECL(TrapInfo)

    private:
    FurnitureType SERIAL(type);
    bool SERIAL(armed) = false;
    bool SERIAL(marked) = false;
  };

  optional<const FurnitureInfo&> getFurniture(Position, FurnitureLayer) const;
  void setTask(Position, FurnitureLayer, UniqueEntity<Task>::Id);
  void removeFurniturePlan(Position, FurnitureLayer);
  void onFurnitureDestroyed(Position, FurnitureLayer, FurnitureType);
  void addFurniture(Position, const FurnitureInfo&, FurnitureLayer);
  bool containsFurniture(Position, FurnitureLayer) const;
  int getBuiltCount(FurnitureType) const;
  int getTotalCount(FurnitureType) const;
  const PositionSet& getBuiltPositions(FurnitureType) const;
  void onConstructed(Position, FurnitureType);
  void clearUnsupportedFurniturePlans();

  optional<const TrapInfo&> getTrap(Position) const;
  optional<TrapInfo&> getTrap(Position);
  void removeTrap(Position);
  void addTrap(Position, const TrapInfo&);

  const vector<pair<Position, FurnitureLayer>>& getAllFurniture() const;
  const vector<Position>& getAllTraps() const;
  int getDebt(CollectiveResourceId) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void checkDebtConsistency();
  EnumMap<FurnitureLayer, PositionMap<FurnitureInfo>> SERIAL(furniture);
  unordered_map<FurnitureType, PositionSet, CustomHash<FurnitureType>> SERIAL(furniturePositions);
  unordered_map<FurnitureType, int, CustomHash<FurnitureType>> SERIAL(unbuiltCounts);
  vector<pair<Position, FurnitureLayer>> SERIAL(allFurniture);
  PositionMap<TrapInfo> SERIAL(traps);
  vector<Position> SERIAL(allTraps);
  unordered_map<CollectiveResourceId, int, CustomHash<CollectiveResourceId>> SERIAL(debt);
  void addDebt(const CostInfo&, const char* reason);
};
