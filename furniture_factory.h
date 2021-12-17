#pragma once

#include "furniture_type.h"
#include "furniture_list.h"
#include "tribe.h"
#include "experience_type.h"
#include "bed_type.h"
#include "view_object.h"

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
  FurnitureFactory(map<FurnitureType, unique_ptr<Furniture>>, map<FurnitureListId, FurnitureList>);
  void initializeInfos();
  void merge(FurnitureFactory);
  bool canBuild(FurnitureType, Position) const;
  bool isUpgrade(FurnitureType base, FurnitureType upgraded) const;
  const vector<FurnitureType>& getUpgrades(FurnitureType base) const;
  PFurniture getFurniture(FurnitureType, TribeId) const;
  const Furniture& getData(FurnitureType) const;
  const ViewObject& getConstructionObject(FurnitureType) const;
  int getPopulationIncrease(FurnitureType, int numBuilt) const;
  FurnitureList getFurnitureList(FurnitureListId) const;
  FurnitureType getWaterType(double depth) const;
  const vector<FurnitureType>& getTrainingFurniture(ExperienceType) const;
  const vector<FurnitureType>& getFurnitureNeedingLight() const;
  const vector<FurnitureType>& getBedFurniture(BedType) const;
  vector<FurnitureType> getAllFurnitureType() const;

  ~FurnitureFactory();
  FurnitureFactory(const FurnitureFactory&) = delete;
  FurnitureFactory(FurnitureFactory&&) noexcept;
  FurnitureFactory& operator = (FurnitureFactory&&);

  SERIALIZATION_DECL(FurnitureFactory)

  private:
  map<FurnitureType, unique_ptr<Furniture>> SERIAL(furniture);
  map<FurnitureListId, FurnitureList> SERIAL(furnitureLists);
  EnumMap<ExperienceType, vector<FurnitureType>> SERIAL(trainingFurniture);
  unordered_map<FurnitureType, vector<FurnitureType>, CustomHash<FurnitureType>> SERIAL(upgrades);
  vector<FurnitureType> SERIAL(needingLight);
  EnumMap<BedType, vector<FurnitureType>> SERIAL(bedFurniture);
  unordered_map<FurnitureType, ViewObject, CustomHash<FurnitureType>> SERIAL(constructionObjects);
};

static_assert(std::is_nothrow_move_constructible<FurnitureFactory>::value, "T should be noexcept MoveConstructible");
