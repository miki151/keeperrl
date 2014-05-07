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
#include "quest.h"




class SquarePredicate {
  public:
  virtual bool apply(Level::Builder* builder, Vec2 pos) = 0;
};

class AttribPredicate : public SquarePredicate {
  public:
  AttribPredicate(SquareAttrib attr) : onAttr(attr) {}

  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    return builder->hasAttrib(pos, onAttr);
  }

  protected:
  SquareAttrib onAttr;
};

class NoAttribPredicate : public AttribPredicate {
  using AttribPredicate::AttribPredicate;

  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    return !builder->hasAttrib(pos, onAttr);
  }
};

class TypePredicate : public SquarePredicate {
  public:
  TypePredicate(SquareType type) : onType(1, type) {}

  TypePredicate(vector<SquareType> types) : onType(types) {}

  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    return contains(onType, builder->getType(pos));
  }

  private:
  vector<SquareType> onType;
};

class BorderPredicate : public SquarePredicate {
  public:
  BorderPredicate(SquareType type, SquareType neighbour) : onType(type), neighbourType(neighbour) {}

  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    bool okNeighbour = false;
    for (Vec2 v : pos.neighbors4())
      if (builder->getType(v) == neighbourType) {
        okNeighbour = true;
        break;
      }
    return okNeighbour && onType == builder->getType(pos);
  }

  private:
  SquareType onType;
  SquareType neighbourType;
};

class AlwaysTrue : public SquarePredicate {
  public:
  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    return true;
  }
};

class AndPredicates : public SquarePredicate {
  public:
  AndPredicates(SquarePredicate* a1, SquarePredicate* a2) : p1(a1), p2(a2) {}

  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    return p1->apply(builder, pos) && p2->apply(builder, pos);
  }

  private:
  SquarePredicate* p1;
  SquarePredicate* p2;
};

class RoomMaker : public LevelMaker {
  public:
  RoomMaker(int _minRooms,int _maxRooms,
            int _minSize, int _maxSize, 
            SquareType _wallType,
            Optional<SquareType> _onType,
            LevelMaker* _shopMaker = nullptr,
            bool _diggableCorners = false) : 
      minRooms(_minRooms),
      maxRooms(_maxRooms),
      minSize(_minSize),
      maxSize(_maxSize),
      wallType(_wallType),
      squareType(_onType),
      shopMaker(_shopMaker),
      diggableCorners(_diggableCorners) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int spaceBetween = 0;
    Table<int> taken(area.getKX(), area.getKY());
    for (Vec2 v : area)
      taken[v] = squareType && *squareType != builder->getType(v);
    int rooms = Random.getRandom(minRooms, maxRooms);
    int numShop = shopMaker ? Random.getRandom(rooms) : -1;
    for (int i : Range(rooms)) {
      Vec2 p, k;
      bool good;
      int cnt = 100;
      do {
        k = Vec2(Random.getRandom(minSize, maxSize), Random.getRandom(minSize, maxSize));
        p = Vec2(area.getPX() + spaceBetween + Random.getRandom(area.getW() - k.x - 2 * spaceBetween),
                 area.getPY() + spaceBetween + Random.getRandom(area.getH() - k.y - 2 * spaceBetween));
        good = true;
        for (Vec2 v : Rectangle(k.x + 2 * spaceBetween, k.y + 2 * spaceBetween))
          if (taken[p + v - Vec2(spaceBetween,spaceBetween)]) {
            good = false;
            break;
          }
      } while (!good && --cnt > 0);
      if (cnt == 0) {
        Debug() << "Placed only " << i << " rooms out of " << rooms;
        break;
      }
      for (Vec2 v : Rectangle(k))
        taken[v + p] = 1;
      for (Vec2 v : Rectangle(k - Vec2(2, 2)))
        builder->putSquare(p + v + Vec2(1, 1), SquareType::FLOOR, SquareAttrib::ROOM);
      for (int i : Range(p.x, p.x + k.x)) {
        if (i == p.x || i == p.x + k.x - 1) {
          builder->putSquare(Vec2(i, p.y), wallType,
              !diggableCorners ? Optional<SquareAttrib>(SquareAttrib::NO_DIG) : Nothing());
          builder->putSquare(Vec2(i, p.y + k.y - 1), wallType,
              !diggableCorners ? Optional<SquareAttrib>(SquareAttrib::NO_DIG) : Nothing());
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
      if (i == numShop)
        shopMaker->make(builder, inside);
      else
        for (Vec2 v : inside)
          builder->addAttrib(v, SquareAttrib::EMPTY_ROOM);
    }
  }

  private:
  int minRooms;
  int maxRooms;
  int minSize;
  int maxSize;
  SquareType wallType;
  Optional<SquareType> squareType;
  LevelMaker* shopMaker;
  bool diggableCorners;
};

class Connector : public LevelMaker {
  public:
  Connector(initializer_list<double> door, double _diggingCost = 3) : doorProb(door), diggingCost(_diggingCost) {
    CHECKEQ((int) door.size(), 3);
  }
  double getValue(Level::Builder* builder, Vec2 pos, Rectangle area) {
    if (builder->getSquare(pos)->canEnter(Creature::getDefault()))
      return 1;
    if (builder->hasAttrib(pos, SquareAttrib::LAKE))
      return 15;
    if (builder->hasAttrib(pos, SquareAttrib::RIVER))
      return 5;
    if (builder->hasAttrib(pos, SquareAttrib::NO_DIG))
      return ShortestPath::infinity;
    int numCorners = 0;
    int numTotal = 0;
    for (Vec2 v : Vec2::directions8())
      if ((pos + v).inRectangle(area) && builder->getSquare(pos + v)->canEnter(Creature::getDefault())) {
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
      if (!builder->getSquare(v)->canEnter(Creature::getDefault())) {
        char sym;
        SquareType newType = SquareType(0);
        SquareType oldType = builder->getType(v);
        if (isWall(oldType) && oldType != SquareType::BLACK_WALL)
          newType = chooseRandom({
              SquareType::PATH,
              SquareType::DOOR,
              SquareType::SECRET_PASS}, doorProb);
        else
          switch (oldType) {
            case SquareType::ABYSS:
            case SquareType::WATER:
            case SquareType::MAGMA:
              newType = SquareType::BRIDGE;
              break;
            case SquareType::BLACK_WALL:
              newType = SquareType::PATH;
              break;
            default:
              FAIL << "Unhandled square type " << (int)builder->getType(v);
          }
        if (newType == SquareType::DOOR)
          builder->putSquare(v, SquareType::FLOOR);
        builder->putSquare(v, newType);
      }
      if (builder->getType(v) == SquareType::PATH || 
          builder->getType(v) == SquareType::BRIDGE || 
          builder->getType(v) == SquareType::DOOR || 
          builder->getType(v) == SquareType::SECRET_PASS) {
        builder->getSquare(v)->addTravelDir(path.getNextMove(v) - v);
        if (prev.x > -100)
          builder->getSquare(v)->addTravelDir(prev - v);
      }
      prev = v;
    }
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int dead_end = 1000000;
    Vec2 p1, p2;
    for (int i : Range(30)) {
      do {
        p1 = Vec2(Random.getRandom(area.getPX(), area.getKX()), Random.getRandom(area.getPY(), area.getKY()));
      } while (!builder->getSquare(p1)->canEnter(Creature::getDefault()) && !Random.roll(dead_end));
      do {
        p2 = Vec2(Random.getRandom(area.getPX(), area.getKX()), Random.getRandom(area.getPY(), area.getKY()));
      } while (!builder->getSquare(p2)->canEnter(Creature::getDefault()) && !Random.roll(dead_end));
      connect(builder, p1, p2, area);
    }
    ShortestPath connections(area,
        [builder](Vec2 pos)->double {
            if (builder->getSquare(pos)->canEnter(Creature::getDefault()))
              return 1;
            else
            return ShortestPath::infinity;},
        [] (Vec2 v) { return v.length4(); },
        Vec2::directions8(), p1, p2);
    for (Vec2 v : area) 
      if (builder->getSquare(v)->canEnter(Creature::getDefault()) && !connections.isReachable(v)) {
        connect(builder, p1, v, area);
      }
  }
  
  private:
  initializer_list<double> doorProb;
  double diggingCost;
};

class DungeonFeatures : public LevelMaker {
  public:
  DungeonFeatures(SquarePredicate* pred, map<SquareType, pair<int, int> > _squareTypes,
      Optional<SquareAttrib> setAttr = Nothing()): 
      squareTypes(_squareTypes), predicate(pred), attr(setAttr) {
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> available;
    for (Vec2 v : area)
      if (predicate->apply(builder, v))
        available.push_back(v);
    int maxTotal = 0;
    for (auto iter : squareTypes) {
      maxTotal += iter.second.second - 1;
    }
    CHECK(maxTotal <= available.size()) << "Not enough available squares " << (int)available.size() 
        << " for " << maxTotal << " features.";
    for (auto iter : squareTypes) {
      int num = Random.getRandom(iter.second.first, iter.second.second);
      for (int i : Range(num)) {
        int vInd = Random.getRandom(available.size());
        builder->putSquare(available[vInd], iter.first);
        if (attr)
          builder->addAttrib(available[vInd], *attr);
        removeIndex(available, vInd);
      }
    }
  }

  private:
  map<SquareType, pair<int, int> > squareTypes;
  SquarePredicate* predicate;
  Optional<SquareAttrib> attr;
};

class Creatures : public LevelMaker {
  public:
  Creatures(CreatureFactory cf, int numC, MonsterAIFactory actorF, Optional<SquareType> type = Nothing()) :
      cfactory(cf), numCreature(numC), actorFactory(actorF), squareType(type) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<char> taken(area.getKX(), area.getKY());
    for (int i : Range(numCreature)) {
      PCreature creature = cfactory.random(actorFactory);
      Vec2 pos;
      int numTries = 100;
      do {
        pos = Vec2(Random.getRandom(area.getPX(), area.getKX()), Random.getRandom(area.getPY(), area.getKY()));
      } while (--numTries > 0 && (!builder->canPutCreature(pos, creature.get())
          || (squareType && builder->getType(pos) != *squareType)));
      CHECK(numTries > 0) << "Failed to find square for creature";
      builder->putCreature(pos, std::move(creature));
      taken[pos] = 1;
    }
  }

  private:
  CreatureFactory cfactory;
  int numCreature;
  MonsterAIFactory actorFactory;
  Optional<SquareType> squareType;
};

class Items : public LevelMaker {
  public:
  Items(ItemFactory _factory, SquareType _onType, int minc, int maxc) : 
      factory(_factory), onType(_onType), minItem(minc), maxItem(maxc) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int numItem = Random.getRandom(minItem, maxItem);
    for (int i : Range(numItem)) {
      Vec2 pos;
      do {
        pos = Vec2(Random.getRandom(area.getPX(), area.getKX()), Random.getRandom(area.getPY(), area.getKY()));
      } while (builder->getType(pos) != onType);
      builder->getSquare(pos)->dropItems(factory.random());
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
    int px = Random.getRandom(middle - wind, middle + width);
    int kx = px + Random.getRandom(-wind, wind); // Random.getRandom(area.getPX(), area.getKX()) - width;
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
      kx = px + Random.getRandom(-wind, wind);
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

/*class MountainRiver : public LevelMaker {
  public:
  MountainRiver(int num, char w) : number(num), water(w) {}
  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (int i : Range(number)) {
      Vec2 pos(Random.getRandom(area.getPX(), area.getKX()),
             Random.getRandom(area.getPY(), area.getKY()));
      while (1) {
        builder->putSquare(pos, SquareFactory::fromSymbol(water), SquareType::RIVER);
        double h = builder->getHeightMap(pos);
        double lowest = 10000000;
        Vec2 dir;
        for (Vec2 v : Vec2::neighbors8(true)) {
          double d;
          if ((pos + v).inRectangle(area) && (d = builder->getHeightMap(pos + v)) < lowest && builder->getType(pos + v) != SquareType::RIVER) {
            lowest = d;
            dir = v;
          }
        }
        if (lowest > 100000)
          break;;
       // if (lowest < h) {
          pos = pos + dir;
       // } else
       //   break;
      }
    }
  }

  private:
  int number;
  char water;
};*/

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
      Vec2 pos = squares[Random.getRandom(squares.size())];
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
  UniformBlob(SquareType insideSquare, Optional<SquareType> borderSquare = Nothing(),
      Optional<SquareAttrib> _attrib = Nothing(), double insideRatio = 0.3333) : Blob(insideRatio),
      inside(insideSquare), border(borderSquare), attrib(_attrib) {}

  virtual void addSquare(Level::Builder* builder, Vec2 pos, int edgeDist) override {
    if (edgeDist == 1 && border)
      builder->putSquare(pos, *border, attrib);
    else
      builder->putSquare(pos, inside, attrib);
  }

  private:
  SquareType inside;
  Optional<SquareType> border;
  Optional<SquareAttrib> attrib;
};

class Lake : public Blob {
  public:
  virtual void addSquare(Level::Builder* builder, Vec2 pos, int edgeDist) override {
    if (edgeDist == 1)
      builder->putSquare(pos, SquareType::SAND, SquareAttrib::LAKE);
    else
      builder->putSquare(pos, SquareFactory::getWater(double(edgeDist) / 2), SquareType::WATER, SquareAttrib::LAKE);
  }
};

class Empty : public LevelMaker {
  public:
  Empty(SquareType s, Optional<SquareAttrib> attr = Nothing()) : square(s), attrib(attr) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area) {
      builder->putSquare(v, square, attrib);
    }
  }

  private:
  SquareType square;
  Optional<SquareAttrib> attrib;
};

Vec2 LevelMaker::getRandomExit(Rectangle rect, int minCornerDist) {
  CHECK(rect.getW() > 2 * minCornerDist && rect.getH() > 2 * minCornerDist);
  int w1 = Random.getRandom(2);
  int w2 = Random.getRandom(2);
  int d1 = Random.getRandom(minCornerDist, rect.getW() - minCornerDist);
  int d2 = Random.getRandom(minCornerDist, rect.getH() - minCornerDist);
  return Vec2(
        rect.getPX() + d1 * w1 + (1 - w1) * w2 * (rect.getW() - 1),
        rect.getPY() + d2 * (1 - w1) + w1 * w2 * (rect.getH() - 1));
}

struct BuildingInfo {
  SquareType wall;
  SquareType floorInside;
  SquareType floorOutside;
  SquareType door;
};

BuildingInfo woodBuilding { SquareType::WOOD_WALL, SquareType::FLOOR, SquareType::GRASS, SquareType::DOOR};
BuildingInfo woodCastleBuilding { SquareType::WOOD_WALL, SquareType::FLOOR, SquareType::MUD, SquareType::DOOR};
BuildingInfo mudBuilding { SquareType::MUD_WALL, SquareType::MUD, SquareType::MUD, SquareType::MUD};
BuildingInfo brickBuilding { SquareType::CASTLE_WALL, SquareType::FLOOR, SquareType::MUD, SquareType::DOOR};

static BuildingInfo get(BuildingId id) {
  switch (id) {
    case BuildingId::WOOD: return woodBuilding;
    case BuildingId::WOOD_CASTLE: return woodCastleBuilding;
    case BuildingId::MUD: return mudBuilding;
    case BuildingId::BRICK: return brickBuilding;
  }
  FAIL << "ref";
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
    int sizeVar = 1;
    int spaceBetween = 1;
    int alignHeight = 0;
    if (align) {
      alignHeight = height / 2 - 2 + Random.getRandom(5);
    }
    int nextw = -1;
    int numBuildings = Random.getRandom(minBuildings, maxBuildings);
    for (int i = 0; i < numBuildings; ++i) {
      bool spaceOk = true;
      int w, h, px, py;
      int cnt = 10000;
      bool buildingRow;
      do {
        buildingRow = Random.getRandom(2);
        spaceOk = true;
        w = Random.getRandom(minSize, maxSize);
        h = Random.getRandom(minSize, maxSize);
        if (nextw > -1 && nextw + w < area.getKX()) {
          px = nextw;
          nextw = -1;
        } else
          px = area.getPX() + Random.getRandom(width - w - 2 * spaceBetween + 1) + spaceBetween;
        if (!align)
          py = area.getPY() + Random.getRandom(height - h - 2 * spaceBetween + 1) + spaceBetween;
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
          FAIL << "Failed to add " << minBuildings - i << " buildings out of " << minBuildings;
        else
          break;
      }
      if (Random.roll(1))
        nextw = px + w;
      for (Vec2 v : Rectangle(w + 1, h + 1)) {
        filled[Vec2(px, py) + v] = true;
        builder->putSquare(Vec2(px, py) + v, building.wall);
        builder->setCovered(Vec2(px, py) + v);
      }
      for (Vec2 v : Rectangle(w - 1, h - 1)) {
        builder->putSquare(Vec2(px + 1, py + 1) + v, building.floorInside, SquareAttrib::ROOM);
      }
      Vec2 doorLoc = align ? 
          Vec2(px + Random.getRandom(1, w),
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
          SquareType::ROAD, SquareAttrib::CONNECT);
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

  BorderGuard(LevelMaker* inside, SquareType _type = SquareType::BORDER_GUARD) : type(_type), insideMaker(inside) {}

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

class RandomLocations : public LevelMaker {
  public:
  RandomLocations(const vector<LevelMaker*>& _insideMakers, const vector<pair<int, int>>& _sizes,
      SquarePredicate* pred, bool _separate = true, map<pair<LevelMaker*, LevelMaker*>, double> _minDistance = {},
      map<pair<LevelMaker*, LevelMaker*>, double> _maxDistance = {})
      : insideMakers(_insideMakers), sizes(_sizes), predicate(sizes.size(), pred), separate(_separate),
        minDistance(_minDistance), maxDistance(_maxDistance) {
        CHECK(insideMakers.size() == sizes.size());
      }

  RandomLocations(const vector<LevelMaker*>& _insideMakers, const vector<pair<int, int>>& _sizes,
      const vector<SquarePredicate*> pred, bool _separate = true,
      map<pair<LevelMaker*, LevelMaker*>, double> _minDistance = {},
      map<pair<LevelMaker*, LevelMaker*>, double> _maxDistance = {})
      : insideMakers(_insideMakers), sizes(_sizes), predicate(pred), separate(_separate), minDistance(_minDistance),
        maxDistance(_maxDistance) {
        CHECK(insideMakers.size() == sizes.size());
        CHECK(pred.size() == sizes.size());
      }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    makeCnt(builder, area, 100);
  }

  void makeCnt(Level::Builder* builder, Rectangle area, int tries) {
    vector<Rectangle> occupied;
    vector<Rectangle> makerBounds;
    vector<Level::Builder::Rot> maps;
    for (int i : All(insideMakers))
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
        px = area.getPX() + Random.getRandom(area.getW() - width);
        py = area.getPY() + Random.getRandom(area.getH() - height);
        for (int j : Range(i))
          if ((maxDistance.count({insideMakers[j], insideMakers[i]}) && 
                maxDistance[{insideMakers[j], insideMakers[i]}] <
                  Vec2(px + width / 2, py + height / 2).dist8(occupied[j].middle())) ||
              minDistance[{insideMakers[j], insideMakers[i]}] >
                  Vec2(px + width / 2, py + height / 2).dist8(occupied[j].middle())) {
            ok = false;
            break;
          }
        if (!predicate[i]->apply(builder, Vec2(px, py)) ||
            !predicate[i]->apply(builder, Vec2(px + width - 1, py)) || 
            !predicate[i]->apply(builder, Vec2(px + width - 1, py + height - 1)) || 
            !predicate[i]->apply(builder, Vec2(px, py + height - 1))) 
          ok = false;
        else
        if (separate)
        for (Rectangle r : occupied)
          if (r.intersects(Rectangle(px, py, px + width, py + height))) {
            ok = false;
            break;
          }
      } while (!ok && --cnt > 0);
      if (cnt == 0) {
        if (tries > 0) {
          makeCnt(builder, area, tries - 1);
          return;
        }
        else {
          FAIL << "Failed to find free space for " << (int)sizes.size() << " areas";
        }
      }
      occupied.push_back(Rectangle(px, py, px + width, py + height));
      makerBounds.push_back(Rectangle(px, py, px + sizes[i].first, py + sizes[i].second));
    }
    CHECK(insideMakers.size() == occupied.size());
    for (int i : All(insideMakers)) {
      builder->pushMap(makerBounds[i], maps[i]);
      insideMakers[i]->make(builder, makerBounds[i]);
      builder->popMap();
    }
  }

  private:
  vector<LevelMaker*> insideMakers;
  vector<pair<int, int>> sizes;
  vector<SquarePredicate*> predicate;
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
  Shrine(Deity* _deity, SquareType _floorType, SquarePredicate* wallPred,
      SquareType _newWall, LevelMaker* _locationMaker) : deity(_deity), floorType(_floorType),
      wallPredicate(wallPred), newWall(_newWall), locationMaker(_locationMaker) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (int i : Range(10009)) {
      Vec2 pos = area.randomVec2();
      if (!wallPredicate->apply(builder, pos))
        continue;
      int numFloor = 0;
      int numWall = 0;
      for (Vec2 fl : pos.neighbors8()) {
        if (builder->getType(fl) == floorType)
          ++numFloor;
        if (wallPredicate->apply(builder, fl))
          ++numWall;
      }
      if (numFloor < 3 || numWall < 5) {
        continue;
      }
      builder->putSquare(pos, floorType);
      builder->putSquare(pos, SquareFactory::getAltar(deity), SquareType::ALTAR);
      for (Vec2 fl : pos.neighbors8())
        if (wallPredicate->apply(builder, fl))
          builder->putSquare(fl, newWall);
      if (locationMaker)
        locationMaker->make(builder, Rectangle(pos - Vec2(1, 1), pos + Vec2(2, 2)));
      Debug() << "Created a shrine of " << deity->getHabitatString();
      return;
    }
    Debug() << "Didn't find a good place for the shrine of " << deity->getHabitatString();
  }

  private:
  Deity* deity;
  SquareType floorType;
  SquarePredicate* wallPredicate;
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
  double heightDiff = 0.1;
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

vector<double> sortedValues(const Table<double>& t) {
  vector<double> values;
  for (Vec2 v : t.getBounds()) {
    values.push_back(t[v]);
  }
  sort(values.begin(), values.end());
/*  vector<double> tmp(values);
  unique(tmp.begin(), tmp.end());
    string out;
    for (double d : values)
      out.append(convertToString(d) + " ");
    Debug() << (int)tmp.size() << " unique values out of " << (int)values.size() << " " << out;*/
  return values;
}

class Mountains : public LevelMaker {
  public:
  Mountains(double r, double varianceM, vector<int> _cornerLevels, bool _makeDark = true,
      vector<SquareType> _types = {SquareType::GLACIER, SquareType::MOUNTAIN, SquareType::HILL}) 
      : ratio(r), cornerLevels(_cornerLevels), types(_types), makeDark(_makeDark), varianceMult(varianceM) {}


  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(area, cornerLevels, varianceMult);
    vector<double> values = sortedValues(wys);
    double cutOffValHill = values[(int)(ratio * double(values.size()))];
    double cutOffVal = values[(int)((0.5 + ratio) / 1.5 * double(values.size()))];
    double cutOffValSnow = values[(int)((3. + ratio) / 4. * double(values.size()))];
    int gCnt = 0, mCnt = 0, hCnt = 0, lCnt = 0, fCnt = 0;
    for (Vec2 v : area) {
      builder->setHeightMap(v, wys[v]);
      if (wys[v] > cutOffValSnow) {
        builder->putSquare(v, types[2]);
        builder->putSquare(v, types[0], {SquareAttrib::GLACIER, SquareAttrib::ROAD_CUT_THRU});
        if (makeDark)
          builder->setDark(v, 1.0);
        ++gCnt;
      }
      else if (wys[v] > cutOffVal) {
        builder->putSquare(v, types[2]);
        builder->putSquare(v, types[1], {SquareAttrib::MOUNTAIN, SquareAttrib::ROAD_CUT_THRU});
        if (makeDark)
          builder->setDark(v, (wys[v] - cutOffVal) / (cutOffValSnow - cutOffVal));
        ++mCnt;
      }
      else if (wys[v] > cutOffValHill) {
        builder->putSquare(v, types[2], SquareAttrib::HILL);
        ++hCnt;
      }
      else {
        builder->addAttrib(v, SquareAttrib::LOWLAND);
        ++lCnt;
      }
    }
    Debug() << "Terrain distribution " << gCnt << " glacier, " << mCnt << " mountain, " << hCnt << " hill, " << lCnt << " lowland, " << fCnt << " fog";
  }

  private:
  double ratio;
  vector<int> cornerLevels;
  vector<SquareType> types;
  bool makeDark;
  double varianceMult;
};

class Roads : public LevelMaker {
  public:
  Roads(SquareType roadSquare) : square(roadSquare) {}

  double getValue(Level::Builder* builder, Vec2 pos) {
    if ((!builder->getSquare(pos)->canEnter(Creature::getDefault()) && 
         !builder->hasAttrib(pos, SquareAttrib::ROAD_CUT_THRU)) ||
        builder->hasAttrib(pos, SquareAttrib::NO_ROAD))
      return ShortestPath::infinity;
    SquareType type = builder->getType(pos);
    if (contains({SquareType::ROAD, SquareType::ROAD}, type))
      return 1;
    return 1 + pow(1 + builder->getHeightMap(pos), 2);
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> points;
    for (Vec2 v : area)
      if (builder->hasAttrib(v, SquareAttrib::CONNECT)) {
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
        SquareType roadType = SquareType::ROAD;
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

  StartingPos(SquarePredicate* pred, StairKey key) : predicate(pred), stairKey(key) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 pos : area)
      if (predicate->apply(builder, pos))
        builder->getSquare(pos)->setLandingLink(StairDirection::UP, stairKey);
  }

  private:
  SquarePredicate* predicate;
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
  ForEachSquare(function<void(Level::Builder*, Vec2 pos)> f,
      SquarePredicate* _onPred = nullptr) : fun(f), onPred(_onPred) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area)
      if (!onPred || onPred->apply(builder, v))
        fun(builder, v);
  }
  
  protected:
  function<void(Level::Builder*, Vec2 pos)> fun;
  SquarePredicate* onPred;
};

class AddAttrib : public ForEachSquare {
  public:
  AddAttrib(SquareAttrib attr, SquarePredicate* onPred = nullptr) 
      : ForEachSquare([attr](Level::Builder* b, Vec2 pos) { b->addAttrib(pos, attr); }, onPred) {}
};

class RemoveAttrib : public ForEachSquare {
  public:
  RemoveAttrib(SquareAttrib attr, SquarePredicate* onPred = nullptr) 
    : ForEachSquare([attr](Level::Builder* b, Vec2 pos) { b->removeAttrib(pos, attr); }, onPred) {}
};

class SetCovered : public ForEachSquare {
  public:
  SetCovered(SquarePredicate* onPred = nullptr)
    : ForEachSquare([](Level::Builder* b, Vec2 pos) { b->setCovered(pos); }, onPred) {}
};

class SetDark : public ForEachSquare {
  public:
  SetDark(SquarePredicate* onPred = nullptr)
    : ForEachSquare([](Level::Builder* b, Vec2 pos) { b->setDark(pos, 1.0); }, onPred) {}
};

/*class FlockAndLeader : public LevelMaker {
  public:
  FlockAndLeader(CreatureId _leader, CreatureId _flock, Tribe* _tribe, int _minFlock, int _maxFlock) :
      leaderId(_leader), flockId(_flock), tribe(_tribe), minFlock(_minFlock), maxFlock(_maxFlock) {
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int num = Random.getRandom(minFlock, maxFlock);
    PCreature leader = CreatureFactory::fromId(leaderId, tribe);
    vector<PCreature> creatures = CreatureFactory::getFlock(num, flockId, leader.get());
    creatures.push_back(std::move(leader));
    for (PCreature& creature : creatures) {
      Vec2 pos;
      int cnt = 100;
      do {
        pos = area.randomVec2();
      } while (!builder->canPutCreature(pos, creature.get()) && --cnt > 0);
      CHECK(cnt > 0) << "Can't find square for flock";
      builder->putCreature(pos, std::move(creature));
    }
  }

  private:
  CreatureId leaderId, flockId;
  Tribe* tribe;
  int minFlock, maxFlock;
};*/

class Stairs : public LevelMaker {
  public:
  Stairs(StairDirection dir, StairKey k, SquarePredicate* onPred, Optional<SquareAttrib> _setAttr = Nothing(),
      StairLook look = StairLook::NORMAL) : direction(dir), key(k), onPredicate(onPred), setAttr(_setAttr),
      stairLook(look) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> pos;
    for (Vec2 v : area)
      if (onPredicate->apply(builder, v))
        pos.push_back(v);
    CHECK(pos.size() > 0) << "Couldn't find position for stairs " << area;
    SquareType type = direction == StairDirection::DOWN ? SquareType::DOWN_STAIRS : SquareType::UP_STAIRS;
    builder->putSquare(pos[Random.getRandom(pos.size())], SquareFactory::getStairs(direction, key, stairLook),
        type, setAttr);
  }

  private:
  StairDirection direction;
  StairKey key;
  SquarePredicate* onPredicate;
  Optional<SquareAttrib> setAttr;
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
    builder->putCreature(pos[Random.getRandom(pos.size())], std::move(shopkeeper));
    for (int i : Range(numItems)) {
      Vec2 v = pos[Random.getRandom(pos.size())];
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
  LevelExit(SquareType _exitType, Optional<SquareAttrib> _attrib = Nothing(), int _minCornerDist = 1)
      : exitType(_exitType), attrib(_attrib), minCornerDist(_minCornerDist) {}
  
  virtual void make(Level::Builder* builder, Rectangle area) override {
    builder->putSquare(getRandomExit(area, minCornerDist), exitType, attrib);
  }

  private:
  SquareType exitType;
  Optional<SquareAttrib> attrib;
  int minCornerDist;
};

class Division : public LevelMaker {
  public:
  Division(double _vRatio, double _hRatio,
      LevelMaker* _upperLeft, LevelMaker* _upperRight, LevelMaker* _lowerLeft, LevelMaker* _lowerRight,
      Optional<SquareType> _wall = Nothing()) : vRatio(_vRatio), hRatio(_hRatio),
      upperLeft(_upperLeft), upperRight(_upperRight), lowerLeft(_lowerLeft), lowerRight(_lowerRight), wall(_wall) {}

  Division(double _hRatio, LevelMaker* _left, LevelMaker* _right, Optional<SquareType> _wall = Nothing()) 
      : vRatio(-1), hRatio(_hRatio), upperLeft(_left), upperRight(_right), wall(_wall) {}

  Division(bool, double _vRatio, LevelMaker* _top, LevelMaker* _bottom, Optional<SquareType> _wall = Nothing()) 
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
  Optional<SquareType> wall;
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
  AreaCorners(LevelMaker* _maker, Vec2 _size) : maker(_maker), size(_size) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    maker->make(builder, Rectangle(area.getTopLeft(), area.getTopLeft() + size));
    maker->make(builder, Rectangle(area.getTopRight() - Vec2(size.x, 0), area.getTopRight() + Vec2(0, size.y)));
    maker->make(builder, Rectangle(area.getBottomLeft() - Vec2(0, size.y), area.getBottomLeft() + Vec2(size.x, 0)));
    maker->make(builder, Rectangle(area.getBottomRight() - size, area.getBottomRight()));
  }

  private:
  LevelMaker* maker;
  Vec2 size;
};

class CastleExit : public LevelMaker {
  public:
  CastleExit(Tribe* _guardTribe, BuildingInfo _building, CreatureId _guardId)
    : guardTribe(_guardTribe), building(_building), guardId(_guardId) {}
  
  virtual void make(Level::Builder* builder, Rectangle area) override {
    Vec2 loc(area.getKX() - 1, area.middle().y);
 //   builder->putSquare(loc, SquareType::DOOR);
    builder->putSquare(loc + Vec2(2, 0), building.floorInside);
    builder->putSquare(loc + Vec2(2, 0), building.door, SquareAttrib::CONNECT);
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

static LevelMaker* underground(bool monsters) {
  MakerQueue* queue = new MakerQueue();
  if (Random.roll(1)) {
    LevelMaker* cavern = new UniformBlob(SquareType::PATH);
    vector<LevelMaker*> vCavern;
    vector<pair<int, int>> sizes;
    int minSize = Random.getRandom(5, 15);
    int maxSize = minSize + Random.getRandom(3, 10);
    for (int i : Range(sqrt(Random.getRandom(4, 100)))) {
      int size = Random.getRandom(minSize, maxSize);
      sizes.push_back(make_pair(size, size));
      MakerQueue* queue = new MakerQueue();
      queue->addMaker(cavern);
   /*   if (Random.roll(4))
        queue->addMaker(new Items(ItemFactory::mushrooms(), SquareType::PATH, 2, 5));*/
      vCavern.push_back(queue);
    }
    queue->addMaker(new RandomLocations(vCavern, sizes, new AlwaysTrue(), false));
  }
  switch (Random.getRandom(3)) {
    case 1: queue->addMaker(new River(3, chooseRandom({SquareType::WATER, SquareType::MAGMA})));
            break;
    case 2:{
          int numLakes = sqrt(Random.getRandom(1, 100));
          SquareType lakeType = chooseRandom({SquareType::WATER, SquareType::MAGMA}, {1, 1});
          vector<pair<int, int>> sizes;
          for (int i : Range(numLakes)) {
            int size = Random.getRandom(6, 20);
            sizes.emplace_back(size, size);
          }
          queue->addMaker(new RandomLocations(
          vector<LevelMaker*>(numLakes, new UniformBlob(lakeType, Nothing(), SquareAttrib::LAKE)),
          sizes, new AlwaysTrue(), false));
          if (monsters) {
            Deity* deity = Deity::getDeity(
                (lakeType == SquareType::MAGMA) ? DeityHabitat::FIRE : DeityHabitat::WATER);
            queue->addMaker(new Shrine(deity, lakeType,
                  new TypePredicate(lakeType), SquareType::ROCK_WALL, nullptr));
            if (lakeType == SquareType::WATER) {
              queue->addMaker(new Creatures(CreatureFactory::singleType(Tribes::get(TribeId::MONSTER),
                    CreatureId::KRAKEN), 1, MonsterAIFactory::monster(), SquareType::WATER));
            }
            if (lakeType == SquareType::MAGMA) {
              queue->addMaker(new Creatures(CreatureFactory::singleType(Tribes::get(TribeId::MONSTER),
                    CreatureId::FIRE_SPHERE), Random.getRandom(1, 4),
                    MonsterAIFactory::monster(), SquareType::MAGMA));
            }
          }
          break;
          }
    default: break;
  }
  queue->addMaker(new SetDark());
  return queue;
}

LevelMaker* LevelMaker::roomLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::FOUNTAIN, make_pair(0, 3) },
      { SquareType::CHEST, make_pair(3, 7)},
      { SquareType::TORTURE_TABLE, make_pair(2, 3)}};
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::BLACK_WALL));
  queue->addMaker(underground(true));
  LevelMaker* shopMaker = nullptr;
  if (Random.roll(3)) {
      shopMaker = new ShopMaker(chooseRandom({
            ItemFactory::villageShop(),
            ItemFactory::dwarfShop(),
            ItemFactory::goblinShop()}),
          Tribes::get(TribeId::HUMAN), Random.getRandom(8, 16), brickBuilding);
  }
  queue->addMaker(new RoomMaker(8, 15, 4, 7, SquareType::ROCK_WALL, SquareType::BLACK_WALL, shopMaker));
  queue->addMaker(new Connector({5, 3, 0}));
  if (Random.roll(3)) {
    Deity* deity = Deity::getDeity(chooseRandom({DeityHabitat::STONE, DeityHabitat::EARTH}));
    queue->addMaker(new Shrine(deity, SquareType::FLOOR,
        new TypePredicate({SquareType::ROCK_WALL, SquareType::BLACK_WALL}), SquareType::ROCK_WALL, nullptr));
  }
  queue->addMaker(new DungeonFeatures(new AttribPredicate(SquareAttrib::EMPTY_ROOM), featureCount));
  queue->addMaker(new DungeonFeatures(new TypePredicate(SquareType::PATH), {
        { SquareType::ROLLING_BOULDER, make_pair(0, 3) },
        { SquareType::POISON_GAS, make_pair(0, 3) }}));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(SquareType::FLOOR)));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, new TypePredicate(SquareType::FLOOR)));
  queue->addMaker(new Creatures(cfactory, Random.getRandom(10, 15), MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareType::FLOOR, 5, 10));
  queue->addMaker(new SetDark());
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

LevelMaker* LevelMaker::cryptLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::COFFIN, make_pair(3, 7)}};
  queue->addMaker(new Empty(SquareType::BLACK_WALL));
  queue->addMaker(new RoomMaker(8, 15, 3, 5, SquareType::ROCK_WALL, SquareType::BLACK_WALL));
  queue->addMaker(new Connector({1, 3, 1}));
  queue->addMaker(new DungeonFeatures(new AttribPredicate(SquareAttrib::EMPTY_ROOM),featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(SquareType::FLOOR)));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, new TypePredicate(SquareType::FLOOR)));
  queue->addMaker(new Creatures(cfactory, Random.getRandom(10, 15), MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareType::FLOOR, 5, 10));
  queue->addMaker(new SetDark());
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

LevelMaker* hatchery(CreatureFactory factory, int numCreatures) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::HATCHERY));
  queue->addMaker(new Creatures(factory, numCreatures, MonsterAIFactory::monster()));
  return queue;
}

MakerQueue* getElderRoom(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* elderRoom = new MakerQueue();
  if (info.elder)
    elderRoom->addMaker(new Creatures(CreatureFactory::singleType(info.tribe, *info.elder), 1,
          MonsterAIFactory::stayInLocation(info.location, false), building.floorInside));
  if (info.elderLoot)
    elderRoom->addMaker(new Items(ItemFactory::singleType(*info.elderLoot), building.floorInside, 1, 2));
  return elderRoom;
}

MakerQueue* village2(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::CHEST, make_pair(0, 3) },
      { SquareType::BED, make_pair(0, 3) }};
  queue->addMaker(new LocationMaker(info.location));
  vector<LevelMaker*> insideMakers {
      new DungeonFeatures(new TypePredicate(building.floorInside), featureCount),
      getElderRoom(info)};
  if (info.shopFactory)
    insideMakers.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.getRandom(8, 16), building));
  queue->addMaker(new Buildings(6, 10, 3, 4, building, false, insideMakers, false));
  queue->addMaker(new Creatures(info.factory, info.numCreatures, MonsterAIFactory::stayInLocation(info.location),
        building.floorOutside));
  return queue;
}

MakerQueue* village(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::CHEST, make_pair(0, 3) },
      { SquareType::BED, make_pair(0, 3) }};
  queue->addMaker(new LocationMaker(info.location));
  queue->addMaker(new UniformBlob(building.floorOutside, Nothing(), Nothing(), 0.6));
  vector<LevelMaker*> insideMakers {
      hatchery(CreatureFactory::singleType(Tribes::get(TribeId::ELVEN), CreatureId::PIG), Random.getRandom(2, 5)),
      new DungeonFeatures(new TypePredicate(building.floorInside), featureCount),
      getElderRoom(info)};
  if (info.shopFactory)
    insideMakers.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.getRandom(8, 16), building));
  queue->addMaker(new Buildings(4, 8, 3, 7, building, true, insideMakers));
  queue->addMaker(new Creatures(info.factory, info.numCreatures, MonsterAIFactory::stayInLocation(info.location),
        building.floorOutside));
  return queue;
}

MakerQueue* cottage(SettlementInfo info, const map<SquareType, pair<int, int> >& featureCount =
    {{ SquareType::CHEST, make_pair(4, 9) }, { SquareType::BED, make_pair(2, 4) }}) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(building.floorOutside));
  MakerQueue* room = getElderRoom(info);
  room->addMaker(new DungeonFeatures(new TypePredicate(building.floorInside), featureCount));
  queue->addMaker(new Buildings(1, 2, 5, 7, building, false, {room}, false));
  queue->addMaker(new LocationMaker(info.location));
  queue->addMaker(new Creatures(info.factory, info.numCreatures, MonsterAIFactory::stayInLocation(info.location),
        building.floorOutside));
  return queue;
}

MakerQueue* castle(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  LevelMaker* castleRoom = new BorderGuard(new Empty(building.floorInside, SquareAttrib::EMPTY_ROOM), building.wall);
  MakerQueue* leftSide = new MakerQueue();
  leftSide->addMaker(new Division(true, Random.getDouble(0.5, 0.5),
      new Margin(1, -1, -1, 1, castleRoom), new Margin(1, 1, -1, -1, castleRoom)));
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::CHEST, make_pair(3, 8) },
      { SquareType::BED, make_pair(5, 8) }};
  leftSide->addMaker(new DungeonFeatures(new AttribPredicate(SquareAttrib::EMPTY_ROOM), featureCount));
  for (StairKey key : info.downStairs)
    leftSide->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(building.floorInside), Nothing()));
  leftSide->addMaker(getElderRoom(info));
  MakerQueue* inside = new MakerQueue();
  inside->addMaker(new Empty(building.floorOutside));
  vector<LevelMaker*> insideMakers;
  if (info.shopFactory)
    insideMakers.push_back(new ShopMaker(*info.shopFactory, info.tribe, Random.getRandom(8, 16), building));
  inside->addMaker(new Division(Random.getDouble(0.25, 0.4), leftSide,
        new Buildings(1, 6, 3, 6, building, false, insideMakers, false), building.wall));
  MakerQueue* insidePlusWall = new MakerQueue();
  insidePlusWall->addMaker(new BorderGuard(inside, building.wall));
  MakerQueue* queue = new MakerQueue();
  int insideMargin = 2;
  queue->addMaker(new Margin(insideMargin, new LocationMaker(info.location)));
  queue->addMaker(new Margin(insideMargin, insidePlusWall));
  queue->addMaker(new AreaCorners(new BorderGuard(new Empty(building.floorInside), building.wall), Vec2(5, 5)));
  queue->addMaker(new Margin(insideMargin, new Connector({0, 1, 0}, 18)));
  queue->addMaker(new Margin(insideMargin, new CastleExit(info.tribe, building, *info.guardId)));
  queue->addMaker(new Creatures(info.factory, info.numCreatures, MonsterAIFactory::stayInLocation(info.location),
        building.floorOutside));
  inside->addMaker(new DungeonFeatures(new TypePredicate(SquareType::MUD), {{ SquareType::WELL, make_pair(1, 2) }}));
  inside->addMaker(new DungeonFeatures(new TypePredicate(SquareType::FLOOR),{
        { SquareType::FOUNTAIN, make_pair(2, 4)}}));
  return queue;
}

MakerQueue* castle2(SettlementInfo info) {
  BuildingInfo building = get(info.buildingId);
  MakerQueue* inside = new MakerQueue();
  inside->addMaker(new Empty(building.floorOutside));
  inside->addMaker(new Buildings(1, 2, 4, 6, building, false, {getElderRoom(info)}, false));
  MakerQueue* insidePlusWall = new MakerQueue();
  insidePlusWall->addMaker(new BorderGuard(inside, building.wall));
  MakerQueue* queue = new MakerQueue();
  int insideMargin = 2;
  queue->addMaker(new Margin(insideMargin, new LocationMaker(info.location)));
  queue->addMaker(new Margin(insideMargin, insidePlusWall));
  queue->addMaker(new Margin(insideMargin, new Connector({0, 1, 0}, 18)));
  queue->addMaker(new Margin(insideMargin, new CastleExit(info.tribe, building, *info.guardId)));
  queue->addMaker(new Creatures(info.factory, info.numCreatures, MonsterAIFactory::stayInLocation(info.location),
        building.floorOutside));
  return queue;
}

LevelMaker* dungeonEntrance(StairKey key, SquareType onType, const string& dungeonDesc) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(onType), SquareAttrib::CONNECT,
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
          {new UniformBlob(SquareType::GRASS, SquareType::SAND)}, {{15, 15}},
          new TypePredicate(SquareType::WATER))));
 // queue->addMaker(new Creatures(CreatureFactory::singleType(CreatureId::VODNIK), 2, 5, MonsterAIFactory::stayInLocation(loc)));
  return queue;
}

LevelMaker* LevelMaker::towerLevel(Optional<StairKey> down, Optional<StairKey> up) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(up ? SquareType::ROCK_WALL : SquareType::LOW_ROCK_WALL));
  queue->addMaker(new Margin(1, new Empty(SquareType::FLOOR)));
  queue->addMaker(new Margin(1, new AddAttrib(SquareAttrib::ROOM)));
  queue->addMaker(new RemoveAttrib(SquareAttrib::ROAD_CUT_THRU));
  LevelMaker* downStairs = (down) ? new Stairs(StairDirection::DOWN, *down,
      new TypePredicate(SquareType::FLOOR)) : nullptr;
  LevelMaker* upStairs = (up) ? new Stairs(StairDirection::UP, *up, new TypePredicate(SquareType::FLOOR)) : nullptr;
  queue->addMaker(new Division(0.5, 0.5, upStairs, nullptr, nullptr, downStairs));
  return queue;
}

LevelMaker* makeDragonSwamp(StairKey down, Quest* dragonQuest) {
  MakerQueue* queue = new MakerQueue();
  Location* loc = new Location();
  dragonQuest->setLocation(loc);
  queue->addMaker(new LocationMaker(loc));
  queue->addMaker(new UniformBlob(SquareType::MUD));
  queue->addMaker(new Margin(3, new Stairs(StairDirection::DOWN, down, new TypePredicate(SquareType::MUD), Nothing(),
      StairLook::DUNGEON_ENTRANCE_MUD)));
  return queue;
}

pair<int, int> getSize(SettlementType type) {
  switch (type) {
    case SettlementType::WITCH_HOUSE:
    case SettlementType::COTTAGE: return {Random.getRandom(8, 12), Random.getRandom(8, 12)};
    case SettlementType::VILLAGE2: return {30, 20};
    case SettlementType::VILLAGE:
    case SettlementType::CASTLE: return {30, 20};
    case SettlementType::CASTLE2: return {15, 15};
  }
  FAIL << "fewf";
  return {0, 0};
}

SquarePredicate* getSettlementPredicate(SettlementType type) {
  if (type == SettlementType::VILLAGE2)
    return new AttribPredicate(SquareAttrib::FORREST);
  else
    return new AndPredicates(new AttribPredicate(SquareAttrib::LOWLAND),
        new NoAttribPredicate(SquareAttrib::FOG));
}

LevelMaker* LevelMaker::topLevel(CreatureFactory forrestCreatures, vector<SettlementInfo> settlements) {
  MakerQueue* queue = new MakerQueue();
  vector<SquareType> vegetationLow { SquareType::CANIF_TREE, SquareType::BUSH };
  vector<SquareType> vegetationHigh { SquareType::DECID_TREE, SquareType::BUSH };
  vector<double> probs { 2, 1 };
  LevelMaker* lake = makeLake();
  int numLakes = 1;//Random.getRandom(1, 2);
  vector<pair<int, int>> subSizes;
  vector<LevelMaker*> subMakers;
  map<pair<LevelMaker*, LevelMaker*>, double> minDistances;
  map<pair<LevelMaker*, LevelMaker*>, double> maxDistances;
  vector<SquarePredicate*> predicates;
  SquarePredicate* lowlandPred = new AndPredicates(new AttribPredicate(SquareAttrib::LOWLAND),
          new NoAttribPredicate(SquareAttrib::FOG));
  for (int i : Range(numLakes)) {
    subSizes.emplace_back(Random.getRandom(60, 120), Random.getRandom(60, 120));
    subMakers.push_back(lake);
    predicates.push_back(lowlandPred);
  }
  LevelMaker* castleMaker = nullptr;
  LevelMaker* elvenVillage = nullptr;
  LevelMaker* bandits = nullptr;
  for (SettlementInfo settlement : settlements) {
    MakerQueue* queue = nullptr;
    switch (settlement.type) {
      case SettlementType::VILLAGE2: queue = village2(settlement); break;
      case SettlementType::VILLAGE: queue = village(settlement); break;
      case SettlementType::CASTLE:
          queue = castle(settlement);
          queue->addMaker(new StartingPos(new TypePredicate(SquareType::MUD), StairKey::PLAYER_SPAWN));
          castleMaker = queue;
          break;
      case SettlementType::CASTLE2: queue = castle2(settlement); break;
      case SettlementType::COTTAGE: queue = cottage(settlement); break;
      case SettlementType::WITCH_HOUSE:
          queue = cottage(settlement, {{ SquareType::LABORATORY, {2, 5}}}); break;
    }
    if (settlement.tribe == Tribes::get(TribeId::ELVEN))
      elvenVillage = queue;
    if (settlement.tribe == Tribes::get(TribeId::BANDIT))
      bandits = queue;
    subMakers.push_back(queue);
    subSizes.push_back(getSize(settlement.type));
    predicates.push_back(getSettlementPredicate(settlement.type));
  }
  maxDistances[{elvenVillage, bandits}] = 100;
  minDistances[{elvenVillage, bandits}] = 50;
  for (int i : Range(Random.getRandom(2, 5))) {
    subMakers.push_back(new Empty(SquareType::CROPS));
    subSizes.emplace_back(Random.getRandom(5, 15), Random.getRandom(5, 15));
    maxDistances[{elvenVillage, subMakers.back()}] = 18;
    predicates.push_back(lowlandPred);
  }
  LevelMaker* swamp = makeDragonSwamp(StairKey::DRAGON, Quests::get(QuestId::DRAGON));
  subMakers.push_back(swamp);
  subSizes.emplace_back(10, 10);
  predicates.push_back(lowlandPred);
  maxDistances[{castleMaker, swamp}] = 50;
  int numCemeteries = 1;
  for (int i : Range(numCemeteries)) {
    Location* loc = new Location("old cemetery", "Terrible evil is said to be lurking there.");
    subMakers.push_back(new MakerQueue({
          new LocationMaker(loc),
          new Margin(1, new Buildings(1, 2, 2, 3, brickBuilding, false, {}, false)),
          new DungeonFeatures(new TypePredicate(SquareType::GRASS), {{ SquareType::GRAVE, make_pair(5, 15) }}),
          new Stairs(StairDirection::DOWN, StairKey::CRYPT, new TypePredicate(SquareType::FLOOR)),
          }));
    subSizes.emplace_back(10, 10);
    predicates.push_back(lowlandPred);
  }
/*  int numSheepFlocks = 0;
  for (int i : Range(numSheepFlocks)) {
    subMakers.push_back(new FlockAndLeader(CreatureId::ELF, CreatureId::SHEEP, Tribes::get(TribeId::ELVEN), 5, 10));
    subSizes.emplace_back(15, 15);
    predicates.push_back(lowlandPred);
  }*/
  int numStoneCircles = Random.getRandom(2, 7);
 /* for (int i : Range(numStoneCircles)) {
    subMakers.push_back(new Circle(ItemId::BOULDER));
    int size = Random.getRandom(14, 32);
    subSizes.emplace_back(size, size);
  }*/
  MakerQueue* pyramid = new MakerQueue();
  pyramid->addMaker(new LocationMaker(new Location("ancient pyramid", "Terrible evil is said to be lurking there.")));
  pyramid->addMaker(new Margin(1, pyramidLevel(Nothing(), {StairKey::PYRAMID}, {})));
 // subMakers.push_back(pyramid);
 // subSizes.emplace_back(17, 17);
  subMakers.push_back(dungeonEntrance(StairKey::DWARF, SquareType::MOUNTAIN, "Our enemies the dwarves are living there."));
  subSizes.emplace_back(1, 1);
  predicates.push_back(new TypePredicate(SquareType::MOUNTAIN));
  queue->addMaker(new Empty(SquareType::GRASS));
  queue->addMaker(new Mountains(0.5, 0.7, {1, 1, 1, 1, 0}, false));
 // queue->addMaker(new MountainRiver(30, '='));
  queue->addMaker(new Forrest(0.7, 0.5, SquareType::GRASS, vegetationLow, probs));
  queue->addMaker(new Forrest(0.4, 0.5, SquareType::HILL, vegetationHigh, probs));
  queue->addMaker(new Forrest(0.2, 0.3, SquareType::MOUNTAIN, vegetationHigh, probs));
  queue->addMaker(new Margin(100, new RandomLocations(subMakers, subSizes, predicates, true,
      minDistances, maxDistances)));
  MakerQueue* tower = new MakerQueue();
  tower->addMaker(towerLevel(StairKey::TOWER, Nothing()));
  tower->addMaker(new LocationMaker(Location::towerTopLocation()));
//  queue->addMaker(new Margin(100, 
//        new RandomLocations({tower}, {make_pair(4, 4)}, new AttribPredicate(SquareAttrib::MOUNTAIN))));
 // queue->addMaker(new Margin(100, dungeonEntrance(StairKey::GOBLIN, SquareType::MOUNTAIN, "Our enemies the goblins are living there.")));
  queue->addMaker(new Roads(SquareType::PATH));
  /*Deity* deity = Deity::getDeity(chooseRandom({DeityHabitat::STARS, DeityHabitat::TREES}));
  queue->addMaker(new Shrine(deity, SquareType::PATH,
        new TypePredicate({SquareType::GRASS, SquareType::DECID_TREE, SquareType::BUSH}), SquareType::WOOD_WALL, new LocationMaker(new Location("shrine", "It is dedicated to the god " + deity->getName()))));*/
  queue->addMaker(new Creatures(forrestCreatures, Random.getRandom(30, 50), MonsterAIFactory::wildlifeNonPredator()));
  queue->addMaker(new Items(ItemFactory::mushrooms(), SquareType::GRASS, 30, 60));
  return new BorderGuard(queue);
}

LevelMaker* LevelMaker::topLevel2(CreatureFactory forrestCreatures, vector<SettlementInfo> settlements) {
  MakerQueue* queue = new MakerQueue();
  vector<SquareType> vegetationLow { SquareType::CANIF_TREE, SquareType::BUSH };
  vector<SquareType> vegetationHigh { SquareType::DECID_TREE, SquareType::BUSH };
  vector<double> probs { 2, 1 };
  vector<pair<int, int>> subSizes;
  vector<LevelMaker*> subMakers;
  vector<SquarePredicate*> predicates;
  LevelMaker* startingPos = new StartingPos(new TypePredicate(SquareType::HILL), StairKey::PLAYER_SPAWN);
  subMakers.push_back(startingPos);
  predicates.push_back(new TypePredicate(SquareType::HILL));
  subSizes.emplace_back(4, 4);
  map<pair<LevelMaker*, LevelMaker*>, double> minDistances;
  map<pair<LevelMaker*, LevelMaker*>, double> maxDistances;
  vector<LevelMaker*> cottages;
  for (SettlementInfo settlement : settlements) {
    MakerQueue* queue = nullptr;
    switch (settlement.type) {
      case SettlementType::VILLAGE: queue = village(settlement); break;
      case SettlementType::VILLAGE2: queue = village2(settlement); break;
      case SettlementType::CASTLE:
          queue = castle(settlement);
          queue->addMaker(new StartingPos(new TypePredicate(SquareType::MUD), StairKey::HERO_SPAWN));
          break;
      case SettlementType::CASTLE2: queue = castle2(settlement); break;
      case SettlementType::COTTAGE:
          queue = cottage(settlement);
          cottages.push_back(queue);
          break;
      case SettlementType::WITCH_HOUSE:
          queue = cottage(settlement, {{ SquareType::LABORATORY, {2, 5}}}); break;
          break;
    }
    minDistances[{startingPos, queue}] = 50;
    subMakers.push_back(queue);
    predicates.push_back(getSettlementPredicate(settlement.type));
    subSizes.emplace_back(getSize(settlement.type));
  }
  for (LevelMaker* cottage : cottages)
    for (int i : Range(Random.getRandom(1, 3))) {
      subMakers.push_back(new Empty(SquareType::CROPS));
      subSizes.emplace_back(Random.getRandom(5, 15), Random.getRandom(5, 15));
      maxDistances[{cottage, subMakers.back()}] = 13;
      predicates.push_back(new AttribPredicate(SquareAttrib::LOWLAND));
    }
  for (int i : Range(Random.getRandom(1, 4))) {
    subMakers.push_back(new Lake());
    subSizes.emplace_back(20, 20);
    predicates.push_back(new AttribPredicate(SquareAttrib::LOWLAND));
  }
  for (int i : Range(Random.getRandom(2, 5))) {
    subMakers.push_back(new UniformBlob(SquareType::WATER, Nothing(), SquareAttrib::LAKE));
    subSizes.emplace_back(Random.getRandom(5, 30), Random.getRandom(5, 30));
    predicates.push_back(new TypePredicate(SquareType::MOUNTAIN2));
  }
  int maxResourceDist = 50;
  int minGold = 1;
  int maxGold = 3;
  int minStone = 3;
  int maxStone = 6;
  int minIron = 7;
  int maxIron = 12;
  for (int i : Range(Random.getRandom(minGold, maxGold))) {
    subMakers.push_back(new UniformBlob(SquareType::GOLD_ORE));
    subSizes.emplace_back(Random.getRandom(5, 6), Random.getRandom(5, 6)); 
    predicates.push_back(new TypePredicate(SquareType::MOUNTAIN2));
    if (i < minGold)
      maxDistances[{startingPos, subMakers.back()}] = maxResourceDist;
  }
  for (int i : Range(Random.getRandom(minStone, maxStone))) {
    subMakers.push_back(new UniformBlob(SquareType::STONE));
    subSizes.emplace_back(Random.getRandom(5, 10), Random.getRandom(5, 10)); 
    predicates.push_back(new TypePredicate(SquareType::MOUNTAIN2));
    if (i < minStone)
      maxDistances[{startingPos, subMakers.back()}] = maxResourceDist;
  }
  for (int i : Range(Random.getRandom(minIron, maxIron))) {
    subMakers.push_back(new UniformBlob(SquareType::IRON_ORE));
    subSizes.emplace_back(Random.getRandom(5, 10), Random.getRandom(5, 10)); 
    predicates.push_back(new TypePredicate(SquareType::MOUNTAIN2));
    if (i < minIron)
      maxDistances[{startingPos, subMakers.back()}] = maxResourceDist;
  }
  subMakers.push_back(dungeonEntrance(StairKey::DWARF, SquareType::MOUNTAIN2, "dwarven halls"));
  subSizes.emplace_back(1, 1);
  predicates.push_back(new BorderPredicate(SquareType::MOUNTAIN2, SquareType::HILL));
  minDistances[{startingPos, subMakers.back()}] = 50;
  queue->addMaker(new Empty(SquareType::GRASS));
  queue->addMaker(new Mountains(0.7, 0.4, {0, 1, 1, 0, 0}, true,
        {SquareType::MOUNTAIN2, SquareType::MOUNTAIN2, SquareType::HILL}));
  queue->addMaker(new SetCovered(new TypePredicate(SquareType::MOUNTAIN2)));
  queue->addMaker(new Forrest(0.7, 0.5, SquareType::GRASS, vegetationLow, probs));
  queue->addMaker(new Forrest(0.4, 0.5, SquareType::HILL, vegetationHigh, probs));
  queue->addMaker(new Forrest(0.2, 0.3, SquareType::MOUNTAIN, vegetationHigh, probs));
  queue->addMaker(new Margin(10, new RandomLocations(subMakers, subSizes, predicates, true, minDistances,
          maxDistances)));
  queue->addMaker(new Roads(SquareType::PATH));
  queue->addMaker(new Creatures(CreatureFactory::singleType(Tribes::get(TribeId::HUMAN), CreatureId::GOAT),
        Random.getRandom(3, 6), MonsterAIFactory::doorEater()));
  queue->addMaker(new Items(ItemFactory::mushrooms(), SquareType::GRASS, 30, 60));
  return new BorderGuard(queue);
}

static LevelMaker* townLevel(CreatureFactory cfactory, int numCreatures, vector<StairKey> up, vector<StairKey> down,
    SquareType furniture, int numCavern, int maxCavernSize, int minRooms, int maxRooms, ItemFactory shopFactory,
    Tribe* shopTribe) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount {
      { SquareType::CHEST, make_pair(4, 8) },
      { furniture, make_pair(4, 8) }};
  queue->addMaker(new Empty(SquareType::ROCK_WALL));
  LevelMaker* cavern = new UniformBlob(SquareType::PATH);
  vector<LevelMaker*> vCavern;
  vector<pair<int, int>> sizes;
  for (int i : Range(numCavern)) {
    sizes.push_back(make_pair(Random.getRandom(5, maxCavernSize), Random.getRandom(5, maxCavernSize)));
    vCavern.push_back(cavern);
  }
  queue->addMaker(new RandomLocations(vCavern, sizes, new AlwaysTrue(), false));
  queue->addMaker(new RoomMaker(minRooms, maxRooms, 4, 7, SquareType::ROCK_WALL, Nothing(),
        new ShopMaker(shopFactory, shopTribe, Random.getRandom(8, 16), brickBuilding), false));
  queue->addMaker(new Connector({1, 0, 0}));
  SquarePredicate* featurePred = new AndPredicates(new AttribPredicate(SquareAttrib::EMPTY_ROOM),
      new TypePredicate(SquareType::FLOOR));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, featurePred));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, featurePred));
  queue->addMaker(new DungeonFeatures(featurePred, featureCount));
  queue->addMaker(new Creatures(cfactory, numCreatures, MonsterAIFactory::monster()));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

LevelMaker* LevelMaker::mineTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  return townLevel(cfactory, Random.getRandom(10, 20), up, down, SquareType::BED, 20, 25, 6, 12,
      ItemFactory::dwarfShop(), Tribes::get(TribeId::DWARVEN));
}

LevelMaker* LevelMaker::goblinTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  return townLevel(cfactory, Random.getRandom(10, 20), up, down, SquareType::TORTURE_TABLE, 40, 10, 5, 8,
      ItemFactory::goblinShop(), Tribes::get(TribeId::GOBLIN));
}

LevelMaker* LevelMaker::pyramidLevel(Optional<CreatureFactory> cfactory, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::YELLOW_WALL));
  queue->addMaker(new AddAttrib(SquareAttrib::NO_DIG));
  queue->addMaker(new Margin(1, new RemoveAttrib(SquareAttrib::NO_DIG)));
  queue->addMaker(new RoomMaker(2, 5, 4, 6, SquareType::YELLOW_WALL, SquareType::YELLOW_WALL, nullptr, true));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(SquareType::FLOOR)));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, new TypePredicate(SquareType::FLOOR)));
  if (down.size() == 0)
    queue->addMaker(new LevelExit(SquareType::PATH, SquareAttrib::CONNECT));
  if (cfactory)
    queue->addMaker(new Creatures(*cfactory, Random.getRandom(3, 5), MonsterAIFactory::monster()));
  queue->addMaker(new Connector({1, 0, 0}));
  return queue;
}

LevelMaker* LevelMaker::cellarLevel(CreatureFactory cfactory, SquareType wallType, StairLook stairLook,
    vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount {
      { SquareType::CHEST, make_pair(3, 7)}};
  queue->addMaker(new Empty(SquareType::FLOOR));
  queue->addMaker(new Empty(wallType));
  queue->addMaker(new RoomMaker(8, 15, 3, 5, wallType, wallType));
  queue->addMaker(new Connector({1, 0, 0}));
  queue->addMaker(new DungeonFeatures(new AttribPredicate(SquareAttrib::EMPTY_ROOM), featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(SquareType::FLOOR), Nothing(),
          stairLook));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, new TypePredicate(SquareType::FLOOR), Nothing(), stairLook));
  queue->addMaker(new Creatures(cfactory, Random.getRandom(10, 15), MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareType::FLOOR, 5, 10));
  queue->addMaker(new SetDark());
  return new BorderGuard(queue, wallType);
}

LevelMaker* LevelMaker::cavernLevel(CreatureFactory cfactory, SquareType wallType, SquareType floorType,
    StairLook stairLook, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::CHEST, make_pair(10, 14)}};
  queue->addMaker(new Empty(wallType));
  queue->addMaker(new UniformBlob(floorType));
  queue->addMaker(new DungeonFeatures(new TypePredicate(floorType), featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, new TypePredicate(floorType), Nothing(),
          stairLook));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, new TypePredicate(floorType), Nothing(), stairLook));
  queue->addMaker(new Creatures(cfactory, 1, MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), floorType, 10, 15));
  queue->addMaker(new SetDark());
  return new BorderGuard(queue, wallType);
}

LevelMaker* LevelMaker::grassAndTrees() {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::GRASS));
  queue->addMaker(new Forrest(0.8, 0.5, SquareType::GRASS, {SquareType::CANIF_TREE}, {1}));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}
