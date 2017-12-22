#include "stdafx.h"
#include "minion_task.h"
#include "creature.h"
#include "collective.h"
#include "collective_config.h"
#include "task.h"
#include "workshops.h"
#include "construction_map.h"
#include "known_tiles.h"
#include "territory.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "task_map.h"
#include "quarters.h"
#include "zones.h"

static bool betterPos(Position from, Position current, Position candidate) {
  double maxDiff = 0.3;
  double curDist = from.dist8(current);
  double newDist = from.dist8(candidate);
  return Random.getDouble() <= 1.0 - (newDist - curDist) / (curDist * maxDiff);
}

static optional<Position> getRandomCloseTile(Position from, const vector<Position>& tiles,
    function<bool(Position)> predicate) {
  optional<Position> ret;
  for (Position pos : tiles)
    if (predicate(pos) && (!ret || betterPos(from, *ret, pos)))
      ret = pos;
  return ret;
}

static optional<Position> getTileToExplore(WConstCollective collective, WConstCreature c, MinionTask task) {
  vector<Position> border = Random.permutation(collective->getKnownTiles().getBorderTiles());
  switch (task) {
    case MinionTask::EXPLORE_CAVES:
      if (auto pos = getRandomCloseTile(c->getPosition(), border,
            [&](Position p) {
                return p.isSameLevel(collective->getLevel()) && p.isCovered() &&
                    (!c->getPosition().isSameLevel(collective->getLevel()) || c->isSameSector(p));}))
        return pos;
      FALLTHROUGH;
    case MinionTask::EXPLORE:
      FALLTHROUGH;
    case MinionTask::EXPLORE_NOCTURNAL:
      return getRandomCloseTile(c->getPosition(), border,
          [&](Position pos) { return pos.isSameLevel(collective->getLevel()) && !pos.isCovered()
              && (!c->getPosition().isSameLevel(collective->getLevel()) || c->isSameSector(pos));});
    default: FATAL << "Unrecognized explore task: " << int(task);
  }
  return none;
}

static WCreature getCopulationTarget(WConstCollective collective, WCreature succubus) {
  for (WCreature c : Random.permutation(collective->getCreatures(MinionTrait::FIGHTER)))
    if (succubus->canCopulateWith(c))
      return c;
  return nullptr;
}

const vector<FurnitureType>& MinionTasks::getAllFurniture(MinionTask task) {
  static EnumMap<MinionTask, vector<FurnitureType>> cache;
  static bool initialized = false;
  if (!initialized) {
    for (auto minionTask : ENUM_ALL(MinionTask)) {
      auto& taskInfo = CollectiveConfig::getTaskInfo(minionTask);
      switch (taskInfo.type) {
        case MinionTaskInfo::ARCHERY:
          cache[minionTask].push_back(FurnitureType::ARCHERY_RANGE);
          break;
        case MinionTaskInfo::FURNITURE:
          for (auto furnitureType : ENUM_ALL(FurnitureType))
            if (taskInfo.furniturePredicate(nullptr, nullptr, furnitureType))
              cache[minionTask].push_back(furnitureType);
          break;
        default: break;
      }
    }
    initialized = true;
  }
  return cache[task];
}

optional<MinionTask> MinionTasks::getTaskFor(WConstCollective col, WConstCreature c, FurnitureType type) {
  static EnumMap<FurnitureType, optional<MinionTask>> cache;
  static bool initialized = false;
  if (!initialized) {
    for (auto task : ENUM_ALL(MinionTask))
      for (auto furnitureType : getAllFurniture(task)) {
        CHECK(!cache[furnitureType]) << "Minion tasks " << EnumInfo<MinionTask>::getString(task) << " and "
            << EnumInfo<MinionTask>::getString(*cache[furnitureType]) << " both assigned to "
            << EnumInfo<FurnitureType>::getString(furnitureType);
        cache[furnitureType] = task;
      }
    initialized = true;
  }
  if (auto task = cache[type]) {
    auto& info = CollectiveConfig::getTaskInfo(*task);
    if (info.furniturePredicate(col, c, type))
      return *task;
  }
  return none;
}

static vector<Position> tryInQuarters(vector<Position> pos, WConstCollective collective, WConstCreature c) {
  auto& quarters = collective->getQuarters();
  auto& zones = collective->getZones();
  auto index = quarters.getAssigned(c->getUniqueId());
  auto inQuarters = pos.filter([&](Position pos) {
    if (index)
      return zones.isZone(pos, quarters.getAllQuarters()[*index].zone);
    else {
      for (auto& q : quarters.getAllQuarters())
        if (zones.isZone(pos, q.zone))
          return false;
      return true;
    }
  });
  if (!inQuarters.empty())
    return inQuarters;
  else
    return pos;
}

vector<Position> MinionTasks::getAllPositions(WConstCollective collective, WConstCreature c, MinionTask task,
    bool onlyActive) {
  vector<Position> ret;
  auto& info = CollectiveConfig::getTaskInfo(task);
  for (auto furnitureType : getAllFurniture(task))
    if (info.furniturePredicate(collective, c, furnitureType) &&
        (!onlyActive || info.activePredicate(collective, furnitureType)))
      append(ret, collective->getConstructions().getBuiltPositions(furnitureType));
  if (c) {
    ret = ret.filter([c](Position pos) { return c->canNavigateTo(pos); });
    ret = tryInQuarters(ret, collective, c);
  }
  return ret;
}



WTask MinionTasks::getExisting(WCollective collective, WCreature c, MinionTask task) {
  auto& info = CollectiveConfig::getTaskInfo(task);
  switch (info.type) {
    case MinionTaskInfo::WORKER:
      return collective->getTaskMap().getClosestTask(c);
    default:
      return nullptr;
  }
}

PTask MinionTasks::generate(WCollective collective, WCreature c, MinionTask task) {
  auto& info = CollectiveConfig::getTaskInfo(task);
  switch (info.type) {
    case MinionTaskInfo::FURNITURE: {
      vector<Position> squares = getAllPositions(collective, c, task, true);
      if (!squares.empty())
        return Task::applySquare(collective, squares, Task::RANDOM_CLOSE, Task::APPLY);
      else {
        vector<Position> squares = getAllPositions(collective, c, task, false);
        if (!squares.empty())
          return Task::applySquare(collective, squares, Task::LAZY, Task::NONE);
      }
      break;
    }
    case MinionTaskInfo::ARCHERY: {
      auto pos = collective->getConstructions().getBuiltPositions(FurnitureType::ARCHERY_RANGE);
      if (!pos.empty())
        return Task::archeryRange(collective, tryInQuarters(vector<Position>(pos.begin(), pos.end()), collective, c));
      else
        return nullptr;
    }
    case MinionTaskInfo::EXPLORE:
      if (auto pos = getTileToExplore(collective, c, task))
        return Task::explore(*pos);
      break;
    case MinionTaskInfo::COPULATE:
      if (WCreature target = getCopulationTarget(collective, c))
        return Task::copulate(collective, target, 20);
      break;
    case MinionTaskInfo::EAT: {
      const set<Position>& hatchery = collective->getConstructions().getBuiltPositions(FurnitureType::PIGSTY);
      if (!hatchery.empty())
        return Task::eat(tryInQuarters(vector<Position>(hatchery.begin(), hatchery.end()), collective, c));
      break;
      }
    case MinionTaskInfo::SPIDER: {
      auto& territory = collective->getTerritory();
      return Task::spider(territory.getAll().front(), territory.getExtended(3));
    }
    default:
      return nullptr;
  }
  return nullptr;
}

optional<TimeInterval> MinionTasks::getDuration(WConstCreature c, MinionTask task) {
  switch (task) {
    case MinionTask::COPULATE:
    case MinionTask::EAT:
    case MinionTask::BE_WHIPPED:
    case MinionTask::BE_TORTURED:
    case MinionTask::SLEEP: return none;
    default: return TimeInterval((int) 500 + 250 * c->getMorale());
  }
}


