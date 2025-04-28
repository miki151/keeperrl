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

SERIALIZE_DEF(FurnitureFactory, furniture, furnitureLists, trainingFurniture, upgrades, bedFurniture, needingLight, increasingPopulation, constructionObjects)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureFactory)

bool FurnitureParams::operator == (const FurnitureParams& p) const {
  return type == p.type && tribe == p.tribe;
}

FurnitureList FurnitureFactory::getFurnitureList(FurnitureListId id) const {
  CHECK(furnitureLists.count(id)) << "Furniture list not found \"" << id.data() << "\"";
  return furnitureLists.at(id);
}

int FurnitureFactory::getPopulationIncrease(FurnitureType type, int numBuilt) const {
  auto info = getData(type).getPopulationIncrease();
  return min(int(numBuilt * info.increase), info.limit.value_or(1000000));
}

PFurniture FurnitureFactory::getFurniture(FurnitureType type, TribeId tribe) const {
  USER_CHECK(furniture.count(type)) << "Furniture type not found " << type.data();
  auto ret = make_unique<Furniture>(*furniture.at(type));
  ret->setTribe(tribe);
  return ret;
}

const Furniture& FurnitureFactory::getData(FurnitureType type) const {
  if (auto ret = getReferenceMaybe(furniture, type))
    return **ret;
  FATAL << "Furniture not found " << type.data();
  fail();
}

const ViewObject& FurnitureFactory::getConstructionObject(FurnitureType type) const {
  return constructionObjects.at(type);
}

void FurnitureFactory::merge(FurnitureFactory f) {
  mergeMap(std::move(f.furniture), furniture);
}

FurnitureFactory::FurnitureFactory(map<FurnitureType, unique_ptr<Furniture> > f, map<FurnitureListId, FurnitureList> l)
    : furniture(std::move(f)), furnitureLists(std::move(l)) {
}

void FurnitureFactory::initializeInfos() {
  for (auto& f : furniture)
    for (auto& elem : f.second->getMaxTraining())
      trainingFurniture[elem.first].push_back(f.first);
  for (auto& base : furniture)
    for (auto& other : furniture)
      if (isUpgrade(base.first, other.first))
        upgrades[base.first].push_back(other.first);
  for (auto& f : furniture) {
    if (f.second->isRequiresLight())
      needingLight.push_back(f.first);
    if (f.second->getPopulationIncrease().increase > 0)
      increasingPopulation.push_back(f.first);
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
  auto groundF = pos.getFurniture(FurnitureLayer::GROUND);
  CHECK(groundF);
  if (data.isBridge())
    return !!groundF->getDefaultBridge();
  if (auto otherPos = data.getSecondPart(pos)) {
    if (auto f = otherPos->getFurniture(FurnitureLayer::MIDDLE))
      // check for castle wall and prison as a hack to disallow digging into goblin matrons
      if (!f->isWall() || f->getType() == FurnitureType("CASTLE_WALL"))
        return false;
    if (auto f = otherPos->getFurniture(FurnitureLayer::FLOOR))
      if (f->getType() == FurnitureType("PRISON"))
        return false;
  }
  if (!data.getBuiltOver().empty()) {
    for (auto f : data.getBuiltOver())
      if (!!pos.getFurniture(f))
        return true;
    return false;
  }
  auto original = pos.getFurniture(getData(type).getLayer());
  return (groundF->getMovementSet().canEnter({MovementTrait::WALK}) || data.getLayer() == FurnitureLayer::GROUND) &&
      (!original || original->silentlyReplace()) && !pos.isWall();
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

FurnitureFactory::FurnitureFactory(FurnitureFactory&&) noexcept = default;
FurnitureFactory& FurnitureFactory::operator =(FurnitureFactory&&) = default;

const vector<FurnitureType>& FurnitureFactory::getTrainingFurniture(AttrType type) const {
  static vector<FurnitureType> empty;
  if (auto elem = getReferenceMaybe(trainingFurniture, type))
    return *elem;
  return empty;
}

const vector<FurnitureType>& FurnitureFactory::getFurnitureNeedingLight() const {
  return needingLight;
}

const vector<FurnitureType>& FurnitureFactory::getFurnitureThatIncreasePopulation() const {
  return increasingPopulation;
}

const vector<FurnitureType>& FurnitureFactory::getBedFurniture(BedType type) const {
  return bedFurniture[type];
}

vector<FurnitureType> FurnitureFactory::getAllFurnitureType() const {
  return getKeys(furniture).transform([](auto key) { return (FurnitureType) key; });
}
