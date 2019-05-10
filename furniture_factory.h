#pragma once

#include "furniture_type.h"
#include "furniture_list.h"
#include "tribe.h"

class Position;
class TribeId;
class LuxuryInfo;
class GameConfig;

struct FurnitureParams {
  FurnitureType SERIAL(type); // HASH(type)
  TribeId SERIAL(tribe); // HASH(tribe)
  bool operator == (const FurnitureParams& p) const;
  SERIALIZE_ALL(type, tribe)
  HASH_ALL(type, tribe)
};


class FurnitureFactory {
  public:
  FurnitureFactory(const GameConfig*);
  bool canBuild(FurnitureType, Position) const;
  bool hasSupport(FurnitureType, Position) const;
  bool isUpgrade(FurnitureType base, FurnitureType upgraded) const;
  const vector<FurnitureType>& getUpgrades(FurnitureType base) const;
  PFurniture getFurniture(FurnitureType, TribeId) const;
  const ViewObject& getConstructionObject(FurnitureType) const;
  ViewId getViewId(FurnitureType) const;
  const string& getName(FurnitureType, int count = 1) const;
  FurnitureLayer getLayer(FurnitureType) const;
  bool isWall(FurnitureType) const;
  LuxuryInfo getLuxuryInfo(FurnitureType) const;
  int getPopulationIncrease(FurnitureType, int numBuilt) const;
  optional<string> getPopulationIncreaseDescription(FurnitureType) const;
  FurnitureList getFurnitureList(FurnitureListId) const;
  FurnitureType getWaterType(double depth) const;

  ~FurnitureFactory();
  FurnitureFactory(const FurnitureFactory&) = delete;
  FurnitureFactory(FurnitureFactory&&);

  SERIALIZATION_DECL(FurnitureFactory)

  private:
  map<FurnitureType, OwnerPointer<Furniture>> SERIAL(furniture);
};
