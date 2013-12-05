#include "stdafx.h"

using namespace std;


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

  private:
  SquareAttrib onAttr;
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

class AlwaysTrue : public SquarePredicate {
  public:
  virtual bool apply(Level::Builder* builder, Vec2 pos) override {
    return true;
  }
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
      taken[v] = squareType && squareType != builder->getType(v);
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
      if (i == numShop)
        shopMaker->make(builder, Rectangle(p.x, p.y, p.x + k.x, p.y + k.y));
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
  Connector(initializer_list<double> door) : doorProb(door) {
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
      return 5;
    return 3;
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
        switch (builder->getType(v)) {
          case SquareType::ROCK_WALL:
              newType = chooseRandom({
                  SquareType::PATH,
                  SquareType::DOOR,
                  SquareType::SECRET_PASS}, doorProb);
              break;
          case SquareType::ABYSS:
          case SquareType::WATER:
          case SquareType::MAGMA:
              newType = SquareType::BRIDGE;
              break;
          case SquareType::YELLOW_WALL:
          case SquareType::BLACK_WALL:
              newType = SquareType::PATH;
              break;
          default:
              Debug(FATAL) << "Unhandled square type " << (int)builder->getType(v);
        }
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
};

class DungeonFeatures : public LevelMaker {
  public:
  DungeonFeatures(SquareType _onType, map<SquareType, pair<int, int> > _squareTypes,
      Optional<SquareAttrib> setAttr = Nothing()): 
      squareTypes(_squareTypes), onType(_onType), attr(setAttr) {
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> available;
    for (Vec2 v : area)
      if (builder->getType(v) == onType)
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
  SquareType onType;
  Optional<SquareAttrib> attr;
};

class Creatures : public LevelMaker {
  public:
  Creatures(CreatureFactory cf, int minc, int maxc, MonsterAIFactory actorF, Optional<SquareType> type = Nothing()) :
      cfactory(cf), minCreature(minc), maxCreature(maxc), actorFactory(actorF), squareType(type) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int numCreature = Random.getRandom(minCreature, maxCreature);
    Table<char> taken(area.getKX(), area.getKY());
    for (int i : Range(numCreature)) {
      PCreature creature = cfactory.random(actorFactory);
      Vec2 pos;
      do {
        pos = Vec2(Random.getRandom(area.getPX(), area.getKX()), Random.getRandom(area.getPY(), area.getKY()));
      } while (!builder->canPutCreature(pos, creature.get())
          || (squareType && builder->getType(pos) != *squareType));
      builder->putCreature(pos, std::move(creature));
      taken[pos] = 1;
    }
  }

  private:
  CreatureFactory cfactory;
  int minCreature;
  int maxCreature;
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
      builder->getSquare(pos)->dropItem(factory.random());
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
  Blob(SquareType insideSquare, Optional<SquareType> borderSquare = Nothing(),
      Optional<SquareAttrib> _attrib = Nothing()) :
      inside(insideSquare), border(borderSquare), attrib(_attrib) {}

  void addSquare(Level::Builder* builder, Vec2 pos) {
    builder->putSquare(pos, inside, attrib);
    if (border)
      for (Vec2 dir : pos.neighbors8())
        if (builder->getType(dir) != inside)
          builder->putSquare(dir, *border, attrib);
  }

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> squares;
    Table<char> isInside(area, 0);
    Vec2 center((area.getKX() + area.getPX()) / 2, (area.getKY() + area.getPY()) / 2);
    squares.push_back(center);
    addSquare(builder, center);
    int maxSquares = area.getW() * area.getH() / 3;
    int numSquares = 0;
    isInside[center] = 1;
    while (1) {
      Vec2 pos = squares[Random.getRandom(squares.size())];
      for (Vec2 next : pos.neighbors4(true)) {
        if (next.inRectangle(Rectangle(area.getPX() + 1, area.getPY() + 1, area.getKX() - 1, area.getKY() - 1))
              && !isInside[next]) {
          Vec2 proj = next - center;
          proj.y *= area.getW();
          proj.y /= area.getH();
          if (Random.getDouble() <= 1. - proj.lengthD() / (area.getW() / 2)) {
            addSquare(builder, next);
            squares.push_back(next);
            isInside[next] = 1;
            if (++numSquares >= maxSquares)
              return;
          }
          break;
        }
      }
    }
  }

  private:
  SquareType inside;
  Optional<SquareType> border;
  Optional<SquareAttrib> attrib;
};

class Empty : public LevelMaker {
  public:
  Empty(SquareType s) : square(s) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area)
      builder->putSquare(v, square);
  }

  private:
  SquareType square;
};

Vec2 LevelMaker::getRandomExit(Rectangle rect) {
  CHECK(rect.getW() > 2 && rect.getH() > 2);
  int w1 = Random.getRandom(2);
  int w2 = Random.getRandom(2);
  int d1 = Random.getRandom(1, rect.getW() - 1);
  int d2 = Random.getRandom(1, rect.getH() - 1);
  return Vec2(
        rect.getPX() + d1 * w1 + (1 - w1) * w2 * (rect.getW() - 1),
        rect.getPY() + d2 * (1 - w1) + w1 * w2 * (rect.getH() - 1));
}

class Buildings : public LevelMaker {
  public:
  Buildings(int buildings,
      int _minSize, int _maxSize,
      SquareType _wall, SquareType _floor, SquareType _door,
      bool _align,
      vector<LevelMaker*> _insideMakers,
      bool _roadConnection = true) :
    numBuildings(buildings),
    minSize(_minSize), maxSize(_maxSize),
    align(_align),
    wall(_wall), door(_door), floor(_floor),
    insideMakers(_insideMakers),
    roadConnection(_roadConnection) {}

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
      CHECK(cnt > 0) << "Failed to add " << numBuildings - i << " buildings out of " << numBuildings;
      if (Random.roll(1))
        nextw = px + w;
      for (Vec2 v : Rectangle(w + 1, h + 1)) {
        filled[Vec2(px, py) + v] = true;
        builder->putSquare(Vec2(px, py) + v, wall);
        builder->setCovered(Vec2(px, py) + v);
      }
      for (Vec2 v : Rectangle(w - 1, h - 1)) {
        builder->putSquare(Vec2(px + 1, py + 1) + v, floor, SquareAttrib::ROOM);
      }
      Vec2 doorLoc = align ? 
          Vec2(px + Random.getRandom(1, w),
               py + (buildingRow * h)) :
          getRandomExit(Rectangle(px, py, px + w + 1, py + h + 1));
      builder->putSquare(doorLoc, door);
      if (i < insideMakers.size()) 
        insideMakers[i]->make(builder, Rectangle(px, py, px + w + 1, py + h + 1));
    }
    if (roadConnection)
      builder->putSquare(Vec2((area.getPX() + area.getKX()) / 2, area.getPY() + alignHeight),
          SquareType::PATH, SquareAttrib::CONNECT);
  }

  private:
  int numBuildings;
  int minSize;
  int maxSize;
  bool align;
  SquareType wall, door, floor;
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
  RandomLocations(const vector<LevelMaker*>& _insideMakers,
      const vector<pair<int, int>>& _sizes, SquarePredicate* pred, bool _separate = true)
      : insideMakers(_insideMakers), sizes(_sizes), predicate(pred), separate(_separate) {
        CHECK(insideMakers.size() == sizes.size());
      }

  virtual void make(Level::Builder* builder, Rectangle area) override {
 /*   if (onAttr) {
          string out;
          for (int i : Range(area.getPX(), area.getKX())) {
            for (int j : Range(area.getPY(), area.getKY()))
              out.append(!builder->hasAttrib(Vec2(i, j), *onAttr) ? "0" : "1");
            Debug() << out;
            out = "";
          }
    }*/
    makeCnt(builder, area, 100);
  }

  void makeCnt(Level::Builder* builder, Rectangle area, int tries) const {
    vector<Rectangle> occupied;
    for (int i : All(insideMakers)) {
      LevelMaker* maker = insideMakers[i];
      int width = sizes[i].first;
      int height = sizes[i].second;
      CHECK(width <= area.getW() && height <= area.getH());
      int px;
      int py;
      int cnt = 10000;
      bool ok;
      do {
        ok = true;
        px = area.getPX() + Random.getRandom(area.getW() - width);
        py = area.getPY() + Random.getRandom(area.getH() - height);
        if (!predicate->apply(builder, Vec2(px, py)) ||
            !predicate->apply(builder, Vec2(px + width - 1, py)) || 
            !predicate->apply(builder, Vec2(px + width - 1, py + height - 1)) || 
            !predicate->apply(builder, Vec2(px, py + height - 1))) 
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
          Debug(FATAL) << "Failed to find free space for " << (int)sizes.size() << " areas";
        }
      }
      Rectangle bounds(px, py, px + width, py + height);
      occupied.push_back(bounds);
    }
    CHECK(insideMakers.size() == occupied.size());
    for (int i : All(insideMakers))
      insideMakers[i]->make(builder, occupied[i]);
  }

  private:
  vector<LevelMaker*> insideMakers;
  vector<pair<int, int>> sizes;
  SquarePredicate* predicate;
  bool separate;
};

class Margin : public LevelMaker {
  public:
  Margin(int s, LevelMaker* in) : size(s), inside(in) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    CHECK(area.getW() > 2 * size && area.getH() > 2 * size);
    inside->make(builder, Rectangle(
          area.getPX() + size,
          area.getPY() + size,
          area.getKX() - size,
          area.getKY() - size));
  }

  private:
  int size;
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
        Debug() << "Numbers " << numFloor << " " << numWall;
        continue;
      }
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

Table<double> genNoiseMap(Rectangle area, bool lowMiddle, double varianceMult) {
  int width = 1;
  while (width < area.getW() - 1 || width < area.getH() - 1)
    width *= 2;
  ++width;
  Table<double> wys(width, width);
  wys[0][0] = 1;
  wys[0][width - 1] = 1;
  wys[width - 1][0] = 1;
  wys[width - 1][width - 1] = 1;
  wys[(width - 1) / 2][(width - 1) / 2] = lowMiddle ? 0 : 1;

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
  Mountains(double r) : ratio(r) {}


  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(area, true, 0.7);
    vector<double> values = sortedValues(wys);
    double cutOffValHill = values[(int)(ratio * double(values.size()))];
    double cutOffVal = values[(int)((0.5 + ratio) / 1.5 * double(values.size()))];
    double cutOffValSnow = values[(int)((3. + ratio) / 4. * double(values.size()))];
    int gCnt = 0, mCnt = 0, hCnt = 0, lCnt = 0;
    for (Vec2 v : area) {
      builder->setHeightMap(v, wys[v]);
      if (wys[v] > cutOffValSnow) {
        builder->putSquare(v, SquareType::GLACIER, SquareAttrib::ROAD_CUT_THRU);
        ++gCnt;
      }
      else if (wys[v] > cutOffVal) {
        builder->putSquare(v, SquareType::MOUNTAIN, {SquareAttrib::MOUNTAIN, SquareAttrib::ROAD_CUT_THRU});
        ++mCnt;
      }
      else if (wys[v] > cutOffValHill) {
        builder->putSquare(v, SquareType::HILL, SquareAttrib::HILL);
        ++hCnt;
      }
      else {
        builder->addAttrib(v, SquareAttrib::LOWLAND);
        ++lCnt;
      }
    }
    Debug() << "Terrain distribution " << gCnt << " glacier, " << mCnt << " mountain, " << hCnt << " hill, " << lCnt << " lowland";
  }

  private:
  double ratio;
};

class Roads : public LevelMaker {
  public:
  Roads(SquareType roadSquare) : square(roadSquare) {}

  double getValue(Level::Builder* builder, Vec2 pos) {
    if (!builder->getSquare(pos)->canEnter(Creature::getDefault()) && 
        !builder->hasAttrib(pos, SquareAttrib::ROAD_CUT_THRU))
      return ShortestPath::infinity;
    SquareType type = builder->getType(pos);
    if (type == SquareType::PATH)
      return 0.1;
    return pow(1 + builder->getHeightMap(pos), 2);
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
        if (v != p2 && v != p1 && builder->getType(v) != SquareType::PATH)
          builder->putSquare(v, SquareType::PATH);
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

  StartingPos(SquareType type) : squareType(type) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 pos : area)
      if (builder->getType(pos) == squareType)
        builder->getSquare(pos)->setLandingLink(StairDirection::UP, StairKey::PLAYER_SPAWN);
  }

  private:
  SquareType squareType;
};

class Vegetation : public LevelMaker {
  public:
  Vegetation(double _ratio, double _density, SquareType _onType, vector<SquareType> _types, vector<double> _probs) 
      : ratio(_ratio), density(_density), types(_types), probs(_probs), onType(_onType) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Table<double> wys = genNoiseMap(area, false, 0.9);
    vector<double> values = sortedValues(wys);
    double cutoff = values[values.size() * ratio];
    for (Vec2 v : area)
      if (builder->getType(v) == onType && wys[v] < cutoff && Random.getDouble() <= density)
        builder->putSquare(v, chooseRandom(types, probs));
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
    location->setBounds(area);
    builder->addLocation(location);
  }
  
  private:
  Location* location;
};

class AddAttrib : public LevelMaker {
  public:
  AddAttrib(SquareAttrib attr, bool _remove = false) : attrib(attr), remove(_remove) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    for (Vec2 v : area)
      if (!remove)
        builder->addAttrib(v, attrib);
      else
        builder->removeAttrib(v, attrib);
  }
  
  private:
  SquareAttrib attrib;
  bool remove;
};

class FlockAndLeader : public LevelMaker {
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
};

class Stairs : public LevelMaker {
  public:
  Stairs(StairDirection dir, StairKey k, SquareType _onType, Optional<SquareAttrib> _setAttr = Nothing()) :
      direction(dir), key(k), onType(_onType), setAttr(_setAttr) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    vector<Vec2> pos;
    for (Vec2 v : area)
      if (builder->getType(v) == onType)
        pos.push_back(v);
    CHECK(pos.size() > 0) << "Couldn't find position for stairs " << area;
    SquareType type = direction == StairDirection::DOWN ? SquareType::DOWN_STAIRS : SquareType::UP_STAIRS;
    builder->putSquare(pos[Random.getRandom(pos.size())], SquareFactory::getStairs(direction, key), type, setAttr);
  }

  private:
  StairDirection direction;
  StairKey key;
  SquareType onType;
  Optional<SquareAttrib> setAttr;
};

class ShopMaker : public LevelMaker {
  public:
  ShopMaker(ItemFactory _factory, Tribe* _tribe, int _numItems)
      : factory(_factory), tribe(_tribe), numItems(_numItems) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    Location *loc = new Location();
    builder->addLocation(loc);
    loc->setBounds(area);
    PCreature shopkeeper = CreatureFactory::getShopkeeper(loc, tribe);
    vector<Vec2> pos;
    for (Vec2 v : area)
      if (builder->getSquare(v)->canEnter(shopkeeper.get()) && builder->getType(v) == SquareType::FLOOR)
        pos.push_back(v);
    builder->putCreature(pos[Random.getRandom(pos.size())], std::move(shopkeeper));
    for (int i : Range(numItems)) {
      Vec2 v = pos[Random.getRandom(pos.size())];
      builder->getSquare(v)->dropItem(factory.random());
    }
  }

  private:
  ItemFactory factory;
  Tribe* tribe;
  int numItems;
};

class LevelExit : public LevelMaker {
  public:
  LevelExit(SquareType _exitType, Optional<SquareAttrib> _attrib = Nothing())
      : exitType(_exitType), attrib(_attrib) {}
  
  virtual void make(Level::Builder* builder, Rectangle area) override {
    builder->putSquare(getRandomExit(area), exitType, attrib);
  }

  private:
  SquareType exitType;
  Optional<SquareAttrib> attrib;
};

class Division : public LevelMaker {
  public:
  Division(double _vRatio, double _hRatio,
      LevelMaker* _upperLeft, LevelMaker* _upperRight, LevelMaker* _lowerLeft, LevelMaker* _lowerRight) :
      vRatio(_vRatio), hRatio(_hRatio),
      upperLeft(_upperLeft), upperRight(_upperRight), lowerLeft(_lowerLeft), lowerRight(_lowerRight) {}

  virtual void make(Level::Builder* builder, Rectangle area) override {
    int vDiv = area.getPY() + min(area.getH() - 1, max(1, (int) (vRatio * area.getH())));
    int hDiv = area.getPX() + min(area.getW() - 1, max(1, (int) (vRatio * area.getW())));
    if (upperLeft)
      upperLeft->make(builder, Rectangle(area.getPX(), area.getPY(), hDiv, vDiv));
    if (upperRight)
      upperRight->make(builder, Rectangle(hDiv, area.getPY(), area.getKX(), vDiv));
    if (lowerLeft)
      lowerLeft->make(builder, Rectangle(area.getPX(), vDiv, hDiv, area.getKY()));
    if (lowerRight)
      lowerRight->make(builder, Rectangle(hDiv, vDiv, area.getKX(), area.getKY()));
  }

  private:
  double vRatio, hRatio;
  LevelMaker *upperLeft, *upperRight, *lowerLeft, *lowerRight;
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

LevelMaker* underground() {
  MakerQueue* queue = new MakerQueue();
  if (Random.roll(1)) {
    LevelMaker* cavern = new Blob(SquareType::PATH);
    vector<LevelMaker*> vCavern;
    vector<pair<int, int>> sizes;
    int minSize = Random.getRandom(5, 15);
    int maxSize = minSize + Random.getRandom(3, 10);
    for (int i : Range(sqrt(Random.getRandom(4, 100)))) {
      int size = Random.getRandom(minSize, maxSize);
      sizes.push_back(make_pair(size, size));
      vCavern.push_back(cavern);
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
          vector<LevelMaker*>(numLakes, new Blob(lakeType, Nothing(), SquareAttrib::LAKE)),
          sizes, new AlwaysTrue(), false));
 /*         Deity* deity = Deity::getDeity(
              (lakeType == SquareType::MAGMA) ? DeityHabitat::FIRE : DeityHabitat::WATER);
          queue->addMaker(new Shrine(deity, lakeType,
              new TypePredicate(lakeType), SquareType::ROCK_WALL, nullptr));
          if (lakeType == SquareType::WATER) {
            queue->addMaker(new Creatures(CreatureFactory::singleType(Tribe::monster, CreatureId::KRAKEN), 1, 2,
                  MonsterAIFactory::monster(), SquareType::WATER));
          }
          if (lakeType == SquareType::MAGMA) {
            queue->addMaker(new Creatures(CreatureFactory::singleType(Tribe::monster, CreatureId::FIRE_SPHERE), 1, 4,
                  MonsterAIFactory::monster(), SquareType::MAGMA));
          }*/
          break;
          }
    default: break;
  }
  return queue;
}

LevelMaker* LevelMaker::roomLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::FOUNTAIN, make_pair(0, 3) },
      { SquareType::CHEST, make_pair(3, 7)},
      { SquareType::TORTURE_TABLE, make_pair(2, 3)}};
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::BLACK_WALL));
  queue->addMaker(underground());
  queue->addMaker(new RoomMaker(8, 15, 4, 7, SquareType::ROCK_WALL, SquareType::BLACK_WALL));
  queue->addMaker(new Connector({5, 3, 0}));
  if (Random.roll(2)) {
    Deity* deity = Deity::getDeity(chooseRandom({DeityHabitat::STONE, DeityHabitat::EARTH}));
    queue->addMaker(new Shrine(deity, SquareType::FLOOR,
        new TypePredicate({SquareType::ROCK_WALL, SquareType::BLACK_WALL}), SquareType::ROCK_WALL, nullptr));
  }
  queue->addMaker(new DungeonFeatures(SquareType::FLOOR,featureCount));
  queue->addMaker(new DungeonFeatures(SquareType::PATH, {{ SquareType::ROLLING_BOULDER, make_pair(0, 3) }}));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, SquareType::FLOOR));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, SquareType::FLOOR));
  queue->addMaker(new Creatures(cfactory, 10, 15, MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareType::FLOOR, 5, 10));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

LevelMaker* LevelMaker::cryptLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::COFFIN, make_pair(3, 7)}};
  queue->addMaker(new Empty(SquareType::BLACK_WALL));
  queue->addMaker(new RoomMaker(8, 15, 3, 5, SquareType::ROCK_WALL, SquareType::BLACK_WALL));
  queue->addMaker(new Connector({1, 3, 1}));
  queue->addMaker(new DungeonFeatures(SquareType::FLOOR,featureCount));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, SquareType::FLOOR));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, SquareType::FLOOR));
  queue->addMaker(new Creatures(cfactory, 10, 15, MonsterAIFactory::monster()));
  queue->addMaker(new Items(ItemFactory::dungeon(), SquareType::FLOOR, 5, 10));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

MakerQueue* village(CreatureFactory factory, Location* loc) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::CHEST, make_pair(0, 3) },
      { SquareType::BED, make_pair(0, 3) }};
  queue->addMaker(new LocationMaker(loc));
  queue->addMaker(new Empty(SquareType::GRASS));
  vector<LevelMaker*> insideMakers {
      new ShopMaker(ItemFactory::villageShop(), Tribe::elven, 10),
      new Creatures(CreatureFactory::singleType(Tribe::elven, CreatureId::PIG), 3, 5, MonsterAIFactory::monster()),
      new DungeonFeatures(SquareType::FLOOR, featureCount)};
  queue->addMaker(new Buildings(6, 3, 7,
        SquareType::WOOD_WALL, SquareType::FLOOR, SquareType::DOOR,
        true, insideMakers));
  queue->addMaker(new Creatures(factory, 10, 20, MonsterAIFactory::stayInLocation(loc), SquareType::GRASS));
  return queue;
}

MakerQueue* banditCamp() {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount { 
      { SquareType::CHEST, make_pair(4, 9) },
      { SquareType::BED, make_pair(2, 4) }};
  queue->addMaker(new Empty(SquareType::GRASS));
  vector<LevelMaker*> insideMakers {
      new DungeonFeatures(SquareType::FLOOR, featureCount)};
  queue->addMaker(new Buildings(1, 5, 7,
        SquareType::WOOD_WALL, SquareType::FLOOR, SquareType::DOOR,
        false, insideMakers, false));
  Location* loc = new Location("bandit hideout", "The bandits have robbed many travelers and townsfolk.");
  queue->addMaker(new LocationMaker(loc));
  queue->addMaker(new Creatures(CreatureFactory::singleType(Tribe::goblin, CreatureId::BANDIT), 3, 7,
        MonsterAIFactory::stayInLocation(loc), SquareType::GRASS));
  return queue;
}

LevelMaker* dungeonEntrance(StairKey key, SquareType onType, const string& dungeonDesc) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Stairs(StairDirection::DOWN, key, onType, SquareAttrib::CONNECT));
  queue->addMaker(new LocationMaker(new Location("dungeon entrance", dungeonDesc)));
  return new RandomLocations({queue}, {make_pair(1, 1)}, new TypePredicate(SquareType::MOUNTAIN));
}

LevelMaker* makeLake() {
  MakerQueue* queue = new MakerQueue();
  Location* loc = new Location();
  queue->addMaker(new LocationMaker(loc));
  queue->addMaker(new Blob(SquareType::WATER, SquareType::SAND, SquareAttrib::LAKE));
 // queue->addMaker(new Creatures(CreatureFactory::singleType(CreatureId::VODNIK), 2, 5, MonsterAIFactory::stayInLocation(loc)));
  return queue;
}

LevelMaker* LevelMaker::towerLevel(Optional<StairKey> down, Optional<StairKey> up) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(up ? SquareType::ROCK_WALL : SquareType::LOW_ROCK_WALL));
  queue->addMaker(new Margin(1, new Empty(SquareType::FLOOR)));
  queue->addMaker(new Margin(1, new AddAttrib(SquareAttrib::ROOM)));
  queue->addMaker(new AddAttrib(SquareAttrib::ROAD_CUT_THRU, true));
  LevelMaker* downStairs = (down) ? new Stairs(StairDirection::DOWN, *down, SquareType::FLOOR) : nullptr;
  LevelMaker* upStairs = (up) ? new Stairs(StairDirection::UP, *up, SquareType::FLOOR) : nullptr;
  queue->addMaker(new Division(0.5, 0.5, upStairs, nullptr, nullptr, downStairs));
  return queue;
}


LevelMaker* LevelMaker::topLevel(
    CreatureFactory forrestCreatures,
    CreatureFactory villageCreatures,
    Location* villageLocation,
    CreatureFactory cementaryCreatures,
    CreatureFactory goblins,
    CreatureFactory pyramidCreatures) {
  MakerQueue* queue = new MakerQueue();
  vector<SquareType> vegetationLow { SquareType::CANIF_TREE, SquareType::BUSH };
  vector<SquareType> vegetationHigh { SquareType::DECID_TREE, SquareType::BUSH };
  vector<double> probs { 2, 1 };
  LevelMaker* lake = makeLake();
  int numLakes = 1;//Random.getRandom(1, 2);
  vector<pair<int, int>> subSizes;
  vector<LevelMaker*> subMakers;
  for (int i : Range(numLakes)) {
    subSizes.emplace_back(Random.getRandom(60, 120), Random.getRandom(60, 120));
    subMakers.push_back(lake);
  }
  MakerQueue* elvenVillage = village(villageCreatures, villageLocation);
  elvenVillage->addMaker(new StartingPos(SquareType::GRASS));
  subMakers.push_back(elvenVillage);
  subSizes.emplace_back(30, 20);
  for (int i : Range(Random.getRandom(3))) {
    subMakers.push_back(banditCamp());
    subSizes.emplace_back(12,8);
  }
  int numCementeries = 1;
  for (int i : Range(numCementeries)) {
    Location* loc = new Location("old cemetery", "Terrible evil is said to be lurking there.");
    subMakers.push_back(new MakerQueue({
          new LocationMaker(loc),
          new Margin(1, new Buildings(1, 2, 3, SquareType::ROCK_WALL, SquareType::FLOOR, SquareType::DOOR, false, {}, false)),
          new DungeonFeatures(SquareType::GRASS, {{ SquareType::GRAVE, make_pair(5, 15) }}),
          new Stairs(StairDirection::DOWN, StairKey::CRYPT, SquareType::FLOOR),
//          new Creatures(cementaryCreatures, 2, 5, MonsterAIFactory::stayInLocation(loc)),
          }));
    subSizes.emplace_back(10, 10);
  }
  int numSheepFlocks = 0;
  for (int i : Range(numSheepFlocks)) {
    subMakers.push_back(new FlockAndLeader(CreatureId::ELF, CreatureId::SHEEP, Tribe::elven, 5, 10));
    subSizes.emplace_back(15, 15);
  }
  int numStoneCircles = Random.getRandom(2, 7);
 /* for (int i : Range(numStoneCircles)) {
    subMakers.push_back(new Circle(ItemId::BOULDER));
    int size = Random.getRandom(14, 32);
    subSizes.emplace_back(size, size);
  }*/
  MakerQueue* pyramid = new MakerQueue();
  pyramid->addMaker(new LocationMaker(new Location("ancient pyramid", "Terrible evil is said to be lurking there.")));
  pyramid->addMaker(new Margin(1, pyramidLevel(pyramidCreatures, {StairKey::PYRAMID}, {})));
  subMakers.push_back(pyramid);
  subSizes.emplace_back(17, 17);
  queue->addMaker(new Empty(SquareType::GRASS));
  queue->addMaker(new Mountains(0.5));
 // queue->addMaker(new MountainRiver(30, '='));
  queue->addMaker(new Vegetation(0.7, 0.5, SquareType::GRASS, vegetationLow, probs));
  queue->addMaker(new Vegetation(0.4, 0.5, SquareType::HILL, vegetationHigh, probs));
  queue->addMaker(new Vegetation(0.2, 0.3, SquareType::MOUNTAIN, vegetationHigh, probs));
  queue->addMaker(new Margin(100,
        new RandomLocations(subMakers, subSizes, new AttribPredicate(SquareAttrib::LOWLAND))));
  MakerQueue* tower = new MakerQueue();
  tower->addMaker(towerLevel(StairKey::TOWER, Nothing()));
  tower->addMaker(new LocationMaker(Location::towerTopLocation()));
//  queue->addMaker(new Margin(100, 
//        new RandomLocations({tower}, {make_pair(4, 4)}, new AttribPredicate(SquareAttrib::MOUNTAIN))));
  queue->addMaker(new Margin(100, dungeonEntrance(StairKey::DWARF, SquareType::MOUNTAIN, "Our enemies the dwarves are living there.")));
 // queue->addMaker(new Margin(100, dungeonEntrance(StairKey::GOBLIN, SquareType::MOUNTAIN, "Our enemies the goblins are living there.")));
  queue->addMaker(new Roads(SquareType::PATH));
  /*Deity* deity = Deity::getDeity(chooseRandom({DeityHabitat::STARS, DeityHabitat::TREES}));
  queue->addMaker(new Shrine(deity, SquareType::PATH,
        new TypePredicate({SquareType::GRASS, SquareType::DECID_TREE, SquareType::BUSH}), SquareType::WOOD_WALL, new LocationMaker(new Location("shrine", "It is dedicated to the god " + deity->getName()))));*/
  queue->addMaker(new Creatures(forrestCreatures, 30, 50, MonsterAIFactory::wildlifeNonPredator()));
  queue->addMaker(new Items(ItemFactory::mushrooms(), SquareType::GRASS, 30, 60));
  return new BorderGuard(queue);
}

LevelMaker* LevelMaker::goblinTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount {
      { SquareType::CHEST, make_pair(4, 8) },
      { SquareType::TORTURE_TABLE, make_pair(4, 8) }};
  queue->addMaker(new Empty(SquareType::ROCK_WALL));
  LevelMaker* cavern = new Blob(SquareType::PATH);
  vector<LevelMaker*> vCavern;
  vector<pair<int, int>> sizes;
  for (int i : Range(40)) {
    sizes.push_back(make_pair(Random.getRandom(5, 10), Random.getRandom(5, 10)));
    vCavern.push_back(cavern);
  }
  queue->addMaker(new RandomLocations(vCavern, sizes, new AlwaysTrue(), false));
  queue->addMaker(new RoomMaker(5, 8, 4, 7, SquareType::ROCK_WALL, Nothing(),
        new ShopMaker(ItemFactory::goblinShop(), Tribe::goblin, 10), false));
  queue->addMaker(new Connector({1, 0, 0}));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, SquareType::FLOOR));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, SquareType::FLOOR));
  queue->addMaker(new DungeonFeatures(SquareType::FLOOR, featureCount));
  queue->addMaker(new Creatures(cfactory, 10, 15, MonsterAIFactory::monster()));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

LevelMaker* LevelMaker::mineTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  map<SquareType, pair<int, int> > featureCount {
      { SquareType::CHEST, make_pair(4, 8) },
      { SquareType::BED, make_pair(4, 8) }};
  queue->addMaker(new Empty(SquareType::ROCK_WALL));
  LevelMaker* cavern = new Blob(SquareType::PATH);
  vector<LevelMaker*> vCavern;
  vector<pair<int, int>> sizes;
  for (int i : Range(20)) {
    sizes.push_back(make_pair(Random.getRandom(5, 25), Random.getRandom(5, 25)));
    vCavern.push_back(cavern);
  }
  queue->addMaker(new RandomLocations(vCavern, sizes, new AlwaysTrue(), false));
  queue->addMaker(new RoomMaker(6, 12, 4, 7, SquareType::ROCK_WALL, Nothing(),
        new ShopMaker(ItemFactory::dwarfShop(), Tribe::dwarven, 10), false));
  queue->addMaker(new Connector({1, 0, 0}));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, SquareType::FLOOR));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, SquareType::FLOOR));
  queue->addMaker(new DungeonFeatures(SquareType::FLOOR, featureCount));
  queue->addMaker(new Creatures(cfactory, 10, 15, MonsterAIFactory::monster()));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}

LevelMaker* LevelMaker::pyramidLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::YELLOW_WALL));
  queue->addMaker(new AddAttrib(SquareAttrib::NO_DIG));
  queue->addMaker(new Margin(1, new AddAttrib(SquareAttrib::NO_DIG, true)));
  queue->addMaker(new RoomMaker(2, 5, 4, 6, SquareType::YELLOW_WALL, SquareType::YELLOW_WALL, nullptr, true));
  for (StairKey key : down)
    queue->addMaker(new Stairs(StairDirection::DOWN, key, SquareType::FLOOR));
  for (StairKey key : up)
    queue->addMaker(new Stairs(StairDirection::UP, key, SquareType::FLOOR));
  if (down.size() == 0)
    queue->addMaker(new LevelExit(SquareType::PATH));
  else
    queue->addMaker(new Creatures(cfactory, 3, 5, MonsterAIFactory::monster()));
  queue->addMaker(new Connector({5, 3, 0}));
  return queue;
}
  
LevelMaker* LevelMaker::collectiveLevel(vector<StairKey> up, vector<StairKey> down) {
  MakerQueue* queue = new MakerQueue();
  queue->addMaker(new Empty(SquareType::ROCK_WALL));
  queue->addMaker(underground());
  vector<LevelMaker*> makers;
  vector<pair<int, int>> sizes;
  for (StairKey key : up) {
    MakerQueue* area = new MakerQueue();
    area->addMaker(new Blob(SquareType::PATH));
    area->addMaker(new Stairs(StairDirection::UP, key, SquareType::PATH));
    makers.push_back(area);
    sizes.emplace_back(10, 10);
  }
  for (StairKey key : down) {
    MakerQueue* area = new MakerQueue();
    area->addMaker(new Blob(SquareType::PATH));
    area->addMaker(new Stairs(StairDirection::DOWN, key, SquareType::PATH));
    makers.push_back(area);
    sizes.emplace_back(10, 10);
  }
  MakerQueue* startPos = new MakerQueue();
  startPos->addMaker(new Blob(SquareType::PATH));
  startPos->addMaker(new StartingPos(SquareType::PATH));
  makers.push_back(startPos);
  sizes.emplace_back(10, 10);
  for (int i : Range(Random.getRandom(4, 7))) {
    makers.push_back(new Blob(SquareType::GOLD_ORE));
    sizes.emplace_back(Random.getRandom(5, 10), Random.getRandom(5, 10));
  }
  queue->addMaker(new RandomLocations(makers, sizes, new AlwaysTrue()));
  return new BorderGuard(queue, SquareType::BLACK_WALL);
}
