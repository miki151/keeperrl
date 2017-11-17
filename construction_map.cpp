#include "stdafx.h"
#include "construction_map.h"
#include "creature.h"
#include "tribe.h"
#include "furniture.h"
#include "furniture_factory.h"

SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::FurnitureInfo, FurnitureInfo);

SERIALIZE_DEF(ConstructionMap::FurnitureInfo, cost, built, type, task);

ConstructionMap::FurnitureInfo::FurnitureInfo(FurnitureType t, CostInfo c) : cost(c), type(t) {
}

ConstructionMap::FurnitureInfo ConstructionMap::FurnitureInfo::getBuilt(FurnitureType type) {
  FurnitureInfo ret(type, CostInfo::noCost());
  ret.setBuilt();
  return ret;
}

void ConstructionMap::FurnitureInfo::setBuilt() {
  built = true;
  task = none;
}

void ConstructionMap::FurnitureInfo::reset() {
  built = false;
  task = none;
}

void ConstructionMap::FurnitureInfo::setTask(UniqueEntity<Task>::Id id) {
  task = id;
}

CostInfo ConstructionMap::FurnitureInfo::getCost() const {
  return cost;
}

bool ConstructionMap::FurnitureInfo::isBuilt() const {
  return built;
}

bool ConstructionMap::FurnitureInfo::hasTask() const {
  return !!task;
}

UniqueEntity<Task>::Id ConstructionMap::FurnitureInfo::getTask() const {
  return *task;
}

FurnitureType ConstructionMap::FurnitureInfo::getFurnitureType() const {
  return type;
}

FurnitureLayer ConstructionMap::FurnitureInfo::getLayer() const {
  return Furniture::getLayer(type);
}

const optional<ConstructionMap::FurnitureInfo>& ConstructionMap::getFurniture(Position pos, FurnitureLayer layer) const {
  return furniture[layer].get(pos);
}

void ConstructionMap::setTask(Position pos, FurnitureLayer layer, UniqueEntity<Task>::Id id) {
  furniture[layer].getOrFail(pos)->setTask(id);
}

void ConstructionMap::removeFurniture(Position pos, FurnitureLayer layer) {
  auto& info = furniture[layer].getOrFail(pos);
  auto type = info->getFurnitureType();
  if (!info->isBuilt()) {
    --unbuiltCounts[type];
    addDebt(-info->getCost());
  }
  furniturePositions[type].erase(pos);
  info = none;
  allFurniture.removeElement({pos, layer});
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::addDebt(const CostInfo& cost) {
  debt[cost.id] += cost.value;
}

void ConstructionMap::onFurnitureDestroyed(Position pos, FurnitureLayer layer) {
  if (auto& info = furniture[layer].getOrInit(pos)) {
    if (info->isBuilt())
      addDebt(info->getCost());
    info->reset();
  }
  for (auto pos2 : pos.neighbors8())
    for (auto layer : ENUM_ALL(FurnitureLayer))
      if (auto info = getFurniture(pos2, layer))
        if (!FurnitureFactory::hasSupport(info->getFurnitureType(), pos2))
          removeFurniture(pos2, layer);
}

void ConstructionMap::addFurniture(Position pos, const FurnitureInfo& info) {
  auto layer = info.getLayer();
  CHECK(!furniture[layer].get(pos));
  allFurniture.push_back({pos, layer});
  furniture[layer].set(pos, info);
  pos.setNeedsRenderUpdate(true);
  if (info.isBuilt())
    furniturePositions[info.getFurnitureType()].insert(pos);
  else {
    ++unbuiltCounts[info.getFurnitureType()];
    addDebt(info.getCost());
  }
}

bool ConstructionMap::containsFurniture(Position pos, FurnitureLayer layer) const {
  return !!furniture[layer].get(pos);
}

int ConstructionMap::getBuiltCount(FurnitureType type) const {
  return (int) furniturePositions[type].size();
}

int ConstructionMap::getTotalCount(FurnitureType type) const {
  return unbuiltCounts[type] + getBuiltCount(type);
}

const set<Position>& ConstructionMap::getBuiltPositions(FurnitureType type) const {
  return furniturePositions[type];
}

const vector<pair<Position, FurnitureLayer>>& ConstructionMap::getAllFurniture() const {
  return allFurniture;
}

void ConstructionMap::onConstructed(Position pos, FurnitureType type) {
  auto layer = Furniture::getLayer(type);
  if (!containsFurniture(pos, layer))
    addFurniture(pos, FurnitureInfo::getBuilt(type));
  furniturePositions[type].insert(pos);
  --unbuiltCounts[type];
  if (furniture[layer].get(pos)) { // why this if?
    auto& info = *furniture[layer].getOrInit(pos);
    info.setBuilt();
    addDebt(-info.getCost());
  }
}

const optional<ConstructionMap::TrapInfo>& ConstructionMap::getTrap(Position pos) const {
  return traps.get(pos);
}

optional<ConstructionMap::TrapInfo>& ConstructionMap::getTrap(Position pos) {
  return traps.getOrInit(pos);
}

void ConstructionMap::removeTrap(Position pos) {
  traps.set(pos, none);
  allTraps.removeElement(pos);
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::addTrap(Position pos, const TrapInfo& info) {
  CHECK(!traps.get(pos));
  traps.set(pos, info);
  allTraps.push_back(pos);
  pos.setNeedsRenderUpdate(true);
}

const vector<Position>& ConstructionMap::getAllTraps() const {
  return allTraps;
}

void ConstructionMap::TrapInfo::setArmed() {
  armed = true;
  marked = false;
}

void ConstructionMap::TrapInfo::reset() {
  armed = false;
  marked = false;
}

ConstructionMap::TrapInfo::TrapInfo(TrapType t) : type(t) {
}

bool ConstructionMap::TrapInfo::isMarked() const {
  return marked;
}

bool ConstructionMap::TrapInfo::isArmed() const {
  return armed;
}

TrapType ConstructionMap::TrapInfo::getType() const {
  return type;
}

void ConstructionMap::TrapInfo::setMarked() {
  marked = true;
}

int ConstructionMap::getDebt(CollectiveResourceId id) const {
  return debt[id];
}

template <class Archive>
void ConstructionMap::TrapInfo::serialize(Archive& ar, const unsigned int version) {
  ar(type, armed, marked);
}

SERIALIZABLE(ConstructionMap::TrapInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::TrapInfo, TrapInfo);

template <class Archive>
void ConstructionMap::serialize(Archive& ar, const unsigned int version) {
  ar(debt, traps, furniture, furniturePositions, unbuiltCounts, allFurniture, allTraps);
}

SERIALIZABLE(ConstructionMap);
