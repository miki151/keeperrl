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
    void setBuilt();
    void reset();
    void setTask(UniqueEntity<Task>::Id);
    CostInfo getCost() const;
    bool isBuilt() const;
    UniqueEntity<Task>::Id getTask() const;
    bool hasTask() const;
    FurnitureType getFurnitureType() const;
    FurnitureLayer getLayer() const;

    SERIALIZATION_DECL(FurnitureInfo);

    private:
    CostInfo SERIAL(cost);
    bool SERIAL(built) = false;
    FurnitureType SERIAL(type);
    optional<UniqueEntity<Task>::Id> SERIAL(task);
  };

  class TrapInfo {
    public:
    TrapInfo(TrapType);
    bool isMarked() const;
    bool isArmed() const;
    TrapType getType() const;
    void setArmed();
    void setTask(WConstTask);
    void reset();

    SERIALIZATION_DECL(TrapInfo);

    private:
    TrapType SERIAL(type);
    bool SERIAL(armed) = false;
    WConstTask SERIAL(task);
  };

  optional<const FurnitureInfo&> getFurniture(Position, FurnitureLayer) const;
  void setTask(Position, FurnitureLayer, UniqueEntity<Task>::Id);
  void removeFurniture(Position, FurnitureLayer);
  void onFurnitureDestroyed(Position, FurnitureLayer);
  void addFurniture(Position, const FurnitureInfo&);
  bool containsFurniture(Position, FurnitureLayer) const;
  int getBuiltCount(FurnitureType) const;
  int getTotalCount(FurnitureType) const;
  const PositionSet& getBuiltPositions(FurnitureType) const;
  void onConstructed(Position, FurnitureType);

  optional<const TrapInfo&> getTrap(Position) const;
  optional<TrapInfo&> getTrap(Position);
  void removeTrap(Position);
  void addTrap(Position, const TrapInfo&);

  const vector<Position>& getSquares() const;
  const vector<pair<Position, FurnitureLayer>>& getAllFurniture() const;
  const vector<Position>& getAllTraps() const;
  int getDebt(CollectiveResourceId) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EnumMap<FurnitureLayer, PositionMap<FurnitureInfo>> SERIAL(furniture);
  EnumMap<FurnitureType, PositionSet> SERIAL(furniturePositions);
  EnumMap<FurnitureType, int> SERIAL(unbuiltCounts);
  vector<pair<Position, FurnitureLayer>> SERIAL(allFurniture);
  PositionMap<TrapInfo> SERIAL(traps);
  vector<Position> SERIAL(allTraps);
  EnumMap<CollectiveResourceId, int> SERIAL(debt);
  void addDebt(const CostInfo&);
};
