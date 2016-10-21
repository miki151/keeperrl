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

static optional<Position> getTileToExplore(const Collective* collective, const Creature* c, MinionTask task) {
  vector<Position> border = Random.permutation(collective->getKnownTiles().getBorderTiles());
  switch (task) {
    case MinionTask::EXPLORE_CAVES:
      if (auto pos = getRandomCloseTile(c->getPosition(), border,
            [&](Position p) {
                return p.isSameLevel(collective->getLevel()) && p.isCovered() &&
                    (!c->getPosition().isSameLevel(collective->getLevel()) || c->isSameSector(p));}))
        return pos;
    case MinionTask::EXPLORE:
    case MinionTask::EXPLORE_NOCTURNAL:
      return getRandomCloseTile(c->getPosition(), border,
          [&](Position pos) { return pos.isSameLevel(collective->getLevel()) && !pos.isCovered()
              && (!c->getPosition().isSameLevel(collective->getLevel()) || c->isSameSector(pos));});
    default: FAIL << "Unrecognized explore task: " << int(task);
  }
  return none;
}

static Creature* getCopulationTarget(const Collective* collective, Creature* succubus) {
  for (Creature* c : Random.permutation(collective->getCreatures(MinionTrait::FIGHTER)))
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
        case MinionTaskInfo::FURNITURE:
          for (auto furnitureType : ENUM_ALL(FurnitureType))
            if (taskInfo.furniturePredicate(nullptr, furnitureType))
              cache[minionTask].push_back(furnitureType);
          break;
        default: break;
      }
    }
    initialized = true;
  }
  return cache[task];
}

optional<MinionTask> MinionTasks::getTaskFor(const Creature* c, FurnitureType type) {
  static EnumMap<FurnitureType, optional<MinionTask>> cache;
  static bool initialized = false;
  if (!initialized) {
    for (auto task : ENUM_ALL(MinionTask))
      for (auto furnitureType : getAllFurniture(task))
        cache[furnitureType] = task;
    initialized = true;
  }
  if (auto task = cache[type]) {
    auto& info = CollectiveConfig::getTaskInfo(*task);
    if (info.furniturePredicate(c, type))
      return *task;
  }
  return none;
}

vector<Position> MinionTasks::getAllPositions(const Collective* collective, const Creature* c, MinionTask task,
    bool onlyActive) {
  vector<Position> ret;
  auto& info = CollectiveConfig::getTaskInfo(task);
  for (auto furnitureType : getAllFurniture(task))
    if (info.furniturePredicate(c, furnitureType) && (!onlyActive || info.activePredicate(collective, furnitureType)))
      append(ret, collective->getConstructions().getBuiltPositions(furnitureType));
  return ret;
}

PTask MinionTasks::generate(Collective* collective, Creature* c, MinionTask task) {
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
    case MinionTaskInfo::EXPLORE:
      if (auto pos = getTileToExplore(collective, c, task))
        return Task::explore(*pos);
      break;
    case MinionTaskInfo::COPULATE:
      if (Creature* target = getCopulationTarget(collective, c))
        return Task::copulate(collective, target, 20);
      break;
    case MinionTaskInfo::EAT: {
      const set<Position>& hatchery = collective->getConstructions().getBuiltPositions(FurnitureType::PIGSTY);
      if (!hatchery.empty())
        return Task::eat(hatchery);
      break;
      }
    case MinionTaskInfo::SPIDER: {
      auto& territory = collective->getTerritory();
      return Task::spider(territory.getAll().front(), territory.getExtended(3), territory.getExtended(6));
    }
  }
  return nullptr;
}

optional<double> MinionTasks::getDuration(const Creature* c, MinionTask task) {
  switch (task) {
    case MinionTask::COPULATE:
    case MinionTask::GRAVE:
    case MinionTask::LAIR:
    case MinionTask::EAT:
    case MinionTask::BE_WHIPPED:
    case MinionTask::BE_TORTURED:
    case MinionTask::SLEEP: return none;
    default: return 500 + 250 * c->getMorale();
  }
}


