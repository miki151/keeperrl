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

#ifndef _SHORTEST_PATH_H
#define _SHORTEST_PATH_H

#include <functional>

#include "util.h"
#include "position.h"

class Creature;
class Level;

class ShortestPath {
  public:
  ShortestPath(
      Rectangle area,
      function<double(Vec2)> entryFun,
      function<int(Vec2)> lengthFun,
      vector<Vec2> directions,
      Vec2 target,
      Vec2 from,
      double mult = 0);
  bool isReachable(Vec2 pos) const;
  Vec2 getNextMove(Vec2 pos);
  Vec2 getTarget() const;
  bool isReversed() const;

  static const double infinity;

  SERIALIZATION_DECL(ShortestPath);

  private:
  void init(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, Vec2 target, optional<Vec2> from,
      optional<int> limit = none);
  void reverse(function<double(Vec2)> entryFun, function<double(Vec2)> lengthFun, double mult, Vec2 from, int limit);
  void constructPath(Vec2 start, bool reversed = false);
  vector<Vec2> SERIAL(path);
  Vec2 SERIAL(target);
  vector<Vec2> SERIAL(directions);
  Rectangle SERIAL(bounds);
  bool SERIAL(reversed);
};

class LevelShortestPath {
  public:
  LevelShortestPath(const Creature* creature, Position target, Position from, double mult = 0);
  bool isReachable(Position) const;
  Position getNextMove(Position);
  Position getTarget() const;
  bool isReversed() const;
  Level* getLevel() const;

  static const double infinity;

  SERIALIZATION_DECL(LevelShortestPath);

  private:
  static ShortestPath makeShortestPath(const Creature* creature, Position to, Position from, double mult);
  ShortestPath SERIAL(path);
  Level* SERIAL(level);
};

class Dijkstra {
  public:
  Dijkstra(Rectangle bounds, Vec2 from, int maxDist, function<double(Vec2)> entryFun,
      vector<Vec2> directions = Vec2::directions8());
  bool isReachable(Vec2) const;
  double getDist(Vec2) const;
  const map<Vec2, double>& getAllReachable() const;
  
  private:
  map<Vec2, double> reachable;
};

class BfSearch {
  public:
  BfSearch(Rectangle bounds, Vec2 from, function<bool(Vec2)> entryFun, vector<Vec2> directions = Vec2::directions8());
  bool isReachable(Vec2) const;
  const set<Vec2>& getAllReachable() const;

  private:
  set<Vec2> reachable;
};

#endif
