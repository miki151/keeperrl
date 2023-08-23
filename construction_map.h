#pragma once

#include "cost_info.h"
#include "util.h"
#include "unique_entity.h"
#include "position.h"
#include "furniture_type.h"
#include "furniture_layer.h"
#include "resource_id.h"
#include "position_map.h"
#include "storage_id.h"
#include "storage_positions.h"

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

  const vector<pair<Position, FurnitureLayer>>& getAllFurniture() const;
  int getDebt(CollectiveResourceId) const;

  const StoragePositions& getStoragePositions(StorageId) const;
  StoragePositions& getStoragePositions(StorageId);
  const StoragePositions& getAllStoragePositions() const;
  StoragePositions& getAllStoragePositions();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void checkDebtConsistency();
  EnumMap<FurnitureLayer, PositionMap<FurnitureInfo>> SERIAL(furniture);
  HashMap<FurnitureType, PositionSet> SERIAL(furniturePositions);
  HashMap<FurnitureType, int> SERIAL(unbuiltCounts);
  vector<pair<Position, FurnitureLayer>> SERIAL(allFurniture);
  HashMap<CollectiveResourceId, int> SERIAL(debt);
  void addDebt(const CostInfo&, const char* reason);
  HashMap<StorageId, StoragePositions> SERIAL(storagePositions);
  StoragePositions SERIAL(allStoragePositions);
};
