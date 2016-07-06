#include "stdafx.h"
#include "construction_map.h"
#include "creature.h"
#include "trigger.h"
#include "tribe.h"

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
  serializeAll(ar, squares, typeCounts, traps, torches);
  if (Archive::is_loading::value)
    squarePos = getKeys(squares);
}

SERIALIZABLE(ConstructionMap);
