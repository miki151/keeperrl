#ifndef _SHORTEST_PATH_H
#define _SHORTEST_PATH_H

#include <functional>

#include "util.h"

class Creature;
class Level;

class ShortestPath {
  public:
  ShortestPath(const Level* level, const Creature* creature, Vec2 target, Vec2 from, double mult = 0,
      bool avoidEnemies = false);
  ShortestPath(
      Rectangle area,
      function<double(Vec2)> entryFun,
      function<int(Vec2)> lengthFun,
      vector<Vec2> directions,
      Vec2 target,
      Vec2 from,
      double mult = 0);
  ShortestPath(ShortestPath&&) = default;
  ShortestPath& operator=(ShortestPath&&) = default;
  bool isReachable(Vec2 pos) const;
  Vec2 getNextMove(Vec2 pos);
  Vec2 getTarget() const;
  bool isReversed() const;

  static const double infinity;

  private:
  void init(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, Vec2 target, Optional<Vec2> from,
      Optional<int> limit = Nothing());
  void reverse(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, double mult, Vec2 from, int limit);
  void setDistance(Vec2, double);
  double getDistance(Vec2 pos) const;
  void constructPath(Vec2 start, bool reversed = false);
  vector<Vec2> path;
  Vec2 target;
  vector<Vec2> directions;
  Rectangle bounds;
  bool reversed;
};

#endif
