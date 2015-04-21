#include "stdafx.h"
#include "construction_map.h"
#include "creature.h"
#include "trigger.h"

const ConstructionMap::SquareInfo& ConstructionMap::getSquare(Vec2 pos) const {
  return squares.at(pos);
}

ConstructionMap::SquareInfo& ConstructionMap::getSquare(Vec2 pos) {
  return squares.at(pos);
}

void ConstructionMap::removeSquare(Vec2 pos) {
  --typeCounts[squares.at(pos).getSquareType()];
  squares.erase(pos);
}

void ConstructionMap::addSquare(Vec2 pos, const ConstructionMap::SquareInfo& info) {
  CHECK(!containsSquare(pos));
  squares.insert(make_pair(pos, info));
  ++typeCounts[info.getSquareType()];
}

bool ConstructionMap::containsSquare(Vec2 pos) const {
  return squares.count(pos);
}

int ConstructionMap::getSquareCount(SquareType type) const {
  if (typeCounts.count(type))
    return typeCounts.at(type);
  else
    return 0;
}

const map<Vec2, ConstructionMap::SquareInfo>& ConstructionMap::getSquares() const {
  return squares;
}

const ConstructionMap::TrapInfo& ConstructionMap::getTrap(Vec2 pos) const {
  return traps.at(pos);
}

ConstructionMap::TrapInfo& ConstructionMap::getTrap(Vec2 pos) {
  return traps.at(pos);
}

void ConstructionMap::removeTrap(Vec2 pos) {
  traps.erase(pos);
}

void ConstructionMap::addTrap(Vec2 pos, const TrapInfo& info) {
  CHECK(!containsTrap(pos));
  traps.insert(make_pair(pos, info));
}

bool ConstructionMap::containsTrap(Vec2 pos) const {
  return traps.count(pos);
}

const map<Vec2, ConstructionMap::TrapInfo>& ConstructionMap::getTraps() const {
  return traps;
}

ConstructionMap::SquareInfo::SquareInfo(SquareType t, CostInfo c) : cost(c), type(t) {
}

void ConstructionMap::SquareInfo::setBuilt() {
  built = true;
  task = -1;
}

void ConstructionMap::SquareInfo::reset() {
  built = false;
  task = -1;
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
  return task > -1;
}

UniqueEntity<Task>::Id ConstructionMap::SquareInfo::getTask() const {
  return task;
}

SquareType ConstructionMap::SquareInfo::getSquareType() const {
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
  task = -1;
  trigger = t;
}

ConstructionMap::TorchInfo::TorchInfo(Dir d) : attachmentDir(d) {
}

Dir ConstructionMap::TorchInfo::getAttachmentDir() const {
  return attachmentDir;
}

UniqueEntity<Task>::Id ConstructionMap::TorchInfo::getTask() const {
  CHECK(hasTask());
  return task;
}

bool ConstructionMap::TorchInfo::hasTask() const {
  return task > -1;
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

const ConstructionMap::TorchInfo& ConstructionMap::getTorch(Vec2 pos) const {
  return torches.at(pos);
}

ConstructionMap::TorchInfo& ConstructionMap::getTorch(Vec2 pos) {
  return torches.at(pos);
}

void ConstructionMap::removeTorch(Vec2 pos) {
  torches.erase(pos);
}

void ConstructionMap::addTorch(Vec2 pos, const TorchInfo& info) {
  CHECK(!containsTorch(pos));
  torches.insert(make_pair(pos, info));
}

bool ConstructionMap::containsTorch(Vec2 pos) const {
  return torches.count(pos);
}

const map<Vec2, ConstructionMap::TorchInfo>& ConstructionMap::getTorches() const {
  return torches;
}

template <class Archive>
void ConstructionMap::SquareInfo::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(cost) & SVAR(built) & SVAR(type) & SVAR(task);
}

SERIALIZABLE(ConstructionMap::SquareInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::SquareInfo, SquareInfo);

template <class Archive>
void ConstructionMap::TrapInfo::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(type) & SVAR(armed) & SVAR(marked);
}

SERIALIZABLE(ConstructionMap::TrapInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::TrapInfo, TrapInfo);

template <class Archive>
void ConstructionMap::TorchInfo::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(built) & SVAR(task) & SVAR(attachmentDir) & SVAR(trigger);
}

SERIALIZABLE(ConstructionMap::TorchInfo);
SERIALIZATION_CONSTRUCTOR_IMPL2(ConstructionMap::TorchInfo, TorchInfo);

template <class Archive>
void ConstructionMap::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(squares) & SVAR(typeCounts) & SVAR(traps) & SVAR(torches);
}

SERIALIZABLE(ConstructionMap);
