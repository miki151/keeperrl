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
#include "content_factory.h"

SERIALIZE_DEF(MinionActivities, allFurniture, activities)
SERIALIZATION_CONSTRUCTOR_IMPL(MinionActivities)

static bool betterPos(Position from, Position current, Position candidate) {
  PROFILE;
  return from.dist8(current).value_or(1000000) > from.dist8(candidate).value_or(1000000);
}

static optional<Position> getTileToExplore(WCollective collective, const Creature* c, MinionActivity task) {
  auto& borderTiles = collective->getKnownTiles().getBorderTiles();
  auto blockName = "get tile to explore " + toString(borderTiles.size());
  PROFILE_BLOCK(blockName.data());
  auto movementType = c->getMovementType();
  optional<Position> caveTile;
  optional<Position> outdoorTile;
  for (auto& pos : Random.permutation(borderTiles)) {
    //CHECK(pos.getModel() == collective->getModel());
    if (pos.isCovered()) {
      if ((!caveTile || betterPos(c->getPosition(), *caveTile, pos)) &&
          pos.canNavigateTo(c->getPosition(), movementType))
        caveTile = pos;
    } else {
      if ((!outdoorTile || betterPos(c->getPosition(), *outdoorTile, pos)) &&
          pos.canNavigateTo(c->getPosition(), movementType))
        outdoorTile = pos;
    }
  }
  switch (task) {
    case MinionActivity::EXPLORE_CAVES: {
      if (caveTile) {
        PROFILE_BLOCK("found in cave");
        return *caveTile;
      }
      FALLTHROUGH;
    }
    case MinionActivity::EXPLORE:
      FALLTHROUGH;
    case MinionActivity::EXPLORE_NOCTURNAL:
      if (outdoorTile) {
        PROFILE_BLOCK("found outdoor");
        return outdoorTile;
      }
      break;
    default: FATAL << "Unrecognized explore task: " << int(task);
  }
  {
    PROFILE_BLOCK("not found");
    return none;
  }
}

static Creature* getCopulationTarget(WConstCollective collective, const Creature* succubus) {
  for (Creature* c : Random.permutation(collective->getCreatures(MinionTrait::FIGHTER)))
    if (succubus->canCopulateWith(c))
      return c;
  return nullptr;
}

const vector<FurnitureType>& MinionActivities::getAllFurniture(MinionActivity task) const {
  return allFurniture[task];
}

optional<MinionActivity> MinionActivities::getActivityFor(WConstCollective col, const Creature* c,
    FurnitureType type) const {
  if (auto task = getReferenceMaybe(activities, type)) {
    auto& info = CollectiveConfig::getActivityInfo(*task);
    if (info.furniturePredicate(col->getGame()->getContentFactory(), col, c, type))
      return *task;
  }
  return none;
}

template <typename PosType, typename PosFun>
static vector<PosType> tryInQuarters(vector<PosType> pos, WConstCollective collective, const Creature* c, PosFun posFun) {
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
  vector<PosType> inQuarters;
  if (index) {
    PROFILE_BLOCK("has quarters");
    inQuarters = pos.filter([&](auto& pos) {return zones.isZone(posFun(pos), quarters.getAllQuarters()[*index].zone);});
  } else {
    PROFILE_BLOCK("no quarters");
    EnumSet<ZoneId> allZones;
    for (auto& q : quarters.getAllQuarters())
      allZones.insert(q.zone);
    inQuarters = pos.filter([&](auto& pos) { return !zones.isAnyZone(posFun(pos), allZones); });
  }
  if (!inQuarters.empty())
    return inQuarters;
  else
    return pos;
}

static vector<Position> tryInQuarters(vector<Position> pos, WConstCollective collective, const Creature* c) {
  return tryInQuarters(std::move(pos), collective, c, [](const Position& pos) -> const Position& { return pos; });
}

vector<pair<Position, FurnitureLayer>> MinionActivities::getAllPositions(WConstCollective collective, const Creature* c,
    MinionActivity activity) const {
  PROFILE;
  vector<pair<Position, FurnitureLayer>> ret;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  for (auto furnitureType : getAllFurniture(activity))
    if (info.furniturePredicate(collective->getGame()->getContentFactory(), collective, c, furnitureType)) {
      auto toPair = [layer = collective->getGame()->getContentFactory()->furniture.getData(furnitureType).getLayer()]
          (Position p) { return make_pair(p, layer); };
      append(ret, collective->getConstructions().getBuiltPositions(furnitureType).transform(toPair));
    }
  if (c) {
    auto movement = c->getMovementType();
    ret = ret.filter([&](auto& pos) { return pos.first.canNavigateToOrNeighbor(c->getPosition(), movement); });
    ret = tryInQuarters(ret, collective, c, [](const pair<Position, FurnitureLayer>& p) -> const Position& { return p.first; });
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


MinionActivities::MinionActivities(const ContentFactory* contentFactory) {
  for (auto minionTask : ENUM_ALL(MinionActivity)) {
    auto& taskInfo = CollectiveConfig::getActivityInfo(minionTask);
    switch (taskInfo.type) {
      case MinionActivityInfo::ARCHERY:
        allFurniture[minionTask].push_back(FurnitureType("ARCHERY_RANGE"));
        break;
      case MinionActivityInfo::FURNITURE:
        for (auto furnitureType : contentFactory->furniture.getAllFurnitureType())
          if (taskInfo.furniturePredicate(contentFactory, nullptr, nullptr, furnitureType))
            allFurniture[minionTask].push_back(furnitureType);
        break;
      default: break;
    }
  }
  for (auto task : ENUM_ALL(MinionActivity))
    for (auto furnitureType : getAllFurniture(task)) {
      CHECK(!activities.count(furnitureType)) << "Minion tasks " << EnumInfo<MinionActivity>::getString(task)
          << " and "
          << EnumInfo<MinionActivity>::getString(activities[furnitureType]) << " both assigned to "
          << furnitureType.data();
      activities[furnitureType] = task;
    }
}

WTask MinionActivities::getExisting(WCollective collective, Creature* c, MinionActivity activity) {
  PROFILE;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  switch (info.type) {
    case MinionActivityInfo::WORKER: {
      return collective->getTaskMap().getClosestTask(c, activity, false, collective);
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

static vector<Position> limitToIndoors(const PositionSet& v) {
  vector<Position> ret;
  ret.reserve(v.size());
  for (auto& pos : v)
    if (pos.isCovered())
      ret.push_back(pos);
  return ret;
}

const PositionSet& getIdlePositions(const Collective* collective, const Creature* c) {
  if (auto q = collective->getQuarters().getAssigned(c->getUniqueId()))
    return collective->getZones().getPositions(Quarters::getAllQuarters()[*q].zone);
  if (!collective->getZones().getPositions(ZoneId::LEISURE).empty() &&
      !collective->hasTrait(c, MinionTrait::WORKER))
    return collective->getZones().getPositions(ZoneId::LEISURE);
  else
    return collective->getTerritory().getAllAsSet();
}

PTask MinionActivities::generate(WCollective collective, Creature* c, MinionActivity activity) const {
  PROFILE;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  switch (info.type) {
    case MinionActivityInfo::IDLE: {
      PROFILE_BLOCK("Idle");
      auto& myTerritory = getIdlePositions(collective, c);
      if (collective->getGame()->getSunlightInfo().getState() == SunlightState::NIGHT) {
        if ((c->getPosition().isCovered() && myTerritory.count(c->getPosition()))) {
          PROFILE_BLOCK("Stay in for the night");
          return Task::idle();
        }
        auto indoors = limitToIndoors(myTerritory);
        if (!indoors.empty())
          return Task::chain(Task::transferTo(collective->getModel()), Task::stayIn(std::move(indoors)));
        else
          return Task::idle();
      }
      auto& pigstyPos = collective->getConstructions().getBuiltPositions(FurnitureType("PIGSTY"));
      if (pigstyPos.count(c->getPosition()) && !myTerritory.empty()) {
        PROFILE_BLOCK("Leave pigsty");
        return Task::doneWhen(Task::goTo(Random.choose(myTerritory)),
            TaskPredicate::outsidePositions(c, pigstyPos));
      }
      auto leader = collective->getLeader();
      if (!myTerritory.empty()) {
        PROFILE_BLOCK("Stay in territory");
        return Task::chain(Task::transferTo(collective->getModel()), Task::stayIn(myTerritory.asVector()));
      } else if (collective->getConfig().getFollowLeaderIfNoTerritory() && leader) {
        PROFILE_BLOCK("Follor leader");
        return Task::alwaysDone(Task::follow(leader));
      }
      {
        PROFILE_BLOCK("Just idle");
        return Task::idle();
      }
    }
    case MinionActivityInfo::FURNITURE: {
      PROFILE_BLOCK("Furniture");
      vector<pair<Position, FurnitureLayer>> squares = getAllPositions(collective, c, activity);
      if (!squares.empty())
        return Task::applySquare(collective, squares, Task::RANDOM_CLOSE, Task::APPLY);
      break;
    }
    case MinionActivityInfo::ARCHERY: {
      PROFILE_BLOCK("Archery");
      auto pos = collective->getConstructions().getBuiltPositions(FurnitureType("ARCHERY_RANGE"));
      if (!pos.empty())
        return Task::archeryRange(collective, tryInQuarters(vector<Position>(pos.begin(), pos.end()), collective, c));
      else
        return nullptr;
    }
    case MinionActivityInfo::EXPLORE: {
      PROFILE_BLOCK("Explore");
      if (auto pos = getTileToExplore(collective, c, activity))
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
      const auto& hatchery = collective->getConstructions().getBuiltPositions(FurnitureType("PIGSTY"));
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


