#pragma once

#include "furniture_type.h"
#include "tribe.h"

class RandomGen;
class Position;

struct FurnitureParams {
  FurnitureType SERIAL(type); // HASH(type)
  TribeId SERIAL(tribe); // HASH(tribe)
  bool operator == (const FurnitureParams& p) const;
  SERIALIZE_ALL(type, tribe)
  HASH_ALL(type, tribe)
};

class FurnitureFactory {
  public:
  static bool canBuild(FurnitureType, Position);
  static bool hasSupport(FurnitureType, Position);
  static bool isUpgrade(FurnitureType base, FurnitureType upgraded);
  static const vector<FurnitureType>& getUpgrades(FurnitureType base);

  FurnitureFactory(TribeId, const EnumMap<FurnitureType, double>& distribution,
      const vector<FurnitureType>& unique = {});
  FurnitureFactory(TribeId, FurnitureType);
  static PFurniture get(FurnitureType, TribeId);

  FurnitureParams getRandom(RandomGen&);
  int numUnique() const;

  static FurnitureFactory roomFurniture(TribeId);
  static FurnitureFactory castleFurniture(TribeId);
  static FurnitureFactory dungeonOutside(TribeId);
  static FurnitureFactory castleOutside(TribeId);
  static FurnitureFactory villageOutside(TribeId);
  static FurnitureFactory cryptCoffins(TribeId);

  static FurnitureType getWaterType(double depth);

  private:
  HeapAllocated<TribeId> tribe;
  EnumMap<FurnitureType, double> distribution;
  vector<FurnitureType> unique;
};
