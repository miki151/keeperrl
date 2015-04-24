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

#include "level_maker.h"
#include "pantheon.h"
#include "item_factory.h"
#include "square.h"
#include "collective_builder.h"
#include "collective.h"
#include "shortest_path.h"
#include "creature.h"

namespace {

void failGen() {
  throw LevelGenException();
}

void checkGen(bool b) {
  if (!b)
    failGen();
}

class Predicate {
  public:
  bool apply(Level::Builder* builder, Vec2 pos) const {
    return predFun(builder, pos);
  }

  Vec2 getRandomPosition(Level::Builder* builder, Rectangle area) {
    vector<Vec2> good;
    for (Vec2 v : area)
      if (apply(builder, v))
        good.push_back(v);
    if (good.empty())
      failGen();
    return chooseRandom(good);
  }

  static Predicate attrib(SquareAttrib attr) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return builder->hasAttrib(pos, attr);});
  }

  static Predicate negate(Predicate p) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return !p.apply(builder, pos);});
  }

  static Predicate type(SquareType t) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return builder->getType(pos) == t;});
  }

  static Predicate type(vector<SquareType> t) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return contains(t, builder->getType(pos));});
  }

  static Predicate alwaysTrue() {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return true;});
  }

  static Predicate alwaysFalse() {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return false;});
  }

  static Predicate andPred(Predicate p1, Predicate p2) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) {
        return p1.apply(builder, pos) && p2.apply(builder, pos);});
  }

  static Predicate orPred(Predicate p1, Predicate p2) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) {
        return p1.apply(builder, pos) || p2.apply(builder, pos);});
  }

  static Predicate canEnter(MovementType m) {
    return Predicate([=] (Level::Builder* builder, Vec2 pos) { return builder->getSquare(pos)->canEnter(m);});
  }

  private:
  typedef function<bool(Level::Builder*, Vec2)> PredFun;
  Predicate(PredFun fun) : predFun(fun) {}
  PredFun predFun;
};

class Empty : public LevelMaker {
  public:
  Empty(SquareType s, optional<SquareAttrib> attr = none) : square(s), attrib(attr) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area) {
      builder->putSquare(v, square, attrib);
    }
  }

  private:
  SquareType square;
  optional<SquareAttrib> attrib;
};

class RoomMaker : public LevelMaker {
  public:
  RoomMaker(int _numRooms,
            int _minSize, int _maxSize, 
            SquareType _wallType,
            optional<SquareType> _onType,
            LevelMaker* _roomContents = new Empty(SquareId::FLOOR),
            vector<LevelMaker*> _insideMakers = {},
            bool _diggableCorners = false) : 
      numRooms(_numRooms),
      minSize(_minSize),
      maxSize(_maxSize),
      wallType(_wallType),
      squareType(_onType),
      roomContents(_roomContents),
      insideMakers(_insideMakers),
      diggableCorners(_diggableCorners) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int spaceBetween = 0;
    Table<int> taken(area.getKX(), area.getKY());
    for (Vec2 v : area)
      taken[v] = squareType && *squareType != builder->getType(v);
    for (int i : Range(numRooms)) {
      Vec2 p, k;
      bool good;
      int cnt = 100;
      do {
        k = Vec2(Random.get(minSize, maxSize), Random.get(minSize, maxSize));
        p = Vec2(area.getPX() + spaceBetween + Random.get(area.getW() - k.x - 2 * spaceBetween),
                 area.getPY() + spaceBetween + Random.get(area.getH() - k.y - 2 * spaceBetween));
        good = true;
        for (Vec2 v : Rectangle(k.x + 2 * spaceBetween, k.y + 2 * spaceBetween))
          if (taken[p + v - Vec2(spaceBetween,spaceBetween)]) {
            good = false;
            break;
          }
      } while (!good && --cnt > 0);
      if (cnt == 0) {
        Debug() << "Placed only " << i << " rooms out of " << numRooms;
        break;
      }
      for (Vec2 v : Rectangle(k))
        taken[v + p] = 1;
      for (Vec2 v : Rectangle(k - Vec2(2, 2)))
        builder->putSquare(p + v + Vec2(1, 1), SquareId::FLOOR, SquareAttrib::ROOM);
      for (int i : Range(p.x, p.x + k.x)) {
        if (i == p.x || i == p.x + k.x - 1) {
          builder->putSquare(Vec2(i, p.y), wallType,
              !diggableCorners ? optional<SquareAttrib>(SquareAttrib::NO_DIG) : none);
          builder->putSquare(Vec2(i, p.y + k.y - 1), wallType,
              !diggableCorners ? optional<SquareAttrib>(SquareAttrib::NO_DIG) : none);
        } else {
          builder->putSquare(Vec2(i, p.y), wallType);
          builder->putSquare(Vec2(i, p.y + k.y - 1), wallType);
        }
      }
      for (int i : Range(p.y, p.y + k.y)) {
        if (i > p.y && i < p.y + k.y - 1) {
          builder->putSquare(Vec2(p.x, i), wallType);
          builder->putSquare(Vec2(p.x + k.x - 1, i), wallType);
        }
      }
      Rectangle inside(p.x + 1, p.y + 1, p.x + k.x - 1, p.y + k.y - 1);
      roomContents->make(builder, inside);
      if (i < insideMakers.size())
        insideMakers[i]->make(builder, inside);
      else
        for (Vec2 v : inside)
          builder->addAttrib(v, SquareAttrib::EMPTY_ROOM);
    }
  }

  private:
  int numRooms;
  int minSize;
  int maxSize;
  SquareType wallType;
  optional<SquareType> squareType;
  LevelMaker* roomContents;
  vector<LevelMaker*> insideMakers;
  bool diggableCorners;
};

class Connector : public LevelMaker {
  public:
  Connector(double _floorProb, double _doorProb, double _diggingCost = 3,
        Predicate pred = Predicate::canEnter({MovementTrait::WALK}))
      : floorProb(_floorProb), doorProb(_doorProb), diggingCost(_diggingCost), connectPred(pred) {
  }
  double getValue(Level::Builder* builder, Vec2 pos, Rectangle area) {
    if (builder->getSquare(pos)->canEnter({MovementTrait::WALK}))
      return 1;
    if (builder->hasAttrib(pos, SquareAttrib::NO_DIG))
      return ShortestPath::infinity;
    if (builder->hasAttrib(pos, SquareAttrib::LAKE))
      return 15;
    if (builder->hasAttrib(pos, SquareAttrib::RIVER))
      return 5;
    int numCorners = 0;
    int numTotal = 0;
    for (Vec2 v : Vec2::directions8())
      if ((pos + v).inRectangle(area) && builder->getSquare(pos + v)->canEnter({MovementTrait::WALK})) {
        if (abs(v.x) == abs(v.y))
          ++numCorners;
        ++numTotal;
      }
    if (numCorners == 1)
      return 1000;
    if (numTotal - numCorners > 1)
      return diggingCost + 5;
    return diggingCost;
  }

  void connect(Level::Builder* builder, Vec2 p1, Vec2 p2, Rectangle area) {
    ShortestPath path(area,
        [builder, this, &area](Vec2 pos) { return getValue(builder, pos, area); }, 
        [] (Vec2 v) { return v.length4(); },
        Vec2::directions4(true), p1 ,p2);
    Vec2 prev(-100, -100);
    for (Vec2 v = p2; v != p1; v = path.getNextMove(v)) {
      if (!builder->getSquare(v)->canEnter({MovementTrait::WALK})) {
        SquareType newType;
        SquareType oldType = builder->getType(v);
        if (isWall(oldType) && oldType != SquareId::BLACK_WALL)
          newType = chooseRandom<SquareType>(
              {SquareId::FLOOR, SquareId::DOOR},
              {floorProb, doorProb});
        else
          switch (oldType.getId()) {
            case SquareId::ABYSS:
            case SquareId::WATER:
            case SquareId::MAGMA:
              newType = SquareId::BRIDGE;
              break;
            case SquareId::MOUNTAIN2:
            case SquareId::BLACK_WALL:
              newType = SquareId::FLOOR;
              break;
            default:
              FAIL << "Unhandled square type " << (int)builder->getType(v).getId();
          }
        if (newType == SquareId::DOOR)
          builder->putSquare(v, SquareId::FLOOR);
        builder->putSquare(v, newType);
      }
      if (builder->getType(v) == SquareId::FLOOR || 
          builder->getType(v) == SquareId::BRIDGE || 
          builder->getType(v) == SquareId::DOOR) {
        if (!path.isReachable(v))
          failGen();
        builder->getSquare(v)->addTravelDir(path.getNextMove(v) - v);
        if (prev.x > -100)
          builder->getSquare(v)->addTravelDir(prev - v);
      }
      prev = v;
    }
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Vec2 p1, p2;
    vector<Vec2> points = filter(area.getAllSquares(), [&] (Vec2 v) { return connectPred.apply(builder, v);});
    if (points.size() < 2)
      return;
    for (int __attribute__((unused)) i : Range(30)) {
      p1 = chooseRandom(points);
      p2 = chooseRandom(points);
      if (p1 != p2)
        connect(builder, p1, p2, area);
    }
    auto dijkstraFun = [&] (Vec2 pos) {
      if (builder->getSquare(pos)->canEnterEmpty({MovementTrait::WALK}))
        return 1.;
      else
        return ShortestPath::infinity;};
    Table<bool> connected(area, false);
    while (1) {
      Dijkstra dijkstra(area, p1, 10000, dijkstraFun);
      for (Vec2 v : area)
        if (dijkstra.isReachable(v))
          connected[v] = true;
      bool found = false;
      for (Vec2 v : area)
        if (connectPred.apply(builder, v) && !connected[v]) {
          connect(builder, p1, v, area);
          p1 = v;
          found = true;
          break;
         }
      if (!found)
        break;
    }
  }
  
  private:
  double floorProb;
  double doorProb;
  double diggingCost;
  Predicate connectPred;
};

struct FeatureInfo {
  SquareType type;
  int min;
  int max;
};

class DungeonFeatures : public LevelMaker {
  public:
  DungeonFeatures(Predicate pred, vector<FeatureInfo> _squareTypes,
      optional<SquareAttrib> setAttr = none): 
      squareTypes(_squareTypes), predicate(pred), attr(setAttr) {
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> available;
    for (Vec2 v : area)
      if (predicate.apply(builder, v))
        available.push_back(v);
    int maxTotal = 0;
    for (auto iter : squareTypes) {
      maxTotal += iter.max - 1;
    }
    checkGen(maxTotal <= available.size());
    for (auto iter : squareTypes) {
      int num = Random.get(iter.min, iter.max);
      for (int __attribute__((unused)) i : Range(num)) {
        int vInd = Random.get(available.size());
        builder->putSquare(available[vInd], iter.type);
        if (attr)
          builder->addAttrib(available[vInd], *attr);
        removeIndex(available, vInd);
      }
    }
  }

  private:
  vector<FeatureInfo> squareTypes;
  Predicate predicate;
  optional<SquareAttrib> attr;
};

class Creatures : public LevelMaker {
  public:
  Creatures(CreatureFactory cf, int numC, MonsterAIFactory actorF, optional<SquareType> type = none) :
      cfactory(cf), numCreature(numC), actorFactory(actorF), squareType(type) {}

  Creatures(CreatureFactory cf, int numC, CollectiveBuilder* col, optional<SquareType> type = none) :
      cfactory(cf), numCreature(numC), actorFactory(MonsterAIFactory::monster()), squareType(type),
      collective(col) {}

  Creatures(CreatureFactory cf, int numC, optional<SquareType> type = none) :
      cfactory(cf), numCreature(numC), squareType(type) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    if (!actorFactory) {
      Location* loc = new Location();
      builder->addLocation(loc, area);
      actorFactory = MonsterAIFactory::stayInLocation(loc);
    }
    Table<char> taken(area.getKX(), area.getKY());
    for (int __attribute__((unused)) i : Range(numCreature)) {
      PCreature creature = cfactory.random(*actorFactory);
      Vec2 pos;
      int numTries = 100;
      do {
        pos = Vec2(Random.get(area.getPX(), area.getKX()), Random.get(area.getPY(), area.getKY()));
      } while (--numTries > 0 && (!builder->canPutCreature(pos, creature.get())
          || (squareType && builder->getType(pos) != *squareType)));
      checkGen(numTries > 0);
      if (collective) {
        collective->addCreature(creature.get(),
            { creature->isInnocent() ? MinionTrait::WORKER : MinionTrait::FIGHTER });
        builder->addCollective(collective);
      }
      builder->putCreature(pos, std::move(creature));
      taken[pos] = 1;
    }
  }

  private:
  CreatureFactory cfactory;
  int numCreature;
  optional<MonsterAIFactory> actorFactory;
  optional<SquareType> squareType;
  CollectiveBuilder* collective = nullptr;
};

class Items : public LevelMaker {
  public:
  Items(ItemFactory _factory, SquareType _onType, int minc, int maxc) : 
      factory(_factory), onType(_onType), minItem(minc), maxItem(maxc) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int numItem = Random.get(minItem, maxItem);
    for (int __attribute__((unused)) i : Range(numItem)) {
      Vec2 pos;
      do {
        pos = Vec2(Random.get(area.getPX(), area.getKX()), Random.get(area.getPY(), area.getKY()));
      } while (builder->getType(pos) != onType);
      builder->putItems(pos, factory.random());
    }
  }

  private:
  ItemFactory factory;
  SquareType onType;
  int minItem;
  int maxItem;
};

class River : public LevelMaker {
  public:
  River(int _width, SquareType _squareType) : width(_width), squareType(_squareType){}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int wind = 5;
    int middle = (area.getPX() + area.getKX()) / 2;
    int px = Random.get(middle - wind, middle + width);
    int kx = px + Random.get(-wind, wind); // Random.get(area.getPX(), area.getKX()) - width;
    if (kx < 0)
      kx = 0;
    if (kx >= area.getKX() - width)
      kx = area.getKX() - width - 1;
    int tot = 5;
    for (int h : Range(tot)) {
      int height = area.getPY() * (tot - h) / tot + area.getKY() * h / tot;
      int height2 = area.getPY() * (tot - h - 1) / tot + area.getKY() * (h + 1) / tot;
      vector<Vec2> line = straightLine(px, height, kx, (h == tot - 1) ? area.getKY() : height2);
      for (Vec2 v : line)
        for (int i : Range(width))
          builder->putSquare(v + Vec2(i, 0), squareType, SquareAttrib::RIVER);
      px = kx;
      kx = px + Random.get(-wind, wind);
      if (kx < 0)
        kx = 0;
      if (kx >= area.getKX() - width)
        kx = area.getKX() - width - 1;
    }
  }

  private:

  vector<Vec2> straightLine(int x0, int y0, int x1, int y1){
    Debug() << "Line " << x1 << " " << y0 << " " << x1 << " " << y1;
    int dx = x1 - x0;
    int dy = y1 - y0;
    vector<Vec2> ret{ Vec2(x0, y0)};
    if (abs(dx) > abs(dy)) {          // slope < 1
      double m = (double) dy / (double) dx;      // compute slope
      double b = y0 - m*x0;
      dx = (dx < 0) ? -1 : 1;
      while (x0+dx != x1) {
        x0 += dx;
        ret.push_back(Vec2(x0,(int)roundf(m*x0+b)));
      }
    } else
      if (dy != 0) {                              // slope >= 1
        double m = (double) dx / (double) dy;      // compute slope
        double b = x0 - m*y0;
        dy = (dy < 0) ? -1 : 1;
        while (y0+dy != y1) {
          y0 += dy;
          ret.push_back(Vec2((int)round(m*y0+b),y0));
        }
      }
    return ret;
  }

  int width;
  SquareType squareType;
};


class RepeatMaker : public LevelMaker {
  public:
  RepeatMaker(int num, LevelMaker* m) : number(num), maker(m) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (int __attribute__((unused)) i : Range(number))
      maker->make(builder, area);
  }

  private:
  int number;
  LevelMaker* maker;
};


class MountainRiver : public LevelMaker {
  public:
  MountainRiver(int num, SquareType waterType, SquareType sandType, Predicate startPred)
    : number(num), water(waterType), sand(sandType), startPredicate(startPred) {}

  optional<Vec2> fillLake(Level::Builder* builder, set<Vec2>& waterTiles, Rectangle area, Vec2 pos) {
    vector<Vec2> ret;
    double height = builder->getHeightMap(pos);
    queue<Vec2> q;
    set<Vec2> visited {pos};
    map<Vec2, Vec2> predecessor {{ pos, pos}};
    q.push(pos);
    while (!q.empty()) {
      pos = q.front();
      q.pop();
      for (Vec2 v : pos.neighbors8(true))
        if (v.inRectangle(area) && !visited.count(v)) {
          visited.insert(v);
          predecessor[v] = pos;
          if (fabs(builder->getHeightMap(v) - height) < 0.000001)
            q.push(v);
          else
          if (builder->getHeightMap(v) < height)
            ret.push_back(v);
        }
    }
    if (Random.roll(predecessor.size()) || ret.empty()) {
      for (auto& elem : predecessor)
        if (!contains(ret, elem.first))
          waterTiles.insert(elem.first);
      if (ret.empty())
        return none;
      else
        return chooseRandom(ret);
    } else {
      Vec2 end = chooseRandom(ret);
      for (Vec2 v = predecessor.at(end);; v = predecessor.at(v)) {
        waterTiles.insert(v);
        if (v == predecessor.at(v))
          break;
      }
      return end;
    }
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    set<Vec2> allWaterTiles;
    for (int __attribute__((unused)) i : Range(number)) {
      set<Vec2> waterTiles;
      Vec2 pos = startPredicate.getRandomPosition(builder, area);
      int width = Random.get(3, 6);
      while (1) {
        if (builder->getType(pos) == water)
          break;
        if (auto next = fillLake(builder, waterTiles, area, pos))
          pos = *next;
        else
          break;
      }
      for (Vec2 v : waterTiles)
        for (Vec2 v2 : Rectangle(v, v + Vec2(width, width)))
          allWaterTiles.insert(v2);
    }
    double depth = 0;
    for (set<Vec2> layer : Vec2::calculateLayers(allWaterTiles)) {
      for (Vec2 v : layer)
        if (v.inRectangle(area)) {
          if (contains({SquareId::WATER, SquareId::SAND}, builder->getType(v).getId()))
            continue;
          if (builder->getType(v) == SquareId::GRASS) {
            if (depth == 0)
              builder->putSquare(v, SquareId::SAND, SquareAttrib::RIVER);
            else
              builder->putSquare(v, SquareFactory::getWater(depth), SquareId::WATER, SquareAttrib::RIVER);
          } else
            builder->putSquare(v, SquareId::WATER, SquareAttrib::RIVER);

        }
      depth += 0.6;
    }
  }

  private:
  int number;
  SquareType water;
  SquareType sand;
  Predicate startPredicate;
};

class Blob : public LevelMaker {
  public:
  Blob(double _insideRatio = 0.333) : insideRatio(_insideRatio) {}

  virtual void addSquare(Level::Builder* builder, Vec2 pos, int edgeDist) = 0;

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> squares;
    Table<char> isInside(area, 0);
    Vec2 center((area.getKX() + area.getPX()) / 2, (area.getKY() + area.getPY()) / 2);
    squares.push_back(center);
    isInside[center] = 1;
    int maxSquares = area.getW() * area.getH() * insideRatio;
    int numSquares = 0;
    bool done = false;
    while (!done) {
      Vec2 pos = squares[Random.get(squares.size())];
      for (Vec2 next : pos.neighbors4(true)) {
        if (next.inRectangle(area.minusMargin(1)) && !isInside[next]) {
          Vec2 proj = next - center;
          proj.y *= area.getW();
          proj.y /= area.getH();
          if (Random.getDouble() <= 1. - proj.lengthD() / (area.getW() / 2)) {
            isInside[next] = 1;
            squares.push_back(next);
            if (++numSquares >= maxSquares)
              done = true;
          }
          break;
        }
      }
    }
    queue<Vec2> q;
    int inf = 10000;
    Table<int> distance(area, inf);
    for (Vec2 v : isInside.getBounds())
      if (!isInside[v]) {
        distance[v] = 0;
        q.push(v);
      }
    while (!q.empty()) {
      Vec2 pos = q.front();
      q.pop();
      for (Vec2 v : pos.neighbors8())
        if (v.inRectangle(area) && distance[v] == inf) {
          distance[v] = distance[pos] + 1;
          q.push(v);
          addSquare(builder, v, distance[v]);
        }
    }
  }

  private:
  double insideRatio;
};

class UniformBlob : public Blob {
  public:
  UniformBlob(SquareType insideSquare, optional<SquareType> borderSquare = none,
      optional<SquareAttrib> _attrib = none, double insideRatio = 0.3333) : Blob(insideRatio),
      inside(insideSquare), border(borderSquare), attrib(_attrib) {}

  virtual void addSquare(Level::Builder* builder, Vec2 pos, int edgeDist) override {
    if (edgeDist == 1 && border)
      builder->putSquare(pos, *border, attrib);
    else
      builder->putSquare(pos, inside, attrib);
  }

  private:
  SquareType inside;
  optional<SquareType> border;
  optional<SquareAttrib> attrib;
};

class Lake : public Blob {
  public:
  virtual void addSquare(Level::Builder* builder, Vec2 pos, int edgeDist) override {
    if (edgeDist == 1)
      builder->putSquare(pos, SquareId::SAND, SquareAttrib::LAKE);
    else
      builder->putSquare(pos, SquareFactory::getWater(double(edgeDist) / 2), SquareId::WATER, SquareAttrib::LAKE);
  }
};

struct BuildingInfo {
  SquareType wall;
  SquareType floorInside;
  SquareType floorOutside;
  SquareType door;
};

BuildingInfo woodBuilding = CONSTRUCT_GLOBAL(BuildingInfo,
  c.wall = SquareId::WOOD_WALL;
  c.floorInside = SquareId::FLOOR;
  c.floorOutside = SquareId::GRASS;
  c.door = SquareId::DOOR;
);

BuildingInfo woodCastleBuilding = CONSTRUCT_GLOBAL(BuildingInfo,
  c.wall = SquareId::WOOD_WALL;
  c.floorInside = SquareId::FLOOR;
  c.floorOutside = SquareId::MUD;
  c.door = SquareId::DOOR;
);

BuildingInfo mudBuilding = CONSTRUCT_GLOBAL(BuildingInfo,
  c.wall = SquareId::MUD_WALL;
  c.floorInside = SquareId::MUD;
  c.floorOutside = SquareId::MUD;
  c.door = SquareId::MUD;
);

BuildingInfo brickBuilding = CONSTRUCT_GLOBAL(BuildingInfo,
  c.wall = SquareId::CASTLE_WALL;
  c.floorInside = SquareId::FLOOR;
  c.floorOutside = SquareId::MUD;
  c.door = SquareId::DOOR;
);

BuildingInfo dungeonBuilding = CONSTRUCT_GLOBAL(BuildingInfo,
  c.wall = SquareId::MOUNTAIN2;
  c.floorInside = SquareId::FLOOR;
  c.floorOutside = SquareId::FLOOR;
  c.door = SquareId::DOOR;
);

static BuildingInfo get(BuildingId id) {
  switch (id) {
    case BuildingId::WOOD: return woodBuilding;
    case BuildingId::WOOD_CASTLE: return woodCastleBuilding;
    case BuildingId::MUD: return mudBuilding;
    case BuildingId::BRICK: return brickBuilding;
    case BuildingId::DUNGEON: return dungeonBuilding;
  }
  assert("__FUNCTION__ case unknown building id");
  return woodBuilding;
}

class Buildings : public LevelMaker {
  public:
  Buildings(int _minBuildings, int _maxBuildings,
      int _minSize, int _maxSize,
      BuildingInfo _building,
      bool _align,
      vector<LevelMaker*> _insideMakers,
      bool _roadConnection = true) :
    minBuildings(_minBuildings),
    maxBuildings(_maxBuildings),
    minSize(_minSize), maxSize(_maxSize),
    align(_align),
    building(_building),
    insideMakers(_insideMakers),
    roadConnection(_roadConnection) {
      CHECK(insideMakers.size() <= minBuildings);
    }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<bool> filled(area);
    int width = area.getW();
    int height = area.getH();
    for (Vec2 v : area)
      filled[v] =  0;
    int spaceBetween = 1;
    int alignHeight = 0;
    if (align) {
      alignHeight = height / 2 - 2 + Random.get(5);
    }
    int nextw = -1;
    int numBuildings = Random.get(minBuildings, maxBuildings);
    for (int i = 0; i < numBuildings; ++i) {
      bool spaceOk = true;
      int w, h, px, py;
      int cnt = 10000;
      bool buildingRow;
      do {
        buildingRow = Random.get(2);
        spaceOk = true;
        w = Random.get(minSize, maxSize);
        h = Random.get(minSize, maxSize);
        if (nextw > -1 && nextw + w < area.getKX()) {
          px = nextw;
          nextw = -1;
        } else
          px = area.getPX() + Random.get(width - w - 2 * spaceBetween + 1) + spaceBetween;
        if (!align)
          py = area.getPY() + Random.get(height - h - 2 * spaceBetween + 1) + spaceBetween;
        else {
          py = area.getPY() + (buildingRow == 1 ? alignHeight - h - 1 : alignHeight + 2);
          if (py + h >= area.getKY() || py < area.getPY()) {
            spaceOk = false;
            continue;
          }
        }
        Vec2 tmp(px - spaceBetween, py - spaceBetween);
        for (Vec2 v : Rectangle(w + spaceBetween * 2 + 1, h + spaceBetween * 2 + 1))
          if (!(tmp + v).inRectangle(area) || filled[px + v.x - spaceBetween][py + v.y - spaceBetween]) {
            spaceOk = false;
            break;
          }
      } while (!spaceOk && --cnt > 0);
      if (cnt == 0) {
        if (i < minBuildings)
          failGen(); // "Failed to add " << minBuildings - i << " buildings out of " << minBuildings;
        else
          break;
      }
      if (Random.roll(1))
        nextw = px + w;
      for (Vec2 v : Rectangle(w + 1, h + 1)) {
        filled[Vec2(px, py) + v] = true;
        builder->putSquare(Vec2(px, py) + v, building.wall);
        builder->setCoverInfo(Vec2(px, py) + v, {true, 1.0});
      }
      for (Vec2 v : Rectangle(w - 1, h - 1)) {
        builder->putSquare(Vec2(px + 1, py + 1) + v, building.floorInside, SquareAttrib::ROOM);
      }
      Vec2 doorLoc = align ? 
          Vec2(px + Random.get(1, w),
               py + (buildingRow * h)) :
          getRandomExit(Rectangle(px, py, px + w + 1, py + h + 1));
      builder->putSquare(doorLoc, building.floorInside);
      builder->putSquare(doorLoc, building.door);
      Rectangle inside(px + 1, py + 1, px + w, py + h);
      if (i < insideMakers.size()) 
        insideMakers[i]->make(builder, inside);
      else
        for (Vec2 v : inside)
          builder->addAttrib(v, SquareAttrib::EMPTY_ROOM);
   }
    if (roadConnection)
      builder->putSquare(Vec2((area.getPX() + area.getKX()) / 2, area.getPY() + alignHeight),
          SquareId::ROAD, SquareAttrib::CONNECT_ROAD);
  }

  private:
  int minBuildings;
  int maxBuildings;
  int minSize;
  int maxSize;
  bool align;
  BuildingInfo building;
  vector<LevelMaker*> insideMakers;
  bool roadConnection;
};

class BorderGuard : public LevelMaker {
  public:

  BorderGuard(LevelMaker* inside, SquareType _type = SquareId::BORDER_GUARD) : type(_type), insideMaker(inside) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (int i : Range(area.getPX(), area.getKX())) {
      builder->putSquare(Vec2(i, area.getPY()), type);
      builder->putSquare(Vec2(i, area.getKY() - 1), type);
    }
    for (int i : Range(area.getPY(), area.getKY())) {
      builder->putSquare(Vec2(area.getPX(), i), type);
      builder->putSquare(Vec2(area.getKX() - 1, i), type);
    }
    insideMaker->make(builder, Rectangle(area.getPX() + 1, area.getPY() + 1, area.getKX() - 1, area.getKY() - 1));
  }

  private:
  SquareType type;
  LevelMaker* insideMaker;

};

class MakerQueue : public LevelMaker {
  public:
  MakerQueue() = default;
  MakerQueue(vector<LevelMaker*> _makers) : makers(_makers) {}

  void addMaker(LevelMaker* maker) {
    makers.push_back(maker);
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (LevelMaker* maker : makers)
      maker->make(builder, area);
  }

  private:
  vector<LevelMaker*> makers;
};

class PredicatePrecalc {
  public:
  PredicatePrecalc(const Predicate& predicate, Level::Builder* builder, Rectangle area)
      : counts(Rectangle(area.getTopLeft(), area.getBottomRight() + Vec2(1, 1))) {
    int px = counts.getBounds().getPX();
    int py = counts.getBounds().getPY();
    for (int x : Range(px, counts.getBounds().getKX()))
      counts[x][py] = 0;
    for (int y : Range(py, counts.getBounds().getKY()))
      counts[px][y] = 0;
    for (Vec2 v : Rectangle(area.getTopLeft() + Vec2(1, 1), counts.getBounds().getBottomRight()))
      counts[v] = (predicate.apply(builder, v - Vec2(1, 1)) ? 1 : 0) +
        counts[v.x - 1][v.y] + counts[v.x][v.y - 1] -counts[v.x - 1][v.y - 1];
  }

  int getCount(Rectangle area) const {
    return counts[area.getBottomRight()] + counts[area.getTopLeft()]
      -counts[area.getBottomLeft()] - counts[area.getTopRight()];
  }

  private:
  Table<int> counts;
};


class RandomLocations : public LevelMaker {
  public:
  RandomLocations(const vector<LevelMaker*>& _insideMakers, const vector<pair<int, int>>& _sizes,
      Predicate pred, bool _separate = true)
      : insideMakers(_insideMakers), sizes(_sizes), predicate(sizes.size(), pred), separate(_separate) {
        CHECK(insideMakers.size() == sizes.size());
        CHECK(predicate.size() == sizes.size());
      }

  RandomLocations(const vector<LevelMaker*>& _insideMakers, const vector<pair<int, int>>& _sizes,
      const vector<Predicate>& pred, bool _separate = true)
      : insideMakers(_insideMakers), sizes(_sizes), predicate(pred.begin(), pred.end()), separate(_separate) {
    CHECK(insideMakers.size() == sizes.size());
    CHECK(pred.size() == sizes.size());
  }

  class LocationPredicate {
    public:
    LocationPredicate(Predicate main, Predicate sec, int minSec, int maxSec)
      // main and sec must be mutually exclusive!!!
        : predicate(main), second(sec), minSecond(minSec), maxSecond(maxSec) {
    }

    LocationPredicate(Predicate p) : predicate(p) {}

    class Precomputed {
      public:
      Precomputed(Level::Builder* builder, Rectangle area, Predicate p1, Predicate p2, int minSec, int maxSec)
        : pred1(p1, builder, area), pred2(p2, builder, area), minSecond(minSec), maxSecond(maxSec) {
      }

      bool apply(Rectangle rect) const {
        int numFirst = pred1.getCount(rect);
        int numSecond = pred2.getCount(rect);
        return numSecond >= minSecond && numSecond < maxSecond && numSecond + numFirst == rect.getW() * rect.getH();
      }

      private:
      PredicatePrecalc pred1;
      PredicatePrecalc pred2;
      int minSecond;
      int maxSecond;
    };

    Precomputed precompute(Level::Builder* builder, Rectangle area) {
      return Precomputed(builder, area, predicate, second, minSecond, maxSecond);
    }

    private:
    Predicate predicate;
    Predicate second = Predicate::alwaysFalse();
    int minSecond = 0;
    int maxSecond = 1;
  };

  RandomLocations(bool _separate = true) : separate(_separate) {}

  void add(LevelMaker* maker, Vec2 size, LocationPredicate pred) {
    insideMakers.push_back(maker);
    sizes.push_back({size.x, size.y});
    predicate.push_back(pred);
  }

  void setMinDistance(LevelMaker* m1, LevelMaker* m2, double dist) {
    minDistance[{m1, m2}] = dist;
  }

  void setMaxDistance(LevelMaker* m1, LevelMaker* m2, double dist) {
    maxDistance[{m1, m2}] = dist;
  }

  void setMinDistanceLast(LevelMaker* m, double dist) {
    minDistance[{m, insideMakers.back()}]  = dist;
  }

  void setMaxDistanceLast(LevelMaker* m, double dist) {
    maxDistance[{m, insideMakers.back()}] = dist;
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<LocationPredicate::Precomputed> precomputed;
    for (int i : All(insideMakers))
      precomputed.push_back(predicate[i].precompute(builder, area));
    for (int __attribute__((unused)) i : Range(3000))
      if (tryMake(builder, precomputed, area))
        return;
    failGen(); // "Failed to find free space for " << (int)sizes.size() << " areas";
  }

  bool tryMake(Level::Builder* builder, vector<LocationPredicate::Precomputed>& precomputed, Rectangle area) {
    vector<Rectangle> occupied;
    vector<Rectangle> makerBounds;
    vector<Level::Builder::Rot> maps;
    for (int __attribute__((unused)) i : All(insideMakers))
      maps.push_back(chooseRandom(
            {Level::Builder::CW0, Level::Builder::CW1, Level::Builder::CW2, Level::Builder::CW3}));
    for (int i : All(insideMakers)) {
      int width = sizes[i].first;
      int height = sizes[i].second;
      if (contains({Level::Builder::CW1, Level::Builder::CW3}, maps[i]))
        std::swap(width, height);
      CHECK(width <= area.getW() && height <= area.getH());
      int px;
      int py;
      int cnt = 1000;
      bool ok;
      do {
        ok = true;
        px = area.getPX() + Random.get(area.getW() - width);
        py = area.getPY() + Random.get(area.getH() - height);
        for (int j : Range(i))
          if ((maxDistance.count({insideMakers[j], insideMakers[i]}) && 
                maxDistance[{insideMakers[j], insideMakers[i]}] <
                  Vec2(px + width / 2, py + height / 2).dist8(occupied[j].middle())) ||
              minDistance[{insideMakers[j], insideMakers[i]}] >
                  Vec2(px + width / 2, py + height / 2).dist8(occupied[j].middle())) {
            ok = false;
            break;
          }
        if (!precomputed[i].apply(Rectangle(px, py, px + width, py + height)))
          ok = false;
        else
          if (separate)
            for (Rectangle r : occupied)
              if (r.intersects(Rectangle(px, py, px + width, py + height))) {
                ok = false;
                break;
              }
      } while (!ok && --cnt > 0);
      if (cnt == 0)
        return false;
      occupied.push_back(Rectangle(px, py, px + width, py + height));
      makerBounds.push_back(Rectangle(px, py, px + sizes[i].first, py + sizes[i].second));
    }
    CHECK(insideMakers.size() == occupied.size());
    for (int i : All(insideMakers)) {
      builder->pushMap(makerBounds[i], maps[i]);
      insideMakers[i]->make(builder, makerBounds[i]);
      builder->popMap();
    }
    return true;
  }

  private:
  vector<LevelMaker*> insideMakers;
  vector<pair<int, int>> sizes;
  vector<LocationPredicate> predicate;
  bool separate;
  map<pair<LevelMaker*, LevelMaker*>, double> minDistance;
  map<pair<LevelMaker*, LevelMaker*>, double> maxDistance;
};

class Margin : public LevelMaker {
  public:
  Margin(int s, LevelMaker* in) : left(s), top(s), right(s), bottom(s), inside(in) {}
  Margin(int _left, int _top, int _right, int _bottom, LevelMaker* in) 
      :left(_left) ,top(_top), right(_right), bottom(_bottom), inside(in) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    CHECK(area.getW() > left + right && area.getH() > top + bottom);
    inside->make(builder, Rectangle(
          area.getPX() + left,
          area.getPY() + top,
          area.getKX() - right,
          area.getKY() - bottom));
  }

  private:
  int left, top, right, bottom;
  LevelMaker* inside;
};

class Shrine : public LevelMaker {
  public:
  Shrine(DeityHabitat _deity, SquareType _floorType, Predicate wallPred, SquareType _newWall,
      LevelMaker* _locationMaker = nullptr) : deity(_deity), floorType(_floorType),
      wallPredicate(wallPred), newWall(_newWall), locationMaker(_locationMaker) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (int __attribute__((unused)) i : Range(10009)) {
      Vec2 pos = area.randomVec2();
      if (!wallPredicate.apply(builder, pos))
        continue;
      int numFloor = 0;
      int numWall = 0;
      for (Vec2 fl : pos.neighbors8()) {
        if (builder->getType(fl) == floorType)
          ++numFloor;
        if (wallPredicate.apply(builder, fl))
          ++numWall;
      }
      if (numFloor < 3 || numWall < 5) {
        continue;
      }
      builder->putSquare(pos, floorType);
      builder->putSquare(pos, SquareType(SquareId::ALTAR, deity));
      for (Vec2 fl : pos.neighbors8())
        if (wallPredicate.apply(builder, fl))
          builder->putSquare(fl, newWall);
      if (locationMaker)
        locationMaker->make(builder, Rectangle(pos - Vec2(1, 1), pos + Vec2(2, 2)));
      Debug() << "Created a shrine of " << int(deity);
      return;
    }
    Debug() << "Didn't find a good place for the shrine of " << int(deity);
  }

  private:
  DeityHabitat deity;
  SquareType floorType;
  Predicate wallPredicate;
  SquareType newWall;
  LevelMaker* locationMaker;
};

void addAvg(int x, int y, const Table<double>& wys, double& avg, int& num) {
  Vec2 pos(x, y);
  if (pos.inRectangle(wys.getBounds())) {
    avg += wys[pos];
    ++num;
  } 
}

Table<double> genNoiseMap(Rectangle area, vector<int> cornerLevels, double varianceMult) {
  int width = 1;
  while (width < area.getW() - 1 || width < area.getH() - 1)
    width *= 2;
  width /= 2;
  ++width;
  Table<double> wys(width, width);
  wys[0][0] = cornerLevels[0];
  wys[width - 1][0] = cornerLevels[1];
  wys[width - 1][width - 1] = cornerLevels[2];
  wys[0][width - 1] = cornerLevels[3];
  wys[(width - 1) / 2][(width - 1) / 2] = cornerLevels[4];

  double variance = 0.5;
  for (int a = width - 1; a >= 2; a /= 2) {
    if (a < width - 1)
      for (Vec2 pos1 : Rectangle((width - 1) / a, (width - 1) / a)) {
        Vec2 pos = pos1 * a;
        double avg = (wys[pos] + wys[pos.x + a][pos.y] + wys[pos.x][pos.y + a] + wys[pos.x + a][pos.y + a]) / 4;
        wys[pos.x + a / 2][pos.y + a / 2] =
            avg + variance * (Random.getDouble() * 2 - 1);
      }
    for (Vec2 pos1 : Rectangle((width - 1) / a, (width - 1) / a + 1)) {
      Vec2 pos = pos1 * a;
      double avg = 0;
      int num = 0;
      addAvg(pos.x + a / 2, pos.y - a / 2, wys, avg, num);
      addAvg(pos.x, pos.y, wys, avg, num);
      addAvg(pos.x + a, pos.y, wys, avg, num);
      addAvg(pos.x + a / 2, pos.y + a / 2, wys, avg, num);
      wys[pos.x + a / 2][pos.y] =
          avg / num + variance * (Random.getDouble() * 2 - 1);
    }
    for (Vec2 pos1 : Rectangle((width - 1) / a + 1, (width - 1) / a)) {
      Vec2 pos = pos1 * a;
      double avg = 0;
      int num = 0;
      addAvg(pos.x - a / 2, pos.y + a / 2, wys, avg, num);
      addAvg(pos.x, pos.y, wys, avg, num);
      addAvg(pos.x, pos.y + a , wys, avg, num);
      addAvg(pos.x + a / 2, pos.y + a / 2, wys, avg, num);
      wys[pos.x][pos.y + a / 2] =
          avg / num + variance * (Random.getDouble() * 2 - 1);
    }
    variance *= varianceMult;
  }
  Table<double> ret(area);
  Vec2 offset(area.getPX(), area.getPY());
  for (Vec2 v : area) {
    Vec2 lv((v.x - offset.x) * width / area.getW(), (v.y - offset.y) * width / area.getH());
    ret[v] = wys[lv];
  }
  return ret;
}

void raiseLocalMinima(Table<double>& t) {
  Vec2 minPos = t.getBounds().getTopLeft();
  for (Vec2 v : t.getBounds())
    if (t[v] < t[minPos])
      minPos = v;
  Table<bool> visited(t.getBounds(), false);
  auto comparator = [&](const Vec2& v1, const Vec2& v2) { return t[v1] > t[v2];};
  priority_queue<Vec2, vector<Vec2>, decltype(comparator)> q(comparator);
  q.push(minPos);
  visited[minPos] = true;
  while (!q.empty()) {
    Vec2 pos = q.top();
    q.pop();
    for (Vec2 v : pos.neighbors4())
      if (v.inRectangle(t.getBounds()) && !visited[v]) {
        if (t[v] < t[pos])
          t[v] = t[pos];
        q.push(v);
        visited[v] = true;
      }
  }
}

vector<double> sortedValues(const Table<double>& t) {
  vector<double> values;
  for (Vec2 v : t.getBounds()) {
    values.push_back(t[v]);
  }
  sort(values.begin(), values.end());
  return values;
}

class Mountains : public LevelMaker {
  public:
  Mountains(vector<double> r, double varianceM, vector<int> _cornerLevels, bool _makeDark = true,
      vector<SquareType> _types = {SquareId::GLACIER, SquareId::MOUNTAIN, SquareId::HILL, SquareId::GRASS,
      SquareId::SAND}) 
      : ratio(r), cornerLevels(_cornerLevels), types(_types), makeDark(_makeDark), varianceMult(varianceM) {
    CHECK(r.size() == 5);
  }


  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(area, cornerLevels, varianceMult);
    raiseLocalMinima(wys);
    vector<double> values = sortedValues(wys);
    double cutOffValSand = values[(int)(ratio[0] * double(values.size()))];
    double cutOffValLand = values[(int)(ratio[1] * double(values.size()))];
    double cutOffValHill = values[(int)(ratio[2] * double(values.size()))];
    double cutOffVal = values[(int)(ratio[3] * double(values.size()))];
    double cutOffValSnow = values[(int)(ratio[4] * double(values.size()))];
    int gCnt = 0, mCnt = 0, hCnt = 0, lCnt = 0, sCnt = 0, wCnt = 0;
    for (Vec2 v : area) {
      builder->setHeightMap(v, wys[v]);
      if (wys[v] > cutOffValSnow) {
        builder->putSquare(v, types[0], SquareAttrib::GLACIER);
        if (makeDark)
          builder->setCoverInfo(v, {true, 0.0});
        else
          builder->addAttrib(v, SquareAttrib::ROAD_CUT_THRU);
        ++gCnt;
      }
      else if (wys[v] > cutOffVal) {
        builder->putSquare(v, types[1], SquareAttrib::MOUNTAIN);
        if (makeDark)
          builder->setCoverInfo(v, {true, 1. - (wys[v] - cutOffVal) / (cutOffValSnow - cutOffVal)});
        else
          builder->addAttrib(v, SquareAttrib::ROAD_CUT_THRU);
        ++mCnt;
      }
      else if (wys[v] > cutOffValHill) {
        builder->putSquare(v, types[2], SquareAttrib::HILL);
        ++hCnt;
      }
      else if (wys[v] > cutOffValLand) {
        builder->putSquare(v, types[3]);
        builder->addAttrib(v, SquareAttrib::LOWLAND);
        ++lCnt;
      }
      else if (wys[v] > cutOffValSand) {
        builder->putSquare(v, types[4]);
        builder->addAttrib(v, SquareAttrib::LOWLAND);
        ++sCnt;
      }
      else {
        builder->addAttrib(v, SquareAttrib::LAKE);
        ++wCnt;
      }
    }
    Debug() << "Terrain distribution " << gCnt << " glacier, " << mCnt << " mountain, " << hCnt << " hill, " << lCnt << " lowland, " << wCnt << " water, " << sCnt << " sand";
  }

  private:
  vector<double> ratio;
  vector<int> cornerLevels;
  vector<SquareType> types;
  bool makeDark;
  double varianceMult;
};

class Roads : public LevelMaker {
  public:
  Roads(SquareType roadSquare) : square(roadSquare) {}

  double getValue(Level::Builder* builder, Vec2 pos) {
    if ((!builder->getSquare(pos)->canEnter(MovementType({MovementTrait::WALK, MovementTrait::SWIM})) && 
         !builder->hasAttrib(pos, SquareAttrib::ROAD_CUT_THRU)) ||
        builder->hasAttrib(pos, SquareAttrib::NO_ROAD))
      return ShortestPath::infinity;
    SquareType type = builder->getType(pos);
    if (type == SquareId::WATER)
      return 10;
    if (contains({SquareId::ROAD, SquareId::ROAD}, type.getId()))
      return 1;
    return 1 + pow(1 + builder->getHeightMap(pos), 2);
  }

  SquareType getRoadType(Level::Builder* builder, Vec2 pos) {
    if (builder->getType(pos) == SquareId::WATER)
      return SquareId::BRIDGE;
    else
      return SquareId::ROAD;
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> points;
    for (Vec2 v : area)
      if (builder->hasAttrib(v, SquareAttrib::CONNECT_ROAD)) {
        points.push_back(v);
        Debug() << "Connecting point " << v;
      }
    for (int ind : Range(1, points.size())) {
      Vec2 p1 = points[ind];
      Vec2 p2 = points[ind - 1];
      ShortestPath path(area,
          [builder, this, &area](Vec2 pos) { return getValue(builder, pos); }, 
          [] (Vec2 v) { return v.length4(); },
          Vec2::directions4(true), p1 ,p2);
      Vec2 prev(-1, -1);
      for (Vec2 v = p2; v != p1; v = path.getNextMove(v)) {
        if (!path.isReachable(v))
          failGen();
        SquareType roadType = getRoadType(builder, v);
        if (v != p2 && v != p1 && builder->getType(v) != roadType)
          builder->putSquare(v, roadType);
        if (prev.x > -1)
          builder->getSquare(v)->addTravelDir(prev - v);
        builder->getSquare(v)->addTravelDir(path.getNextMove(v) - v);
        prev = v;
      }
      builder->getSquare(p1)->addTravelDir(prev - p1);
    }
  }

  private:
  SquareType square;
};

class StartingPos : public LevelMaker {
  public:

  StartingPos(Predicate pred, StairKey key) : predicate(pred), stairKey(key) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 pos : area)
      if (predicate.apply(builder, pos))
        builder->getSquare(pos)->setLandingLink(StairDirection::UP, stairKey);
  }

  private:
  Predicate predicate;
  StairKey stairKey;
};

class Forrest : public LevelMaker {
  public:
  Forrest(double _ratio, double _density, SquareType _onType, vector<SquareType> _types, vector<double> _probs) 
      : ratio(_ratio), density(_density), types(_types), probs(_probs), onType(_onType) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(area, {0, 0, 0, 0, 0}, 0.9);
    vector<double> values = sortedValues(wys);
    double cutoff = values[values.size() * ratio];
    for (Vec2 v : area)
      if (builder->getType(v) == onType && wys[v] < cutoff) {
        if (Random.getDouble() <= density)
          builder->putSquare(v, chooseRandom(types, probs));
        builder->addAttrib(v, SquareAttrib::FORREST);
      }
  }

  private:
  double ratio;
  double density;
  vector<SquareType> types;
  vector<double> probs;
  SquareType onType;
};

class LocationMaker : public LevelMaker {
  public:
  LocationMaker(Location* l) : location(l) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    builder->addLocation(location, area);
  }
  
  private:
  Location* location;
};

class ForEachSquare : public LevelMaker {
  public:
  ForEachSquare(function<void(Level::Builder*, Vec2 pos)> f, Predicate _onPred = Predicate::alwaysTrue())
    : fun(f), onPred(_onPred) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area)
      if (onPred.apply(builder, v))
        fun(builder, v);
  }
  
  protected:
  function<void(Level::Builder*, Vec2 pos)> fun;
  Predicate onPred;
};

class AddAttrib : public ForEachSquare {
  public:
  AddAttrib(SquareAttrib attr, Predicate onPred = Predicate::alwaysTrue()) 
      : ForEachSquare([attr](Level::Builder* b, Vec2 pos) { b->addAttrib(pos, attr); }, onPred) {}
};

class RemoveAttrib : public ForEachSquare {
  public:
  RemoveAttrib(SquareAttrib attr, Predicate onPred = Predicate::alwaysTrue()) 
    : ForEachSquare([attr](Level::Builder* b, Vec2 pos) { b->removeAttrib(pos, attr); }, onPred) {}
};

class Stairs : public LevelMaker {
  public:
  Stairs(StairDirection dir, StairKey k, Predicate onPred, optional<SquareAttrib> _setAttr = none,
      StairLook look = StairLook::NORMAL) : direction(dir), key(k), onPredicate(onPred), setAttr(_setAttr),
      stairLook(look) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> pos;
    for (Vec2 v : area)
      if (onPredicate.apply(builder, v))
        pos.push_back(v);
    checkGen(pos.size() > 0);
    SquareType type = direction == StairDirection::DOWN ? SquareId::DOWN_STAIRS : SquareId::UP_STAIRS;
    builder->putSquare(pos[Random.get(pos.size())], SquareFactory::getStairs(direction, key, stairLook),
        type, setAttr);
  }

  private:
  StairDirection direction;
  StairKey key;
  Predicate onPredicate;
  optional<SquareAttrib> setAttr;
  StairLook stairLook;
};

class ShopMaker : public LevelMaker {
  public:
  ShopMaker(ItemFactory _factory, Tribe* _tribe, int _numItems, BuildingInfo _building)
      : factory(_factory), tribe(_tribe), numItems(_numItems), building(_building) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Location *loc = new Location();
    builder->addLocation(loc, area);
    PCreature shopkeeper = CreatureFactory::getShopkeeper(loc, tribe);
    vector<Vec2> pos;
    for (Vec2 v : area)
      if (builder->getSquare(v)->canEnter(shopkeeper.get()) && builder->getType(v) == building.floorInside)
        pos.push_back(v);
    builder->putCreature(pos[Random.get(pos.size())], std::move(shopkeeper));
    builder->putSquare(pos[Random.get(pos.size())], SquareId::TORCH);
    for (int __attribute__((unused)) i : Range(numItems)) {
      Vec2 v = pos[Random.get(pos.size())];
      builder->getSquare(v)->dropItems(factory.random());
    }
  }

  private:
  ItemFactory factory;
  Tribe* tribe;
  int numItems;
  BuildingInfo building;
};

class LevelExit : public LevelMaker {
  public:
  LevelExit(SquareType _exitType, optional<SquareAttrib> _attrib = none, int _minCornerDist = 1)
      : exitType(_exitType), attrib(_attrib), minCornerDist(_minCornerDist) {}
  
  virtual void make(Level::Builder* builder, Rectangle area) override {
    builder->putSquare(getRandomExit(area, minCornerDist), exitType, attrib);
  }

  private:
  SquareType exitType;
  optional<SquareAttrib> attrib;
  int minCornerDist;
};

class Division : public LevelMaker {
  public:
  Division(double _vRatio, double _hRatio,
      LevelMaker* _upperLeft, LevelMaker* _upperRight, LevelMaker* _lowerLeft, LevelMaker* _lowerRight,
      optional<SquareType> _wall = none) : vRatio(_vRatio), hRatio(_hRatio),
      upperLeft(_upperLeft), upperRight(_upperRight), lowerLeft(_lowerLeft), lowerRight(_lowerRight), wall(_wall) {}

  Division(double _hRatio, LevelMaker* _left, LevelMaker* _right, optional<SquareType> _wall = none) 
      : vRatio(-1), hRatio(_hRatio), upperLeft(_left), upperRight(_right), wall(_wall) {}

  Division(bool, double _vRatio, LevelMaker* _top, LevelMaker* _bottom, optional<SquareType> _wall = none) 
      : vRatio(_vRatio), hRatio(-1), upperLeft(_top), lowerLeft(_bottom), wall(_wall) {}

  void makeHorizDiv(Level::Builder* builder, Rectangle area) {
    int hDiv = area.getPX() + min(area.getW() - 1, max(1, (int) (hRatio * area.getW())));
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.getPX(), area.getPY(), hDiv, area.getKY()));
    if (upperRight)
      upperRight->make(builder, Rectangle(hDiv + (wall ? 1 : 0), area.getPY(), area.getKX(), area.getKY()));
    if (wall)
      for (int i : Range(area.getPY(), area.getKY()))
        builder->putSquare(Vec2(hDiv, i), *wall);
  }

  void makeVertDiv(Level::Builder* builder, Rectangle area) {
    int vDiv = area.getPY() + min(area.getH() - 1, max(1, (int) (vRatio * area.getH())));
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.getPX(), area.getPY(), area.getKX(), vDiv));
    if (lowerLeft)
      lowerLeft->make(builder, Rectangle(area.getPX(), vDiv + (wall ? 1 : 0), area.getKX(), area.getKY()));
    if (wall)
      for (int i : Range(area.getPX(), area.getKX()))
        builder->putSquare(Vec2(i, vDiv), *wall);
  }

  void makeDiv(Level::Builder* builder, Rectangle area) {
    int vDiv = area.getPY() + min(area.getH() - 1, max(1, (int) (vRatio * area.getH())));
    int hDiv = area.getPX() + min(area.getW() - 1, max(1, (int) (hRatio * area.getW())));
    int wallSpace = wall ? 1 : 0;
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.getPX(), area.getPY(), hDiv, vDiv));
    if (upperRight)
      upperRight->make(builder, Rectangle(hDiv + wallSpace, area.getPY(), area.getKX(), vDiv));
    if (lowerLeft)
      lowerLeft->make(builder, Rectangle(area.getPX(), vDiv + wallSpace, hDiv, area.getKY()));
    if (lowerRight)
      lowerRight->make(builder, Rectangle(hDiv + wallSpace, vDiv + wallSpace, area.getKX(), area.getKY()));
    if (wall) {
      for (int i : Range(area.getPY(), area.getKY()))
        builder->putSquare(Vec2(hDiv, i), *wall);
      for (int i : Range(area.getPX(), area.getKX()))
        builder->putSquare(Vec2(i, vDiv), *wall);
    }
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    if (vRatio < 0)
      makeHorizDiv(builder, area);
    else if (hRatio < 0)
      makeVertDiv(builder, area);
    else
      makeDiv(builder, area);
  }

  private:
  double vRatio, hRatio;
  LevelMaker *upperLeft = nullptr;
  LevelMaker *upperRight = nullptr;
  LevelMaker *lowerLeft = nullptr;
  LevelMaker *lowerRight = nullptr;
  optional<SquareType> wall;
};

class Circle : public LevelMaker {
  public:
  Circle(ItemId _item) : item(_item) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Vec2 center = area.middle();
    double r = min(area.getH(), area.getW()) / 2 - 1;
    Vec2 lastPos;
    for (double a = 0; a < 3.1415 * 2; a += Random.getDouble() * r / 10) {
      Vec2 pos = center + Vec2(sin(a) * r, cos(a) * r);
      if (pos != lastPos) {
        builder->getSquare(pos)->dropItem(ItemFactory::fromId(item));
        lastPos = pos;
      }
    }
  }

  private:
  ItemId item;
};

class AreaCorners : public LevelMaker {
  public:
  AreaCorners(LevelMaker* _maker, Vec2 _size, vector<LevelMaker*> _insideMakers)
      : maker(_maker), size(_size), insideMakers(_insideMakers) {}

  vector<Rectangle> getCorners(Rectangle area) {
    return {
      Rectangle(area.getTopLeft(), area.getTopLeft() + size),
      Rectangle(area.getTopRight() - Vec2(size.x, 0), area.getTopRight() + Vec2(0, size.y)),
      Rectangle(area.getBottomLeft() - Vec2(0, size.y), area.getBottomLeft() + Vec2(size.x, 0)),
      Rectangle(area.getBottomRight() - size, area.getBottomRight())};
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Rectangle> corners = randomPermutation(getCorners(area));
    for (int i : All(corners)) {
      maker->make(builder, corners[i]);
      if (i < insideMakers.size())
        insideMakers[i]->make(builder, corners[i]);
    }
  }

  private:
  LevelMaker* maker;
  Vec2 size;
  vector<LevelMaker*> insideMakers;
};

class CastleExit : public LevelMaker {
  public:
  CastleExit(Tribe* _guardTribe, BuildingInfo _building, CreatureId _guardId)
    : guardTribe(_guardTribe), building(_building), guardId(_guardId) {}
  
  virtual void make(Level::Builder* builder, Rectangle area) override {
    Vec2 loc(area.getKX() - 1, area.middle().y);
 //   builder->putSquare(loc, SquareId::DOOR);
    builder->putSquare(loc + Vec2(2, 0), building.floorInside);
    builder->putSquare(loc + Vec2(2, 0), building.door, SquareAttrib::CONNECT_ROAD);
    vector<Vec2> walls { Vec2(1, -2), Vec2(2, -2), Vec2(2, -1), Vec2(2, 1), Vec2(2, 2), Vec2(1, 2)};
    for (Vec2 v : walls)
      builder->putSquare(loc + v, building.wall);
    vector<Vec2> floor { Vec2(1, -1), Vec2(1, 0), Vec2(1, 1), Vec2(0, -1), Vec2(0, 0), Vec2(0, 1) };
    for (Vec2 v : floor)
      builder->putSquare(loc + v, building.floorInside);
    vector<Vec2> guardPos { Vec2(1, 1), Vec2(1, -1) };
    for (Vec2 pos : guardPos) {
      Location* guard = new Location();
      builder->addLocation(guard, Rectangle(loc + pos, loc + pos + Vec2(1, 1)));
      builder->putCreature(loc + pos, CreatureFactory::fromId(guardId, guardTribe,
            MonsterAIFactory::stayInLocation(guard, false)));
    }
  }

  private:
  Tribe* guardTribe;
  BuildingInfo building;
  CreatureId guardId;
};

class SetCovered : public LevelMaker {
  public:
  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area)
      builder->setCoverInfo(v, {true, 1.0});
  }
};

}

static MakerQueue* stockpileMaker(StockpileInfo info) {
  SquareId square = SquareId(0);
  switch (info.type) {
    case StockpileInfo::GOLD: square = SquareId::TREASURE_CHEST; break;
    case StockpileInfo::MINERALS: square = SquareId::STOCKPILE_RES; break;
  }
  ItemFactory items;
  switch (info.type) {
    case StockpileInfo::GOLD: items = ItemFactory::singleType(ItemId::GOLD_PIECE); break;
    case StockpileInfo::MINERALS: items = ItemFactory::minerals(); break;
  }
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(square));
  queue->addMaker(new Items(items, square, info.number, info.number + 1));
  return queue;
}

LevelMaker* LevelMaker::cryptLevel(CreatureFactory roomFactory, CreatureId coffinCreature,
    vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  vector<FeatureInfo> featureCount { 
      { {SquareId::COFFIN, coffinCreature}, 3, 7}};
  queue->addMaker(new Empty(SquareId::BLACK_WALL));
  queue->addMaker(new RoomMaker(Random.get(8, 15), 3, 5, SquareId::ROCK_WALL,
        SquareType(SquareId::BLACK_WALL)));
  queue->addMaker(new Connector(1, 3));
  queue->addMaker(new DungeonFeatures(Predicate::attrib(SquareAttrib::EMPTY_ROOM),featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, Predicate::type(SquareId::FLOOR)));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, Predicate::type(SquareId::FLOOR)));
  queue->addMaker(new Creatures(roomFactory, Random.get(10, 15), MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareId::FLOOR, 5, 10));
  return new BorderGuard(queue, SquareId::BLACK_WALL);
}

LevelMaker* hatchery(CreatureFactory factory, int numCreatures) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareId::MUD));
  queue->addMaker(new Creatures(factory, numCreatures));
  return queue;
}

MakerQueue* getElderRoom(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* elderRoom = new MakerQueue();
  if (info.elderLoot)
    elderRoom->addMaker(new Items(ItemFactory::singleType(*info.elderLoot), building.floorInside, 1, 2));
  return elderRoom;
}

MakerQueue* village2(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  vector<FeatureInfo> featureCount { 
      { {SquareId::CHEST, CreatureId::RAT}, 1, 4 },
      { SquareId::TORCH, 2, 5 },
      { SquareId::BED, 4, 7 }};
  queue->addMaker(new LocationMaker(info.location));
  vector<LevelMaker*> insideMakers {getElderRoom(info)};
  for (auto& elem : info.stockpiles)
    insideMakers.push_back(stockpileMaker(elem));
  if (info.shopFactory)
    insideMakers.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.get(8, 16), building));
  queue->addMaker(new Buildings(6, 10, 3, 4, building, false, insideMakers));
  queue->addMaker(new DungeonFeatures(Predicate::attrib(SquareAttrib::EMPTY_ROOM), featureCount));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective, building.floorOutside));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
  return queue;
}

MakerQueue* village(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  vector<FeatureInfo> featureCount { 
      { {SquareId::CHEST, CreatureId::RAT}, 1, 3 },
      { SquareId::TORCH, 2, 4 },
      { SquareId::BED, 3, 5 }};
  queue->addMaker(new LocationMaker(info.location));
  queue->addMaker(new UniformBlob(building.floorOutside, none, none, 0.6));
  vector<LevelMaker*> insideMakers {
 //     hatchery(CreatureFactory::singleType(info.tribe, CreatureId::PIG), Random.get(2, 5)),
      getElderRoom(info)};
  if (info.shopFactory)
    insideMakers.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.get(8, 16), building));
  for (auto& elem : info.stockpiles)
    insideMakers.push_back(stockpileMaker(elem));
  queue->addMaker(new Buildings(4, 8, 3, 7, building, true, insideMakers));
  queue->addMaker(new DungeonFeatures(Predicate::attrib(SquareAttrib::EMPTY_ROOM), featureCount));
  queue->addMaker(new DungeonFeatures(Predicate::type(building.floorOutside),
        {{SquareId::TORCH, 1, 3 }}));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective, building.floorOutside));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
  return queue;
}

MakerQueue* cottage(SettlementInfo info, const vector<FeatureInfo>& featureCount =
    {{ {SquareId::CHEST, CreatureId::RAT}, 4, 9 }, { SquareId::BED, 2, 4 }, { SquareId::TORCH, 1, 3 }}) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(building.floorOutside));
  MakerQueue* room = getElderRoom(info);
  room->addMaker(new DungeonFeatures(Predicate::type(building.floorInside), featureCount));
  queue->addMaker(new Buildings(1, 2, 5, 7, building, false, {room}, false));
  queue->addMaker(new LocationMaker(info.location));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective, building.floorOutside));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
   return queue;
}

MakerQueue* castle(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  LevelMaker* castleRoom = new BorderGuard(new Empty(building.floorInside, SquareAttrib::EMPTY_ROOM), building.wall);
  MakerQueue* leftSide = new MakerQueue();
  leftSide->addMaker(new Division(true, Random.getDouble(0.5, 0.5),
      new Margin(1, -1, -1, 1, castleRoom), new Margin(1, 1, -1, -1, castleRoom)));
  vector<FeatureInfo> featureCount { 
      { {SquareId::CHEST, CreatureId::RAT}, 3, 8 },
      { SquareId::BED, 5, 8 }};
  leftSide->addMaker(new DungeonFeatures(Predicate::attrib(SquareAttrib::EMPTY_ROOM), featureCount));
  for (StairKey key : info.downStairs)
    leftSide->addMaker(new Stairs(StairDirection::DOWN, key, Predicate::type(building.floorInside), none));
  leftSide->addMaker(getElderRoom(info));
  MakerQueue* inside = new MakerQueue();
  inside->addMaker(new Empty(building.floorOutside));
  vector<LevelMaker*> insideMakers;
  if (info.shopFactory)
    insideMakers.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.get(8, 16), building));
  inside->addMaker(new Division(Random.getDouble(0.25, 0.4), leftSide,
        new Buildings(1, 3, 3, 6, building, false, insideMakers, false), building.wall));
  MakerQueue* insidePlusWall = new MakerQueue();
  insidePlusWall->addMaker(new BorderGuard(inside, building.wall));
  MakerQueue* queue = new MakerQueue();
  int insideMargin = 2;
  queue->addMaker(new Margin(insideMargin, new LocationMaker(info.location)));
  queue->addMaker(new Margin(insideMargin, insidePlusWall));
  vector<LevelMaker*> cornerMakers;
  for (auto& elem : info.stockpiles)
    cornerMakers.push_back(new Margin(1, stockpileMaker(elem)));
  queue->addMaker(new AreaCorners(new BorderGuard(new Empty(building.floorInside), building.wall),
        Vec2(5, 5), cornerMakers));
  queue->addMaker(new Margin(insideMargin, new Connector(0, 1, 18)));
  queue->addMaker(new Margin(insideMargin, new CastleExit(info.tribe, building, *info.guardId)));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective, building.floorOutside));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
   inside->addMaker(new DungeonFeatures(Predicate::type(building.floorOutside),
        {{ SquareId::WELL, 1, 2 }, { SquareId::TORCH, 2, 5 }}));
  inside->addMaker(new DungeonFeatures(Predicate::type(building.floorInside),{
        { SquareId::FOUNTAIN, 2, 4}, { SquareId::TORCH, 2, 5 }}));
  queue->addMaker(new StartingPos(Predicate::type(SquareId::MUD), StairKey::HERO_SPAWN));
  return queue;
}

MakerQueue* castle2(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* inside = new MakerQueue();
  inside->addMaker(new Empty(building.floorOutside));
  vector<LevelMaker*> insideMakers {new MakerQueue({
      getElderRoom(info),
      stockpileMaker(getOnlyElement(info.stockpiles))})};
  inside->addMaker(new Buildings(1, 2, 3, 4, building, false, insideMakers, false));
  MakerQueue* insidePlusWall = new MakerQueue();
  insidePlusWall->addMaker(new BorderGuard(inside, building.wall));
  MakerQueue* queue = new MakerQueue();
  int insideMargin = 2;
  queue->addMaker(new Margin(insideMargin, new LocationMaker(info.location)));
  queue->addMaker(new Margin(insideMargin, insidePlusWall));
  queue->addMaker(new Margin(insideMargin, new Connector(0, 1, 18)));
  queue->addMaker(new Margin(insideMargin, new CastleExit(info.tribe, building, *info.guardId)));
  queue->addMaker(new DungeonFeatures(Predicate::type(building.floorOutside),
        {{ SquareId::WELL, 1, 2 }, { SquareId::TORCH, 2, 5 }}));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective, building.floorOutside));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
   return queue;
}

LevelMaker* dungeonEntrance(StairKey key, SquareType onType, const string& dungeonDesc) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Stairs(StairDirection::DOWN, key, Predicate::type(onType), SquareAttrib::CONNECT_ROAD,
      StairLook::DUNGEON_ENTRANCE));
  queue->addMaker(new LocationMaker(new Location("dungeon entrance", dungeonDesc)));
  return queue;
}

LevelMaker* makeLake() {
  MakerQueue* queue = new MakerQueue();
  Location* loc = new Location();
  queue->addMaker(new LocationMaker(loc));
  queue->addMaker(new Lake());
  queue->addMaker(new Margin(10, new RandomLocations(
          {new UniformBlob(SquareId::GRASS, SquareType(SquareId::SAND))}, {{15, 15}},
          Predicate::type(SquareId::WATER))));
 // queue->addMaker(new Creatures(CreatureFactory::singleType(CreatureId::VODNIK), 2, 5, MonsterAIFactory::stayInLocation(loc)));
  return queue;
}

LevelMaker* LevelMaker::towerLevel(optional<StairKey> down, optional<StairKey> up) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(up ? SquareId::ROCK_WALL : SquareId::LOW_ROCK_WALL));
  queue->addMaker(new Margin(1, new Empty(SquareId::FLOOR)));
  queue->addMaker(new Margin(1, new AddAttrib(SquareAttrib::ROOM)));
  queue->addMaker(new RemoveAttrib(SquareAttrib::ROAD_CUT_THRU));
  LevelMaker* downStairs = (down) ? new Stairs(StairDirection::DOWN, *down,
      Predicate::type(SquareId::FLOOR)) : nullptr;
  LevelMaker* upStairs = (up) ? new Stairs(StairDirection::UP, *up, Predicate::type(SquareId::FLOOR)) : nullptr;
  queue->addMaker(new Division(0.5, 0.5, upStairs, nullptr, nullptr, downStairs));
  return queue;
}

Vec2 getSize(SettlementType type) {
  switch (type) {
    case SettlementType::WITCH_HOUSE:
    case SettlementType::COTTAGE: return {Random.get(8, 12), Random.get(8, 12)};
    case SettlementType::VILLAGE2: return {30, 20};
    case SettlementType::VILLAGE:
    case SettlementType::CASTLE: return {30, 20};
    case SettlementType::CASTLE2: return {15, 15};
    case SettlementType::MINETOWN: return {30, 20};
    case SettlementType::SMALL_MINETOWN: return {15, 15};
    case SettlementType::CAVE: return {12, 12};
    case SettlementType::VAULT: return {10, 10};
    case SettlementType::ISLAND_VAULT: return {Random.get(15, 25), Random.get(15, 25)};
  }
  assert("__FUNCTION__ case unknown settlement type");
  return {0,0};
}

RandomLocations::LocationPredicate getSettlementPredicate(SettlementType type) {
  switch (type) {
    case SettlementType::VILLAGE2:
        return Predicate::andPred(
            Predicate::negate(Predicate::attrib(SquareAttrib::RIVER)),
            Predicate::attrib(SquareAttrib::FORREST));
    case SettlementType::CAVE: return RandomLocations::LocationPredicate(
        Predicate::type(SquareId::MOUNTAIN2), Predicate::type(SquareId::HILL), 5, 15);
    case SettlementType::VAULT:
    case SettlementType::SMALL_MINETOWN:
    case SettlementType::MINETOWN: return Predicate::type(SquareId::MOUNTAIN2);
    case SettlementType::ISLAND_VAULT:
        return Predicate::Predicate::attrib(SquareAttrib::MOUNTAIN);
    default: return Predicate::andPred(Predicate::attrib(SquareAttrib::LOWLAND),
                 Predicate::negate(Predicate::attrib(SquareAttrib::RIVER)));
  }
}

static MakerQueue* genericMineTownMaker(SettlementInfo info, int numCavern, int maxCavernSize, int numRooms,
    int numTorches, int minRoomSize, int maxRoomSize, int minFeatures, int maxFeatures) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  vector<FeatureInfo> featureCount {
      { {SquareId::CHEST, CreatureId::RAT}, minFeatures, maxFeatures },
      { SquareId::BED, minFeatures, maxFeatures }};
  LevelMaker* cavern = new UniformBlob(SquareId::FLOOR);
  vector<LevelMaker*> vCavern;
  vector<pair<int, int>> sizes;
  for (int __attribute__((unused)) i : Range(numCavern)) {
    sizes.push_back(make_pair(Random.get(5, maxCavernSize), Random.get(5, maxCavernSize)));
    vCavern.push_back(cavern);
  }
  queue->addMaker(new RandomLocations(vCavern, sizes, Predicate::alwaysTrue(), false));
  vector<LevelMaker*> roomInsides;
  if (info.shopFactory)
    roomInsides.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.get(8, 16), building));
  for (auto& elem : info.stockpiles)
    roomInsides.push_back(stockpileMaker(elem));
  queue->addMaker(new RoomMaker(numRooms, minRoomSize, maxRoomSize, building.wall, none,
      new Empty(SquareId::FLOOR, SquareAttrib::CONNECT_CORRIDOR), roomInsides));
  queue->addMaker(new Connector(1, 0));
  Predicate featurePred = Predicate::andPred(
      Predicate::attrib(SquareAttrib::EMPTY_ROOM),
      Predicate::type(SquareId::FLOOR));
  for (StairKey key : info.downStairs)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, featurePred));
  for (StairKey key : info.upStairs)
    queue->addMaker(new Stairs(StairDirection::UP, key, featurePred));
  queue->addMaker(new DungeonFeatures(featurePred, featureCount));
  queue->addMaker(new DungeonFeatures(Predicate::orPred(Predicate::type(SquareId::FLOOR),
          Predicate::type(SquareId::FLOOR)), {{SquareId::TORCH, numTorches, numTorches + 1}}));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
   queue->addMaker(new LocationMaker(info.location));
  return queue;
}

static MakerQueue* mineTownMaker(SettlementInfo info) {
  return genericMineTownMaker(info, 10, 12, Random.get(5, 7), Random.get(10, 16), 6, 8, 4, 8);
}

static MakerQueue* smallMineTownMaker(SettlementInfo info) {
  return genericMineTownMaker(info, 2, 7, Random.get(3, 5), Random.get(2, 5), 5, 7, 2, 4);
}

static MakerQueue* vaultMaker(SettlementInfo info, bool connection) {
  MakerQueue* queue = new MakerQueue();
  BuildingInfo building = get(info.buildingId);
  if (connection)
    queue->addMaker(new UniformBlob(building.floorOutside, none, SquareAttrib::CONNECT_CORRIDOR));
  else
    queue->addMaker(new UniformBlob(building.floorOutside));
  queue->addMaker(new Creatures(info.creatures, info.numCreatures, info.collective));
  if (info.shopFactory)
    queue->addMaker(new Items(*info.shopFactory, building.floorOutside, 16, 20));
  if (info.neutralCreatures)
    queue->addMaker(
        new Creatures(info.neutralCreatures->first, info.neutralCreatures->second, building.floorOutside));
   queue->addMaker(new LocationMaker(info.location));
  return queue;
}

static LevelMaker* islandVaultMaker(SettlementInfo info, bool connection) {
  MakerQueue* queue = new MakerQueue();
  BuildingInfo building = get(info.buildingId);
  queue->addMaker(new UniformBlob(SquareId::WATER, none, SquareAttrib::LAKE));
  RandomLocations* locations = new RandomLocations();
  LevelMaker* inside;
  if (!info.stockpiles.empty())
    inside = stockpileMaker(getOnlyElement(info.stockpiles));
  else
    inside = new Empty(building.floorInside);
  locations->add(new Margin(1, new MakerQueue({
      new Empty(building.wall),
      new AddAttrib(SquareAttrib::NO_DIG),
      new RemoveAttrib(SquareAttrib::LAKE),
      //new AddAttrib(SquareAttrib::CONNECT_CORRIDOR),
      new Margin(1, inside),
      new LevelExit(SquareId::DOOR)})),
      Vec2(Random.get(6, 8), Random.get(6, 8)), Predicate::type(SquareId::WATER));
  queue->addMaker(new Margin(3, locations));
  return queue;
}

static MakerQueue* dragonCaveMaker(SettlementInfo info) {
  MakerQueue* queue = vaultMaker(info, true);
/*  queue->addMaker(new RandomLocations({new CreatureAltarMaker(info.collective)}, {{1, 1}},
      {Predicate::type(SquareId::HILL)}));*/
  return queue;
}

LevelMaker* LevelMaker::mineTownLevel(SettlementInfo info) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareId::ROCK_WALL));
  queue->addMaker(mineTownMaker(info));
  return new BorderGuard(queue, SquareId::BLACK_WALL);
}

static void addResources(RandomLocations* locations, int count, int countHere, int minSize, int maxSize, int maxDist,
    int maxDist2, SquareType type, LevelMaker* hereMaker) {
  for (int i : Range(count)) {
    locations->add(new UniformBlob(type, none, SquareAttrib::NO_DIG),
        {Random.get(minSize, maxSize), Random.get(minSize, maxSize)}, 
        Predicate::type(SquareId::MOUNTAIN2));
    if (i < countHere)
      locations->setMaxDistanceLast(hereMaker, maxDist);
    else {
      locations->setMaxDistanceLast(hereMaker, maxDist2);
    }
  }
}

static SquareType getTreesType(SquareId id) {
  return {id, CreatureId::ENT};
}

LevelMaker* LevelMaker::topLevel(CreatureFactory forrestCreatures, vector<SettlementInfo> settlements) {
  MakerQueue* queue = new MakerQueue();
  vector<SquareType> vegetationLow {
      getTreesType(SquareId::CANIF_TREE), getTreesType(SquareId::BUSH) };
  vector<SquareType> vegetationHigh { getTreesType(SquareId::DECID_TREE), getTreesType(SquareId::BUSH) };
  vector<double> probs { 2, 1 };
  RandomLocations* locations = new RandomLocations();
  LevelMaker* startingPos = new StartingPos(Predicate::type(SquareId::HILL), StairKey::PLAYER_SPAWN);
  locations->add(startingPos, Vec2(4, 4), Predicate::type(SquareId::HILL));
  vector<LevelMaker*> cottages;
  for (SettlementInfo settlement : settlements) {
    LevelMaker* queue = nullptr;
    switch (settlement.type) {
      case SettlementType::VILLAGE: queue = village(settlement); break;
      case SettlementType::VILLAGE2: queue = village2(settlement); break;
      case SettlementType::CASTLE:
          queue = castle(settlement);
          break;
      case SettlementType::CASTLE2: queue = castle2(settlement); break;
      case SettlementType::COTTAGE:
          queue = cottage(settlement);
          cottages.push_back(queue);
          break;
      case SettlementType::WITCH_HOUSE:
          queue = cottage(settlement, {{ SquareId::CAULDRON, 2, 5}});
          break;
      case SettlementType::MINETOWN:
          queue = mineTownMaker(settlement);
          break;
      case SettlementType::SMALL_MINETOWN:
          queue = smallMineTownMaker(settlement); break;
          break;
      case SettlementType::VAULT:
          queue = vaultMaker(settlement, false);
          locations->setMaxDistance(startingPos, queue, 100);
          break;
      case SettlementType::ISLAND_VAULT:
          queue = islandVaultMaker(settlement, false);
          break;
      case SettlementType::CAVE:
          queue = dragonCaveMaker(settlement); break;
          break;
    }
    locations->setMinDistance(startingPos, queue, 70);
    locations->add(queue, getSize(settlement.type), getSettlementPredicate(settlement.type));
  }
  Predicate lowlandPred = Predicate::andPred(
      Predicate::attrib(SquareAttrib::LOWLAND),
      Predicate::negate(Predicate::attrib(SquareAttrib::RIVER)));
  for (LevelMaker* cottage : cottages)
    for (int __attribute__((unused)) i : Range(Random.get(1, 3))) {
      locations->add(new UniformBlob(SquareId::CROPS), {Random.get(7, 12), Random.get(7, 12)},
          lowlandPred);
      locations->setMaxDistanceLast(cottage, 13);
    }
 /* for (int i : Range(Random.get(1, 4)))
    locations->add(new Lake(), {20, 20}, Predicate::attrib(SquareAttrib::LOWLAND));*/
 for (int __attribute__((unused)) i : Range(Random.get(3, 6))) {
    locations->add(new UniformBlob(SquareId::WATER, none, SquareAttrib::LAKE), 
        {Random.get(5, 30), Random.get(5, 30)}, Predicate::type(SquareId::MOUNTAIN2));
 //   locations->setMaxDistanceLast(startingPos, i == 0 ? 25 : 60);
  }
  for (int __attribute__((unused)) i : Range(Random.get(3, 5))) {
    locations->add(new UniformBlob(SquareId::FLOOR, none), 
        {Random.get(5, 12), Random.get(5, 12)}, Predicate::type(SquareId::MOUNTAIN2));
 //   locations->setMaxDistanceLast(startingPos, i == 0 ? 25 : 40);
  }
  int maxDist = 35;
  int maxDist2 = 80;
  addResources(locations, Random.get(4, 6), 1, 5, 10, maxDist, maxDist2, SquareId::GOLD_ORE, startingPos);
  addResources(locations, Random.get(3, 6), 2, 5, 10, maxDist, maxDist2, SquareId::STONE, startingPos);
  addResources(locations, Random.get(7, 12), 4, 5, 10, maxDist, maxDist2, SquareId::IRON_ORE, startingPos);
  queue->addMaker(new Empty(SquareId::WATER));
  queue->addMaker(new Mountains({0.0, 0.0, 0.6, 0.68, 0.95}, 0.45, {0, 1, 0, 0, 0}, true,
        {SquareId::MOUNTAIN2, SquareId::MOUNTAIN2, SquareId::HILL, SquareId::GRASS, SquareId::SAND}));
  queue->addMaker(new AddAttrib(SquareAttrib::CONNECT_CORRIDOR, Predicate::attrib(SquareAttrib::LOWLAND)));
  queue->addMaker(new AddAttrib(SquareAttrib::CONNECT_CORRIDOR, Predicate::attrib(SquareAttrib::HILL)));
  queue->addMaker(new MountainRiver(2, SquareId::WATER, SquareId::SAND,
          Predicate::type(SquareId::MOUNTAIN2)));
  queue->addMaker(new Forrest(0.7, 0.5, SquareId::GRASS, vegetationLow, probs));
  queue->addMaker(new Forrest(0.4, 0.5, SquareId::HILL, vegetationHigh, probs));
  queue->addMaker(new Margin(10, locations));
  queue->addMaker(new Roads(SquareId::FLOOR));
  queue->addMaker(new Connector(5, 0, 5, Predicate::andPred(Predicate::canEnter({MovementTrait::WALK}),
          Predicate::attrib(SquareAttrib::CONNECT_CORRIDOR))));
  queue->addMaker(new Items(ItemFactory::mushrooms(), SquareId::GRASS, 30, 60));
  return new BorderGuard(queue);
}

LevelMaker* LevelMaker::pyramidLevel(optional<CreatureFactory> cfactory, vector<StairKey> up,
    vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareId::YELLOW_WALL));
  queue->addMaker(new AddAttrib(SquareAttrib::NO_DIG));
  queue->addMaker(new Margin(1, new RemoveAttrib(SquareAttrib::NO_DIG)));
  queue->addMaker(new RoomMaker(Random.get(2, 5), 4, 6, SquareId::YELLOW_WALL,
        SquareType(SquareId::YELLOW_WALL)));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, Predicate::type(SquareId::FLOOR)));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, Predicate::type(SquareId::FLOOR)));
  if (down.size() == 0)
    queue->addMaker(new LevelExit(SquareId::FLOOR, SquareAttrib::CONNECT_ROAD));
  if (cfactory)
    queue->addMaker(new Creatures(*cfactory, Random.get(3, 5), MonsterAIFactory::monster()));
  queue->addMaker(new Connector(1, 0));
  return queue;
}

LevelMaker* LevelMaker::cellarLevel(CreatureFactory cfactory, SquareType wallType, StairLook stairLook,
    vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  vector<FeatureInfo> featureCount {
      { {SquareId::CHEST, CreatureId::RAT}, 3, 7}};
  queue->addMaker(new Empty(SquareId::FLOOR));
  queue->addMaker(new Empty(wallType));
  queue->addMaker(new RoomMaker(Random.get(8, 15), 3, 5, wallType, wallType));
  queue->addMaker(new Connector(1, 0));
  queue->addMaker(new DungeonFeatures(Predicate::attrib(SquareAttrib::EMPTY_ROOM), featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, Predicate::type(SquareId::FLOOR), none,
          stairLook));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, Predicate::type(SquareId::FLOOR), none, stairLook));
  queue->addMaker(new Creatures(cfactory, Random.get(10, 15), MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareId::FLOOR, 5, 10));
  return new BorderGuard(queue, wallType);
}

LevelMaker* LevelMaker::cavernLevel(CreatureFactory cfactory, SquareType wallType, SquareType floorType,
    StairLook stairLook, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  vector<FeatureInfo> featureCount { 
      { {SquareId::CHEST, CreatureId::RAT}, 10, 14}};
  queue->addMaker(new Empty(wallType));
  queue->addMaker(new UniformBlob(floorType));
  queue->addMaker(new DungeonFeatures(Predicate::type(floorType), featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, Predicate::type(floorType), none,
          stairLook));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, Predicate::type(floorType), none, stairLook));
  queue->addMaker(new Creatures(cfactory, 1, MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), floorType, 10, 15));
  return new BorderGuard(queue, wallType);
}

LevelMaker* LevelMaker::grassAndTrees() {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareId::GRASS));
  queue->addMaker(new Forrest(0.8, 0.5, SquareId::GRASS, {getTreesType(SquareId::CANIF_TREE)}, {1}));
  return new BorderGuard(queue, SquareId::BLACK_WALL);
}

Vec2 LevelMaker::getRandomExit(Rectangle rect, int minCornerDist) {
  CHECK(rect.getW() > 2 * minCornerDist && rect.getH() > 2 * minCornerDist);
  int w1 = Random.get(2);
  int w2 = Random.get(2);
  int d1 = Random.get(minCornerDist, rect.getW() - minCornerDist);
  int d2 = Random.get(minCornerDist, rect.getH() - minCornerDist);
  return Vec2(
        rect.getPX() + d1 * w1 + (1 - w1) * w2 * (rect.getW() - 1),
        rect.getPY() + d2 * (1 - w1) + w1 * w2 * (rect.getH() - 1));
}

class SpecificArea : public LevelMaker {
  public:
  SpecificArea(Rectangle a, LevelMaker* m) : area(a), maker(m) {}

  virtual void make(Level::Builder* builder, Rectangle) override {
    maker->make(builder, area);
  }

  private:
  Rectangle area;
  LevelMaker* maker;
};

LevelMaker* LevelMaker::splashLevel(CreatureFactory heroLeader, CreatureFactory heroes, CreatureFactory monsters,
    CreatureFactory imps, const string& splashPath) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareId::BLACK_FLOOR));
  queue->addMaker(new SetCovered());
  Rectangle leaderSpawn(
          Level::getSplashVisibleBounds().getKX() + 1, Level::getSplashVisibleBounds().middle().y,
          Level::getSplashVisibleBounds().getKX() + 2, Level::getSplashVisibleBounds().middle().y + 1);
  Rectangle heroSpawn(
          Level::getSplashVisibleBounds().getKX() + 2, Level::getSplashVisibleBounds().middle().y - 1,
          Level::getSplashBounds().getKX(), Level::getSplashVisibleBounds().middle().y + 2);
  Rectangle monsterSpawn1(
          Level::getSplashVisibleBounds().getPX(), 0,
          Level::getSplashVisibleBounds().getKX(), Level::getSplashVisibleBounds().getPY() - 1);
  Rectangle monsterSpawn2(
          Level::getSplashVisibleBounds().getPX(), Level::getSplashVisibleBounds().getKY() + 2,
          Level::getSplashVisibleBounds().getKX(), Level::getSplashBounds().getKY());
  queue->addMaker(new SpecificArea(leaderSpawn, new Creatures(heroLeader, 1,MonsterAIFactory::splashHeroes(true))));
  queue->addMaker(new SpecificArea(heroSpawn, new Creatures(heroes, 22, MonsterAIFactory::splashHeroes(false))));
  queue->addMaker(new SpecificArea(monsterSpawn1, new Creatures(monsters, 17, MonsterAIFactory::splashMonsters())));
  queue->addMaker(new SpecificArea(monsterSpawn2, new Creatures(monsters, 17, MonsterAIFactory::splashMonsters())));
  queue->addMaker(new SpecificArea(monsterSpawn1, new Creatures(imps, 15,
          MonsterAIFactory::splashImps(splashPath))));
  queue->addMaker(new SpecificArea(monsterSpawn2, new Creatures(imps, 15,
          MonsterAIFactory::splashImps(splashPath))));
  return new BorderGuard(queue, SquareId::BLACK_WALL);
}

