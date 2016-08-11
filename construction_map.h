#ifndef _CONSTRUCTION_MAP_H
#define _CONSTRUCTION_MAP_H

#include "cost_info.h"
#include "util.h"
#include "square_type.h"
#include "unique_entity.h"
#include "position.h"
#include "furniture_type.h"

class ConstructionMap {
  public:

  class SquareInfo {
    public:
    SquareInfo(SquareType, CostInfo);
    void setBuilt();
    void reset();
    void setTask(UniqueEntity<Task>::Id);
    CostInfo getCost() const;
    bool isBuilt() const;
    UniqueEntity<Task>::Id getTask() const;
    bool hasTask() const;
    const SquareType& getSquareType() const;

    SERIALIZATION_DECL(SquareInfo);

    private:
    CostInfo SERIAL(cost);
    bool SERIAL(built) = false;
    SquareType SERIAL(type);
    optional<UniqueEntity<Task>::Id> SERIAL(task);
  };

  class FurnitureInfo {
    public:
    FurnitureInfo(FurnitureType, CostInfo);
    void setBuilt();
    void reset();
    void setTask(UniqueEntity<Task>::Id);
    CostInfo getCost() const;
    bool isBuilt() const;
    UniqueEntity<Task>::Id getTask() const;
    bool hasTask() const;
    const FurnitureType& getFurnitureType() const;

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

  class TorchInfo {
    public:
    TorchInfo(Dir);
    Dir getAttachmentDir() const;
    UniqueEntity<Task>::Id getTask() const;
    bool hasTask() const;
    bool isBuilt() const;
    Trigger* getTrigger();
    void setTask(UniqueEntity<Task>::Id);
    void setBuilt(Trigger*);

    SERIALIZATION_DECL(TorchInfo);
    
    private:
    bool SERIAL(built) = false;
    optional<UniqueEntity<Task>::Id> SERIAL(task);
    Dir SERIAL(attachmentDir);
    Trigger* SERIAL(trigger) = nullptr;
  };

  const SquareInfo& getSquare(Position) const;
  SquareInfo& getSquare(Position);
  void removeSquare(Position);
  void onSquareDestroyed(Position);
  void addSquare(Position, const SquareInfo&);
  bool containsSquare(Position) const;
  int getSquareCount(SquareType) const;

  const FurnitureInfo& getFurniture(Position) const;
  FurnitureInfo& getFurniture(Position);
  void removeFurniture(Position);
  void onFurnitureDestroyed(Position);
  void addFurniture(Position, const FurnitureInfo&);
  bool containsFurniture(Position) const;
  int getFurnitureCount(FurnitureType) const;
  const set<Position>& getFurniturePositions(FurnitureType) const;
  void onConstructed(Position, FurnitureType);

  const TrapInfo& getTrap(Position) const;
  TrapInfo& getTrap(Position);
  void removeTrap(Position);
  void addTrap(Position, const TrapInfo&);
  bool containsTrap(Position) const;

  const TorchInfo& getTorch(Position) const;
  TorchInfo& getTorch(Position);
  void removeTorch(Position);
  void addTorch(Position, const TorchInfo&);
  bool containsTorch(Position) const;
  const vector<Position>& getSquares() const;
  const vector<Position>& getAllFurniture() const;
  const map<Position, TrapInfo>& getTraps() const;
  const map<Position, TorchInfo>& getTorches() const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  map<Position, vector<SquareInfo>> SERIAL(squares);
  map<Position, FurnitureInfo> SERIAL(furniture);
  EnumMap<FurnitureType, set<Position>> SERIAL(furniturePositions);
  EnumMap<FurnitureType, int> SERIAL(furnitureCounts);
  vector<Position> squarePos;
  vector<Position> allFurniture;
  unordered_map<SquareType, int> SERIAL(typeCounts);
  map<Position, TrapInfo> SERIAL(traps);
  map<Position, TorchInfo> SERIAL(torches);
};

#endif
