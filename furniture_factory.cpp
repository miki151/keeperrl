#include "stdafx.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "furniture_type.h"
#include "view_id.h"
#include "view_layer.h"
#include "view_object.h"
#include "tribe.h"
#include "item_type.h"
#include "level.h"
#include "creature.h"
#include "creature_factory.h"
#include "movement_set.h"
#include "lasting_effect.h"
#include "furniture_usage.h"
#include "furniture_click.h"
#include "furniture_tick.h"
#include "furniture_entry.h"
#include "furniture_dropped_items.h"
#include "furniture_on_built.h"
#include "game_config.h"

SERIALIZE_DEF(FurnitureFactory, furniture, trainingFurniture, upgrades, bedFurniture, needingLight, constructionObjects)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureFactory)

bool FurnitureParams::operator == (const FurnitureParams& p) const {
  return type == p.type && tribe == p.tribe;
}

optional<string> FurnitureFactory::getPopulationIncreaseDescription(FurnitureType type) const {
  auto info = getData(type).getPopulationIncrease();
  if (info.increase > 0) {
    auto ret = "Increases population limit by " + toString(info.increase);
    if (auto limit = info.limit)
      ret += ", up to " + toString(*limit);
    ret += ".";
    return ret;
  }
  return none;
}

FurnitureList FurnitureFactory::getFurnitureList(FurnitureListId id) const {
  if (id == "roomFurniture")
    return FurnitureList({
        {FurnitureType("BED1"), 2},
        {FurnitureType("GROUND_TORCH"), 1},
        {FurnitureType("CHEST"), 2}
    });
  if (id == "castleFurniture")
    return FurnitureList({
        {FurnitureType("BED1"), 2},
        {FurnitureType("GROUND_TORCH"), 1},
        {FurnitureType("FOUNTAIN"), 1},
        {FurnitureType("CHEST"), 2}
    });
  if (id == "dungeonOutside")
    return FurnitureList({
        {FurnitureType("GROUND_TORCH"), 1},
    });
  if (id == "castleOutside")
    return FurnitureList({
        {FurnitureType("INVISIBLE_ALARM"), 10},
        {FurnitureType("GROUND_TORCH"), 1},
        {FurnitureType("WELL"), 1},
    });
  if (id == "villageOutside")
    return FurnitureList({
        {FurnitureType("GROUND_TORCH"), 1},
        {FurnitureType("WELL"), 1},
    });
  if (id == "vegetationLow")
    return FurnitureList({
        {FurnitureType("CANIF_TREE"), 2},
        {FurnitureType("BUSH"), 1 }
    });
  if (id == "vegetationHigh")
    return FurnitureList({
        {FurnitureType("DECID_TREE"), 2},
        {FurnitureType("BUSH"), 1 }
    });
  if (id == "cryptCoffins")
    return FurnitureList({
        {FurnitureType("LOOT_COFFIN"), 1},
    }, {
        FurnitureType("VAMPIRE_COFFIN")
    });  
  if (id == "towerInside")
    return FurnitureList({
        {FurnitureType("GROUND_TORCH"), 1},
    });
  if (id == "graves")
    return FurnitureList({
        {FurnitureType("GRAVE"), 1},
    });
  if (id == "templeInside")
    return FurnitureList({
        {FurnitureType("CHEST"), 1}
    }, {FurnitureType("ALTAR")});
  USER_FATAL << "FurnitueListId not found: " << id;
  fail();
}

int FurnitureFactory::getPopulationIncrease(FurnitureType type, int numBuilt) const {
  auto info = getData(type).getPopulationIncrease();
  return min(int(numBuilt * info.increase), info.limit.value_or(1000000));
}

PFurniture FurnitureFactory::getFurniture(FurnitureType type, TribeId tribe) const {
  USER_CHECK(furniture.count(type)) << "Furniture type not found " << type.data();
  auto ret = makeOwner<Furniture>(*furniture.at(type));
  ret->setTribe(tribe);
  return ret;
}

const Furniture& FurnitureFactory::getData(FurnitureType type) const {
  return *furniture.at(type);
}

const ViewObject& FurnitureFactory::getConstructionObject(FurnitureType type) const {
  return constructionObjects.at(type);
}

FurnitureFactory::FurnitureFactory(const GameConfig* config) {
  while (1) {
    FurnitureType::startContentIdGeneration();
    vector<pair<FurnitureType, Furniture>> elems;
    if (auto res = config->readObject(elems, GameConfigId::FURNITURE)) {
      USER_INFO << *res;
      continue;
    }
    for (auto& elem : elems) {
      elem.second.setType(elem.first);
      furniture.insert(make_pair(elem.first, makeOwner<Furniture>(elem.second)));
    }
    FurnitureType::validateContentIds();
    break;
  }
  initializeInfos();
}

void FurnitureFactory::initializeInfos() {
  for (auto expType : ENUM_ALL(ExperienceType))
    for (auto& elem : furniture)
      if (auto limit = elem.second->getMaxTraining(expType))
        trainingFurniture[expType].push_back(elem.first);
  for (auto& base : furniture)
    for (auto& other : furniture)
      if (isUpgrade(base.first, other.first))
        upgrades[base.first].push_back(other.first);
  for (auto& f : furniture) {
    if (f.second->isRequiresLight())
      needingLight.push_back(f.first);
    if (auto type = f.second->getBedType())
      bedFurniture[*type].push_back(f.first);
    if (auto obj1 = f.second->getViewObject()) {
      auto obj = *obj1;
      obj.setModifier(ViewObject::Modifier::PLANNED);
      constructionObjects[f.first] = obj;
    }
  }
}

bool FurnitureFactory::canBuild(FurnitureType type, Position pos) const {
  auto& data = getData(type);
  if (data.isBridge())
    return pos.getFurniture(FurnitureLayer::GROUND)->canBuildBridgeOver();
  if (auto over = data.getBuiltOver())
    return !!pos.getFurniture(*over);
  auto original = pos.getFurniture(getData(type).getLayer());
  return pos.getFurniture(FurnitureLayer::GROUND)->getMovementSet().canEnter({MovementTrait::WALK}) &&
      (!original || data.silentlyReplace()) && !pos.isWall();
}

bool FurnitureFactory::isUpgrade(FurnitureType base, FurnitureType upgraded) const {
  if (base == upgraded)
    return false;
  while (upgraded != base)
    if (auto u = getData(upgraded).getUpgrade())
      upgraded = *u;
    else
      return false;
  return true;
}

const static vector<FurnitureType> emptyUpgrades;

const vector<FurnitureType>& FurnitureFactory::getUpgrades(FurnitureType base) const {
  if (auto res = getReferenceMaybe(upgrades, base))
    return *res;
  else
    return emptyUpgrades;
}

FurnitureType FurnitureFactory::getWaterType(double depth) const {
  if (depth >= 2.0)
    return FurnitureType("WATER");
  else if (depth >= 1.0)
    return FurnitureType("SHALLOW_WATER1");
  else
    return FurnitureType("SHALLOW_WATER2");
}

FurnitureFactory::~FurnitureFactory() {
}

FurnitureFactory::FurnitureFactory(FurnitureFactory&&) = default;

const vector<FurnitureType>& FurnitureFactory::getTrainingFurniture(ExperienceType type) const {
  return trainingFurniture[type];
}

const vector<FurnitureType>& FurnitureFactory::getFurnitureNeedingLight() const {
  return needingLight;
}

const vector<FurnitureType>& FurnitureFactory::getBedFurniture(BedType type) const {
  return bedFurniture[type];
}

vector<FurnitureType> FurnitureFactory::getAllFurnitureType() const {
  return getKeys(furniture);
}
