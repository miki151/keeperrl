/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"
#include "util.h"
#include "shortest_path.h"
#include "level.h"
#include "creature.h"
#include "lasting_effect.h"
#include "furniture.h"
#include "furniture_usage.h"

SERIALIZE_DEF(ShortestPath, path, target, bounds, reversed)
SERIALIZATION_CONSTRUCTOR_IMPL(ShortestPath)

const double ShortestPath::infinity = 1000000000;
const double LevelShortestPath::infinity = 1000000000;

const int revShortestLimit = 15;

class DistanceTable {
  public:
  DistanceTable(Rectangle bounds) : ddist(bounds), dirty(bounds, 0) {}

  double getDistance(Vec2 v) const {
    return dirty[v] < counter ? ShortestPath::infinity : ddist[v];
  }

  void setDistance(Vec2 v, double d) {
    ddist[v] = d;
    dirty[v] = counter;
  }

  void clear() {
    ++counter;
  }

  private:
  Table<double> ddist;
  Table<int> dirty;
  int counter = 1;
};

static DistanceTable distanceTable(Level::getMaxBounds());
static DirtyTable<double> navigationCostCache(Level::getMaxBounds(), 0);

template <typename Fun>
static auto getCached(Fun fun) {
  return [fun] (Vec2 v) {
    if (navigationCostCache.isDirty(v))
      return navigationCostCache.getDirtyValue(v);
    else {
      auto res = fun(v);
      navigationCostCache.setValue(v, res);
      return res;
    }
  };
}

const int margin = 15;

ShortestPath::ShortestPath(Rectangle a, function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun,
    function<vector<Vec2>(Vec2)> directions, Vec2 to, Vec2 from, double mult) : ShortestPath(TemplateConstr{},
    std::move(a), std::move(entryFun), std::move(lengthFun), std::move(directions), to, from, mult) {}

template <typename EntryFun, typename LengthFun, typename DirectionsFun>
ShortestPath::ShortestPath(TemplateConstr, Rectangle a, EntryFun entryFun, LengthFun lengthFun,
    DirectionsFun directions, Vec2 to, Vec2 from, double mult) : target(to), bounds(a) {
  PROFILE;
  CHECK(Level::getMaxBounds().contains(a));
  navigationCostCache.clear();
  if (mult == 0)
    init(getCached(entryFun), lengthFun, directions, target, from);
  else {
    init(getCached(entryFun), lengthFun, directions, target, none, revShortestLimit);
    distanceTable.setDistance(target, infinity);
    navigationCostCache.clear();
    reverse(getCached(entryFun), lengthFun, directions, mult, from, revShortestLimit);
  }
}

ShortestPath::ShortestPath(Rectangle area, function<double (Vec2)> entryFun, function<double(Vec2)> lengthFun,
    vector<Vec2> directions, Vec2 target, Vec2 from, double mult) : ShortestPath(area, entryFun, lengthFun,
    [directions](Vec2) { return directions; }, target, from, mult)
{
}

struct QueueElem {
  Vec2 pos;
  double value;
};

bool inline operator < (const QueueElem& e1, const QueueElem& e2) {
  return e1.value > e2.value || (e1.value == e2.value && e1.pos < e2.pos);
}

template <typename EntryFun, typename LengthFun, typename DirectionsFun>
void ShortestPath::init(EntryFun entryFun, LengthFun lengthFun, DirectionsFun directions,
    Vec2 target, optional<Vec2> from, optional<int> limit) {
  PROFILE;
  reversed = false;
  distanceTable.clear();
  function<QueueElem(Vec2)> makeElem;
  if (from)
    makeElem = [&](Vec2 pos) ->QueueElem { return {pos, distanceTable.getDistance(pos) + lengthFun(pos)}; };
  else
    makeElem = [&](Vec2 pos) ->QueueElem { return {pos, distanceTable.getDistance(pos)}; };
  priority_queue<QueueElem, vector<QueueElem>> q;
  distanceTable.setDistance(target, 0);
  q.push(makeElem(target));
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top().pos;
    double posDist = distanceTable.getDistance(pos);
   // INFO << "Popping " << pos << " " << distance[pos]  << " " << (from ? (*from - pos).length4() : 0);
    if (from == pos || (limit && distanceTable.getDistance(pos) >= *limit)) {
      INFO << "Shortest path from " << (from ? *from : Vec2(-1, -1)) << " to " << target << " " << numPopped
        << " visited distance " << distanceTable.getDistance(pos);
      constructPath(pos, directions);
      return;
    }
    q.pop();
    for (Vec2 dir : directions(pos)) {
      PROFILE_BLOCK("loop body");
      Vec2 next = pos + dir;
      if (next.inRectangle(bounds)) {
        double nextDist = distanceTable.getDistance(next);
        if (posDist < nextDist) {
          PROFILE_BLOCK("check entry");
          double dist = posDist + entryFun(next);
          CHECK(dist > posDist) << "Entry fun non positive " << dist - posDist;
          if (dist < nextDist) {
            distanceTable.setDistance(next, dist);
            {
              PROFILE_BLOCK("queue push");
              q.push(makeElem(next));
            }
          }
        }
      }
    }
  }
  INFO << "Shortest path exhausted, " << numPopped << " visited";
}

void ShortestPath::reverse(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, function<vector<Vec2>(Vec2)> directions,
    double mult, Vec2 from, int limit) {
  PROFILE;
  reversed = true;
  function<QueueElem(Vec2)> makeElem = [&](Vec2 pos)->QueueElem { return {pos, distanceTable.getDistance(pos) + lengthFun(pos)};};
  priority_queue<QueueElem, vector<QueueElem>> q;
  for (Vec2 v : bounds) {
    double dist = distanceTable.getDistance(v);
    if (dist <= limit) {
      distanceTable.setDistance(v, mult * dist);
      q.push(makeElem(v));
    }
  }
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top().pos;
    if (from == pos) {
      INFO << "Rev shortest path from " << " from " << target << " " << numPopped << " visited";
      constructPath(pos, directions, true);
      return;
    }
    q.pop();
    for (Vec2 dir : directions(pos))
      if ((pos + dir).inRectangle(bounds)) {
        if (distanceTable.getDistance(pos + dir) > distanceTable.getDistance(pos) + entryFun(pos + dir) &&
            distanceTable.getDistance(pos + dir) < 0) {
          distanceTable.setDistance(pos + dir, distanceTable.getDistance(pos) + entryFun(pos + dir));
          q.push(makeElem(pos + dir));
        }
      }
  }
  INFO << "Rev shortest path from " << " from " << target << " " << numPopped << " visited";
}

void ShortestPath::constructPath(Vec2 pos, function<vector<Vec2>(Vec2)> directions, bool reversed) {
  vector<Vec2> ret;
  auto origPos = pos;
  while (pos != target) {
    Vec2 next;
    double lowest = distanceTable.getDistance(pos);
    CHECK(lowest < infinity);
    for (Vec2 dir : directions(pos)) {
      double dist;
      if ((pos + dir).inRectangle(bounds) && (dist = distanceTable.getDistance(pos + dir)) < lowest) {
        lowest = dist;
        next = pos + dir;
      }
    }
    if (lowest >= distanceTable.getDistance(pos)) {
      if (reversed)
        break;
      else
        FATAL << "can't track path " << lowest << " " << distanceTable.getDistance(pos) << " " << origPos
            << " " << target << " " << pos << " " << next;
    }
    ret.push_back(pos);
    pos = next;
  }
  if (!reversed)
    ret.push_back(target);
  path = ret.reverse();
}

bool ShortestPath::isReversed() const {
  return reversed;
}

const vector<Vec2>& ShortestPath::getPath() const {
  return path;
}

bool ShortestPath::isReachable(Vec2 pos) const {
  return (path.size() >= 2 && path.back() == pos) || (path.size() >= 3 && path[path.size() - 2] == pos);
}

Vec2 ShortestPath::getNextMove(Vec2 pos) {
  CHECK(isReachable(pos));
  if (pos != path.back())
    path.pop_back();
  return path[path.size() - 2];
}

optional<Vec2> ShortestPath::getNextNextMove(Vec2 pos) {
  CHECK(isReachable(pos));
  if (pos != path.back())
    path.pop_back();
  if (path.size() > 2)
    return path[path.size() - 3];
  else
    return path[path.size() - 2];
}

Vec2 ShortestPath::getTarget() const {
  return target;
}

ShortestPath LevelShortestPath::makeShortestPath(Position from, MovementType movementType, Position to, double mult,
    vector<Vec2>* visited) {
  PROFILE;
  Level* level = from.getLevel();
  Rectangle bounds = level->getBounds();
  CHECK(to.isSameLevel(from));
  auto& sectors = level->getSectors(movementType);
  auto& movementSectors = level->getSectors(copyOf(movementType).setCanBuildBridge(false).setDestroyActions({}));
  auto entryFun = [=, &sectors, &movementSectors, fromCoord = from.getCoord()](Vec2 v) {
    PROFILE_BLOCK("entry fun");
    if (visited)
      visited->push_back(v);
    if (fromCoord == v)
      return 1.0;
    if (!sectors.contains(v))
      return ShortestPath::infinity;
    return Position(v, level, Position::IsValid{}).getNavigationCost(movementType, movementSectors);
  };
  auto directionsFun = [=] (Vec2 v) {
    Position pos(v, level);
    vector<Vec2> ret = Vec2::directions8();
    if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
      if (f->hasUsageType(BuiltinUsageId::PORTAL))
        if (auto otherPos = pos.getOtherPortal())
          if (otherPos->isSameLevel(pos))
            if (auto f2 = otherPos->getFurniture(FurnitureLayer::MIDDLE))
              if (f2->hasUsageType(BuiltinUsageId::PORTAL))
                ret.push_back(otherPos->getCoord() - v);
    return ret;
  };
  CHECK(to.getCoord().inRectangle(level->getBounds()));
  CHECK(from.getCoord().inRectangle(level->getBounds()));
  if (mult == 0) {
    auto dist1 = from.getDistanceToNearestPortal().value_or(10000);
    auto lengthFun = [level, from = from.getCoord(), dist1](Vec2 to) {
      PROFILE_BLOCK("length fun");
      auto dist2 = Position(to, level, Position::IsValid{}).getDistanceToNearestPortal().value_or(10000);
      // Use a suboptimal, but faster pathfinding.
      return 2 * min<double>(from.dist8(to) + 0.01 * from.distD(to), dist1 + dist2);
    };
    return ShortestPath(ShortestPath::TemplateConstr{}, bounds, entryFun, lengthFun, directionsFun, to.getCoord(), from.getCoord(), mult);
  } else {
    auto lengthFun = [from = from.getCoord()](Vec2 to)->double { return from.dist8(to); };
    Vec2 vTo = to.getCoord();
    Vec2 vFrom = from.getCoord();
    bounds = bounds.intersection(Rectangle(min(vTo.x, vFrom.x) - margin, min(vTo.y, vFrom.y) - margin,
        max(vTo.x, vFrom.x) + margin, max(vTo.y, vFrom.y) + margin));
    return ShortestPath(ShortestPath::TemplateConstr{}, bounds, entryFun, lengthFun, directionsFun, to.getCoord(), from.getCoord(), mult);
  }
}

SERIALIZE_DEF(LevelShortestPath, path, level)
SERIALIZATION_CONSTRUCTOR_IMPL(LevelShortestPath);


LevelShortestPath::LevelShortestPath(const Creature* creature, Position target, double mult, vector<Vec2>* visited)
    : LevelShortestPath(creature->getPosition(), creature->getMovementType(), target, mult, visited) {}

LevelShortestPath::LevelShortestPath(Position from, MovementType type, Position to, double mult, vector<Vec2>* visited)
    : path(makeShortestPath(from, type, to, mult, visited)), level(to.getLevel()) {
}

Level* LevelShortestPath::getLevel() const {
  return level;
}

vector<Position> LevelShortestPath::getPath() const {
  return path.getPath().transform([this](Vec2 v) { return Position(v, level); });
}

bool LevelShortestPath::isReachable(Position pos) const {
  return pos.getLevel() == level && path.isReachable(pos.getCoord());
}

Position LevelShortestPath::getNextMove(Position pos) {
  CHECK(pos.getLevel() == level);
  return Position(path.getNextMove(pos.getCoord()), level);
}

optional<Position> LevelShortestPath::getNextNextMove(Position pos) {
  CHECK(pos.getLevel() == level);
  if (auto next = path.getNextNextMove(pos.getCoord())) {
    if (next->dist8(pos.getCoord()) > 2)
      return Position(path.getNextMove(pos.getCoord()), level);
    return Position(*next, level);
  } else
    return none;
}

Position LevelShortestPath::getTarget() const {
  return Position(path.getTarget(), level);
}

bool LevelShortestPath::isReversed() const {
  return path.isReversed();
}

Dijkstra::Dijkstra(Rectangle bounds, vector<Vec2> from, int maxDist, function<double(Vec2)> entryFun,
      vector<Vec2> directions) {
  distanceTable.clear();
  auto comparator = [](Vec2 pos1, Vec2 pos2) {
      double diff = distanceTable.getDistance(pos1) - distanceTable.getDistance(pos2);
      if (diff > 0 || (diff == 0 && pos1 < pos2))
        return 1;
      else
        return 0;};
  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  for (auto& v : from) {
    distanceTable.setDistance(v, 0);
    q.push(v);
  }
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top();
    double cdist = distanceTable.getDistance(pos);
    if (cdist > maxDist)
      return;
    q.pop();
    CHECK(!reachable.count(pos));
    reachable[pos] = cdist;
    for (Vec2 dir : directions) {
      Vec2 next = pos + dir;
      if (next.inRectangle(bounds)) {
        double ndist = distanceTable.getDistance(next);
        if (cdist < ndist) {
          double dist = cdist + entryFun(next);
          CHECK(dist > cdist) << "Entry fun non positive " << dist - cdist;
          if (dist < ndist && dist <= maxDist) {
            distanceTable.setDistance(next, dist);
            q.push(next);
          }
        }
      }
    }
  }

}

bool Dijkstra::isReachable(Vec2 pos) const {
  return reachable.count(pos);
}

double Dijkstra::getDist(Vec2 v) const {
  return reachable.at(v);
}

const Dijkstra::DistanceMap& Dijkstra::getAllReachable() const {
  return reachable;
}

BfSearch::BfSearch(Rectangle bounds, Vec2 from, function<bool(Vec2)> entryFun, vector<Vec2> directions) {
  distanceTable.clear();
  queue<Vec2> q;
  distanceTable.setDistance(from, 0);
  q.push(from);
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.front();
    q.pop();
    CHECK(!reachable.count(pos));
    reachable.insert(pos);
    for (Vec2 dir : directions) {
      Vec2 next = pos + dir;
      if (next.inRectangle(bounds) && distanceTable.getDistance(next) == ShortestPath::infinity && entryFun(next)) {
        distanceTable.setDistance(next, 0);
        q.push(next);
      }
    }
  }

}

bool BfSearch::isReachable(Vec2 pos) const {
  return reachable.count(pos);
}

const BfSearch::ReachableSet& BfSearch::getAllReachable() const {
  return reachable;
}

