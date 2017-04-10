#pragma once

#include "cost_info.h"
#include "util.h"
#include "square_type.h"
#include "unique_entity.h"
#include "position.h"
#include "furniture_type.h"
#include "furniture_layer.h"
#include "resource_id.h"

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
    void setMarked();
    void reset();

    SERIALIZATION_DECL(TrapInfo);

    private:
    TrapType SERIAL(type);
    bool SERIAL(armed) = false;
    bool SERIAL(marked) = false;
  };

  optional<const FurnitureInfo&> getFurniture(Position, FurnitureLayer) const;
  void setTask(Position, FurnitureLayer, UniqueEntity<Task>::Id);
  void removeFurniture(Position, FurnitureLayer);
  void onFurnitureDestroyed(Position, FurnitureLayer);
  void addFurniture(Position, const FurnitureInfo&);
  bool containsFurniture(Position, FurnitureLayer) const;
  int getBuiltCount(FurnitureType) const;
  int getTotalCount(FurnitureType) const;
  const set<Position>& getBuiltPositions(FurnitureType) const;
  void onConstructed(Position, FurnitureType);

  const TrapInfo& getTrap(Position) const;
  TrapInfo& getTrap(Position);
  void removeTrap(Position);
  void addTrap(Position, const TrapInfo&);
  bool containsTrap(Position) const;

  const vector<Position>& getSquares() const;
  const vector<pair<Position, FurnitureLayer>>& getAllFurniture() const;
  const map<Position, TrapInfo>& getTraps() const;
  int getDebt(CollectiveResourceId) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  optional<FurnitureInfo&> modFurniture(Position, FurnitureLayer);
  EnumMap<FurnitureLayer, map<Position, FurnitureInfo>> SERIAL(furniture);
  EnumMap<FurnitureType, set<Position>> SERIAL(furniturePositions);
  EnumMap<FurnitureType, int> SERIAL(unbuiltCounts);
  vector<pair<Position, FurnitureLayer>> SERIAL(allFurniture);
  map<Position, TrapInfo> SERIAL(traps);
  EnumMap<CollectiveResourceId, int> SERIAL(debt);
  void addDebt(const CostInfo&);
};
