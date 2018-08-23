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
#include "item_factory.h"
#include "square.h"
#include "collective_builder.h"
#include "collective.h"
#include "shortest_path.h"
#include "creature.h"
#include "level_builder.h"
#include "model.h"
#include "monster_ai.h"
#include "item.h"
#include "view_id.h"
#include "furniture_type.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "progress.h"
#include "file_path.h"
#include "movement_set.h"
#include "container_range.h"
#include "settlement_info.h"
#include "task.h"
#include "equipment.h"

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
  bool apply(LevelBuilder* builder, Vec2 pos) const {
    return predFun(builder, pos);
  }

  Vec2 getRandomPosition(LevelBuilder* builder, Rectangle area) {
    vector<Vec2> good;
    for (Vec2 v : area)
      if (apply(builder, v))
        good.push_back(v);
    if (good.empty())
      failGen();
    return builder->getRandom().choose(good);
  }

  static Predicate attrib(SquareAttrib attr) {
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) { return builder->hasAttrib(pos, attr);});
  }

  Predicate operator !() const {
    PredFun self(predFun);
    return Predicate([self] (LevelBuilder* builder, Vec2 pos) { return !self(builder, pos);});
  }

  Predicate operator && (const Predicate& p1) const {
    PredFun self(predFun);
    return Predicate([self, p1] (LevelBuilder* builder, Vec2 pos) {
        return p1.apply(builder, pos) && self(builder, pos);});
  }

  Predicate operator || (const Predicate& p1) const {
    PredFun self(predFun);
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) {
        return p1.apply(builder, pos) || self(builder, pos);});
  }

  static Predicate type(FurnitureType t) {
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) {
      return builder->isFurnitureType(pos, t);});
  }

  static Predicate inRectangle(Rectangle r) {
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) {
      return pos.inRectangle(r);});
  }

  static Predicate alwaysTrue() {
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) { return true;});
  }

  static Predicate alwaysFalse() {
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) { return false;});
  }

  static Predicate canEnter(MovementType m) {
    return Predicate([=] (LevelBuilder* builder, Vec2 pos) { return builder->canNavigate(pos, m);});
  }

  private:
  typedef function<bool(LevelBuilder*, Vec2)> PredFun;
  Predicate(PredFun fun) : predFun(fun) {}
  PredFun predFun;
};

class SquareChange {
  public:
  static SquareChange none() {
    return SquareChange([](LevelBuilder*, Vec2) {});
  }

  SquareChange& add(SquareChange added) {
    auto funCopy = changeFun; // copy just the function because storing "this" leads to a crash
    changeFun = [added, funCopy] (LevelBuilder* builder, Vec2 pos) {
        funCopy(builder, pos); added.changeFun(builder, pos); };
    return *this;
  }

  SquareChange(FurnitureType type, optional<SquareAttrib> attrib = ::none)
      : changeFun([=](LevelBuilder* builder, Vec2 pos) {
    builder->putFurniture(pos, type);
    if (attrib)
      builder->addAttrib(pos, *attrib);
  }) {}

  SquareChange(FurnitureParams f)
      : changeFun([=](LevelBuilder* builder, Vec2 pos) {
    builder->putFurniture(pos, f);
  }) {}

  SquareChange(SquareAttrib attrib)
      : changeFun([=](LevelBuilder* builder, Vec2 pos) {
    builder->addAttrib(pos, attrib);
  }) {}

  SquareChange(FurnitureType f1, FurnitureType f2)
      : changeFun([=](LevelBuilder* builder, Vec2 pos) {
    builder->putFurniture(pos, f1);
    builder->putFurniture(pos, f2); }) {}

  static SquareChange reset(FurnitureType f1, optional<SquareAttrib> attrib = ::none) {
    return SquareChange([=](LevelBuilder* builder, Vec2 pos) {
      builder->resetFurniture(pos, f1, attrib);
    });
  }

  static SquareChange reset(FurnitureParams params, optional<SquareAttrib> attrib = ::none) {
    return SquareChange([=](LevelBuilder* builder, Vec2 pos) {
      builder->resetFurniture(pos, params, attrib);
    });
  }

  static SquareChange addTerritory(CollectiveBuilder* collective) {
    return SquareChange([=](LevelBuilder* builder, Vec2 pos) {
      collective->addArea(builder->toGlobalCoordinates(vector<Vec2>({pos})));
    });
  }

  void apply(LevelBuilder* builder, Vec2 pos) {
    changeFun(builder, pos);
  }

  private:
  typedef function<void(LevelBuilder*, Vec2)> ChangeFun;
  SquareChange(ChangeFun fun) : changeFun(fun) {}
  ChangeFun changeFun;

};

class Empty : public LevelMaker {
  public:
  Empty(SquareChange s) : square(s) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (Vec2 v : area)
      square.apply(builder, v);
  }

  private:
  SquareChange square;
};

class RoomMaker : public LevelMaker {
  public:
  RoomMaker(int _numRooms,
            int _minSize, int _maxSize, 
            SquareChange wall = SquareChange::none(),
            optional<FurnitureType> _onType = none,
            PLevelMaker _roomContents = unique<Empty>(FurnitureType::FLOOR),
            vector<PLevelMaker> _insideMakers = {},
            bool _diggableCorners = false) : 
      numRooms(_numRooms),
      minSize(_minSize),
      maxSize(_maxSize),
      wallChange(wall.add(SquareChange(SquareAttrib::ROOM_WALL))),
      onType(_onType),
      roomContents(std::move(_roomContents)),
      insideMakers(std::move(_insideMakers)),
      diggableCorners(_diggableCorners) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    int spaceBetween = 0;
    Table<int> taken(area.right(), area.bottom());
    for (Vec2 v : area)
      taken[v] = onType && !builder->isFurnitureType(v, *onType);
    for (int i : Range(numRooms)) {
      Vec2 p, k;
      bool good;
      int cnt = 100;
      do {
        k = Vec2(builder->getRandom().get(minSize, maxSize), builder->getRandom().get(minSize, maxSize));
        p = Vec2(area.left() + spaceBetween + builder->getRandom().get(area.width() - k.x - 2 * spaceBetween),
                 area.top() + spaceBetween + builder->getRandom().get(area.height() - k.y - 2 * spaceBetween));
        good = true;
        for (Vec2 v : Rectangle(k.x + 2 * spaceBetween, k.y + 2 * spaceBetween))
          if (taken[p + v - Vec2(spaceBetween,spaceBetween)]) {
            good = false;
            break;
          }
      } while (!good && --cnt > 0);
      if (cnt == 0) {
        INFO << "Placed only " << i << " rooms out of " << numRooms;
        break;
      }
      for (Vec2 v : Rectangle(k))
        taken[v + p] = 1;
      for (Vec2 v : Rectangle(k - Vec2(2, 2)))
        builder->resetFurniture(p + v + Vec2(1, 1), FurnitureType::FLOOR, SquareAttrib::ROOM);
      for (int i : Range(p.x, p.x + k.x)) {
        wallChange.apply(builder, Vec2(i, p.y));
        wallChange.apply(builder, Vec2(i, p.y + k.y - 1));
        if ((i == p.x || i == p.x + k.x - 1) && !diggableCorners) {
          builder->addAttrib(Vec2(i, p.y), SquareAttrib::NO_DIG);
          builder->addAttrib(Vec2(i, p.y + k.y - 1), SquareAttrib::NO_DIG);
        }
      }
      for (int i : Range(p.y + 1, p.y + k.y - 1)) {
        wallChange.apply(builder, Vec2(p.x, i));
        wallChange.apply(builder, Vec2(p.x + k.x - 1, i));
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
  SquareChange wallChange;
  optional<FurnitureType> onType;
  PLevelMaker roomContents;
  vector<PLevelMaker> insideMakers;
  bool diggableCorners;
};

class Connector : public LevelMaker {
  public:
  Connector(optional<FurnitureFactory> _door, double _doorProb, double _diggingCost = 3,
        Predicate pred = Predicate::canEnter({MovementTrait::WALK}), optional<SquareAttrib> setAttr = none)
      : door(_door), doorProb(_doorProb), diggingCost(_diggingCost), connectPred(pred), setAttrib(setAttr) {
  }
  double getValue(LevelBuilder* builder, Vec2 pos, Rectangle area) {
    if (builder->canNavigate(pos, {MovementTrait::WALK}))
      return 1;
    if (builder->hasAttrib(pos, SquareAttrib::NO_DIG))
      return ShortestPath::infinity;
    if (builder->hasAttrib(pos, SquareAttrib::LAKE))
      return 15;
    if (builder->hasAttrib(pos, SquareAttrib::RIVER))
      return 15;
    int numCorners = 0;
    int numTotal = 0;
    for (Vec2 v : Vec2::directions8())
      if ((pos + v).inRectangle(area) && builder->canNavigate(pos + v, MovementTrait::WALK)) {
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

  void connect(LevelBuilder* builder, Vec2 p1, Vec2 p2, Rectangle area) {
    ShortestPath path(area,
        [builder, this, &area](Vec2 pos) { return getValue(builder, pos, area); }, 
        [] (Vec2 from, Vec2 to) { return from.dist4(to); },
        Vec2::directions4(builder->getRandom()), p1 ,p2);
    for (Vec2 v = p2; v != p1; v = path.getNextMove(v)) {
      if (!builder->canNavigate(v, {MovementTrait::WALK})) {
        if (auto furniture = builder->getFurniture(v, FurnitureLayer::MIDDLE)) {
          bool placeDoor = furniture->isWall() && builder->hasAttrib(v, SquareAttrib::ROOM_WALL);
          if (!furniture->getMovementSet().canEnter({MovementTrait::WALK}))
            builder->removeFurniture(v, FurnitureLayer::MIDDLE);
          if (placeDoor && door && builder->getRandom().chance(doorProb))
            builder->putFurniture(v, *door);
        }
        if (builder->canNavigate(v, {MovementTrait::WALK}))
          continue;
        if (builder->getFurniture(v, FurnitureLayer::GROUND)->canBuildBridgeOver())
          builder->putFurniture(v, FurnitureType::BRIDGE);
      }
      if (!path.isReachable(v))
        failGen();
    }
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Vec2 p1, p2;
    vector<Vec2> points = area.getAllSquares().filter([&] (Vec2 v) { return connectPred.apply(builder, v);});
    if (points.size() < 2)
      return;
    for (int i : Range(30)) {
      p1 = builder->getRandom().choose(points);
      p2 = builder->getRandom().choose(points);
      if (p1 != p2)
        connect(builder, p1, p2, area);
    }
    auto dijkstraFun = [&] (Vec2 pos) {
      if (builder->canNavigate(pos, MovementTrait::WALK))
        return 1.;
      else
        return ShortestPath::infinity;};
    Table<bool> connected(area, false);
    while (1) {
      Dijkstra dijkstra(area, {p1}, 10000, dijkstraFun);
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
  optional<FurnitureFactory> door;
  double doorProb;
  double diggingCost;
  Predicate connectPred;
  optional<SquareAttrib> setAttrib;
};

namespace {
class Furnitures : public LevelMaker {
  public:
  Furnitures(Predicate pred, double _density, FurnitureFactory _factory, optional<SquareAttrib> setAttr = none): 
      factory(_factory), density(_density), predicate(pred), attr(setAttr) {
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    vector<Vec2> available;
    for (Vec2 v : area)
      if (predicate.apply(builder, v) && builder->canPutFurniture(v, FurnitureLayer::MIDDLE))
        available.push_back(v);
    for (int i : Range(available.size() * density)) {
      Vec2 pos = builder->getRandom().choose(available);
      builder->putFurniture(pos, factory);
      if (attr)
        builder->addAttrib(pos, *attr);
      available.removeElement(pos);
    }
  }

  private:
  FurnitureFactory factory;
  double density;
  Predicate predicate;
  optional<SquareAttrib> attr;
};
}

class Inhabitants : public LevelMaker {
  public:

  Inhabitants(InhabitantsInfo inhab, CollectiveBuilder* col, Predicate pred = Predicate::alwaysTrue()) :
      inhabitants(inhab), actorFactory(MonsterAIFactory::monster()), onPred(pred), collective(col) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    if (!actorFactory)
      actorFactory = MonsterAIFactory::stayInLocation(builder->toGlobalCoordinates(area));
    Table<char> taken(area.right(), area.bottom());
    for (auto& minion : inhabitants.generateCreatures(builder->getRandom(), collective->getTribe(), *actorFactory)) {
      PCreature& creature = minion.first;
      Vec2 pos;
      int numTries = 100;
      do {
        pos = Vec2(builder->getRandom().get(area.left(), area.right()),
            builder->getRandom().get(area.top(), area.bottom()));
      } while (--numTries > 0 && (!builder->canPutCreature(pos, creature.get()) || (!onPred.apply(builder, pos))));
      checkGen(numTries > 0);
      if (collective) {
        collective->addCreature(creature.get(), minion.second);
        builder->addCollective(collective);
      }
      builder->putCreature(pos, std::move(creature));
      taken[pos] = 1;
    }
  }

  private:
  InhabitantsInfo inhabitants;
  optional<MonsterAIFactory> actorFactory;
  Predicate onPred;
  CollectiveBuilder* collective = nullptr;
};

class Corpses : public LevelMaker {
  public:

  Corpses(InhabitantsInfo inhab, Predicate pred = Predicate::alwaysTrue()) : inhabitants(inhab), onPred(pred) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Table<char> taken(area.right(), area.bottom());
    auto creatures = inhabitants.generateCreatures(builder->getRandom(), TribeId::getMonster(),
        MonsterAIFactory::monster());
    for (auto& minion : creatures) {
      PCreature& creature = minion.first;
      Vec2 pos;
      int numTries = 100;
      do {
        pos = Vec2(builder->getRandom().get(area.left(), area.right()),
            builder->getRandom().get(area.top(), area.bottom()));
      } while (--numTries > 0 && (!builder->canPutItems(pos) || (!onPred.apply(builder, pos))));
      checkGen(numTries > 0);
      if (builder->getRandom().roll(10))
        builder->putItems(pos, creature->getEquipment().removeAllItems(creature.get()));
      builder->putItems(pos, creature->generateCorpse(true));
      taken[pos] = 1;
    }
  }

  private:
  InhabitantsInfo inhabitants;
  Predicate onPred;
};

class Creatures : public LevelMaker {
  public:
  Creatures(CreatureFactory f, int num, MonsterAIFactory actorF, Predicate pred = Predicate::alwaysTrue()) :
      creatures(f), numCreatures(num), actorFactory(actorF), onPred(pred) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    if (!actorFactory)
      actorFactory = MonsterAIFactory::stayInLocation(builder->toGlobalCoordinates(area));
    Table<char> taken(area.right(), area.bottom());
    for (int i : Range(numCreatures)) {
      PCreature creature = creatures.random(*actorFactory);
      Vec2 pos;
      int numTries = 100;
      do {
        pos = Vec2(builder->getRandom().get(area.left(), area.right()),
            builder->getRandom().get(area.top(), area.bottom()));
      } while (--numTries > 0 && (!builder->canPutCreature(pos, creature.get()) || (!onPred.apply(builder, pos))));
      checkGen(numTries > 0);
      builder->putCreature(pos, std::move(creature));
      taken[pos] = 1;
    }
  }

  private:
  CreatureFactory creatures;
  int numCreatures;
  optional<MonsterAIFactory> actorFactory;
  Predicate onPred;
};

class Items : public LevelMaker {
  public:
  Items(ItemFactory _factory, int minc, int maxc, Predicate pred = Predicate::alwaysTrue(),
      bool _placeOnFurniture = false) :
      factory(_factory), minItem(minc), maxItem(maxc), predicate(pred), placeOnFurniture(_placeOnFurniture) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    int numItem = builder->getRandom().get(minItem, maxItem);
    for (int i : Range(numItem)) {
      Vec2 pos;
      do {
        pos = Vec2(builder->getRandom().get(area.left(), area.right()), builder->getRandom().get(area.top(),
              area.bottom()));
      } while (!predicate.apply(builder, pos) ||
           !builder->canNavigate(pos, MovementTrait::WALK) ||
           (!placeOnFurniture && builder->getFurniture(pos, FurnitureLayer::MIDDLE)));
      builder->putItems(pos, factory.random());
    }
  }

  private:
  ItemFactory factory;
  int minItem;
  int maxItem;
  Predicate predicate;
  bool placeOnFurniture;
};

class River : public LevelMaker {
  public:
  River(int _width, FurnitureType type) : width(_width), furnitureType(type){}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    int wind = 5;
    int middle = (area.left() + area.right()) / 2;
    int px = builder->getRandom().get(middle - wind, middle + width);
    int kx = px + builder->getRandom().get(-wind, wind); // builder->getRandom().get(area.left(), area.right()) - width;
    if (kx < 0)
      kx = 0;
    if (kx >= area.right() - width)
      kx = area.right() - width - 1;
    int tot = 5;
    for (int h : Range(tot)) {
      int height = area.top() * (tot - h) / tot + area.bottom() * h / tot;
      int height2 = area.top() * (tot - h - 1) / tot + area.bottom() * (h + 1) / tot;
      vector<Vec2> line = straightLine(px, height, kx, (h == tot - 1) ? area.bottom() : height2);
      for (Vec2 v : line)
        for (int i : Range(width)) {
          Vec2 pos = v + Vec2(i, 0);
          builder->resetFurniture(pos, furnitureType, SquareAttrib::RIVER);
        }
      px = kx;
      kx = px + builder->getRandom().get(-wind, wind);
      if (kx < 0)
        kx = 0;
      if (kx >= area.right() - width)
        kx = area.right() - width - 1;
    }
  }

  private:

  vector<Vec2> straightLine(int x0, int y0, int x1, int y1){
    INFO << "Line " << x1 << " " << y0 << " " << x1 << " " << y1;
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
  FurnitureType furnitureType;
};

class MountainRiver : public LevelMaker {
  public:
  MountainRiver(int num, Predicate startPred)
    : number(num), startPredicate(startPred) {}

  optional<Vec2> fillLake(LevelBuilder* builder, set<Vec2>& waterTiles, Rectangle area, Vec2 pos) {
    vector<Vec2> ret;
    double height = builder->getHeightMap(pos);
    queue<Vec2> q;
    set<Vec2> visited {pos};
    map<Vec2, Vec2> predecessor {{ pos, pos}};
    q.push(pos);
    while (!q.empty()) {
      pos = q.front();
      q.pop();
      for (Vec2 v : pos.neighbors8(builder->getRandom()))
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
    if (builder->getRandom().roll(predecessor.size()) || ret.empty()) {
      for (auto& elem : predecessor)
        if (!ret.contains(elem.first))
          waterTiles.insert(elem.first);
      if (ret.empty())
        return none;
      else
        return builder->getRandom().choose(ret);
    } else {
      Vec2 end = builder->getRandom().choose(ret);
      for (Vec2 v = predecessor.at(end);; v = predecessor.at(v)) {
        waterTiles.insert(v);
        if (v == predecessor.at(v))
          break;
      }
      return end;
    }
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    set<Vec2> allWaterTiles;
    for (int i : Range(number)) {
      set<Vec2> waterTiles;
      Vec2 pos = startPredicate.getRandomPosition(builder, area);
      int width = builder->getRandom().get(3, 6);
      while (1) {
        if (builder->hasAttrib(pos, SquareAttrib::RIVER))
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
    for (auto layer : Iter(Vec2::calculateLayers(allWaterTiles))) {
      for (Vec2 v : *layer)
        if (v.inRectangle(area) && !builder->hasAttrib(v, SquareAttrib::RIVER)) {
          builder->addAttrib(v, SquareAttrib::RIVER);
          builder->resetFurniture(v, getWaterType(builder, v, layer.index()));
        }
    }
  }

  FurnitureParams getWaterType(LevelBuilder* builder, Vec2 pos, int numLayer) {
    if (builder->hasAttrib(pos, SquareAttrib::MOUNTAIN))
      return FurnitureParams{FurnitureFactory::getWaterType(100), TribeId::getKeeper()};
    else if (numLayer == 0)
      return FurnitureParams{FurnitureType::SAND, TribeId::getKeeper()};
    else
      return FurnitureParams{FurnitureFactory::getWaterType(1.1 * (numLayer - 1)), TribeId::getKeeper()};
  }

  private:
  int number;
  Predicate startPredicate;
};

class Blob : public LevelMaker {
  public:
  Blob(double _insideRatio = 0.333) : insideRatio(_insideRatio) {}

  virtual void addSquare(LevelBuilder* builder, Vec2 pos, int edgeDist) = 0;

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    vector<Vec2> squares;
    Table<char> isInside(area, 0);
    Vec2 center = area.middle();
    squares.push_back(center);
    isInside[center] = 1;
    for (int i : Range(area.width() * area.height() * insideRatio)) {
      vector<Vec2> nextPos;
      for (auto pos : squares)
        for (Vec2 next : pos.neighbors4())
          if (next.inRectangle(area) && !squares.contains(next))
            nextPos.push_back(next);
      vector<double> probs = nextPos.transform([&](Vec2 v) {
          double px = std::abs(v.x - center.x);
          double py = std::abs(v.y - center.y);
          py *= area.width();
          py /= area.height();
          double coeff = -1.0 + 1.0 / (sqrt(px * px + py * py) / sqrt(2 * area.width() * area.width()));
          CHECK(coeff > 0.0);
          return coeff;
        });
      Vec2 chosen = builder->getRandom().choose(nextPos, probs);
      isInside[chosen] = 1;
      squares.push_back(chosen);
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
  UniformBlob(SquareChange insideSquare, optional<SquareChange> borderSquare = none,
      double insideRatio = 0.3333) : Blob(insideRatio),
      inside(insideSquare), border(borderSquare) {}

  virtual void addSquare(LevelBuilder* builder, Vec2 pos, int edgeDist) override {
    if (edgeDist == 1 && border)
      border->apply(builder, pos);
    else
      inside.apply(builder, pos);
  }

  private:
  SquareChange inside;
  optional<SquareChange> border;
};

class FurnitureBlob : public Blob {
  public:
  FurnitureBlob(SquareChange in, double insideRatio = 0.3333) : Blob(insideRatio), inside(in) {}

  virtual void addSquare(LevelBuilder* builder, Vec2 pos, int edgeDist) override {
    inside.apply(builder, pos);
  }

  private:
  SquareChange inside;
};

class Lake : public Blob {
  public:
  Lake(bool s = true) : sand(s) {}
  virtual void addSquare(LevelBuilder* builder, Vec2 pos, int edgeDist) override {
    builder->addAttrib(pos, SquareAttrib::LAKE);
    if (sand && edgeDist == 1 && !builder->isFurnitureType(pos, FurnitureType::WATER))
      builder->resetFurniture(pos, FurnitureType::SAND);
    else
      builder->resetFurniture(pos,
          FurnitureParams{FurnitureFactory::getWaterType(double(edgeDist) / 2), TribeId::getKeeper()});
  }

  private:
  bool sand;
};

class RemoveFurniture : public LevelMaker {
  public:
  RemoveFurniture(FurnitureLayer l) : layer(l) {
    CHECK(layer != FurnitureLayer::GROUND);
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (Vec2 v : area)
      builder->removeFurniture(v, layer);
  }

  private:
  FurnitureLayer layer;
};

struct BuildingType {
  FurnitureType wall;
  FurnitureType floorInside;
  FurnitureType floorOutside;
  optional<FurnitureFactory> door;
};

static BuildingType getBuildingInfo(SettlementInfo info) {
  switch (info.buildingId) {
    case BuildingId::WOOD:
      return CONSTRUCT(BuildingType,
          c.wall = FurnitureType::WOOD_WALL;
          c.floorInside = FurnitureType::FLOOR;
          c.floorOutside = FurnitureType::GRASS;
          c.door = FurnitureFactory(info.tribe, FurnitureType::WOOD_DOOR);
      );
    case BuildingId::WOOD_CASTLE:
      return CONSTRUCT(BuildingType,
          c.wall = FurnitureType::WOOD_WALL;
          c.floorInside = FurnitureType::FLOOR;
          c.floorOutside = FurnitureType::MUD;
          c.door = FurnitureFactory(info.tribe, FurnitureType::WOOD_DOOR);
      );
    case BuildingId::MUD: 
      return CONSTRUCT(BuildingType,
          c.wall = FurnitureType::MUD_WALL;
          c.floorInside = FurnitureType::MUD;
          c.floorOutside = FurnitureType::MUD;);
    case BuildingId::BRICK:
      return CONSTRUCT(BuildingType,
          c.wall = FurnitureType::CASTLE_WALL;
          c.floorInside = FurnitureType::FLOOR;
          c.floorOutside = FurnitureType::MUD;
          c.door = FurnitureFactory(info.tribe, FurnitureType::IRON_DOOR);
      );
    case BuildingId::DUNGEON:
      return CONSTRUCT(BuildingType,
          c.wall = FurnitureType::MOUNTAIN;
          c.floorInside = FurnitureType::FLOOR;
          c.floorOutside = FurnitureType::FLOOR;
          c.door = FurnitureFactory(info.tribe, FurnitureType::WOOD_DOOR);
      );
    case BuildingId::DUNGEON_SURFACE:
      return CONSTRUCT(BuildingType,
          c.wall = FurnitureType::MOUNTAIN;
          c.floorInside = FurnitureType::FLOOR;
          c.floorOutside = FurnitureType::HILL;
          c.door = FurnitureFactory(info.tribe, FurnitureType::WOOD_DOOR);
      );
  }
}

class Buildings : public LevelMaker {
  public:
  Buildings(int _minBuildings, int _maxBuildings,
      int _minSize, int _maxSize,
      BuildingType _building,
      bool _align,
      vector<PLevelMaker> _insideMakers,
      bool _roadConnection = true) :
    minBuildings(_minBuildings),
    maxBuildings(_maxBuildings),
    minSize(_minSize), maxSize(_maxSize),
    align(_align),
    building(_building),
    insideMakers(std::move(_insideMakers)),
    roadConnection(_roadConnection) {
      CHECK(insideMakers.size() <= minBuildings);
    }

  Buildings(int _minBuildings, int _maxBuildings,
      int _minSize, int _maxSize,
      BuildingType _building,
      bool _align,
      PLevelMaker _insideMaker,
      bool _roadConnection = true) : Buildings(_minBuildings, _maxBuildings, _minSize, _maxSize, _building, _align,
        _insideMaker ? makeVec<PLevelMaker>(std::move(_insideMaker)) : vector<PLevelMaker>(), _roadConnection) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Table<bool> filled(area);
    int width = area.width();
    int height = area.height();
    for (Vec2 v : area)
      filled[v] =  0;
    int sizeVar = 1;
    int spaceBetween = 1;
    int alignHeight = 0;
    if (align) {
      alignHeight = height / 2 - 2 + builder->getRandom().get(5);
    }
    int nextw = -1;
    int numBuildings = builder->getRandom().get(minBuildings, maxBuildings);
    for (int i = 0; i < numBuildings; ++i) {
      bool spaceOk = true;
      int w, h, px, py;
      int cnt = 10000;
      bool buildingRow;
      do {
        buildingRow = builder->getRandom().get(2);
        spaceOk = true;
        w = builder->getRandom().get(minSize, maxSize);
        h = builder->getRandom().get(minSize, maxSize);
        if (nextw > -1 && nextw + w < area.right()) {
          px = nextw;
          nextw = -1;
        } else
          px = area.left() + builder->getRandom().get(width - w - 2 * spaceBetween + 1) + spaceBetween;
        if (!align)
          py = area.top() + builder->getRandom().get(height - h - 2 * spaceBetween + 1) + spaceBetween;
        else {
          py = area.top() + (buildingRow == 1 ? alignHeight - h - 1 : alignHeight + 2);
          if (py + h >= area.bottom() || py < area.top()) {
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
      if (builder->getRandom().roll(1))
        nextw = px + w;
      for (Vec2 v : Rectangle(w + 1, h + 1)) {
        filled[Vec2(px, py) + v] = true;
        builder->putFurniture(Vec2(px, py) + v, building.wall);
        builder->setCovered(Vec2(px, py) + v, true);
      }
      for (Vec2 v : Rectangle(w - 1, h - 1)) {
        auto pos = Vec2(px + 1, py + 1) + v;
        builder->resetFurniture(pos, building.floorInside, SquareAttrib::ROOM);
      }
      Vec2 doorLoc = align ? 
          Vec2(px + builder->getRandom().get(1, w),
               py + (buildingRow * h)) :
          getRandomExit(Random, Rectangle(px, py, px + w + 1, py + h + 1));
      builder->resetFurniture(doorLoc, building.floorInside);
      if (building.door)
        builder->putFurniture(doorLoc, *building.door);
      Rectangle inside(px + 1, py + 1, px + w, py + h);
      if (i < insideMakers.size()) 
        insideMakers[i]->make(builder, inside);
      else
        for (Vec2 v : inside)
          builder->addAttrib(v, SquareAttrib::EMPTY_ROOM);
    }
    if (align)
      for (Vec2 v : Rectangle(area.left() + area.width() / 3, area.top() + alignHeight,
            area.right() - area.width() / 3, area.top() + alignHeight + 2))
        builder->addAttrib(v, SquareAttrib::BUILDINGS_CENTER);
    if (roadConnection) {
      Vec2 pos = Vec2((area.left() + area.right()) / 2, area.top() + alignHeight);
      builder->removeFurniture(pos, FurnitureLayer::MIDDLE);
      builder->putFurniture(pos, FurnitureParams{FurnitureType::ROAD, TribeId::getMonster()});
      builder->addAttrib(pos, SquareAttrib::CONNECT_ROAD);
    }
  }

  private:
  int minBuildings;
  int maxBuildings;
  int minSize;
  int maxSize;
  bool align;
  BuildingType building;
  vector<PLevelMaker> insideMakers;
  bool roadConnection;
};

DEF_UNIQUE_PTR(MakerQueue);

class MakerQueue : public LevelMaker {
  public:
  MakerQueue() = default;
  MakerQueue(vector<PLevelMaker> _makers) : makers(std::move(_makers)) {}

  template <typename T1, typename... Args>
  MakerQueue(T1&& t1, Args&&... args) : MakerQueue(makeVec<PLevelMaker>(std::move(t1), std::move(args)...)) {}

  void addMaker(PLevelMaker maker) {
    makers.push_back(std::move(maker));
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (auto& maker : makers)
      maker->make(builder, area);
  }

  private:
  vector<PLevelMaker> makers;
};

class PredicatePrecalc {
  public:
  PredicatePrecalc(const Predicate& predicate, LevelBuilder* builder, Rectangle area)
      : counts(Rectangle(area.topLeft(), area.bottomRight() + Vec2(1, 1))) {
    int px = counts.getBounds().left();
    int py = counts.getBounds().top();
    for (int x : Range(px, counts.getBounds().right()))
      counts[x][py] = 0;
    for (int y : Range(py, counts.getBounds().bottom()))
      counts[px][y] = 0;
    for (Vec2 v : Rectangle(area.topLeft() + Vec2(1, 1), counts.getBounds().bottomRight()))
      counts[v] = (predicate.apply(builder, v - Vec2(1, 1)) ? 1 : 0) +
        counts[v.x - 1][v.y] + counts[v.x][v.y - 1] -counts[v.x - 1][v.y - 1];
  }

  int getCount(Rectangle area) const {
    return counts[area.bottomRight()] + counts[area.topLeft()]
      -counts[area.bottomLeft()] - counts[area.topRight()];
  }

  private:
  Table<int> counts;
};

class RandomLocations : public LevelMaker {
  public:
  RandomLocations(vector<PLevelMaker> _insideMakers, const vector<pair<int, int>>& _sizes, Predicate pred)
      : insideMakers(std::move(_insideMakers)), sizes(_sizes), predicate(sizes.size(), pred) {
    CHECK(insideMakers.size() == sizes.size());
    CHECK(predicate.size() == sizes.size());
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
      Precomputed(LevelBuilder* builder, Rectangle area, Predicate p1, Predicate p2, int minSec, int maxSec)
        : pred1(p1, builder, area), pred2(p2, builder, area), minSecond(minSec), maxSecond(maxSec) {
      }

      bool apply(Rectangle rect) const {
        int numFirst = pred1.getCount(rect);
        int numSecond = pred2.getCount(rect);
        return numSecond >= minSecond && numSecond < maxSecond && numSecond + numFirst == rect.width() * rect.height();
      }

      private:
      PredicatePrecalc pred1;
      PredicatePrecalc pred2;
      int minSecond;
      int maxSecond;
    };

    Precomputed precompute(LevelBuilder* builder, Rectangle area) {
      return Precomputed(builder, area, predicate, second, minSecond, maxSecond);
    }

    private:
    Predicate predicate;
    Predicate second = Predicate::alwaysFalse();
    int minSecond = 0;
    int maxSecond = 1;
  };

  RandomLocations() {}

  void add(PLevelMaker maker, Vec2 size, LocationPredicate pred) {
    insideMakers.push_back(std::move(maker));
    sizes.push_back({size.x, size.y});
    predicate.push_back(pred);
  }

  void setMinDistance(LevelMaker* m1, LevelMaker* m2, double dist) {
    minDistance[{m1, m2}] = dist;
    minDistance[{m2, m1}] = dist;
  }

  void setMaxDistance(LevelMaker* m1, LevelMaker* m2, double dist) {
    maxDistance[{m1, m2}] = dist;
    maxDistance[{m2, m1}] = dist;
  }

  void setMinMargin(LevelMaker *m1, int margin) {
    minMargin[m1] = margin;
  }

  void setMinDistanceLast(LevelMaker* m, double dist) {
    minDistance[{m, insideMakers.back().get()}]  = dist;
    minDistance[{insideMakers.back().get(), m}]  = dist;
  }

  void setMaxDistanceLast(LevelMaker* m, double dist) {
    maxDistance[{m, insideMakers.back().get()}] = dist;
    maxDistance[{insideMakers.back().get(), m}] = dist;
  }

  void setCanOverlap(LevelMaker* m) {
    overlapping.insert(m);
  }

  LevelMaker* getLast() {
    return insideMakers.back().get();
  }


  virtual void make(LevelBuilder* builder, Rectangle area) override {
    PROFILE;
    vector<vector<Vec2>> allowedPositions;
    vector<LevelBuilder::Rot> rotations;
    {
      PROFILE_BLOCK("precomputing");
      for (int i : All(insideMakers)) {
        rotations.push_back(builder->getRandom().choose(
              LevelBuilder::CW0, LevelBuilder::CW1, LevelBuilder::CW2, LevelBuilder::CW3));
        auto maker = insideMakers[i].get();
        auto precomputed = predicate[i].precompute(builder, area);
        vector<Vec2> pos;
        const int margin = getValueMaybe(minMargin, maker).value_or(0);
        int width = sizes[i].first;
        int height = sizes[i].second;
        if (contains({LevelBuilder::CW1, LevelBuilder::CW3}, rotations[i]))
          std::swap(width, height);
        for (int x : Range(area.left() + margin, area.right() - margin - width))
          for (int y : Range(area.top() + margin, area.bottom() - margin - height))
            if (precomputed.apply(Rectangle(x, y, x + width, y + height)))
              pos.push_back(Vec2(x, y));
        allowedPositions.push_back(pos);
      }
    }
    {
      PROFILE_BLOCK("generating positions");
      for (int i : Range(300))
        if (tryMake(builder, allowedPositions, rotations))
          return;
      failGen(); // "Failed to find free space for " << (int)sizes.size() << " areas";
    }
  }

  bool checkDistances(int makerIndex, Rectangle area, const vector<Rectangle>& occupied,
      const vector<optional<double>>& minDist, const vector<optional<double>>& maxDist) {
    for (int j : Range(makerIndex)) {
      auto distance = area.getDistance(occupied[j]);
      if ((maxDist[j] && *maxDist[j] < distance) || (minDist[j] && *minDist[j] > distance))
        return false;
    }
    return true;
  }

  bool checkIntersections(Rectangle area, const vector<Rectangle>& occupied) {
    for (Rectangle r : occupied)
      if (r.intersects(area))
        return false;
    return true;
  }

  bool tryMake(LevelBuilder* builder, const vector<vector<Vec2>>& allowedPositions,
      const vector<LevelBuilder::Rot>& rotations) {
    PROFILE;
    vector<Rectangle> occupied;
    vector<Rectangle> makerBounds;
    for (int makerIndex : All(insideMakers)) {
      PROFILE_BLOCK("maker");
      auto maker = insideMakers[makerIndex].get();
      bool canOverlap = overlapping.count(maker);
      int width = sizes[makerIndex].first;
      int height = sizes[makerIndex].second;
      if (contains({LevelBuilder::CW1, LevelBuilder::CW3}, rotations[makerIndex]))
        std::swap(width, height);
      vector<optional<double>> maxDist(makerIndex);
      vector<optional<double>> minDist(makerIndex);
      for (int j : Range(makerIndex)) {
        maxDist[j] = getValueMaybe(maxDistance, make_pair(insideMakers[j].get(), maker));
        minDist[j] = getValueMaybe(minDistance, make_pair(insideMakers[j].get(), maker));
      }
      auto findGoodPosition = [&] () -> optional<Vec2> {
        for (auto& pos : builder->getRandom().permutation(allowedPositions[makerIndex])) {
          Progress::checkIfInterrupted();
          Rectangle area(pos, pos + Vec2(width, height));
          if ((canOverlap || checkIntersections(area, occupied)) &&
              checkDistances(makerIndex, area, occupied, minDist, maxDist)) {
            return pos;
          }
        }
        return none;
      };
      if (auto pos = findGoodPosition()) {
        occupied.push_back(Rectangle(*pos, *pos + Vec2(width, height)));
        makerBounds.push_back(Rectangle(*pos, *pos + Vec2(sizes[makerIndex].first, sizes[makerIndex].second)));
      } else
        return false;
    }
    CHECK(insideMakers.size() == occupied.size());
    for (int i : All(insideMakers)) {
      PROFILE_BLOCK("insider makers");
      builder->pushMap(makerBounds[i], rotations[i]);
      insideMakers[i]->make(builder, makerBounds[i]);
      builder->popMap();
    }
    return true;
  }

  private:
  vector<PLevelMaker> insideMakers;
  vector<pair<int, int>> sizes;
  vector<LocationPredicate> predicate;
  set<LevelMaker*> overlapping;
  map<pair<LevelMaker*, LevelMaker*>, double> minDistance;
  map<pair<LevelMaker*, LevelMaker*>, double> maxDistance;
  map<LevelMaker*, int> minMargin;
};

class Margin : public LevelMaker {
  public:
  Margin(int s, PLevelMaker in) : left(s), top(s), right(s), bottom(s), inside(std::move(in)) {}
  Margin(int _left, int _top, int _right, int _bottom, PLevelMaker in)
      :left(_left) ,top(_top), right(_right), bottom(_bottom), inside(std::move(in)) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    CHECK(area.width() > left + right && area.height() > top + bottom);
    inside->make(builder, Rectangle(
          area.left() + left,
          area.top() + top,
          area.right() - right,
          area.bottom() - bottom));
  }

  private:
  int left, top, right, bottom;
  PLevelMaker inside;
};

void addAvg(int x, int y, const Table<double>& wys, double& avg, int& num) {
  Vec2 pos(x, y);
  if (pos.inRectangle(wys.getBounds())) {
    avg += wys[pos];
    ++num;
  }
}

struct NoiseInit {
  int topLeft;
  int topRight;
  int bottomRight;
  int bottomLeft;
  int middle;
};

Table<double> genNoiseMap(RandomGen& random, Rectangle area, NoiseInit init, double varianceMult) {
  int width = 1;
  while (width < area.width() - 1 || width < area.height() - 1)
    width *= 2;
  width /= 2;
  ++width;
  Table<double> wys(width, width);
  wys[0][0] = init.topLeft;
  wys[width - 1][0] = init.topRight;
  wys[width - 1][width - 1] = init.bottomRight;
  wys[0][width - 1] = init.bottomLeft;
  wys[(width - 1) / 2][(width - 1) / 2] = init.middle;

  double variance = 0.5;
  double heightDiff = 0.1;
  for (int a = width - 1; a >= 2; a /= 2) {
    if (a < width - 1)
      for (Vec2 pos1 : Rectangle((width - 1) / a, (width - 1) / a)) {
        Vec2 pos = pos1 * a;
        double avg = (wys[pos] + wys[pos.x + a][pos.y] + wys[pos.x][pos.y + a] + wys[pos.x + a][pos.y + a]) / 4;
        wys[pos.x + a / 2][pos.y + a / 2] =
            avg + variance * (random.getDouble() * 2 - 1);
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
          avg / num + variance * (random.getDouble() * 2 - 1);
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
          avg / num + variance * (random.getDouble() * 2 - 1);
    }
    variance *= varianceMult;
  }
  Table<double> ret(area);
  Vec2 offset(area.left(), area.top());
  for (Vec2 v : area) {
    Vec2 lv((v.x - offset.x) * width / area.width(), (v.y - offset.y) * width / area.height());
    ret[v] = wys[lv];
  }
  return ret;
}

void raiseLocalMinima(Table<double>& t) {
  Vec2 minPos = t.getBounds().topLeft();
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
  std::sort(values.begin(), values.end());
  return values;
}

class SetSunlight : public LevelMaker {
  public:
  SetSunlight(double a, Predicate p) : amount(a), pred(p) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (Vec2 v : area)
      if (pred.apply(builder, v))
        builder->setSunlight(v, amount);
  }

  private:
  double amount;
  Predicate pred;
};

static void removeEdge(Table<bool>& values, int thickness) {
  auto bounds = values.getBounds();
  Table<int> distance(bounds, 1000000);
  queue<Vec2> q;
  for (auto v : bounds)
    if (!values[v]) {
      distance[v] = 0;
      q.push(v);
    }
  while (!q.empty()) {
    auto v = q.front();
    q.pop();
    if (distance[v] <= thickness)
      values[v] = false;
    else
      break;
    for (auto neighbor : v.neighbors4())
      if (neighbor.inRectangle(bounds) && distance[neighbor] > distance[v] + 1) {
        distance[neighbor] = distance[v] + 1;
        q.push(neighbor);
      }
  }
}

class Mountains : public LevelMaker {
  public:
  static constexpr double varianceM = 0.45;
  Mountains(double lowland, double hill, NoiseInit init)
      : ratioLowland(lowland), ratioHill(hill), noiseInit(init), varianceMult(varianceM) {
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(builder->getRandom(), area, noiseInit, varianceMult);
    raiseLocalMinima(wys);
    vector<double> values = sortedValues(wys);
    double cutOffLowland = values[(int)(ratioLowland * double(values.size() - 1))];
    double cutOffHill = values[(int)((ratioHill + ratioLowland) * double(values.size() - 1))];
    double cutOffDarkness = values[(int)((ratioHill + ratioLowland + 1.0) * 0.5 * double(values.size() - 1))];
    int dCnt = 0, mCnt = 0, hCnt = 0, lCnt = 0;
    Table<bool> isMountain(area, false);
    for (Vec2 v : area) {
      builder->setHeightMap(v, wys[v]);
      if (wys[v] >= cutOffHill) {
        isMountain[v] = true;
        builder->putFurniture(v, FurnitureType::FLOOR);
        auto type = FurnitureType::MOUNTAIN;
        builder->putFurniture(v, {type, TribeId::getKeeper()}, SquareAttrib::MOUNTAIN);
        builder->setSunlight(v, max(0.0, 1. - (wys[v] - cutOffHill) / (cutOffDarkness - cutOffHill)));
        builder->setCovered(v, true);
        ++mCnt;
      }
      else if (wys[v] >= cutOffLowland) {
        builder->putFurniture(v, FurnitureType::HILL, SquareAttrib::HILL);
        ++hCnt;
      }
      else {
        builder->putFurniture(v, FurnitureType::GRASS, SquareAttrib::LOWLAND);
        ++lCnt;
      }
    }
    // Remove the MOUNTAIN2 tiles that are to close to the edge of the mountain
    removeEdge(isMountain, 20);
    for (auto v : area)
      if (isMountain[v])
        builder->putFurniture(v, {FurnitureType::MOUNTAIN2, TribeId::getKeeper()}, SquareAttrib::MOUNTAIN);
    INFO << "Terrain distribution " << dCnt << " darkness, " << mCnt << " mountain, " << hCnt << " hill, " << lCnt << " lowland";
  }

  private:
  double ratioLowland;
  double ratioHill;
  NoiseInit noiseInit;
  double varianceMult;
};

class Roads : public LevelMaker {
  public:
  Roads() {}

  bool makeBridge(LevelBuilder* builder, Vec2 pos) {
    return !builder->canNavigate(pos, {MovementTrait::WALK}) && builder->canNavigate(pos, {MovementTrait::SWIM});
  }

  double getValue(LevelBuilder* builder, Vec2 pos) {
    if ((!builder->canNavigate(pos, MovementType({MovementTrait::WALK, MovementTrait::SWIM})) &&
         !builder->hasAttrib(pos, SquareAttrib::ROAD_CUT_THRU)) ||
        builder->hasAttrib(pos, SquareAttrib::NO_ROAD))
      return ShortestPath::infinity;
    if (makeBridge(builder, pos))
      return 10;
    if (builder->isFurnitureType(pos, FurnitureType::ROAD) || builder->isFurnitureType(pos, FurnitureType::BRIDGE))
      return 1;
    return 1 + pow(1 + builder->getHeightMap(pos), 2);
  }

  FurnitureType getRoadType(LevelBuilder* builder, Vec2 pos) {
    if (makeBridge(builder, pos))
      return FurnitureType::BRIDGE;
    else
      return FurnitureType::ROAD;
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    vector<Vec2> points;
    for (Vec2 v : area)
      if (builder->hasAttrib(v, SquareAttrib::CONNECT_ROAD)) {
        points.push_back(v);
        INFO << "Connecting point " << v;
      }
    for (int ind : Range(1, points.size())) {
      Vec2 p1 = points[ind];
      Vec2 p2 = points[ind - 1];
      ShortestPath path(area,
          [=](Vec2 pos) { return (pos == p1 || pos == p2) ? 1 : getValue(builder, pos); },
          [] (Vec2 from, Vec2 to) { return from.dist4(to); },
          Vec2::directions4(builder->getRandom()), p1, p2);
      for (Vec2 v = p2; v != p1; v = path.getNextMove(v)) {
        if (!path.isReachable(v))
          failGen();
        auto roadType = getRoadType(builder, v);
        if (v != p2 && v != p1 && !builder->isFurnitureType(v, roadType))
          builder->putFurniture(v, roadType);
      }
    }
  }
};

class StartingPos : public LevelMaker {
  public:

  StartingPos(Predicate pred, StairKey key) : predicate(pred), stairKey(key) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (Vec2 pos : area)
      if (predicate.apply(builder, pos))
        builder->modSquare(pos)->setLandingLink(stairKey);
  }

  private:
  Predicate predicate;
  StairKey stairKey;
};

class TransferPos : public LevelMaker {
  public:

  TransferPos(Predicate pred, StairKey key, int w) : predicate(pred), stairKey(key), width(w) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    bool found = false;
    for (Vec2 pos : area)
      if (((pos.x - area.left() < width) || (pos.y - area.top() < width) ||
          (area.right() - pos.x <= width) || (area.bottom() - pos.y <= width)) &&
          predicate.apply(builder, pos)) {
        builder->modSquare(pos)->setLandingLink(stairKey);
        found = true;
      }
    checkGen(found);
  }

  private:
  Predicate predicate;
  StairKey stairKey;
  int width;
};

class Forrest : public LevelMaker {
  public:
  Forrest(double _ratio, double _density, FurnitureType _onType, FurnitureFactory _factory)
      : ratio(_ratio), density(_density), factory(_factory), onType(_onType) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(builder->getRandom(), area, {0, 0, 0, 0, 0}, 0.65);
    vector<double> values = sortedValues(wys);
    double cutoff = values[values.size() * ratio];
    for (Vec2 v : area)
      if (builder->isFurnitureType(v, onType) && builder->canNavigate(v, {MovementTrait::WALK}) && wys[v] < cutoff) {
        if (builder->getRandom().getDouble() <= density)
          builder->putFurniture(v, factory);
        builder->addAttrib(v, SquareAttrib::FORREST);
      }
  }

  private:
  double ratio;
  double density;
  FurnitureFactory factory;
  FurnitureType onType;
};

class PlaceCollective : public LevelMaker {
  public:
  PlaceCollective(CollectiveBuilder* c, Predicate pred = Predicate::alwaysTrue())
      : collective(NOTNULL(c)), predicate(pred) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    if (!collective->hasCentralPoint())
      collective->setCentralPoint(builder->toGlobalCoordinates(area).middle());
    collective->addArea(builder->toGlobalCoordinates(area.getAllSquares()
        .filter([&](Vec2 pos) { return predicate.apply(builder, pos); })));
    builder->addCollective(collective);
  }

  private:
  CollectiveBuilder* collective;
  Predicate predicate;
};

class ForEachSquare : public LevelMaker {
  public:
  ForEachSquare(function<void(LevelBuilder*, Vec2 pos)> f, Predicate _onPred = Predicate::alwaysTrue())
    : fun(f), onPred(_onPred) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (Vec2 v : area)
      if (onPred.apply(builder, v))
        fun(builder, v);
  }

  protected:
  function<void(LevelBuilder*, Vec2 pos)> fun;
  Predicate onPred;
};

class AddAttrib : public ForEachSquare {
  public:
  AddAttrib(SquareAttrib attr, Predicate onPred = Predicate::alwaysTrue())
      : ForEachSquare([attr](LevelBuilder* b, Vec2 pos) { b->addAttrib(pos, attr); }, onPred) {}
};

class RemoveAttrib : public ForEachSquare {
  public:
  RemoveAttrib(SquareAttrib attr, Predicate onPred = Predicate::alwaysTrue())
    : ForEachSquare([attr](LevelBuilder* b, Vec2 pos) { b->removeAttrib(pos, attr); }, onPred) {}
};

enum class StairDirection {
  UP, DOWN
};

class Stairs : public LevelMaker {
  public:
  Stairs(StairDirection dir, StairKey k, Predicate onPred, optional<SquareAttrib> _setAttr = none)
    : direction(dir), key(k), onPredicate(onPred), setAttr(_setAttr) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    auto type = direction == StairDirection::DOWN ? FurnitureType::DOWN_STAIRS : FurnitureType::UP_STAIRS;
    vector<Vec2> allPos;
    for (Vec2 v : area)
      if (onPredicate.apply(builder, v) && builder->canPutFurniture(v, Furniture::getLayer(type)))
        allPos.push_back(v);
    checkGen(allPos.size() > 0);
    Vec2 pos = allPos[builder->getRandom().get(allPos.size())];
    builder->putFurniture(pos, FurnitureParams{type, TribeId::getHostile()});
    builder->setLandingLink(pos, key);
  }

  private:
  StairDirection direction;
  StairKey key;
  Predicate onPredicate;
  optional<SquareAttrib> setAttr;
};

class ShopMaker : public LevelMaker {
  public:
  ShopMaker(SettlementInfo& info, int _numItems)
      : factory(*info.shopFactory), tribe(info.tribe), numItems(_numItems),
        building(getBuildingInfo(info)), shopkeeperDead(info.shopkeeperDead)  {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    PCreature shopkeeper = CreatureFactory::getShopkeeper(builder->toGlobalCoordinates(area), tribe);
    vector<Vec2> pos;
    for (Vec2 v : area)
      if (builder->canNavigate(v, MovementTrait::WALK) && builder->isFurnitureType(v, building.floorInside))
        pos.push_back(v);
    Vec2 shopkeeperPos = pos[builder->getRandom().get(pos.size())];
    if (!shopkeeperDead)
      builder->putCreature(shopkeeperPos, std::move(shopkeeper));
    else {
      builder->putItems(shopkeeperPos, shopkeeper->getEquipment().removeAllItems(shopkeeper.get()));
      builder->putItems(shopkeeperPos, shopkeeper->generateCorpse(true));
    }
    builder->putFurniture(pos[builder->getRandom().get(pos.size())], FurnitureParams{FurnitureType::GROUND_TORCH, tribe});
    for (int i : Range(numItems)) {
      Vec2 v = pos[builder->getRandom().get(pos.size())];
      builder->putItems(v, factory.random());
    }
  }

  private:
  ItemFactory factory;
  TribeId tribe;
  int numItems;
  BuildingType building;
  bool shopkeeperDead;
};

class LevelExit : public LevelMaker {
  public:
  LevelExit(FurnitureFactory e, int _minCornerDist = 1)
      : exit(e), minCornerDist(_minCornerDist) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Vec2 pos = getRandomExit(builder->getRandom(), area, minCornerDist);
    builder->putFurniture(pos, exit);
  }

  private:
  FurnitureFactory exit;
  optional<SquareAttrib> attrib;
  int minCornerDist;
};

class Division : public LevelMaker {
  public:
  Division(double _vRatio, double _hRatio,
      PLevelMaker _upperLeft, PLevelMaker _upperRight, PLevelMaker _lowerLeft, PLevelMaker _lowerRight,
      optional<SquareChange> _wall = none) : vRatio(_vRatio), hRatio(_hRatio),
      upperLeft(std::move(_upperLeft)), upperRight(std::move(_upperRight)), lowerLeft(std::move(_lowerLeft)),
      lowerRight(std::move(_lowerRight)), wall(_wall) {}

  Division(double _hRatio, PLevelMaker _left, PLevelMaker _right, optional<SquareChange> _wall = none)
      : vRatio(-1), hRatio(_hRatio), upperLeft(std::move(_left)), upperRight(std::move(_right)), wall(_wall) {}

  Division(bool, double _vRatio, PLevelMaker _top, PLevelMaker _bottom, optional<SquareChange> _wall = none)
      : vRatio(_vRatio), hRatio(-1), upperLeft(std::move(_top)), lowerLeft(std::move(_bottom)), wall(_wall) {}

  void makeHorizDiv(LevelBuilder* builder, Rectangle area) {
    int hDiv = area.left() + min(area.width() - 1, max(1, (int) (hRatio * area.width())));
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.left(), area.top(), hDiv, area.bottom()));
    if (upperRight)
      upperRight->make(builder, Rectangle(hDiv + (wall ? 1 : 0), area.top(), area.right(), area.bottom()));
    if (wall)
      for (int i : Range(area.top(), area.bottom()))
        wall->apply(builder, Vec2(hDiv, i));
  }

  void makeVertDiv(LevelBuilder* builder, Rectangle area) {
    int vDiv = area.top() + min(area.height() - 1, max(1, (int) (vRatio * area.height())));
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.left(), area.top(), area.right(), vDiv));
    if (lowerLeft)
      lowerLeft->make(builder, Rectangle(area.left(), vDiv + (wall ? 1 : 0), area.right(), area.bottom()));
    if (wall)
      for (int i : Range(area.left(), area.right()))
        wall->apply(builder, Vec2(i, vDiv));
  }

  void makeDiv(LevelBuilder* builder, Rectangle area) {
    int vDiv = area.top() + min(area.height() - 1, max(1, (int) (vRatio * area.height())));
    int hDiv = area.left() + min(area.width() - 1, max(1, (int) (hRatio * area.width())));
    int wallSpace = wall ? 1 : 0;
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.left(), area.top(), hDiv, vDiv));
    if (upperRight)
      upperRight->make(builder, Rectangle(hDiv + wallSpace, area.top(), area.right(), vDiv));
    if (lowerLeft)
      lowerLeft->make(builder, Rectangle(area.left(), vDiv + wallSpace, hDiv, area.bottom()));
    if (lowerRight)
      lowerRight->make(builder, Rectangle(hDiv + wallSpace, vDiv + wallSpace, area.right(), area.bottom()));
    if (wall) {
      for (int i : Range(area.top(), area.bottom()))
        wall->apply(builder, Vec2(hDiv, i));
      for (int i : Range(area.left(), area.right()))
        wall->apply(builder, Vec2(i, vDiv));
    }
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    if (vRatio < 0)
      makeHorizDiv(builder, area);
    else if (hRatio < 0)
      makeVertDiv(builder, area);
    else
      makeDiv(builder, area);
  }

  private:
  double vRatio, hRatio;
  PLevelMaker upperLeft;
  PLevelMaker upperRight;
  PLevelMaker lowerLeft;
  PLevelMaker lowerRight;
  optional<SquareChange> wall;
};

class AreaCorners : public LevelMaker {
  public:
  AreaCorners(PLevelMaker _maker, Vec2 _size, vector<PLevelMaker> _insideMakers)
      : maker(std::move(_maker)), size(_size), insideMakers(std::move(_insideMakers)) {}

  vector<Rectangle> getCorners(Rectangle area) {
    return {
      Rectangle(area.topLeft(), area.topLeft() + size),
      Rectangle(area.topRight() - Vec2(size.x, 0), area.topRight() + Vec2(0, size.y)),
      Rectangle(area.bottomLeft() - Vec2(0, size.y), area.bottomLeft() + Vec2(size.x, 0)),
      Rectangle(area.bottomRight() - size, area.bottomRight())};
  }

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    vector<Rectangle> corners = builder->getRandom().permutation(getCorners(area));
    for (int i : All(corners)) {
      maker->make(builder, corners[i]);
      if (i < insideMakers.size())
        insideMakers[i]->make(builder, corners[i]);
    }
  }

  private:
  PLevelMaker maker;
  Vec2 size;
  vector<PLevelMaker> insideMakers;
};

class CastleExit : public LevelMaker {
  public:
  CastleExit(TribeId _guardTribe, BuildingType _building, optional<CreatureId> _guardId)
    : guardTribe(_guardTribe), building(_building), guardId(_guardId) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    Vec2 loc(area.right() - 1, area.middle().y);
    builder->resetFurniture(loc + Vec2(2, 0), building.floorInside);
    if (building.door)
      builder->putFurniture(loc + Vec2(2, 0), *building.door);
    builder->addAttrib(loc + Vec2(2, 0), SquareAttrib::CONNECT_ROAD);
    vector<Vec2> walls { Vec2(1, -2), Vec2(2, -2), Vec2(2, -1), Vec2(2, 1), Vec2(2, 2), Vec2(1, 2)};
    for (Vec2 v : walls)
      builder->putFurniture(loc + v, building.wall);
    vector<Vec2> floor { Vec2(1, -1), Vec2(1, 0), Vec2(1, 1), Vec2(0, -1), Vec2(0, 0), Vec2(0, 1) };
    for (Vec2 v : floor)
      builder->resetFurniture(loc + v, building.floorInside);
    vector<Vec2> guardPos { Vec2(1, 1), Vec2(1, -1) };
    if (guardId)
      for (Vec2 pos : guardPos) {
        builder->putCreature(loc + pos, CreatureFactory::fromId(*guardId, guardTribe,
            MonsterAIFactory::stayInLocation(
                builder->toGlobalCoordinates(Rectangle(loc + pos, loc + pos + Vec2(1, 1))), false)));
    }
  }

  private:
  TribeId guardTribe;
  BuildingType building;
  optional<CreatureId> guardId;
};

class AddMapBorder : public LevelMaker {
  public:
  AddMapBorder(int w) : width(w) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (Vec2 v : area)
      if (!v.inRectangle(area.minusMargin(width)))
        builder->setUnavailable(v);
  }

  private:
  int width;
};

class BorderGuard : public LevelMaker {
  public:

  BorderGuard(PLevelMaker inside, SquareChange c)
      : change(c), insideMaker(std::move(inside)) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    for (int i : Range(area.left(), area.right())) {
      change.apply(builder, Vec2(i, area.top()));
      change.apply(builder, Vec2(i, area.bottom() - 1));
    }
    for (int i : Range(area.top(), area.bottom())) {
      change.apply(builder, Vec2(area.left(), i));
      change.apply(builder, Vec2(area.right() - 1, i));
    }
    insideMaker->make(builder, Rectangle(area.left() + 1, area.top() + 1, area.right() - 1, area.bottom() - 1));
  }

  private:
  SquareChange change;
  PLevelMaker insideMaker;

};
}



static PMakerQueue stockpileMaker(StockpileInfo info) {
  auto floor = FurnitureType::FLOOR_STONE1;
  ItemFactory items;
  optional<FurnitureType> furniture;
  switch (info.type) {
    case StockpileInfo::GOLD:
      furniture = FurnitureType::TREASURE_CHEST;
      items = ItemFactory::singleType(ItemType::GoldPiece{});
      break;
    case StockpileInfo::MINERALS:
      items = ItemFactory::minerals();
      break;
  }
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(floor));
  if (furniture)
    queue->addMaker(unique<Empty>(SquareChange(*furniture)));
  queue->addMaker(unique<Items>(items, info.number, info.number + 1, Predicate::alwaysTrue(), !!furniture));
  return queue;
}

PLevelMaker LevelMaker::cryptLevel(RandomGen& random, SettlementInfo info) {
  auto queue = unique<MakerQueue>();
  BuildingType building = getBuildingInfo(info);
  queue->addMaker(unique<Empty>(SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN)));
  queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(unique<RoomMaker>(random.get(8, 15), 3, 5));
  queue->addMaker(unique<Connector>(building.door, 0));
  if (info.furniture)
    queue->addMaker(unique<Furnitures>(Predicate::attrib(SquareAttrib::EMPTY_ROOM), 0.3, *info.furniture));
  for (StairKey key : info.downStairs)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::type(FurnitureType::FLOOR)));
  for (StairKey key : info.upStairs)
    queue->addMaker(unique<Stairs>(StairDirection::UP, key, Predicate::type(FurnitureType::FLOOR)));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  queue->addMaker(unique<Items>(ItemFactory::dungeon(), 5, 10));
  return unique<BorderGuard>(std::move(queue), SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN));
}

PLevelMaker LevelMaker::mazeLevel(RandomGen& random, SettlementInfo info) {
  auto queue = unique<MakerQueue>();
  BuildingType building = getBuildingInfo(info);
  queue->addMaker(unique<Empty>(SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN)));
  queue->addMaker(unique<RoomMaker>(random.get(8, 15), 3, 5));
  queue->addMaker(unique<Connector>(building.door, 0.75));
  if (info.furniture)
    queue->addMaker(unique<Furnitures>(Predicate::attrib(SquareAttrib::EMPTY_ROOM), 0.3, *info.furniture));
  for (StairKey key : info.downStairs)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::type(FurnitureType::FLOOR)));
  for (StairKey key : info.upStairs)
    queue->addMaker(unique<Stairs>(StairDirection::UP, key, Predicate::type(FurnitureType::FLOOR)));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  queue->addMaker(unique<Items>(ItemFactory::dungeon(), 5, 10));
  return unique<BorderGuard>(std::move(queue), SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN));
}

static PMakerQueue getElderRoom(SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  PMakerQueue elderRoom = unique<MakerQueue>();
  if (info.elderLoot)
    elderRoom->addMaker(unique<Items>(ItemFactory::singleType(*info.elderLoot), 1, 2));
  return elderRoom;
}

static PMakerQueue village2(RandomGen& random, SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<PlaceCollective>(info.collective));
  vector<PLevelMaker> insideMakers = makeVec<PLevelMaker>(getElderRoom(info));
  for (auto& elem : info.stockpiles)
    insideMakers.push_back(stockpileMaker(elem));
  if (info.shopFactory)
    insideMakers.push_back(unique<ShopMaker>(info, random.get(8, 16)));
  queue->addMaker(unique<Buildings>(6, 10, 3, 4, building, false, std::move(insideMakers)));
  if (info.furniture)
    queue->addMaker(unique<Furnitures>(Predicate::attrib(SquareAttrib::EMPTY_ROOM), 0.3, *info.furniture));
  if (info.outsideFeatures)
    queue->addMaker(unique<Furnitures>(Predicate::type(building.floorOutside), 0.01, *info.outsideFeatures));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorOutside)));
  return queue;
}

static PMakerQueue village(RandomGen& random, SettlementInfo info, int minRooms, int maxRooms) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(unique<UniformBlob>(building.floorOutside, none, 0.6));
  vector<PLevelMaker> insideMakers = makeVec<PLevelMaker>(
 //     hatchery(CreatureFactory::singleType(info.tribe, CreatureId::PIG), random.get(2, 5)),
      getElderRoom(info));
  if (info.shopFactory)
    insideMakers.push_back(unique<ShopMaker>(info, random.get(8, 16)));
  for (auto& elem : info.stockpiles)
    insideMakers.push_back(stockpileMaker(elem));
  queue->addMaker(unique<Buildings>(minRooms, maxRooms, 3, 7, building, true, std::move(insideMakers)));
  if (info.furniture)
    queue->addMaker(unique<Furnitures>(Predicate::attrib(SquareAttrib::EMPTY_ROOM), 0.3, *info.furniture));
  if (info.outsideFeatures)
    queue->addMaker(unique<Furnitures>(
        Predicate::type(building.floorOutside) &&
        Predicate::attrib(SquareAttrib::BUILDINGS_CENTER), 0.2, *info.outsideFeatures, SquareAttrib::NO_ROAD));
  for (StairKey key : info.downStairs)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::attrib(SquareAttrib::EMPTY_ROOM)));
  for (StairKey key : info.upStairs)
    queue->addMaker(unique<Stairs>(StairDirection::UP, key, Predicate::attrib(SquareAttrib::EMPTY_ROOM)));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorOutside)));
  return queue;
}

static PMakerQueue cottage(SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(building.floorOutside));
  auto room = getElderRoom(info);
  for (StairKey key : info.upStairs)
    room->addMaker(unique<Stairs>(StairDirection::UP, key, Predicate::type(building.floorInside), none));
  for (StairKey key : info.downStairs)
    room->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::type(building.floorInside), none));
  if (info.furniture)
    room->addMaker(unique<Furnitures>(Predicate::type(building.floorInside), 0.3, *info.furniture));
  if (info.outsideFeatures)
    room->addMaker(unique<Furnitures>(Predicate::type(building.floorOutside), 0.1, *info.outsideFeatures));
  queue->addMaker(unique<Buildings>(1, 2, 5, 7, building, false, std::move(room), false));
  queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorOutside)));
  return queue;
}

static PMakerQueue forrestCottage(SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>();
  auto room = getElderRoom(info);
  for (StairKey key : info.upStairs)
    room->addMaker(unique<Stairs>(StairDirection::UP, key, Predicate::type(building.floorInside), none));
  for (StairKey key : info.downStairs)
    room->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::type(building.floorInside), none));
  if (info.furniture)
    room->addMaker(unique<Furnitures>(Predicate::type(building.floorInside), 0.3, *info.furniture));
  if (info.outsideFeatures)
    room->addMaker(unique<Furnitures>(Predicate::type(building.floorOutside), 0.1, *info.outsideFeatures));
  queue->addMaker(unique<Buildings>(1, 3, 3, 4, building, false, std::move(room), false));
  queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorOutside)));
  return queue;
}

static PMakerQueue castle(RandomGen& random, SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  auto castleRoom = [&] { return unique<BorderGuard>(unique<Empty>(SquareChange::reset(building.floorInside).add(SquareAttrib::EMPTY_ROOM)),
      SquareChange(building.wall, SquareAttrib::ROOM_WALL)); };
  auto leftSide = unique<MakerQueue>();
  leftSide->addMaker(unique<Division>(true, random.getDouble(0.5, 0.5),
      unique<Margin>(1, -1, -1, 1, castleRoom()), unique<Margin>(1, 1, -1, -1, castleRoom())));
  leftSide->addMaker(getElderRoom(info));
  auto inside = unique<MakerQueue>();
  vector<PLevelMaker> insideMakers;
  if (info.shopFactory)
    insideMakers.push_back(unique<ShopMaker>(info, random.get(8, 16)));
  inside->addMaker(unique<Division>(random.getDouble(0.25, 0.4), std::move(leftSide),
        unique<Buildings>(1, 3, 3, 6, building, false, std::move(insideMakers), false),
            SquareChange(building.wall, SquareAttrib::ROOM_WALL)));
  auto insidePlusWall = unique<MakerQueue>();
  if (info.outsideFeatures)
    inside->addMaker(unique<Furnitures>(Predicate::type(building.floorOutside), 0.18, *info.outsideFeatures));
  if (info.furniture)
    inside->addMaker(unique<Furnitures>(Predicate::attrib(SquareAttrib::EMPTY_ROOM), 0.35, *info.furniture));
  insidePlusWall->addMaker(unique<Empty>(SquareChange::reset(building.floorOutside)));
  insidePlusWall->addMaker(unique<BorderGuard>(std::move(inside), building.wall));
  auto queue = unique<MakerQueue>();
  int insideMargin = 2;
  queue->addMaker(unique<Margin>(insideMargin, unique<PlaceCollective>(info.collective)));
  queue->addMaker(unique<Margin>(insideMargin, std::move(insidePlusWall)));
  vector<PLevelMaker> cornerMakers;
  for (auto& elem : info.stockpiles)
    cornerMakers.push_back(unique<Margin>(1, stockpileMaker(elem)));
  queue->addMaker(unique<AreaCorners>(
      unique<BorderGuard>(unique<Empty>(SquareChange::reset(building.floorInside).add(SquareAttrib::CASTLE_CORNER)),
          SquareChange(building.wall, SquareAttrib::ROOM_WALL)),
      Vec2(5, 5),
      std::move(cornerMakers)));
  queue->addMaker(unique<Margin>(insideMargin, unique<Connector>(building.door, 1, 18)));
  queue->addMaker(unique<Margin>(insideMargin, unique<CastleExit>(info.tribe, building, info.guardId)));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorOutside)));
  for (StairKey key : info.downStairs)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key,
          Predicate::attrib(SquareAttrib::CASTLE_CORNER) &&
          Predicate::type(building.floorInside), none));
  queue->addMaker(unique<StartingPos>(Predicate::type(FurnitureType::MUD), StairKey::heroSpawn()));
  queue->addMaker(unique<AddAttrib>(SquareAttrib::NO_DIG, Predicate::type(building.wall)));
  return queue;
}

static PMakerQueue castle2(RandomGen& random, SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  auto inside = unique<MakerQueue>();
  auto insideMaker = unique<MakerQueue>(
      getElderRoom(info),
      stockpileMaker(info.stockpiles.getOnlyElement()));
  inside->addMaker(unique<Buildings>(1, 2, 3, 4, building, false, std::move(insideMaker), false));
  auto insidePlusWall = unique<MakerQueue>();
  insidePlusWall->addMaker(unique<Empty>(building.floorOutside));
  insidePlusWall->addMaker(unique<BorderGuard>(std::move(inside), building.wall));
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(std::move(insidePlusWall));
  queue->addMaker(unique<Connector>(building.door, 1, 18));
  queue->addMaker(unique<CastleExit>(info.tribe, building, *info.guardId));
  if (info.outsideFeatures)
    queue->addMaker(unique<Furnitures>(Predicate::type(building.floorOutside), 0.05, *info.outsideFeatures));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorOutside)));
  queue->addMaker(unique<AddAttrib>(SquareAttrib::NO_DIG, Predicate::type(building.wall)));
  return queue;
}

static PMakerQueue tower(RandomGen& random, SettlementInfo info, bool withExit) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(SquareChange(FurnitureType::FLOOR, building.wall)));
  if (withExit)
    queue->addMaker(unique<LevelExit>(*building.door, 2));
  queue->addMaker(unique<Margin>(1, unique<Empty>(SquareChange::reset(building.floorInside))));
  queue->addMaker(unique<Margin>(1, unique<AddAttrib>(SquareAttrib::ROOM)));
  queue->addMaker(unique<RemoveAttrib>(SquareAttrib::ROAD_CUT_THRU));
  if (info.collective)
    queue->addMaker(unique<PlaceCollective>(info.collective));
  PLevelMaker downStairs;
  for (StairKey key : info.downStairs)
    downStairs = unique<Stairs>(StairDirection::DOWN, key, Predicate::type(building.floorInside));
  PLevelMaker upStairs;
  for (StairKey key : info.upStairs)
    upStairs = unique<Stairs>(StairDirection::UP, key, Predicate::type(building.floorInside));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::type(building.floorInside)));
  queue->addMaker(unique<Division>(0.5, 0.5, std::move(upStairs), nullptr, nullptr, std::move(downStairs)));
  if (info.furniture)
    queue->addMaker(unique<Furnitures>(Predicate::type(building.floorInside), 0.5, *info.furniture));
  if (info.corpses)
    queue->addMaker(unique<Corpses>(*info.corpses));
  return queue;
}

PLevelMaker LevelMaker::towerLevel(RandomGen& random, SettlementInfo info) {
  return PLevelMaker(tower(random, info, false));
}

Vec2 getSize(RandomGen& random, SettlementType type) {
  switch (type) {
    case SettlementType::WITCH_HOUSE:
    case SettlementType::CEMETERY:
    case SettlementType::MOUNTAIN_LAKE:
    case SettlementType::SMALL_VILLAGE: return {15, 15};
    case SettlementType::SWAMP: return {random.get(12, 16), random.get(12, 16)};
    case SettlementType::COTTAGE: return {random.get(8, 10), random.get(8, 10)};
    case SettlementType::FORREST_COTTAGE: return {15, 15};
    case SettlementType::FOREST: return {18, 13};
    case SettlementType::FORREST_VILLAGE: return {20, 20};
    case SettlementType::VILLAGE:
    case SettlementType::ANT_NEST:  return {20, 20};
    case SettlementType::CASTLE: return {30, 20};
    case SettlementType::CASTLE2: return {13, 13};
    case SettlementType::MINETOWN: return {30, 20};
    case SettlementType::SMALL_MINETOWN: return {15, 15};
    case SettlementType::CAVE: return {12, 12};
    case SettlementType::SPIDER_CAVE: return {12, 12};
    case SettlementType::VAULT: return {10, 10};
    case SettlementType::TOWER: return {5, 5};
    case SettlementType::ISLAND_VAULT_DOOR:
    case SettlementType::ISLAND_VAULT: return {6, 6};
  }
}

RandomLocations::LocationPredicate getSettlementPredicate(SettlementType type) {
  switch (type) {
    case SettlementType::FOREST:
    case SettlementType::FORREST_COTTAGE:
    case SettlementType::FORREST_VILLAGE:
      return !Predicate::attrib(SquareAttrib::RIVER) && Predicate::attrib(SquareAttrib::FORREST);
    case SettlementType::CAVE:
      return RandomLocations::LocationPredicate(
          Predicate::attrib(SquareAttrib::MOUNTAIN), Predicate::attrib(SquareAttrib::HILL), 5, 15);
    case SettlementType::VAULT:
    case SettlementType::ANT_NEST:
    case SettlementType::SMALL_MINETOWN:
    case SettlementType::MINETOWN:
      return Predicate::attrib(SquareAttrib::MOUNTAIN);
    case SettlementType::SPIDER_CAVE:
      return RandomLocations::LocationPredicate(
          Predicate::attrib(SquareAttrib::MOUNTAIN),
          Predicate::attrib(SquareAttrib::CONNECTOR), 1, 2);
    case SettlementType::MOUNTAIN_LAKE:
    case SettlementType::ISLAND_VAULT:
      return Predicate::attrib(SquareAttrib::MOUNTAIN);
    case SettlementType::ISLAND_VAULT_DOOR:
      return RandomLocations::LocationPredicate(
            Predicate::attrib(SquareAttrib::MOUNTAIN) && !Predicate::attrib(SquareAttrib::RIVER),
            Predicate::attrib(SquareAttrib::RIVER), 10, 30);
    default:
      return Predicate::attrib(SquareAttrib::LOWLAND) &&
          !Predicate::attrib(SquareAttrib::RIVER);
  }
}

static PMakerQueue genericMineTownMaker(RandomGen& random, SettlementInfo info, int numCavern, int maxCavernSize,
    int numRooms, int minRoomSize, int maxRoomSize) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>();
  auto caverns = unique<RandomLocations>();
  vector<PLevelMaker> vCavern;
  vector<pair<int, int>> sizes;
  for (int i : Range(numCavern)) {
    caverns->add(unique<UniformBlob>(building.floorInside),
        Vec2(random.get(5, maxCavernSize), random.get(5, maxCavernSize)),
        Predicate::alwaysTrue());
    caverns->setCanOverlap(caverns->getLast());
  }
  queue->addMaker(std::move(caverns));
  vector<PLevelMaker> roomInsides;
  if (info.shopFactory)
    roomInsides.push_back(unique<ShopMaker>(info, random.get(8, 16)));
  for (auto& elem : info.stockpiles)
    roomInsides.push_back(stockpileMaker(elem));
  queue->addMaker(unique<RoomMaker>(numRooms, minRoomSize, maxRoomSize, SquareChange::none(), none,
      unique<Empty>(SquareChange(building.floorInside, ifTrue(!info.dontConnectCave, SquareAttrib::CONNECT_CORRIDOR))),
      std::move(roomInsides), true));
  queue->addMaker(unique<Connector>(none, 0));
  Predicate featurePred = Predicate::attrib(SquareAttrib::EMPTY_ROOM) && Predicate::type(building.floorInside);
  for (StairKey key : info.downStairs)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key, featurePred));
  for (StairKey key : info.upStairs)
    queue->addMaker(unique<Stairs>(StairDirection::UP, key, featurePred));
  if (info.furniture)
    queue->addMaker(unique<Furnitures>(featurePred, 0.3, *info.furniture));
  if (info.outsideFeatures)
    queue->addMaker(unique<Furnitures>(Predicate::type(building.floorInside), 0.09, *info.outsideFeatures));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  queue->addMaker(unique<PlaceCollective>(info.collective, Predicate::canEnter(MovementTrait::WALK)));
  return queue;
}

static PMakerQueue mineTownMaker(RandomGen& random, SettlementInfo info) {
  return genericMineTownMaker(random, info, 10, 12, random.get(5, 7), 6, 8);
}

static PMakerQueue antNestMaker(RandomGen& random, SettlementInfo info) {
  auto ret = genericMineTownMaker(random, info, 4, 6, random.get(5, 7), 3, 4);
  if (info.dontConnectCave)
    ret->addMaker(unique<AddAttrib>(SquareAttrib::NO_DIG));
  return ret;
}

static PMakerQueue smallMineTownMaker(RandomGen& random, SettlementInfo info) {
  auto ret = genericMineTownMaker(random, info, 2, 7, random.get(3, 5), 5, 7);
  if (info.dontConnectCave)
    ret->addMaker(unique<AddAttrib>(SquareAttrib::NO_DIG));
  return ret;
}

static PMakerQueue vaultMaker(SettlementInfo info) {
  auto queue = unique<MakerQueue>();
  BuildingType building = getBuildingInfo(info);
  if (!info.dontConnectCave)
    queue->addMaker(unique<UniformBlob>(SquareChange::reset(building.floorOutside, SquareAttrib::CONNECT_CORRIDOR), none));
  else
    queue->addMaker(unique<UniformBlob>(SquareChange::reset(building.floorOutside)));
  auto insidePredicate = Predicate::type(building.floorOutside) && Predicate::canEnter(MovementTrait::WALK);
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, insidePredicate));
  if (info.shopFactory)
    queue->addMaker(unique<Items>(*info.shopFactory, 16, 20, insidePredicate));
  queue->addMaker(unique<PlaceCollective>(info.collective, insidePredicate));
  if (info.dontConnectCave)
    queue->addMaker(unique<AddAttrib>(SquareAttrib::NO_DIG));
  return queue;
}

static PMakerQueue spiderCaveMaker(SettlementInfo info) {
  auto queue = unique<MakerQueue>();
  BuildingType building = getBuildingInfo(info);
  auto inside = unique<MakerQueue>();
  inside->addMaker(unique<UniformBlob>(SquareChange::reset(building.floorOutside, SquareAttrib::CONNECT_CORRIDOR), none));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  if (info.shopFactory)
    inside->addMaker(unique<Items>(*info.shopFactory, 5, 10));
  queue->addMaker(unique<Margin>(3, std::move(inside)));
  queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(unique<Connector>(none, 0));
  return queue;
}

static PMakerQueue islandVaultMaker(RandomGen& random, SettlementInfo info, bool door) {
  BuildingType building = getBuildingInfo(info);
  auto inside = unique<MakerQueue>();
  inside->addMaker(unique<PlaceCollective>(info.collective));
  Predicate featurePred = Predicate::type(building.floorInside);
  if (!info.stockpiles.empty())
    inside->addMaker(stockpileMaker(info.stockpiles.getOnlyElement()));
  else
    inside->addMaker(unique<Empty>(SquareChange::reset(building.floorInside)));
  for (StairKey key : info.downStairs)
    inside->addMaker(unique<Stairs>(StairDirection::DOWN, key, featurePred));
  for (StairKey key : info.upStairs)
    inside->addMaker(unique<Stairs>(StairDirection::UP, key, featurePred));
  auto buildingMaker = unique<MakerQueue>(
      unique<Empty>(SquareChange(building.wall)),
      unique<AddAttrib>(SquareAttrib::NO_DIG),
      unique<RemoveAttrib>(SquareAttrib::CONNECT_CORRIDOR),
      unique<Margin>(1, std::move(inside))
      );
  if (door)
    buildingMaker->addMaker(unique<LevelExit>(FurnitureFactory(TribeId::getMonster(), FurnitureType::WOOD_DOOR)));
  return unique<MakerQueue>(
        unique<Empty>(SquareChange::reset(FurnitureParams{FurnitureType::WATER, TribeId::getKeeper()})),
        unique<Margin>(1, std::move(buildingMaker)));
}

PLevelMaker LevelMaker::mineTownLevel(RandomGen& random, SettlementInfo info) {
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN)));
  queue->addMaker(mineTownMaker(random, info));
  return unique<BorderGuard>(std::move(queue), SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN));
}

static PMakerQueue cemetery(SettlementInfo info) {
  BuildingType building = getBuildingInfo(info);
  auto queue = unique<MakerQueue>(
          unique<PlaceCollective>(info.collective),
          unique<Margin>(1, unique<Buildings>(1, 2, 2, 3, building, false, nullptr, false)),
          unique<Furnitures>(Predicate::type(FurnitureType::GRASS), 0.15,
              FurnitureFactory(info.tribe, FurnitureType::GRAVE)));
  for (StairKey key : info.downStairs)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::type(building.floorInside)));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  return queue;
}

static PMakerQueue emptyCollective(SettlementInfo info) {
  return unique<MakerQueue>(
      unique<PlaceCollective>(info.collective),
      unique<Inhabitants>(info.inhabitants, info.collective));
}

static PMakerQueue swamp(SettlementInfo info) {
  auto queue = unique<MakerQueue>(
      unique<Lake>(false),
      unique<PlaceCollective>(info.collective)
  );
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  return queue;
}

static PMakerQueue mountainLake(SettlementInfo info) {
  auto queue = unique<MakerQueue>(
      unique<UniformBlob>(SquareChange::reset(FurnitureParams{FurnitureType::WATER, TribeId::getKeeper()},
          SquareAttrib::LAKE), none),
      unique<PlaceCollective>(info.collective)
  );
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective));
  return queue;
}

static PLevelMaker getMountains(BiomeId id) {
  switch (id) {
    case BiomeId::GRASSLAND:
    case BiomeId::FORREST:
      return unique<Mountains>(0.45, 0.06, NoiseInit{0, 1, 0, 0, 0});
    case BiomeId::MOUNTAIN:
      return unique<Mountains>(0.25, 0.1, NoiseInit{0, 1, 0, 0, 0});
  }
}

static PLevelMaker getForrest(BiomeId id) {
  FurnitureFactory vegetationLow(TribeId::getHostile(),
      {{FurnitureType::CANIF_TREE, 2}, {FurnitureType::BUSH, 1 }});
  FurnitureFactory vegetationHigh(TribeId::getHostile(),
      {{FurnitureType::DECID_TREE, 2}, {FurnitureType::BUSH, 1 }});
  switch (id) {
    case BiomeId::MOUNTAIN:
      return unique<MakerQueue>(
          unique<Forrest>(0.2, 0.5, FurnitureType::GRASS, vegetationLow),
          unique<Forrest>(0.8, 0.5, FurnitureType::HILL, vegetationHigh));
    case BiomeId::GRASSLAND:
      return unique<MakerQueue>(
          unique<Forrest>(0.3, 0.25, FurnitureType::GRASS, vegetationLow),
          unique<Forrest>(0.8, 0.25, FurnitureType::HILL, vegetationHigh));
    case BiomeId::FORREST:
      return unique<MakerQueue>(
          unique<Forrest>(0.8, 0.5, FurnitureType::GRASS, vegetationLow),
          unique<Forrest>(0.8, 0.5, FurnitureType::HILL, vegetationHigh));
  }
}

static PLevelMaker getForrestCreatures(CreatureFactory factory, int levelWidth, BiomeId biome) {
  int div;
  switch (biome) {
    case BiomeId::FORREST: div = 2000; break;
    case BiomeId::GRASSLAND:
    case BiomeId::MOUNTAIN: div = 7000; break;
  }
  return unique<Creatures>(factory, levelWidth * levelWidth / div, MonsterAIFactory::wildlifeNonPredator());
}

struct SurroundWithResourcesInfo {
  LevelMaker* maker;
  SettlementInfo info;
};

static void generateResources(RandomGen& random, LevelMaker* startingPos, RandomLocations* locations,
    const vector<SurroundWithResourcesInfo>& surroundWithResources, int mapWidth) {
  auto addResources = [&](int count, Range size, int maxDist, FurnitureType type, LevelMaker* center,
      CollectiveBuilder* collective) {
    for (int i : Range(count)) {
      SquareChange change(FurnitureParams{type, TribeId::getKeeper()});
      if (collective)
        change.add(SquareChange::addTerritory(collective));
      auto queue = unique<MakerQueue>(unique<FurnitureBlob>(std::move(change)));
      locations->add(std::move(queue), {random.get(size), random.get(size)},
          Predicate::type(FurnitureType::MOUNTAIN2));
      locations->setMaxDistanceLast(center, maxDist);
    }
  };
  struct ResourceInfo {
    FurnitureType type;
    int countStartingPos;
    int countFurther;
  };
  vector<ResourceInfo> resourceInfo = {
      {FurnitureType::STONE, 2, 4},
      {FurnitureType::IRON_ORE, 3, 4},
      {FurnitureType::GOLD_ORE, 1, 3},
  };
  const int closeDist = 0;
  for (auto& info : resourceInfo)
    addResources(info.countStartingPos, Range(5, 10), 20, info.type, startingPos, nullptr);
  for (auto enemy : surroundWithResources)
    for (int i : Range(enemy.info.surroundWithResources))
      if (auto type = enemy.info.extraResources)
        addResources(1, Range(5, 10), closeDist, *type, enemy.maker, enemy.info.collective);
      else {
        auto& info = resourceInfo[i % resourceInfo.size()];
        if (info.countFurther > 0) {
          addResources(1, Range(5, 10), closeDist, info.type, enemy.maker, enemy.info.collective);
          --info.countFurther;
      }
    }
  for (auto& info : resourceInfo)
    if (info.countFurther > 0)
      addResources(info.countFurther, Range(5, 10), mapWidth / 3, info.type, startingPos, nullptr);
}

PLevelMaker LevelMaker::topLevel(RandomGen& random, optional<CreatureFactory> forrestCreatures,
    vector<SettlementInfo> settlements, int mapWidth, bool keeperSpawn, BiomeId biomeId) {
  auto queue = unique<MakerQueue>();
  auto locations = unique<RandomLocations>();
  auto locations2 = unique<RandomLocations>();
  LevelMaker* startingPos = nullptr;
  int locationMargin = 10;
  if (keeperSpawn) {
    auto startingPosMaker = unique<StartingPos>(Predicate::alwaysTrue(), StairKey::keeperSpawn());
    startingPos = startingPosMaker.get();
    locations->add(std::move(startingPosMaker), Vec2(4, 4), RandomLocations::LocationPredicate(
        Predicate::attrib(SquareAttrib::HILL) && Predicate::canEnter({MovementTrait::WALK}),
        Predicate::attrib(SquareAttrib::MOUNTAIN), 1, 8));
    int minMargin = 50;
    locations->setMinMargin(startingPos, minMargin - locationMargin);
  }
  struct CottageInfo {
    LevelMaker* maker;
    CollectiveBuilder* collective;
    TribeId tribe;
    int maxDistance;
  };
  vector<CottageInfo> cottages;
  vector<SurroundWithResourcesInfo> surroundWithResources;
  for (SettlementInfo settlement : settlements) {
    PMakerQueue queue;
    switch (settlement.type) {
      case SettlementType::SMALL_VILLAGE:
        queue = village(random, settlement, 3, 4);
        cottages.push_back({queue.get(), settlement.collective, settlement.tribe, 16});
        break;
      case SettlementType::VILLAGE:
        queue = village(random, settlement, 4, 8);
        break;
      case SettlementType::FORREST_VILLAGE:
        queue = village2(random, settlement);
        break;
      case SettlementType::CASTLE:
        queue = castle(random, settlement);
        break;
      case SettlementType::CASTLE2:
        queue = castle2(random, settlement);
        break;
      case SettlementType::COTTAGE:
        queue = cottage(settlement);
        cottages.push_back({queue.get(), settlement.collective, settlement.tribe, 13});
        break;
      case SettlementType::FORREST_COTTAGE:
        queue = forrestCottage(settlement);
        break;
      case SettlementType::TOWER:
        queue = tower(random, settlement, true);
        break;
      case SettlementType::WITCH_HOUSE:
        queue = cottage(settlement);
        break;
      case SettlementType::FOREST:
        queue = emptyCollective(settlement);
        break;
      case SettlementType::MINETOWN:
        queue = mineTownMaker(random, settlement);
        break;
      case SettlementType::ANT_NEST:
        queue = antNestMaker(random, settlement);
        break;
      case SettlementType::SMALL_MINETOWN:
        queue = smallMineTownMaker(random, settlement);
        break;
      case SettlementType::ISLAND_VAULT:
        queue = islandVaultMaker(random, settlement, false);
        break;
      case SettlementType::ISLAND_VAULT_DOOR:
        queue = islandVaultMaker(random, settlement, true);
        break;
      case SettlementType::VAULT:
      case SettlementType::CAVE:
        queue = vaultMaker(settlement);
        break;
      case SettlementType::SPIDER_CAVE:
        queue = spiderCaveMaker(settlement);
        break;
      case SettlementType::CEMETERY:
        queue = cemetery(settlement);
        break;
      case SettlementType::MOUNTAIN_LAKE:
        queue = mountainLake(settlement);
        break;
      case SettlementType::SWAMP:
        queue = swamp(settlement);
        break;
    }
    if (settlement.corpses)
      queue->addMaker(unique<Corpses>(*settlement.corpses));
    if (settlement.surroundWithResources > 0)
      surroundWithResources.push_back({queue.get(), settlement});
    if (settlement.type == SettlementType::SPIDER_CAVE)
      locations2->add(std::move(queue), getSize(random, settlement.type), getSettlementPredicate(settlement.type));
    else {
      if (keeperSpawn) {
        if (settlement.closeToPlayer) {
          locations->setMinDistance(startingPos, queue.get(), 40);
          locations->setMaxDistance(startingPos, queue.get(), 55);
        } else
          locations->setMinDistance(startingPos, queue.get(), 70);
      }
      locations->add(std::move(queue), getSize(random, settlement.type), getSettlementPredicate(settlement.type));
    }
  }
  Predicate lowlandPred = Predicate::attrib(SquareAttrib::LOWLAND) && !Predicate::attrib(SquareAttrib::RIVER);
  for (auto& cottage : cottages)
    for (int i : Range(random.get(1, 3))) {
      locations->add(unique<MakerQueue>(
            unique<RemoveFurniture>(FurnitureLayer::MIDDLE),
            unique<FurnitureBlob>(SquareChange(FurnitureParams{FurnitureType::CROPS, cottage.tribe})),
            unique<PlaceCollective>(cottage.collective)),
          {random.get(7, 12), random.get(7, 12)},
          lowlandPred);
      locations->setMaxDistanceLast(cottage.maker, cottage.maxDistance);
    }
  if (biomeId == BiomeId::GRASSLAND || biomeId == BiomeId::FORREST)
    for (int i : Range(random.get(0, 3)))
      locations->add(unique<Lake>(), {random.get(20, 30), random.get(20, 30)}, Predicate::attrib(SquareAttrib::LOWLAND));
  if (biomeId == BiomeId::MOUNTAIN)
    for (int i : Range(random.get(3, 6))) {
      locations->add(unique<UniformBlob>(
              SquareChange::reset(FurnitureParams{FurnitureType::WATER, TribeId::getKeeper()}, SquareAttrib::LAKE), none),
          {random.get(10, 30), random.get(10, 30)}, Predicate::attrib(SquareAttrib::MOUNTAIN));
    //  locations->setMaxDistanceLast(startingPos, i == 0 ? 25 : 60);
  }
/*  for (int i : Range(random.get(3, 5))) {
    locations->add(unique<UniformBlob>(FurnitureType::FLOOR, none),
        {random.get(5, 12), random.get(5, 12)}, Predicate::type(SquareId::MOUNTAIN));
 //   locations->setMaxDistanceLast(startingPos, i == 0 ? 25 : 40);
  }*/
  if (keeperSpawn)
    generateResources(random, startingPos, locations.get(), surroundWithResources, mapWidth);
  int mapBorder = 2;
  queue->addMaker(unique<Empty>(FurnitureType::WATER));
  queue->addMaker(getMountains(biomeId));
  queue->addMaker(unique<MountainRiver>(1, Predicate::attrib(SquareAttrib::MOUNTAIN)));
  queue->addMaker(unique<AddAttrib>(SquareAttrib::CONNECT_CORRIDOR, Predicate::attrib(SquareAttrib::LOWLAND)));
  queue->addMaker(unique<AddAttrib>(SquareAttrib::CONNECT_CORRIDOR, Predicate::attrib(SquareAttrib::HILL)));
  queue->addMaker(getForrest(biomeId));
  queue->addMaker(unique<Margin>(mapBorder + locationMargin, std::move(locations)));
  queue->addMaker(unique<Margin>(mapBorder, unique<Roads>()));
  queue->addMaker(unique<Margin>(mapBorder,
        unique<TransferPos>(Predicate::canEnter(MovementTrait::WALK), StairKey::transferLanding(), 2)));
  queue->addMaker(unique<Margin>(mapBorder, unique<Connector>(none, 0, 5,
          Predicate::canEnter({MovementTrait::WALK}) &&
          Predicate::attrib(SquareAttrib::CONNECT_CORRIDOR),
      SquareAttrib::CONNECTOR)));
  queue->addMaker(unique<Margin>(mapBorder + locationMargin, std::move(locations2)));
  queue->addMaker(unique<Items>(ItemFactory::mushrooms(), mapWidth / 10, mapWidth / 5));
  queue->addMaker(unique<AddMapBorder>(mapBorder));
  if (forrestCreatures)
    queue->addMaker(unique<Margin>(mapBorder, getForrestCreatures(*forrestCreatures, mapWidth - 2 * mapBorder, biomeId)));
  return std::move(queue);
}

Vec2 LevelMaker::getRandomExit(RandomGen& random, Rectangle rect, int minCornerDist) {
  CHECK(rect.width() > 2 * minCornerDist && rect.height() > 2 * minCornerDist);
  int w1 = random.get(2);
  int w2 = random.get(2);
  int d1 = random.get(minCornerDist, rect.width() - minCornerDist);
  int d2 = random.get(minCornerDist, rect.height() - minCornerDist);
  return Vec2(
        rect.left() + d1 * w1 + (1 - w1) * w2 * (rect.width() - 1),
        rect.top() + d2 * (1 - w1) + w1 * w2 * (rect.height() - 1));
}

class SpecificArea : public LevelMaker {
  public:
  SpecificArea(Rectangle a, PLevelMaker m) : area(a), maker(std::move(m)) {}

  virtual void make(LevelBuilder* builder, Rectangle) override {
    maker->make(builder, area);
  }

  private:
  Rectangle area;
  PLevelMaker maker;
};

PLevelMaker LevelMaker::splashLevel(CreatureFactory heroLeader, CreatureFactory heroes, CreatureFactory monsters,
    CreatureFactory imps, const FilePath& splashPath) {
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(FurnitureType::BLACK_FLOOR));
  Rectangle leaderSpawn(
          Level::getSplashVisibleBounds().right() + 1, Level::getSplashVisibleBounds().middle().y,
          Level::getSplashVisibleBounds().right() + 2, Level::getSplashVisibleBounds().middle().y + 1);
  Rectangle heroSpawn(
          Level::getSplashVisibleBounds().right() + 2, Level::getSplashVisibleBounds().middle().y - 1,
          Level::getSplashBounds().right(), Level::getSplashVisibleBounds().middle().y + 2);
  Rectangle monsterSpawn1(
          Level::getSplashVisibleBounds().left(), 0,
          Level::getSplashVisibleBounds().right(), Level::getSplashVisibleBounds().top() - 1);
  Rectangle monsterSpawn2(
          Level::getSplashVisibleBounds().left(), Level::getSplashVisibleBounds().bottom() + 2,
          Level::getSplashVisibleBounds().right(), Level::getSplashBounds().bottom());
  queue->addMaker(unique<SpecificArea>(leaderSpawn, unique<Creatures>(heroLeader, 1,MonsterAIFactory::splashHeroes(true))));
  queue->addMaker(unique<SpecificArea>(heroSpawn, unique<Creatures>(heroes, 22, MonsterAIFactory::splashHeroes(false))));
  queue->addMaker(unique<SpecificArea>(monsterSpawn1, unique<Creatures>(monsters, 17, MonsterAIFactory::splashMonsters())));
  queue->addMaker(unique<SpecificArea>(monsterSpawn2, unique<Creatures>(monsters, 17, MonsterAIFactory::splashMonsters())));
  queue->addMaker(unique<SpecificArea>(monsterSpawn1, unique<Creatures>(imps, 15,
          MonsterAIFactory::splashImps(splashPath))));
  queue->addMaker(unique<SpecificArea>(monsterSpawn2, unique<Creatures>(imps, 15,
          MonsterAIFactory::splashImps(splashPath))));
  queue->addMaker(unique<SetSunlight>(0.0, !Predicate::inRectangle(Level::getSplashVisibleBounds())));
  return std::move(queue);
}


static PLevelMaker underground(RandomGen& random, CreatureFactory waterFactory, CreatureFactory lavaFactory) {
  auto queue = unique<MakerQueue>();
  if (random.roll(1)) {
    auto caverns = unique<RandomLocations>();
    int minSize = random.get(5, 15);
    int maxSize = minSize + random.get(3, 10);
    for (int i : Range(sqrt(random.get(4, 100)))) {
      int size = random.get(minSize, maxSize);
      caverns->add(unique<UniformBlob>(FurnitureType::FLOOR), Vec2(size, size), Predicate::alwaysTrue());
      caverns->setCanOverlap(caverns->getLast());
    }
    queue->addMaker(std::move(caverns));
  }
  switch (random.get(1, 3)) {
    case 1:
      queue->addMaker(unique<River>(3, random.choose(FurnitureType::WATER, FurnitureType::MAGMA)));
      break;
    case 2: {
      int numLakes = sqrt(random.get(1, 100));
      auto lakeType = random.choose(FurnitureType::WATER, FurnitureType::MAGMA);
      auto caverns = unique<RandomLocations>();
      for (int i : Range(numLakes)) {
        int size = random.get(6, 20);
        caverns->add(unique<UniformBlob>(SquareChange::reset(lakeType, SquareAttrib::LAKE), none), Vec2(size, size), Predicate::alwaysTrue());
        caverns->setCanOverlap(caverns->getLast());
      }
      queue->addMaker(std::move(caverns));
      if (lakeType == FurnitureType::WATER) {
        queue->addMaker(unique<Creatures>(waterFactory, 1, MonsterAIFactory::monster(),
              Predicate::type(FurnitureType::WATER)));
      }
      if (lakeType == FurnitureType::MAGMA) {
        queue->addMaker(unique<Creatures>(lavaFactory, random.get(1, 4),
              MonsterAIFactory::monster(), Predicate::type(FurnitureType::MAGMA)));
      }
      break;
    }
    default: break;
  }
  return std::move(queue);
}

PLevelMaker LevelMaker::roomLevel(RandomGen& random, CreatureFactory roomFactory, CreatureFactory waterFactory,
    CreatureFactory lavaFactory, vector<StairKey> up, vector<StairKey> down, FurnitureFactory furniture) {
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN)));
  queue->addMaker(underground(random, waterFactory, lavaFactory));
  queue->addMaker(unique<RoomMaker>(random.get(8, 15), 4, 7, SquareChange::none(),
        FurnitureType::MOUNTAIN, unique<Empty>(FurnitureType::FLOOR)));
  queue->addMaker(unique<Connector>(FurnitureFactory(TribeId::getHostile(), FurnitureType::WOOD_DOOR), 0.5));
  queue->addMaker(unique<Furnitures>(Predicate::attrib(SquareAttrib::EMPTY_ROOM), 0.05, furniture));
  for (StairKey key : down)
    queue->addMaker(unique<Stairs>(StairDirection::DOWN, key, Predicate::type(FurnitureType::FLOOR)));
  for (StairKey key : up)
    queue->addMaker(unique<Stairs>(StairDirection::UP, key, Predicate::type(FurnitureType::FLOOR)));
  queue->addMaker(unique<Creatures>(roomFactory, random.get(10, 15), MonsterAIFactory::monster()));
  queue->addMaker(unique<Items>(ItemFactory::dungeon(), 5, 10));
  return unique<BorderGuard>(std::move(queue), SquareChange(FurnitureType::FLOOR, FurnitureType::MOUNTAIN));
}

namespace {

class SokobanFromFile : public LevelMaker {
  public:
  SokobanFromFile(Table<char> f, StairKey hole) : file(f), holeKey(hole) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    CHECK(area == file.getBounds()) << "Bad size of sokoban input.";
    builder->setNoDiagonalPassing();
    for (Vec2 v : area) {
      builder->resetFurniture(v, FurnitureType::FLOOR);
      switch (file[v]) {
        case '.':
          break;
        case '#':
          builder->putFurniture(v, FurnitureType::DUNGEON_WALL);
          break;
        case '^':
          builder->putFurniture(v, FurnitureType::SOKOBAN_HOLE);
          break;
        case '$':
          builder->addAttrib(v, SquareAttrib::SOKOBAN_PRIZE);
          break;
        case '@':
          builder->addAttrib(v, SquareAttrib::SOKOBAN_ENTRY);
          break;
        case '+':
          builder->putFurniture(v, FurnitureParams{FurnitureType::IRON_DOOR, TribeId::getHostile()});
          break;
        case '0':
          builder->putCreature(v, CreatureFactory::fromId(CreatureId::SOKOBAN_BOULDER, TribeId::getPeaceful()));
          break;
        default: FATAL << "Unknown symbol in sokoban data: " << file[v];
      }
    }
  }

  Table<char> file;
  StairKey holeKey;
};

}

PLevelMaker LevelMaker::sokobanFromFile(RandomGen& random, SettlementInfo info, Table<char> file) {
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<SokobanFromFile>(file, info.downStairs.getOnlyElement()));
  queue->addMaker(unique<Stairs>(StairDirection::DOWN, info.downStairs.getOnlyElement(),
        Predicate::attrib(SquareAttrib::SOKOBAN_ENTRY)));
  //queue->addMaker(unique<PlaceCollective>(info.collective));
  queue->addMaker(unique<Inhabitants>(info.inhabitants, info.collective, Predicate::attrib(SquareAttrib::SOKOBAN_PRIZE)));
  return std::move(queue);
}

namespace {

class BattleFromFile : public LevelMaker {
  public:
  BattleFromFile(Table<char> f, CreatureList a, CreatureList e)
      : level(f), allies(a), enemies(e) {}

  virtual void make(LevelBuilder* builder, Rectangle area) override {
    CHECK(area == level.getBounds()) << "Bad size of battle level input.";
    auto alliesList = allies.generate(builder->getRandom(), TribeId::getKeeper(), MonsterAIFactory::guard());
    int allyIndex = 0;
    auto enemyList = enemies.generate(builder->getRandom(), TribeId::getHuman(),
        MonsterAIFactory::singleTask(Task::attackCreatures(getWeakPointers(alliesList))));
    int enemyIndex = 0;
    for (Vec2 v : area) {
      builder->resetFurniture(v, FurnitureType::FLOOR);
      switch (level[v]) {
        case '.':
          break;
        case '#':
          builder->putFurniture(v, FurnitureType::MOUNTAIN);
          break;
        case 'a':
          if (allyIndex < alliesList.size()) {
            builder->putCreature(v, std::move(alliesList[allyIndex]));
            ++allyIndex;
          }
          break;
        case 'e':
          if (enemyIndex < enemyList.size()) {
            builder->putCreature(v, std::move(enemyList[enemyIndex]));
            ++enemyIndex;
          }
          break;
        default: FATAL << "Unknown symbol in battle test data: " << level[v];
      }
    }
  }

  Table<char> level;
  CreatureList allies;
  CreatureList enemies;
};

}

PLevelMaker LevelMaker::battleLevel(Table<char> level, CreatureList allies, CreatureList enemies) {
  return unique<BattleFromFile>(level, allies, enemies);
}

PLevelMaker LevelMaker::emptyLevel(FurnitureType t) {
  auto queue = unique<MakerQueue>();
  queue->addMaker(unique<Empty>(t));
  return std::move(queue);
}
