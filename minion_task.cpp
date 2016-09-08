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

static vector<Creature*> getConsumptionTargets(const Collective* collective, Creature* consumer) {
  vector<Creature*> ret;
  for (Creature* c : Random.permutation(collective->getCreatures(MinionTrait::FIGHTER)))
    if (consumer->canConsume(c) && c != collective->getLeader())
      ret.push_back(c);
  return ret;
}

static Creature* getConsumptionTarget(const Collective* collective, Creature* consumer) {
  vector<Creature*> v = getConsumptionTargets(collective, consumer);
  if (!v.empty())
    return Random.choose(v);
  else
    return nullptr;
}

PTask MinionTasks::generate(Collective* collective, Creature* c, MinionTask task) {
  auto& info = CollectiveConfig::getTaskInfo(task);
  switch (info.type) {
    case MinionTaskInfo::FURNITURE: {
      const set<Position>& squares = collective->getConstructions().getBuiltPositions(info.furniture);
      if (!squares.empty()) {
        auto searchType = Task::RANDOM_CLOSE;
        if (auto workshopType = CollectiveConfig::getWorkshopType(task))
          if (collective->getWorkshops().get(*workshopType).isIdle())
            searchType = Task::LAZY;
        return Task::applySquare(collective, squares, searchType);
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
    case MinionTaskInfo::CONSUME:
      if (Creature* target = getConsumptionTarget(collective, c))
        return Task::consume(collective, target);
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

