#pragma once

#include "furniture_type.h"

class TribeId;
class RandomGen;
class Collective;
class Position;

class FurnitureFactory {
  public:
  static Furniture get(FurnitureType, TribeId);
  static bool canBuild(FurnitureType, Position, const Collective*);

  FurnitureFactory(TribeId, const EnumMap<FurnitureType, double>& distribution,
      const vector<FurnitureType>& unique = {});
  FurnitureFactory(TribeId, FurnitureType);

  Furniture getRandom(RandomGen&);

  static FurnitureFactory roomFurniture(TribeId);
  static FurnitureFactory castleFurniture(TribeId);
  static FurnitureFactory dungeonOutside(TribeId);
  static FurnitureFactory castleOutside(TribeId);
  static FurnitureFactory villageOutside(TribeId);
  static FurnitureFactory cryptCoffins(TribeId);

  private:
  HeapAllocated<TribeId> tribe;
  EnumMap<FurnitureType, double> distribution;
  vector<FurnitureType> unique;
};
