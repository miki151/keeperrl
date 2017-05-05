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

optional<const ConstructionMap::FurnitureInfo&> ConstructionMap::getFurniture(Position pos, FurnitureLayer layer) const {
  return getReferenceMaybe(furniture[layer], pos);
}

void ConstructionMap::setTask(Position pos, FurnitureLayer layer, UniqueEntity<Task>::Id id) {
  modFurniture(pos, layer)->setTask(id);
}

optional<ConstructionMap::FurnitureInfo&> ConstructionMap::modFurniture(Position pos, FurnitureLayer layer) {
  return getReferenceMaybe(furniture[layer], pos);
}

void ConstructionMap::removeFurniture(Position pos, FurnitureLayer layer) {
  auto& info = furniture[layer].at(pos);
  auto type = info.getFurnitureType();
  if (!info.isBuilt()) {
    --unbuiltCounts[type];
    addDebt(-info.getCost());
  }
  furniturePositions[type].erase(pos);
  furniture[layer].erase(pos);
  allFurniture.removeElement({pos, layer});
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::addDebt(const CostInfo& cost) {
  debt[cost.id] += cost.value;
}

void ConstructionMap::onFurnitureDestroyed(Position pos, FurnitureLayer layer) {
  if (auto info = modFurniture(pos, layer)) {
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
  CHECK(!furniture[layer].count(pos));
  allFurniture.push_back({pos, layer});
  furniture[layer].emplace(pos, info);
  pos.setNeedsRenderUpdate(true);
  if (info.isBuilt())
    furniturePositions[info.getFurnitureType()].insert(pos);
  else {
    ++unbuiltCounts[info.getFurnitureType()];
    addDebt(info.getCost());
  }
}

bool ConstructionMap::containsFurniture(Position pos, FurnitureLayer layer) const {
  return furniture[layer].count(pos);
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
  if (furniture[layer].count(pos)) { // why this if?
    auto& info = furniture[layer].at(pos);
    info.setBuilt();
    addDebt(-info.getCost());
  }
}

const ConstructionMap::TrapInfo& ConstructionMap::getTrap(Position pos) const {
  return traps.at(pos);
}

ConstructionMap::TrapInfo& ConstructionMap::getTrap(Position pos) {
  return traps.at(pos);
}

void ConstructionMap::removeTrap(Position pos) {
  traps.erase(pos);
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::addTrap(Position pos, const TrapInfo& info) {
  CHECK(!containsTrap(pos));
  traps.insert(make_pair(pos, info));
  pos.setNeedsRenderUpdate(true);
}

bool ConstructionMap::containsTrap(Position pos) const {
  return traps.count(pos);
}

const map<Position, ConstructionMap::TrapInfo>& ConstructionMap::getTraps() const {
  return traps;
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
  ar(debt, traps, furniture, furniturePositions, unbuiltCounts, allFurniture);
}

SERIALIZABLE(ConstructionMap);
