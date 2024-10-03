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
#include "zones.h"
#include "resource_info.h"
#include "equipment.h"
#include "minion_equipment.h"
#include "game.h"
#include "content_factory.h"
#include "body.h"
#include "item.h"
#include "dancing.h"
#include "level.h"

SERIALIZE_DEF(MinionActivities, allFurniture, activities)
SERIALIZATION_CONSTRUCTOR_IMPL(MinionActivities)

static bool betterPos(Position from, Position current, Position candidate) {
  PROFILE;
  return from.dist8(current).value_or(1000000) > from.dist8(candidate).value_or(1000000);
}

static optional<Position> getTileToExplore(Collective* collective, const Creature* c, MinionActivity task) {
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

static Creature* getMinionToAbuse(Collective* collective, const Creature* abuser) {
  auto& minions = collective->getCreatures();
  Creature* target = nullptr;
  auto abuserPos = abuser->getPosition();
  for (auto c : minions) {
    if (c == abuser || c->isAffected(LastingEffect::SPEED) || !collective->getTerritory().contains(c->getPosition()) ||
        !c->getBody().isHumanoid() || !c->getBody().hasBrain(c->getGame()->getContentFactory()) ||
        collective->hasTrait(c, MinionTrait::LEADER) ||
        collective->getCurrentActivity(c).activity == MinionActivity::IDLE)
      continue;
    if (!target) {
      target = c;
      continue;
    }
    auto distTarget = target->getPosition().dist8(abuserPos).value_or(1000);
    auto dist = c->getPosition().dist8(abuserPos).value_or(1000);
    if (dist < distTarget)
      target = c;
  }
  return target;
}

static Creature* getCopulationTarget(const Collective* collective, const Creature* succubus) {
  for (auto trait : {MinionTrait::FIGHTER, MinionTrait::LEADER})
    for (Creature* c : Random.permutation(collective->getCreatures(trait)))
      if (succubus->canCopulateWith(c))
        return c;
  return nullptr;
}

const vector<FurnitureType>& MinionActivities::getAllFurniture(MinionActivity task) const {
  return allFurniture[task];
}

optional<MinionActivity> MinionActivities::getActivityFor(const Collective* col, const Creature* c,
    FurnitureType type) const {
  if (auto task = getReferenceMaybe(activities, type)) {
    auto& info = CollectiveConfig::getActivityInfo(*task);
    if (info.furniturePredicate(col->getGame()->getContentFactory(), col, c, type))
      return *task;
  }
  return none;
}

template <typename PosType, typename PosFun>
static vector<PosType> tryInQuarters(vector<PosType> pos, const Collective* collective, const Creature* c, PosFun posFun) {
  PROFILE;
  auto& zones = collective->getZones();
  if (zones.getPositions(ZoneId::QUARTERS).empty())
    return pos;
  auto& myQuarters = zones.getQuarters(c->getUniqueId());
  vector<PosType> inQuarters;
  if (!myQuarters.empty()) {
    PROFILE_BLOCK("has quarters");
    inQuarters = pos.filter([&](auto& pos) {return myQuarters.count(posFun(pos));});
  } else {
    PROFILE_BLOCK("no quarters");
    auto& allQuarters = zones.getPositions(ZoneId::QUARTERS);
    inQuarters = pos.filter([&](auto& pos) { return !allQuarters.count(posFun(pos)); });
  }
  if (!inQuarters.empty())
    return inQuarters;
  else
    return pos;
}

static vector<Position> tryInQuarters(vector<Position> pos, const Collective* collective, const Creature* c) {
  return tryInQuarters(std::move(pos), collective, c, [](const Position& pos) -> const Position& { return pos; });
}

vector<pair<Position, FurnitureLayer>> MinionActivities::getAllPositions(const Collective* collective,
    const Creature* c, MinionActivity activity) const {
  auto profileName = "MinionActivities::getAllPositions " + EnumInfo<MinionActivity>::getString(activity);
  PROFILE_BLOCK(profileName.data());
  vector<pair<Position, FurnitureLayer>> ret;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  {
    PROFILE_BLOCK("get all positions");
    for (auto furnitureType : getAllFurniture(activity))
      if (info.furniturePredicate(collective->getGame()->getContentFactory(), collective, c, furnitureType)) {
        auto toPair = [layer = collective->getGame()->getContentFactory()->furniture.getData(furnitureType).getLayer()]
            (Position p) { return make_pair(p, layer); };
        append(ret, collective->getConstructions().getBuiltPositions(furnitureType).transform(toPair));
      }
  }
  if (info.secondaryPredicate) {
    PROFILE_BLOCK("secondary predicate");
    ret = ret.filter([&](const auto& elem) {
      return info.secondaryPredicate(elem.first.getFurniture(elem.second), collective);
    });
  }
  if (c) {
    PROFILE_BLOCK("can navigate to");
    auto movement = c->getMovementType();
    ret = ret.filter([&](auto& pos) { return pos.first.canNavigateToOrNeighbor(c->getPosition(), movement); });
    ret = tryInQuarters(ret, collective, c, [](const pair<Position, FurnitureLayer>& p) -> const Position& { return p.first; });
  }
  if (c && info.type == MinionActivityInfo::Type::FURNITURE && info.searchType == Task::SearchType::LAZY)
    ret = ret.filter([&](auto& pos) { return !pos.first.getCreature() || pos.first.getCreature() == c; });
  return ret;
}

static PTask getDropItemsTask(Collective* collective, const Creature* creature) {
  auto& config = collective->getConfig();
  auto& items = creature->getEquipment().getItems();
  HashMap<StorageId, vector<Item*>> itemMap;
  for (auto it : items)
    if (!collective->getMinionEquipment().isOwner(it, creature))
      for (auto id : it->getStorageIds())
        if (!collective->getStoragePositions(id).empty()) {
          itemMap[id].push_back(it);
          break;
        }
  for (auto& elem : itemMap)
      return Task::dropItems(elem.second, elem.first, collective);
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

Task* MinionActivities::getExisting(Collective* collective, Creature* c, MinionActivity activity) {
  PROFILE;
  return collective->getTaskMap().getClosestTask(c, activity, false, collective);
}

PTask MinionActivities::generateDropTask(Collective* collective, Creature* c, MinionActivity task) {
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
  auto& candidate = [&] () -> const PositionSet& {
    auto& quarters = collective->getZones().getQuarters(c->getUniqueId());
    if (!quarters.empty())
      return quarters;
    if (c->isAffected(LastingEffect::STEED))
      return collective->getConstructions().getBuiltPositions(FurnitureType("STABLE"));
    if (collective->hasTrait(c, MinionTrait::PRISONER))
      return collective->getConstructions().getBuiltPositions(FurnitureType("PRISON"));
    if (!collective->hasTrait(c, MinionTrait::NO_LEISURE_ZONE))
      return collective->getZones().getPositions(ZoneId::LEISURE);
    else
      return collective->getTerritory().getAllAsSet();
  }();
  if (!candidate.empty())
    return candidate;
  return collective->getTerritory().getAllAsSet();
}

PTask MinionActivities::generate(Collective* collective, Creature* c, MinionActivity activity) const {
  PROFILE;
  auto& info = CollectiveConfig::getActivityInfo(activity);
  switch (info.type) {
    case MinionActivityInfo::IDLE: {
      PROFILE_BLOCK("Idle");
      if (collective->getDancing().getTarget(c))
        return Task::dance(collective);
      auto& myTerritory = getIdlePositions(collective, c);
      if (c->isAutomaton() && myTerritory.count(c->getPosition()))
        return Task::idle();
      if (auto p = collective->getTerritory().getCentralPoint())
        if (p->getLevel()->depth == 0)
          if (!myTerritory.empty() && collective->getGame()->getSunlightInfo().getState() == SunlightState::NIGHT) {
            if ((c->getPosition().isCovered() && myTerritory.count(c->getPosition()))) {
              PROFILE_BLOCK("Stay in for the night");
              return Task::idle();
            }
            auto indoors = limitToIndoors(myTerritory);
            return Task::chain(Task::transferTo(collective->getModel()),
                Task::stayIn(!indoors.empty() ? std::move(indoors) : myTerritory.asVector()));
          }
      auto& pigstyPos = collective->getConstructions().getBuiltPositions(FurnitureType("PIGSTY"));
      if (pigstyPos.count(c->getPosition()) && !myTerritory.empty()) {
        PROFILE_BLOCK("Leave pigsty");
        return Task::doneWhen(Task::goTo(Random.choose(myTerritory)),
            TaskPredicate::outsidePositions(c, pigstyPos));
      }
      auto& leaders = collective->getLeaders();
      if (!myTerritory.empty()) {
        PROFILE_BLOCK("Stay in territory");
        return Task::chain(Task::transferTo(collective->getModel()), Task::stayIn(myTerritory.asVector()));
      } else if (collective->getConfig().getFollowLeaderIfNoTerritory() && !leaders.empty()) {
        PROFILE_BLOCK("Follow leader");
        return Task::alwaysDone(Task::follow(leaders[0]));
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
        return Task::applySquare(collective, squares, info.searchType, Task::APPLY);
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
    case MinionActivityInfo::MINION_ABUSE: {
      PROFILE_BLOCK("Minion abuse");
      if (auto minion = getMinionToAbuse(collective, c))
        return Task::abuseMinion(collective, minion);
      break;
    }
    case MinionActivityInfo::COPULATE: {
      PROFILE_BLOCK("Copulate");
      if (Creature* target = getCopulationTarget(collective, c))
        return Task::copulate(collective, target, 20);
      break;
    }
    case MinionActivityInfo::CONFESSION: {
      PROFILE_BLOCK("Confess");
      const auto& pos = collective->getConstructions().getBuiltPositions(FurnitureType("CONFESSIONAL"));
      if (!pos.empty())
        return Task::chain(
            Task::applySquare(collective, pos.transform([](auto pos){ return make_pair(pos, FurnitureLayer::MIDDLE); }),
                Task::SearchType::CONFESSION, Task::APPLY),
            Task::wait(10_visible)
        );
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
    case MinionActivityInfo::GUARD1:
    case MinionActivityInfo::GUARD2:
    case MinionActivityInfo::GUARD3:
    case MinionActivityInfo::WORKER:
      return nullptr;
  }
  return nullptr;
}

optional<TimeInterval> MinionActivities::getDuration(const Creature* c, MinionActivity task) {
  switch (task) {
    case MinionActivity::COPULATE:
    case MinionActivity::IDLE:
    case MinionActivity::BE_WHIPPED:
    case MinionActivity::BE_TORTURED:
    case MinionActivity::SLEEP: return none;
    case MinionActivity::EAT:
    case MinionActivity::MINION_ABUSE:
      return TimeInterval((int) 30 + Random.get(-10, 10));
    case MinionActivity::RITUAL:
      return 150_visible;
    case MinionActivity::HEARING_CONFESSION:
      return 50_visible;
    case MinionActivity::PREACHING:
    case MinionActivity::MASS:
      return 200_visible;
    case MinionActivity::CONFESSION:
      return none;
    default:
      return TimeInterval(500 + Random.get(Range(0, 250)));
  }
}


