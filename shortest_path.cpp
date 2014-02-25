#include "stdafx.h"

#include "shortest_path.h"
#include "level.h"
#include "creature.h"



const double ShortestPath::infinity = 1000000000;

const int revShortestLimit = 15;

const int maxSize = 600;

Table<double> ddist(maxSize, maxSize);
Table<int> dirty(maxSize, maxSize, 0);
int counter = 1;

int margin = 15;

ShortestPath::ShortestPath(const Level* level, const Creature* creature, Vec2 to, Vec2 from, double mult,
    bool avoidEnemies) : target(to), directions(Vec2::directions8()), bounds(level->getBounds()) {
  CHECK(level->getBounds().getKX() <= maxSize && level->getBounds().getKY() <= maxSize);
  auto entryFun = [=](Vec2 pos) { 
      if (level->getSquare(pos)->canEnter(creature) || creature->getPosition() == pos) 
        return 1.0;
      if ((level->getSquare(pos)->canEnterEmpty(creature) || level->getSquare(pos)->canDestroy())
          && (!avoidEnemies || !level->getSquare(pos)->getCreature() 
              || !level->getSquare(pos)->getCreature()->isEnemy(creature)))
        return 5.0;
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
    init(entryFun, lengthFun, target, Nothing(), revShortestLimit);
    setDistance(target, infinity);
    reverse(entryFun, lengthFun, mult, from, revShortestLimit);
  }
}

ShortestPath::ShortestPath(Rectangle a, function<double(Vec2)> entryFun, function<int(Vec2)> lengthFun,
    vector<Vec2> dir, Vec2 to, Vec2 from, double mult) : target(to), directions(dir), bounds(a) {
  CHECK(a.getKX() <= maxSize && a.getKY() <= maxSize && a.getPX() >= 0 && a.getPY() >= 0);
  if (mult == 0)
    init(entryFun, lengthFun, target, from);
  else {
    init(entryFun, lengthFun, target, Nothing(), revShortestLimit);
    setDistance(target, infinity);
    reverse(entryFun, lengthFun, mult, from, revShortestLimit);
  }
}

double ShortestPath::getDistance(Vec2 v) const {
  return dirty[v] < counter ? infinity : ddist[v];
}
void ShortestPath::setDistance(Vec2 v, double d) {
  ddist[v] = d;
  dirty[v] = counter;
}

void ShortestPath::init(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, Vec2 target,
    Optional<Vec2> from, Optional<int> limit) {
  reversed = false;
  ++counter;
  function<bool(Vec2, Vec2)> comparator;
  if (from)
    comparator = [=](Vec2 pos1, Vec2 pos2) {
      return this->getDistance(pos1) + lengthFun(*from - pos1) > this->getDistance(pos2) + lengthFun(*from - pos2); };
  else
    comparator = [this](Vec2 pos1, Vec2 pos2) { return this->getDistance(pos1) > this->getDistance(pos2); };
  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  setDistance(target, 0);
  q.push(target);
  int numPopped = 0;
  while (!q.empty()) {
    ++numPopped;
    Vec2 pos = q.top();
   // Debug() << "Popping " << pos << " " << distance[pos]  << " " << (from ? (*from - pos).length4() : 0);
    if (from == pos || (limit && getDistance(pos) >= *limit)) {
      Debug() << "Shortest path from " << (from ? *from : Vec2(-1, -1)) << " to " << target << " " << numPopped << " visited distance " << getDistance(pos);
      constructPath(pos);
      return;
    }
    q.pop();
    for (Vec2 dir : directions) {
      Vec2 next = pos + dir;
      if (next.inRectangle(bounds)) {
        double cdist = getDistance(pos);
        double ndist = getDistance(next);
        if (cdist < ndist) {
          double dist = cdist + entryFun(next);
          CHECK(dist > cdist) << "Entry fun non positive " << dist - cdist;
          if (dist < ndist) {
            setDistance(next, dist);
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
    return this->getDistance(pos1) + lengthFun(from - pos1) > this->getDistance(pos2) + lengthFun(from - pos2); };

  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  for (Vec2 v : bounds) {
    double dist = getDistance(v);
    if (dist <= limit) {
      setDistance(v, mult * dist);
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
        if (getDistance(pos + dir) > getDistance(pos) + entryFun(pos + dir) && getDistance(pos + dir) < 0) {
          setDistance(pos + dir, getDistance(pos) + entryFun(pos + dir));
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
    double lowest = getDistance(pos);
    CHECK(lowest < infinity);
    for (Vec2 dir : directions) {
      double dist;
      if ((pos + dir).inRectangle(bounds) && (dist = getDistance(pos + dir)) < lowest) {
        lowest = dist;
        next = pos + dir;
      }
    }
    if (lowest >= getDistance(pos)) {
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
