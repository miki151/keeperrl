#include "stdafx.h"
#include "construction_map.h"
#include "creature.h"
#include "tribe.h"
#include "furniture.h"
#include "furniture_factory.h"
#include "task.h"
#include "game.h"
#include "content_factory.h"

SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::FurnitureInfo, FurnitureInfo);

SERIALIZE_DEF(ConstructionMap::FurnitureInfo, cost, type, task);

ConstructionMap::FurnitureInfo::FurnitureInfo(FurnitureType t, CostInfo c) : task(-1), cost(c), type(t) {
}

ConstructionMap::FurnitureInfo ConstructionMap::FurnitureInfo::getBuilt(FurnitureType type) {
  return FurnitureInfo(type, CostInfo::noCost());
}

void ConstructionMap::FurnitureInfo::reset() {
  task = -1;
}

void ConstructionMap::FurnitureInfo::setTask(UniqueEntity<Task>::Id id) {
  task = id.getGenericId();
}

CostInfo ConstructionMap::FurnitureInfo::getCost() const {
  return cost;
}

bool ConstructionMap::FurnitureInfo::isBuilt(Position pos, FurnitureLayer layer) const {
  auto f = pos.getFurniture(layer);
  return f &&
      (f->getType() == type ||
      (!f->silentlyReplace() &&
        !pos.getGame()->getContentFactory()->furniture.getData(type).getBuiltOver().contains(f->getType())));
}

bool ConstructionMap::FurnitureInfo::hasTask() const {
  return task != -1;
}

UniqueEntity<Task>::Id ConstructionMap::FurnitureInfo::getTask() const {
  return UniqueEntity<Task>::Id(task);
}

FurnitureType ConstructionMap::FurnitureInfo::getFurnitureType() const {
  return type;
}

const Furniture* ConstructionMap::FurnitureInfo::getBuilt(Position pos) const {
  return pos.getFurniture(type);
}

optional<const ConstructionMap::FurnitureInfo&> ConstructionMap::getFurniture(Position pos, FurnitureLayer layer) const {
  return furniture[layer].getReferenceMaybe(pos);
}

void ConstructionMap::setTask(Position pos, FurnitureLayer layer, UniqueEntity<Task>::Id id) {
  furniture[layer].getOrFail(pos).setTask(id);
}

void ConstructionMap::removeFurniturePlan(Position pos, FurnitureLayer layer) {
  auto info = furniture[layer].getOrFail(pos);
  auto type = info.getFurnitureType();
  --unbuiltCounts[type];
  furniture[layer].erase(pos);
  allFurniture.removeElement({pos, layer});
  pos.setNeedsRenderAndMemoryUpdate(true);
  addDebt(-info.getCost(), type.data());
}

void ConstructionMap::addDebt(const CostInfo& cost, const char* reason) {
  debt[cost.id] += cost.value;
  //checkDebtConsistency();
  CHECK(debt[cost.id] >= 0) << cost.id.data() << " " << reason;
}

void ConstructionMap::onFurnitureDestroyed(Position pos, FurnitureLayer layer, FurnitureType type) {
  PROFILE;
  if (auto info = furniture[layer].getReferenceMaybe(pos))
    if (info->getFurnitureType() == type ||
        (!strcmp(type.data(), "PHYLACTERY_ACTIVE") && !strcmp(info->getFurnitureType().data(), "PHYLACTERY"))) {
      addDebt(info->getCost(), type.data());
      auto type = info->getFurnitureType();
      furniturePositions[type].erase(pos);
      auto& storageIds = pos.getGame()->getContentFactory()->furniture.getData(type).getStorageId();
      for (auto id : storageIds)
        storagePositions[id].remove(pos);
      if (!storageIds.empty())
        allStoragePositions.remove(pos);
      info->reset();
      ++unbuiltCounts[type];
    }
}

void ConstructionMap::addFurniture(Position pos, const FurnitureInfo& info, FurnitureLayer layer) {
  CHECK(!furniture[layer].contains(pos));
  allFurniture.push_back({pos, layer});
  furniture[layer].set(pos, info);
  pos.setNeedsRenderAndMemoryUpdate(true);
  auto type = info.getFurnitureType();
  if (info.isBuilt(pos, layer)) {
    furniturePositions[type].insert(pos);
    auto& storageIds = pos.getFurniture(layer)->getStorageId();
    for (auto id : storageIds)
      storagePositions[id].add(pos);
    if (!storageIds.empty())
      allStoragePositions.add(pos);
  } else {
    ++unbuiltCounts[type];
    addDebt(info.getCost(), type.data());
  }
}

bool ConstructionMap::containsFurniture(Position pos, FurnitureLayer layer) const {
  return furniture[layer].contains(pos);
}

int ConstructionMap::getBuiltCount(FurnitureType type) const {
  return (int) getReferenceMaybe(furniturePositions, type).map([](const auto& v) { return v.size(); }).value_or(0);
}

int ConstructionMap::getTotalCount(FurnitureType type) const {
  return getValueMaybe(unbuiltCounts, type).value_or(0) + getBuiltCount(type);
}

const PositionSet& ConstructionMap::getBuiltPositions(FurnitureType type) const {
  static PositionSet empty;
  if (auto ret = getReferenceMaybe(furniturePositions, type))
    return *ret;
  else
    return empty;
}

const vector<pair<Position, FurnitureLayer>>& ConstructionMap::getAllFurniture() const {
  return allFurniture;
}

void ConstructionMap::onConstructed(Position pos, FurnitureType type) {
  auto layer = pos.getGame()->getContentFactory()->furniture.getData(type).getLayer();
  if (!containsFurniture(pos, layer))
    addFurniture(pos, FurnitureInfo::getBuilt(type), layer);
  furniturePositions[type].insert(pos);
  auto& storageIds = pos.getGame()->getContentFactory()->furniture.getData(type).getStorageId();
  for (auto id : storageIds)
    storagePositions[id].add(pos);
  if (!storageIds.empty())
    allStoragePositions.add(pos);
  --unbuiltCounts[type];
  if (furniture[layer].contains(pos)) { // why this if?
    auto& info = furniture[layer].getOrInit(pos);
    addDebt(-info.getCost(), type.data());
  }
}

void ConstructionMap::clearUnsupportedFurniturePlans() {
  vector<pair<Position, FurnitureLayer>> toRemove;
  for (auto& elem : getAllFurniture())
    if (auto info = getFurniture(elem.first, elem.second))
      if (!elem.first.getGame()->getContentFactory()->furniture.getData(info->getFurnitureType()).hasRequiredSupport(elem.first))
        toRemove.push_back(elem);
  for (auto& elem : toRemove)
    removeFurniturePlan(elem.first, elem.second);
}

int ConstructionMap::getDebt(CollectiveResourceId id) const {
  return getValueMaybe(debt, id).value_or(0);
}

const StoragePositions& ConstructionMap::getStoragePositions(StorageId id) const {
  static StoragePositions empty;
  if (auto res = getReferenceMaybe(storagePositions, id))
    return *res;
  return empty;
}

StoragePositions& ConstructionMap::getStoragePositions(StorageId id) {
  return storagePositions[id];
}

StoragePositions& ConstructionMap::getAllStoragePositions() {
  return allStoragePositions;
}

const StoragePositions& ConstructionMap::getAllStoragePositions() const {
  return allStoragePositions;
}

void ConstructionMap::checkDebtConsistency() {
  HashMap<CollectiveResourceId, int> nowDebt;
  for (auto& f : allFurniture) {
    auto& info = furniture[f.second].getOrFail(f.first);
    if (!info.isBuilt(f.first, f.second))
      nowDebt[info.getCost().id] += info.getCost().value;
  }
  for (auto& elem : nowDebt)
    CHECK(getValueMaybe(debt, elem.first).value_or(0) == elem.second) << getValueMaybe(debt, elem.first).value_or(0) << " " << elem.second;
  for (auto& elem : debt)
    CHECK(getValueMaybe(nowDebt, elem.first).value_or(0) == elem.second);
}

template <class Archive>
void ConstructionMap::serialize(Archive& ar, const unsigned int version) {
  ar(debt, furniture, furniturePositions, storagePositions, allStoragePositions, unbuiltCounts, allFurniture);
}

SERIALIZABLE(ConstructionMap);
