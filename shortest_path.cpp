#include "stdafx.h"

using namespace std;

const double ShortestPath::infinity = 1000000000;

const int revShortestLimit = 15;

static Table<double> initDistTable(Vec2 to, Vec2 from, int levelWidth, int levelHeight) {
  int margin = 20;
  Vec2 p(max(0, min(to.x, from.x) - margin),
         max(0, min(to.y, from.y) - margin));
  Vec2 k(min(levelWidth - p.x, abs(to.x - from.x) + 2 * margin),
         min(levelHeight - p.y, abs(to.y - from.y) + 2 * margin));
  return Table<double>(Rectangle(p, p + k), ShortestPath::infinity);
}

ShortestPath::ShortestPath(const Level* level, const Creature* creature, Vec2 to, Vec2 from, double mult,
    bool avoidEnemies) : distance(initDistTable(to, from, level->getWidth(), level->getHeight())), 
    target(to), 
    directions(Vec2::directions8()) {
  auto entryFun = [=](Vec2 pos) { 
      if (level->getSquare(pos)->canEnter(creature) || creature->getPosition() == pos) 
        return 1.0;
      if ((level->getSquare(pos)->canEnterEmpty(creature) || level->getSquare(pos)->canDestroy())
          && (!avoidEnemies || !level->getSquare(pos)->getCreature() 
              || !level->getSquare(pos)->getCreature()->isEnemy(creature)))
        return 5.0;
      return infinity;};
  auto lengthFun = [](Vec2 v) { return v.length8(); };
  if (mult == 0)
    init(entryFun, lengthFun, target, from);
  else {
    init(entryFun, lengthFun, target, Nothing(), revShortestLimit);
    setDistance(target, infinity);
    reverse(entryFun, lengthFun, mult, from, revShortestLimit);
  }
}

ShortestPath::ShortestPath(Rectangle a, function<double(Vec2)> entryFun, function<int(Vec2)> lengthFun,
    vector<Vec2> dir, Vec2 to, Optional<Vec2> from, double mult)
    : distance(a.getPX(), a.getPY(), a.getW(), a.getH(), infinity), target(to), directions(dir) {
  if (mult == 0)
    init(entryFun, lengthFun, target, from);
  else {
    init(entryFun, lengthFun, target, Nothing(), revShortestLimit);
    setDistance(target, infinity);
    reverse(entryFun, lengthFun, mult, from, revShortestLimit);
  }
}

double ShortestPath::getDistance(Vec2 v) const {
  return distance[v];
}
void ShortestPath::setDistance(Vec2 v, double d) {
  distance[v] = d;
}

void ShortestPath::init(function<double(Vec2)> entryFun, function<int(Vec2)> lengthFun, Vec2 target, Optional<Vec2> from, Optional<int> limit) {
  reversed = false;
  function<bool(Vec2, Vec2)> comparator;
  if (from)
    comparator = [=](Vec2 pos1, Vec2 pos2) { return this->getDistance(pos1) + lengthFun(*from - pos1) > this->getDistance(pos2) + lengthFun(*from - pos2); };
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
      Debug() << "Shortest path from " << (from ? *from : Vec2(-1,-1)) << " to " << target << " " << numPopped << " visited";
      return;
    }
    q.pop();
    for (Vec2 dir : directions) {
      Vec2 next = pos + dir;
      if (next.inRectangle(distance.getBounds())) {
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
//  if (from)
  Debug() << "Shortest path exhausted, " << numPopped << " visited";
}

void ShortestPath::reverse(function<double(Vec2)> entryFun, function<int(Vec2)> lengthFun, double mult, Optional<Vec2> from, int limit) {
  reversed = true;
  function<bool(Vec2, Vec2)> comparator;
  if (from)
    comparator = [=](Vec2 pos1, Vec2 pos2) { return this->getDistance(pos1) + lengthFun(*from - pos1) > this->getDistance(pos2) + lengthFun(*from - pos2); };
  else
    comparator = [this](Vec2 pos1, Vec2 pos2) { return this->getDistance(pos1) > this->getDistance(pos2); }; 

  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator) ;
  for (Vec2 v : distance.getBounds()) {
    double dist = distance[v];
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
      return;
    }
    q.pop();
    for (Vec2 dir : directions)
      if ((pos + dir).inRectangle(distance.getBounds())) {
        if (getDistance(pos + dir) > distance[pos] + entryFun(pos + dir) && distance[pos + dir] < 0) {
          distance[pos + dir] = distance[pos] + entryFun(pos + dir);
          q.push(pos + dir);
        }
      }
  }
  Debug() << "Rev shortest path from " << " from " << target << " " << numPopped << " visited";
}

bool ShortestPath::isReversed() const {
  return reversed;
}

bool ShortestPath::isReachable(Vec2 pos) const {
  if (!pos.inRectangle(distance.getBounds()))
    return false;
  if (getDistance(pos) == infinity)
    return false;
  for (Vec2 dir : directions)
    if ((pos + dir).inRectangle(distance.getBounds()) && getDistance(pos + dir) < getDistance(pos))
      return true;
  return false;
}

Vec2 ShortestPath::getNextMove(Vec2 pos) const {
  CHECK(isReachable(pos));
  CHECK(pos != target) << "Already arrived at target.";
  double lowest = getDistance(pos);
  Vec2 next;
  for (Vec2 dir : directions) {
    double dist;
    if ((pos + dir).inRectangle(distance.getBounds()) && (dist = getDistance(pos + dir)) < lowest) {
      lowest = dist;
      next = pos + dir;
    }
  }
  CHECK(lowest < getDistance(pos)) << "Unable to continue shortest path.";
  return next;
}

Vec2 ShortestPath::getTarget() const {
  return target;
}
