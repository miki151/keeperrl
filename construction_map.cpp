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

bool ConstructionMap::FurnitureInfo::isBuilt(Position pos) const {
  return !!pos.getFurniture(type);
}

bool ConstructionMap::FurnitureInfo::isBuilt(Position pos, FurnitureLayer layer) const {
  if (auto f = pos.getFurniture(layer))
    return f->getType() == type;
  return false;
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
  auto& info = furniture[layer].getOrFail(pos);
  auto type = info.getFurnitureType();
  --unbuiltCounts[type];
  addDebt(-info.getCost());
  furniture[layer].erase(pos);
  allFurniture.removeElement({pos, layer});
  pos.setNeedsRenderAndMemoryUpdate(true);
}

void ConstructionMap::addDebt(const CostInfo& cost) {
  debt[cost.id] += cost.value;
  //checkDebtConsistency();
  CHECK(debt[cost.id] >= 0);
}

void ConstructionMap::onFurnitureDestroyed(Position pos, FurnitureLayer layer, FurnitureType type) {
  PROFILE;
  // We don't care if the destroyed furniture was the same type that we track,
  // for furnitures like phylactery which are replaced upon usage.
  if (auto info = furniture[layer].getReferenceMaybe(pos)) {
    addDebt(info->getCost());
    furniturePositions[info->getFurnitureType()].erase(pos);
    info->reset();
  }
}

void ConstructionMap::addFurniture(Position pos, const FurnitureInfo& info, FurnitureLayer layer) {
  CHECK(!furniture[layer].contains(pos));
  allFurniture.push_back({pos, layer});
  furniture[layer].set(pos, info);
  pos.setNeedsRenderAndMemoryUpdate(true);
  if (info.isBuilt(pos, layer))
    furniturePositions[info.getFurnitureType()].insert(pos);
  else {
    ++unbuiltCounts[info.getFurnitureType()];
    addDebt(info.getCost());
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
  --unbuiltCounts[type];
  if (furniture[layer].contains(pos)) { // why this if?
    auto& info = furniture[layer].getOrInit(pos);
    addDebt(-info.getCost());
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

optional<const ConstructionMap::TrapInfo&> ConstructionMap::getTrap(Position pos) const {
  return traps.getReferenceMaybe(pos);
}

optional<ConstructionMap::TrapInfo&> ConstructionMap::getTrap(Position pos) {
  return traps.getReferenceMaybe(pos);
}

void ConstructionMap::removeTrap(Position pos) {
  traps.erase(pos);
  allTraps.removeElement(pos);
  pos.setNeedsRenderAndMemoryUpdate(true);
}

void ConstructionMap::addTrap(Position pos, const TrapInfo& info) {
  CHECK(!traps.contains(pos));
  traps.set(pos, info);
  allTraps.push_back(pos);
  pos.setNeedsRenderAndMemoryUpdate(true);
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

ConstructionMap::TrapInfo::TrapInfo(FurnitureType t) : type(t) {
}

bool ConstructionMap::TrapInfo::isMarked() const {
  return marked;
}

bool ConstructionMap::TrapInfo::isArmed() const {
  return armed;
}

FurnitureType ConstructionMap::TrapInfo::getType() const {
  return type;
}

void ConstructionMap::TrapInfo::setMarked() {
  marked = true;
}

int ConstructionMap::getDebt(CollectiveResourceId id) const {
  return getValueMaybe(debt, id).value_or(0);
}

void ConstructionMap::checkDebtConsistency() {
  unordered_map<CollectiveResourceId, int, CustomHash<CollectiveResourceId>> nowDebt;
  for (auto& f : allFurniture) {
    auto& info = furniture[f.second].getOrFail(f.first);
    if (!info.isBuilt(f.first))
      nowDebt[info.getCost().id] += info.getCost().value;
  }
  for (auto& elem : nowDebt)
    CHECK(getValueMaybe(debt, elem.first).value_or(0) == elem.second);
  for (auto& elem : debt)
    CHECK(getValueMaybe(nowDebt, elem.first).value_or(0) == elem.second);
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
