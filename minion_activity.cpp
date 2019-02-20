#include "stdafx.h"
#include "minion_activity.h"
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
#include "resource_info.h"
#include "equipment.h"
#include "minion_equipment.h"
#include "game.h"

static bool betterPos(Position from, Position current, Position candidate) {
  double maxDiff = 0.3;
  double curDist = from.dist8(current).value_or(1000000);
  double newDist = from.dist8(candidate).value_or(1000000);
  return Random.getDouble() <= 1.0 - (newDist - curDist) / (curDist * maxDiff);
}

template <typename Pred>
static optional<Position> getRandomCloseTile(Position from, const vector<Position>& tiles, Pred predicate) {
  optional<Position> ret;
  for (Position pos : tiles)
    if (predicate(pos) && (!ret || betterPos(from, *ret, pos)))
      ret = pos;
  return ret;
}

static optional<Position> getTileToExplore(WConstCollective collective, const Creature* c, MinionActivity task) {
  PROFILE;
  vector<Position> border = Random.permutation(collective->getKnownTiles().getBorderTiles());
  switch (task) {
    case MinionActivity::EXPLORE_CAVES:
      if (auto pos = getRandomCloseTile(c->getPosition(), border,
            [&](Position p) { return p.isCovered() && c->canNavigateTo(p);}))
        return pos;
      FALLTHROUGH;
    case MinionActivity::EXPLORE:
      FALLTHROUGH;
    case MinionActivity::EXPLORE_NOCTURNAL:
      return getRandomCloseTile(c->getPosition(), border,
          [&](Position pos) { return !pos.isCovered() && c->canNavigateTo(pos);});
    default: FATAL << "Unrecognized explore task: " << int(task);
  }
  return none;
}

static Creature* getCopulationTarget(WConstCollective collective, const Creature* succubus) {
  for (Creature* c : Random.permutation(collective->getCreatures(MinionTrait::FIGHTER)))
    if (succubus->canCopulateWith(c))
      return c;
  return nullptr;
}

const vector<FurnitureType>& MinionActivities::getAllFurniture(MinionActivity task) {
  static EnumMap<MinionActivity, vector<FurnitureType>> cache;
  static bool initialized = false;
  if (!initialized) {
    for (auto minionTask : ENUM_ALL(MinionActivity)) {
      auto& taskInfo = CollectiveConfig::getActivityInfo(minionTask);
      switch (taskInfo.type) {
        case MinionActivityInfo::ARCHERY:
          cache[minionTask].push_back(FurnitureType::ARCHERY_RANGE);
          break;
        case MinionActivityInfo::FURNITURE:
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

optional<MinionActivity> MinionActivities::getActivityFor(WConstCollective col, const Creature* c, FurnitureType type) {
  static EnumMap<FurnitureType, optional<MinionActivity>> cache;
  static bool initialized = false;
  if (!initialized) {
    for (auto task : ENUM_ALL(MinionActivity))
      for (auto furnitureType : getAllFurniture(task)) {
        CHECK(!cache[furnitureType]) << "Minion tasks " << EnumInfo<MinionActivity>::getString(task) << " and "
            << EnumInfo<MinionActivity>::getString(*cache[furnitureType]) << " both assigned to "
            << EnumInfo<FurnitureType>::getString(furnitureType);
        cache[furnitureType] = task;
      }
    initialized = true;
  }
  if (auto task = cache[type]) {
    auto& info = CollectiveConfig::getActivityInfo(*task);
    if (info.furniturePredicate(col, c, type))
      return *task;
  }
  return none;
}

static vector<Position> tryInQuarters(vector<Position> pos, WConstCollective collective, const Creature* c) {
  PROFILE;
  auto& quarters = collective->getQuarters();
  auto& zones = collective->getZones();
  bool hasAnyQuarters = [&] {
    for (auto& q : quarters.getAllQuarters())
      if (!zones.getPositions(q.zone).empty())
        return true;
    return false;
  }();
  if (!hasAnyQuarters)
    return pos;
  auto index = quarters.getAssigned(c->getUniqueId());
  vector<Position> inQuarters;
  if (index) {
    PROFILE_BLOCK("has quarters");
    inQuarters = pos.filter([&](Position pos) {return zones.isZone(pos, quarters.getAllQuarters()[*index].zone);});
  } else {
    PROFILE_BLOCK("no quarters");
    EnumSet<ZoneId> allZones;
    for (auto& q : quarters.getAllQuarters())
      allZones.insert(q.zone);
    inQuarters = pos.filter([&](Position pos) { return !zones.isAnyZone(pos, allZones); });
  }
  if (!inQuarters.empty())
    return inQuarters;
  else
    return pos;
}

vector<Position> MinionActivities::getAllPositions(WConstCollective collective, const Creature* c,
    MinionActivity activity) {
  PROFILE;
  vector<Position> ret;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  for (auto furnitureType : getAllFurniture(activity))
    if (info.furniturePredicate(collective, c, furnitureType))
      append(ret, collective->getConstructions().getBuiltPositions(furnitureType));
  if (c) {
    auto movement = c->getMovementType();
    ret = ret.filter([&](Position pos) { return pos.canNavigateToOrNeighbor(c->getPosition(), movement); });
    ret = tryInQuarters(ret, collective, c);
  }
  return ret;
}

static PTask getDropItemsTask(WCollective collective, const Creature* creature) {
  auto& config = collective->getConfig();
  for (const ItemFetchInfo& elem : config.getFetchInfo()) {
    vector<Item*> items = creature->getEquipment().getItems(elem.index).filter([&elem, &collective, &creature](const Item* item) {
        return elem.predicate(collective, item) && !collective->getMinionEquipment().isOwner(item, creature); });
    if (!items.empty() && !collective->getStoragePositions(elem.storageId).empty())
      return Task::dropItems(items, elem.storageId, collective);
  }
  return nullptr;
};


WTask MinionActivities::getExisting(WCollective collective, Creature* c, MinionActivity activity) {
  PROFILE;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  switch (info.type) {
    case MinionActivityInfo::WORKER: {
      return collective->getTaskMap().getClosestTask(c, activity, false);
    } default:
      return nullptr;
  }
}

PTask MinionActivities::generateDropTask(WCollective collective, Creature* c, MinionActivity task) {
  if (task != MinionActivity::HAULING)
    if (PTask ret = getDropItemsTask(collective, c))
      return ret;
  return nullptr;
}

static vector<Position> limitToIndoors(vector<Position> v) {
  return v.filter([](const Position& pos) { return pos.isCovered(); });
}

PTask MinionActivities::generate(WCollective collective, Creature* c, MinionActivity task) {
  PROFILE;
  auto& info = CollectiveConfig::getActivityInfo(task);
  switch (info.type) {
    case MinionActivityInfo::IDLE: {
      PROFILE_BLOCK("Idle");
      auto myTerritory = (collective->getZones().getPositions(ZoneId::LEISURE).empty() ||
            collective->hasTrait(c, MinionTrait::WORKER)) ?
          tryInQuarters(collective->getTerritory().getAll(), collective, c) :
          collective->getZones().getPositions(ZoneId::LEISURE).asVector();
      myTerritory = myTerritory.filter([&](const auto& pos) { return pos.canEnterEmpty(c); });
      if (collective->getGame()->getSunlightInfo().getState() == SunlightState::NIGHT) {
        if (c->getPosition().isCovered() && myTerritory.contains(c->getPosition()))
          return Task::idle();
        myTerritory = limitToIndoors(std::move(myTerritory));
      }
      auto& pigstyPos = collective->getConstructions().getBuiltPositions(FurnitureType::PIGSTY);
      if (pigstyPos.count(c->getPosition()))
        return Task::doneWhen(Task::goTo(Random.choose(myTerritory)),
            TaskPredicate::outsidePositions(c, pigstyPos));
      auto leader = collective->getLeader();
      if (!myTerritory.empty())
        return Task::chain(Task::transferTo(collective->getModel()), Task::stayIn(myTerritory));
      else if (collective->getConfig().getFollowLeaderIfNoTerritory() && leader)
        return Task::alwaysDone(Task::follow(leader));
      return Task::idle();
    }
    case MinionActivityInfo::FURNITURE: {
      PROFILE_BLOCK("Furniture");
      vector<Position> squares = getAllPositions(collective, c, task);
      if (!squares.empty())
        return Task::applySquare(collective, squares, Task::RANDOM_CLOSE, Task::APPLY);
      break;
    }
    case MinionActivityInfo::ARCHERY: {
      PROFILE_BLOCK("Archery");
      auto pos = collective->getConstructions().getBuiltPositions(FurnitureType::ARCHERY_RANGE);
      if (!pos.empty())
        return Task::archeryRange(collective, tryInQuarters(vector<Position>(pos.begin(), pos.end()), collective, c));
      else
        return nullptr;
    }
    case MinionActivityInfo::EXPLORE: {
      PROFILE_BLOCK("Explore");
      if (auto pos = getTileToExplore(collective, c, task))
        return Task::explore(*pos);
      break;
    }
    case MinionActivityInfo::COPULATE: {
      PROFILE_BLOCK("Copulate");
      if (Creature* target = getCopulationTarget(collective, c))
        return Task::copulate(collective, target, 20);
      break;
    }
    case MinionActivityInfo::EAT: {
      PROFILE_BLOCK("Eat");
      const auto& hatchery = collective->getConstructions().getBuiltPositions(FurnitureType::PIGSTY);
      if (!hatchery.empty())
        return Task::eat(tryInQuarters(vector<Position>(hatchery.begin(), hatchery.end()), collective, c));
      break;
      }
    case MinionActivityInfo::SPIDER: {
      PROFILE_BLOCK("Spider");
      auto& territory = collective->getTerritory();
      if (!territory.isEmpty())
        return Task::spider(territory.getAll().front(), territory.getExtended(3));
      break;
    }
    case MinionActivityInfo::WORKER: {
      return nullptr;
    }
  }
  return nullptr;
}

optional<TimeInterval> MinionActivities::getDuration(const Creature* c, MinionActivity task) {
  switch (task) {
    case MinionActivity::COPULATE:
    case MinionActivity::EAT:
    case MinionActivity::IDLE:
    case MinionActivity::BE_WHIPPED:
    case MinionActivity::BE_TORTURED:
    case MinionActivity::SLEEP: return none;
    default: return TimeInterval((int) 500 + 250 * c->getMorale());
  }
}


