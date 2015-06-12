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

#include "shortest_path.h"
#include "level.h"
#include "creature.h"
#include "square.h"

template <class Archive> 
void ShortestPath::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(path)
    & SVAR(target)
    & SVAR(directions)
    & SVAR(bounds)
    & SVAR(reversed);
}

SERIALIZABLE(ShortestPath);
SERIALIZATION_CONSTRUCTOR_IMPL(ShortestPath);

const double ShortestPath::infinity = 1000000000;

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

const int margin = 15;

ShortestPath::ShortestPath(const Level* level, const Creature* creature, Vec2 to, Vec2 from, double mult)
    : target(to), directions(Vec2::directions8()), bounds(level->getBounds()) {
  auto entryFun = [=](Vec2 pos) { 
      const Square* target = level->getSafeSquare(pos);
      if (target->canEnter(creature) || creature->getPosition() == pos) 
        return 1.0;
      if (target->canNavigate(creature->getMovementType())) {
        if (const Creature* other = target->getCreature())
          if (other->isFriend(creature))
            return 1.5;
        return 5.0;
      }
      return infinity;};
  CHECK(to.inRectangle(level->getBounds()));
  CHECK(from.inRectangle(level->getBounds()));
  if (mult == 0) {
    // Use a suboptimal, but faster pathfinding.
    init(entryFun, [](Vec2 v)->double { return 2 * v.lengthD(); }, target, from);
  } else {
    auto lengthFun = [](Vec2 v)->double { return v.length8(); };
    bounds = bounds.intersection(Rectangle(min(to.x, from.x) - margin, min(to.y, from.y) - margin,
        max(to.x, from.x) + margin, max(to.y, from.y) + margin));
    init(entryFun, lengthFun, target, none, revShortestLimit);
    distanceTable.setDistance(target, infinity);
    reverse(entryFun, lengthFun, mult, from, revShortestLimit);
  }
}

ShortestPath::ShortestPath(Rectangle a, function<double(Vec2)> entryFun, function<int(Vec2)> lengthFun,
    vector<Vec2> dir, Vec2 to, Vec2 from, double mult) : target(to), directions(dir), bounds(a) {
  CHECK(Level::getMaxBounds().contains(a));
  if (mult == 0)
    init(entryFun, lengthFun, target, from);
  else {
    init(entryFun, lengthFun, target, none, revShortestLimit);
    distanceTable.setDistance(target, infinity);
    reverse(entryFun, lengthFun, mult, from, revShortestLimit);
  }
}

void ShortestPath::init(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, Vec2 target,
    optional<Vec2> from, optional<int> limit) {
  reversed = false;
  distanceTable.clear();
  function<bool(Vec2, Vec2)> comparator;
  if (from)
    comparator = [&](Vec2 pos1, Vec2 pos2) {
      return distanceTable.getDistance(pos1) + lengthFun(*from - pos1) > 
          distanceTable.getDistance(pos2) + lengthFun(*from - pos2); };
  else
    comparator = [this](Vec2 pos1, Vec2 pos2) {
      return distanceTable.getDistance(pos1) > distanceTable.getDistance(pos2); };
  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  distanceTable.setDistance(target, 0);
  q.push(target);
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top();
   // Debug() << "Popping " << pos << " " << distance[pos]  << " " << (from ? (*from - pos).length4() : 0);
    if (from == pos || (limit && distanceTable.getDistance(pos) >= *limit)) {
      Debug() << "Shortest path from " << (from ? *from : Vec2(-1, -1)) << " to " << target << " " << numPopped
        << " visited distance " << distanceTable.getDistance(pos);
      constructPath(pos);
      return;
    }
    q.pop();
    for (Vec2 dir : directions) {
      Vec2 next = pos + dir;
      if (next.inRectangle(bounds)) {
        double cdist = distanceTable.getDistance(pos);
        double ndist = distanceTable.getDistance(next);
        if (cdist < ndist) {
          double dist = cdist + entryFun(next);
          CHECK(dist > cdist) << "Entry fun non positive " << dist - cdist;
          if (dist < ndist) {
            distanceTable.setDistance(next, dist);
            q.push(next);
          }
        }
      }
    }
  }
  Debug() << "Shortest path exhausted, " << numPopped << " visited";
}

void ShortestPath::reverse(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, double mult, Vec2 from,
    int limit) {
  reversed = true;
  function<bool(Vec2, Vec2)> comparator = [=](Vec2 pos1, Vec2 pos2) {
    return distanceTable.getDistance(pos1) + lengthFun(from - pos1) 
      > distanceTable.getDistance(pos2) + lengthFun(from - pos2); };

  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  for (Vec2 v : bounds) {
    double dist = distanceTable.getDistance(v);
    if (dist <= limit) {
      distanceTable.setDistance(v, mult * dist);
      q.push(v);
    }
  }
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top();
    if (from == pos) {
      Debug() << "Rev shortest path from " << " from " << target << " " << numPopped << " visited";
      constructPath(pos, true);
      return;
    }
    q.pop();
    for (Vec2 dir : directions)
      if ((pos + dir).inRectangle(bounds)) {
        if (distanceTable.getDistance(pos + dir) > distanceTable.getDistance(pos) + entryFun(pos + dir) && 
            distanceTable.getDistance(pos + dir) < 0) {
          distanceTable.setDistance(pos + dir, distanceTable.getDistance(pos) + entryFun(pos + dir));
          q.push(pos + dir);
        }
      }
  }
  Debug() << "Rev shortest path from " << " from " << target << " " << numPopped << " visited";
}

void ShortestPath::constructPath(Vec2 pos, bool reversed) {
  vector<Vec2> ret;
  while (pos != target) {
    Vec2 next;
    double lowest = distanceTable.getDistance(pos);
    CHECK(lowest < infinity);
    for (Vec2 dir : directions) {
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
        FAIL << "can't track path";
    }
    ret.push_back(pos);
    pos = next;
  }
  if (!reversed)
    ret.push_back(target);
  path = vector<Vec2>(ret.rbegin(), ret.rend());
}

bool ShortestPath::isReversed() const {
  return reversed;
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

Vec2 ShortestPath::getTarget() const {
  return target;
}

Dijkstra::Dijkstra(Rectangle bounds, Vec2 from, int maxDist, function<double(Vec2)> entryFun,
      vector<Vec2> directions) {
  distanceTable.clear();
  function<bool(Vec2, Vec2)> comparator = [&] (Vec2 pos1, Vec2 pos2) {
    return distanceTable.getDistance(pos1) > distanceTable.getDistance(pos2); };
  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  distanceTable.setDistance(from, 0);
  q.push(from);
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top();
    double cdist = distanceTable.getDistance(pos);
    if (cdist > maxDist)
      return;
    q.pop();
    reachable[pos] = cdist;
    for (Vec2 dir : directions) {
      Vec2 next = pos + dir;
      if (next.inRectangle(bounds)) {
        double ndist = distanceTable.getDistance(next);
        if (cdist < ndist) {
          double dist = cdist + entryFun(next);
          CHECK(dist > cdist) << "Entry fun non positive " << dist - cdist;
          if (dist < ndist) {
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

const map<Vec2, double>& Dijkstra::getAllReachable() const {
  return reachable;
}

