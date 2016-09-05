#include "stdafx.h"
#include "construction_map.h"
#include "creature.h"
#include "trigger.h"
#include "tribe.h"

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

const FurnitureType& ConstructionMap::FurnitureInfo::getFurnitureType() const {
  return type;
}

const ConstructionMap::FurnitureInfo& ConstructionMap::getFurniture(Position pos) const {
  return furniture.at(pos); 
}

ConstructionMap::FurnitureInfo& ConstructionMap::getFurniture(Position pos) {
  return furniture.at(pos); 
}

void ConstructionMap::removeFurniture(Position pos) {
  auto type = furniture.at(pos).getFurnitureType();
  --unbuiltCounts[type];
  furniturePositions[type].erase(pos);
  furniture.erase(pos);
  removeElement(allFurniture, pos);
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::onFurnitureDestroyed(Position pos) {
  getFurniture(pos).reset();
}

void ConstructionMap::addFurniture(Position pos, const FurnitureInfo& info) {
  CHECK(!furniture.count(pos));
  allFurniture.push_back(pos);
  furniture.emplace(pos, info);
  pos.setNeedsRenderUpdate(true);
  if (info.isBuilt())
    furniturePositions[info.getFurnitureType()].insert(pos);
  else
    ++unbuiltCounts[info.getFurnitureType()];
}

bool ConstructionMap::containsFurniture(Position pos) const {
  return furniture.count(pos);
}

int ConstructionMap::getBuiltCount(FurnitureType type) const {
  return furniturePositions[type].size();
}

int ConstructionMap::getTotalCount(FurnitureType type) const {
  return unbuiltCounts[type] + getBuiltCount(type);
}

const set<Position>& ConstructionMap::getBuiltPositions(FurnitureType type) const {
  return furniturePositions[type];
}

const vector<Position>& ConstructionMap::getAllFurniture() const {
  return allFurniture;
}

void ConstructionMap::onConstructed(Position pos, FurnitureType type) {
  furniturePositions[type].insert(pos);
  --unbuiltCounts[type];
  if (furniture.count(pos))
    furniture.at(pos).setBuilt();
}

const ConstructionMap::SquareInfo& ConstructionMap::getSquare(Position pos) const {
  for (auto& info : squares.at(pos))
    if (!info.isBuilt())
      return info;
  CHECK(!squares.at(pos).empty());
  return squares.at(pos).back();
}

ConstructionMap::SquareInfo& ConstructionMap::getSquare(Position pos) {
  for (auto& info : squares.at(pos))
    if (!info.isBuilt())
      return info;
  CHECK(!squares.at(pos).empty());
  return squares.at(pos).back();
}

void ConstructionMap::removeSquare(Position pos) {
  for (auto& info : squares.at(pos))
    --typeCounts[info.getSquareType()];
  squares.erase(pos);
  removeElement(squarePos, pos);
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::addSquare(Position pos, const ConstructionMap::SquareInfo& info) {
  if (!squares.count(pos))
    squarePos.push_back(pos);
  squares[pos].push_back(info);
  ++typeCounts[info.getSquareType()];
  pos.setNeedsRenderUpdate(true);
}

bool ConstructionMap::containsSquare(Position pos) const {
  return squares.count(pos);
}

int ConstructionMap::getSquareCount(SquareType type) const {
  if (typeCounts.count(type))
    return typeCounts.at(type);
  else
    return 0;
}

void ConstructionMap::onSquareDestroyed(Position pos) {
  getSquare(pos).reset();
}

const vector<Position>& ConstructionMap::getSquares() const {
  return squarePos;
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

ConstructionMap::SquareInfo::SquareInfo(SquareType t, CostInfo c) : cost(c), type(t) {
}

void ConstructionMap::SquareInfo::setBuilt() {
  built = true;
  task = none;
}

void ConstructionMap::SquareInfo::reset() {
  built = false;
  task = none;
}

void ConstructionMap::SquareInfo::setTask(UniqueEntity<Task>::Id id) {
  task = id;
}

CostInfo ConstructionMap::SquareInfo::getCost() const {
  return cost;
}

bool ConstructionMap::SquareInfo::isBuilt() const {
  return built;
}

bool ConstructionMap::SquareInfo::hasTask() const {
  return !!task;
}

UniqueEntity<Task>::Id ConstructionMap::SquareInfo::getTask() const {
  return *task;
}

const SquareType& ConstructionMap::SquareInfo::getSquareType() const {
  return type;
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

void ConstructionMap::TorchInfo::setBuilt(Trigger* t) {
  built = true;
  task = none;
  trigger = t;
}

ConstructionMap::TorchInfo::TorchInfo(Dir d) : attachmentDir(d) {
}

Dir ConstructionMap::TorchInfo::getAttachmentDir() const {
  return attachmentDir;
}

UniqueEntity<Task>::Id ConstructionMap::TorchInfo::getTask() const {
  CHECK(hasTask());
  return *task;
}

bool ConstructionMap::TorchInfo::hasTask() const {
  return !!task;
}

bool ConstructionMap::TorchInfo::isBuilt() const {
  return built;
}

Trigger* ConstructionMap::TorchInfo::getTrigger() {
  return trigger;
}

void ConstructionMap::TorchInfo::setTask(UniqueEntity<Task>::Id id) {
  task = id;
}

const ConstructionMap::TorchInfo& ConstructionMap::getTorch(Position pos) const {
  return torches.at(pos);
}

ConstructionMap::TorchInfo& ConstructionMap::getTorch(Position pos) {
  return torches.at(pos);
}

void ConstructionMap::removeTorch(Position pos) {
  torches.erase(pos);
  pos.setNeedsRenderUpdate(true);
}

void ConstructionMap::addTorch(Position pos, const TorchInfo& info) {
  CHECK(!containsTorch(pos));
  torches.insert(make_pair(pos, info));
  pos.setNeedsRenderUpdate(true);
}

bool ConstructionMap::containsTorch(Position pos) const {
  return torches.count(pos);
}

const map<Position, ConstructionMap::TorchInfo>& ConstructionMap::getTorches() const {
  return torches;
}

template <class Archive>
void ConstructionMap::SquareInfo::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, cost, built, type, task);
}

SERIALIZABLE(ConstructionMap::SquareInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::SquareInfo, SquareInfo);

template <class Archive>
void ConstructionMap::TrapInfo::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, type, armed, marked);
}

SERIALIZABLE(ConstructionMap::TrapInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::TrapInfo, TrapInfo);

template <class Archive>
void ConstructionMap::TorchInfo::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, built, task, attachmentDir, trigger);
}

SERIALIZABLE(ConstructionMap::TorchInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::TorchInfo, TorchInfo);

template <class Archive>
void ConstructionMap::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, squares, typeCounts, traps, torches, furniture, furniturePositions, unbuiltCounts);
  if (Archive::is_loading::value) {
    squarePos = getKeys(squares);
    allFurniture = getKeys(furniture);
  }
}

SERIALIZABLE(ConstructionMap);
