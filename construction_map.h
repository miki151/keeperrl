#ifndef _CONSTRUCTION_MAP_H
#define _CONSTRUCTION_MAP_H

#include "cost_info.h"
#include "util.h"
#include "square_type.h"
#include "unique_entity.h"

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
    SquareType getSquareType() const;

    SERIALIZATION_DECL(SquareInfo);

    private:
    CostInfo SERIAL(cost);
    bool SERIAL(built) = false;
    SquareType SERIAL(type);
    UniqueEntity<Task>::Id SERIAL(task) = -1;
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
    UniqueEntity<Task>::Id SERIAL(task) = -1;
    Dir SERIAL(attachmentDir);
    Trigger* SERIAL(trigger) = nullptr;
  };

  const SquareInfo& getSquare(Vec2) const;
  SquareInfo& getSquare(Vec2);
  void removeSquare(Vec2);
  void addSquare(Vec2, const SquareInfo&);
  bool containsSquare(Vec2) const;
  int getSquareCount(SquareType) const;

  const TrapInfo& getTrap(Vec2) const;
  TrapInfo& getTrap(Vec2);
  void removeTrap(Vec2);
  void addTrap(Vec2, const TrapInfo&);
  bool containsTrap(Vec2) const;

  const TorchInfo& getTorch(Vec2) const;
  TorchInfo& getTorch(Vec2);
  void removeTorch(Vec2);
  void addTorch(Vec2, const TorchInfo&);
  bool containsTorch(Vec2) const;

  const map<Vec2, SquareInfo>& getSquares() const;
  const map<Vec2, TrapInfo>& getTraps() const;
  const map<Vec2, TorchInfo>& getTorches() const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  map<Vec2, SquareInfo> SERIAL(squares);
  unordered_map<SquareType, int> SERIAL(typeCounts);
  map<Vec2, TrapInfo> SERIAL(traps);
  map<Vec2, TorchInfo> SERIAL(torches);
};

#endif
