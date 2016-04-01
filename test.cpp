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

#include "debug.h"
#include "util.h"
#include "shortest_path.h"
#include "level_maker.h"
#include "test.h"
#include "sectors.h"

void testStringConvertion() {
  CHECK(toString(1234) == "1234");
  CHECK(fromString<int>("1234") == 1234);
}

void testTimeQueue() {
 /* CreatureAttributes attr = CATTR(c.name = ""; c.speed = 5; c.size = CreatureSize::SMALL; c.strength = 1; c.dexterity = 3; c.humanoid = false; c.weight = 0.1;);
  PCreature b(new Creature(ViewObject(ViewId::JACKAL, ViewLayer::CREATURE, ""), nullptr, attr));
  PCreature a(new Creature(ViewObject(ViewId::PLAYER, ViewLayer::CREATURE, ""), nullptr, attr));
  PCreature c(new Creature(ViewObject(ViewId::JACKAL, ViewLayer::CREATURE, ""), nullptr, attr));
  Creature* rb = b.get(), *ra = a.get(), *rc = c.get();
  a->setTime(1);
  b->setTime(1.33);
  c->setTime(1.66);
  TimeQueue q;
  q.addCreature(move(a));
  q.addCreature(move(b));
  q.addCreature(move(c));
  CHECK(q.getNextCreature() == ra);
  ra->setTime(2);
  CHECK(q.getNextCreature() == rb);
  rb->setTime(2);
  CHECK(q.getNextCreature() == rc);
  rc->setTime(3);
  CHECK(q.getNextCreature() == rb);
  rb->setTime(3);
  CHECK(q.getNextCreature() == ra);*/
}

void testRectangleIterator() {
  vector<Vec2> v1, v2;
  for (Vec2 v : Rectangle(10, 10)) {
    v1.push_back(v);
  }
  for (int i = 0; i < 10; ++i)
    for (int j = 0; j < 10; ++j)
      v2.push_back(Vec2(i, j));
  CHECK(v1 == v2);
  v1.clear(); v2.clear();
  for (Vec2 v : Rectangle(10, 15, 20, 25)) {
    v1.push_back(v);
  }
  for (int i = 10; i < 20; ++i)
    for (int j = 15; j < 25; ++j)
      v2.push_back(Vec2(i, j));
  CHECK(v1 == v2);
}

void testRectangle() {
  CHECK(Rectangle(2, 2, 4, 4).intersects(Rectangle(3, 3, 5, 5)));
  CHECK(!Rectangle(2, 2, 4, 4).intersects(Rectangle(3, 4, 5, 5)));
  CHECK(!Rectangle(2, 2, 4, 4).intersects(Rectangle(4, 3, 5, 5)));
  CHECK(Rectangle(0, 0, 4, 4).intersects(Rectangle(1, -1, 3, 5)));
  CHECK(Rectangle(0, 0, 4, 4).intersects(Rectangle(1, 1, 3, 3)));
  CHECK(Rectangle(0, 0, 4, 4).intersects(Rectangle(0, 0, 3, 4)));
}

void testValueCheck() {
  CHECK(3 == CHECKEQ(1 + 2, 3));
  string* s = new string("wefpok");
  CHECK(s == NOTNULL(s));
}

void testSplit() {
  vector<string> v = { "pok", "pokpok", "", "pok", "" };
  vector<string> w = { "po", ";po", "po", ";;po", ";" };
  CHECK(split("pok;pokpok;;pok;", {';'}) == v) << split("pok;pokpok;;pok;", {';'});
  CHECK(split("pok;pokpok;;pok;", {'k'}) == w) << split("pok;pokpok;;pok;", {'k'});
}

void testShortestPath() {
  vector<vector<double> > table { { 2, 1, 2, 18, 1}, { 1, 1, 18, 1, 2}, {2, 6, 10, 1,1}, {1, 2, 1, 8, 1}, {5, 3, 1, 1, 2}};
  ShortestPath path(Rectangle(5, 5),
      [table](Vec2 pos) { return table[pos.y][pos.x];},
      [] (Vec2 v) { return v.length4(); },
      Vec2::directions4(), Vec2(4, 0), Vec2(1, 0));
  vector<Vec2> res {Vec2(1, 0)};
  while (res.back() != Vec2(4, 0)) {
    res.push_back(path.getNextMove(res.back()));
  }
  vector<Vec2> expected {Vec2(1, 0), Vec2(1, 1), Vec2(0, 1), Vec2(0, 2), Vec2(0, 3), Vec2(1, 3), Vec2(2, 3), Vec2(2, 4), Vec2(3, 4), Vec2(4, 4), Vec2(4, 3), Vec2(4, 2), Vec2(4, 1), Vec2(4, 0)}; 
  CHECK(res == expected);
}

void testAStar() {
  vector<vector<double> > table { { 1, 1, 6, 1, 1}, { 1, 1, 6, 1, 1}, {1, 1, 1, 1,1}, {1, 1, 6, 1, 1}, {1, 1, 6, 1, 1}};
  ShortestPath path(Rectangle(5, 5),
      [table](Vec2 pos) { return table[pos.y][pos.x];},
      [] (Vec2 v) { return v.length4(); },
      Vec2::directions4(), Vec2(4, 0), Vec2(0, 0));
}

void testShortestPath2() {
  vector<vector<double> > table { { 2, 1, 2, ShortestPath::infinity, 1}, { 1, 1, 18, 1, ShortestPath::infinity}, {2, 6, 10, 1,1}, {1, 2, 1, 8, 1}, {5, 3, 1, 1, 2}};
  ShortestPath path(Rectangle(5, 5),
      [table](Vec2 pos) { return table[pos.y][pos.x];},
      [] (Vec2 v) { return v.length4(); },
      Vec2::directions4(), Vec2(4, 0), Vec2(1, 0));
  CHECK(!path.isReachable(Vec2(1, 0)));
}

void testShortestPathReverse() {
  vector<vector<double> > table { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, { 1, 1, 1, ShortestPath::infinity, ShortestPath::infinity, 1, 1, 1, 1, 1, 1}, { 1, 1, 1, ShortestPath::infinity, ShortestPath::infinity, 1, 1, 1, 1, 1, 1}};
  ShortestPath path(Rectangle(11, 3),
      [table](Vec2 pos) { return table[pos.y][pos.x];},
      [] (Vec2 v) { return v.length4(); },
      Vec2::directions4(), Vec2(1, 1), Vec2(1, 0), -1.3);
/*  vector<Vec2> res {Vec2(1, 0)};
  while (res.back() != Vec2(4, 0)) {
    res.push_back(path.getNextMove(res.back()));
  }
  vector<Vec2> expected {Vec2(1, 0), Vec2(1, 1), Vec2(0, 1), Vec2(0, 2), Vec2(0, 3), Vec2(1, 3), Vec2(2, 3), Vec2(2, 4), Vec2(3, 4), Vec2(4, 4), Vec2(4, 3), Vec2(4, 2), Vec2(4, 1), Vec2(4, 0)}; 
  CHECK(res == expected);*/
}

void testRange() {
  vector<int> a;
  vector<int> b {0,1,2,3,4,5,6};
  for (int x : Range(7))
    a.push_back(x);
  CHECKEQ(a, b);
  a.clear();
  for (int x : Range(2, 6))
    a.push_back(x);
  CHECKEQ(getSubsequence(b, 2, 4), a);
  a.clear();
  for (int x : All(b))
    a.push_back(x);
  CHECKEQ(a, b);
}

void testRange2() {
  vector<int> a;
  const vector<int> b {6,5,4,3,2,1,0};
  for (int x : Range(0, 7).reverse())
    a.push_back(x);
  CHECKEQ(a, b);
  a.clear();
  for (int x : Range(1, 5).reverse())
    a.push_back(x);
  CHECKEQ(getSubsequence(b, 2, 4), a);
  a.clear();
  for (int x : All(b)) {
    a.push_back(b[x]);
  }
  CHECKEQ(a, b);
  a.clear();
  for (int x : Range(4, 4))
    a.push_back(x);
  a.clear();
  for (int x : Range(4, 5).reverse())
    a.push_back(x);
  CHECK(getOnlyElement(a) == 4);
}

void testContains() {
  CHECK(contains({0, 1, 2, 3}, 2));
  CHECK(!contains({0, 1, 2, 3}, 4));
  CHECK(contains({Vec2(0, 0), Vec2(0, 1)}, Vec2(0, 0)));
  CHECK(!contains({Vec2(1, 0), Vec2(0, 1)}, Vec2(0, 0)));
  CHECK(contains(string("pokpokpokpikpik"), string("kpok")));
  CHECK(contains(string("pokpokpokpikpik"), string("kpik")));
  CHECK(!contains(string("pokpokpokpikpik"), string("pikpok")));
  CHECK(contains(string("pokpokpokpikpik"), string("pokpokpokpikpik")));
}

void testPredicates() {
  function<bool(const int&)> pred1 = [](int x) { return x % 2 == 0; };
  function<bool(const int&)> pred2 = [](int x) { return x % 3 == 0; };
  for (int i : Range(20)) {
    bool x = andFun(pred1, pred2)(i);
    CHECK(x == (i % 6 == 0));
  }
}

optional<int> getInt(bool yes) {
  if (yes) 
    return 1;
  else
    return none;
}

void testOptional() {
  optional<int> r = getInt(true);
  optional<int> q;
  CHECK(r);
  CHECK(r == 1);
  CHECK(!(q == 1));
  CHECK(!getInt(false));
}

void testMustInitialize() {
  MustInitialize<int> x;
  x = 5;
  int y = *x;
  MustInitialize<int> z(x);
  int v = *z;
}

template<class T>
void checkEqual(pair<T, T> p1, pair<T, T> p2) {
  CHECK(p1 == p2 || (p1.first == p2.second && p1.second == p2.first))
    << p1.first << "," << p1.second << " " << p2.first << "," << p2.second;
}

void testVec2() {
  CHECK(Vec2(5, 0).shorten() == Vec2(1, 0));
  CHECK(Vec2(-7, 0).shorten() == Vec2(-1, 0));
  CHECK(Vec2(0, 4).shorten() == Vec2(0, 1));
  CHECK(Vec2(0, -3).shorten() == Vec2(0, -1));
  CHECK(Vec2(5, 5).shorten() == Vec2(1, 1));
  CHECK(Vec2(-7, -7).shorten() == Vec2(-1, -1));
  CHECK(Vec2(-3, 3).shorten() == Vec2(-1, 1));
  CHECK(Vec2(4, -4).shorten() == Vec2(1, -1));

  checkEqual(Vec2(3, 0).approxL1(), make_pair(Vec2(1, 0), Vec2(1, 0)));
  checkEqual(Vec2(3, -1).approxL1(), make_pair(Vec2(1, 0), Vec2(1, -1)));
  checkEqual(Vec2(3, -3).approxL1(), make_pair(Vec2(1, -1), Vec2(1, -1)));
  checkEqual(Vec2(1, -3).approxL1(), make_pair(Vec2(0, -1), Vec2(1, -1)));
  checkEqual(Vec2(0, -3).approxL1(), make_pair(Vec2(0, -1), Vec2(0, -1)));
  checkEqual(Vec2(-1, -3).approxL1(), make_pair(Vec2(0, -1), Vec2(-1, -1)));
  checkEqual(Vec2(-3, -3).approxL1(), make_pair(Vec2(-1, -1), Vec2(-1, -1)));
  checkEqual(Vec2(-3, -1).approxL1(), make_pair(Vec2(-1, 0), Vec2(-1, -1)));
  checkEqual(Vec2(-3, 0).approxL1(), make_pair(Vec2(-1, 0), Vec2(-1, 0)));
  checkEqual(Vec2(-3, 1).approxL1(), make_pair(Vec2(-1, 0), Vec2(-1, 1)));
  checkEqual(Vec2(-3, 3).approxL1(), make_pair(Vec2(-1, 1), Vec2(-1, 1)));
  checkEqual(Vec2(-1, 3).approxL1(), make_pair(Vec2(0, 1), Vec2(-1, 1)));
  checkEqual(Vec2(0, 3).approxL1(), make_pair(Vec2(0, 1), Vec2(0, 1)));
  checkEqual(Vec2(1, 3).approxL1(), make_pair(Vec2(0, 1), Vec2(1, 1)));
  checkEqual(Vec2(3, 3).approxL1(), make_pair(Vec2(1, 1), Vec2(1, 1)));
  checkEqual(Vec2(3, 1).approxL1(), make_pair(Vec2(1, 1), Vec2(1, 0)));

  CHECKEQ(getCardinalName(Vec2(1, 0).getBearing().getCardinalDir()), "east");
  CHECKEQ(getCardinalName(Vec2(3, 1).getBearing().getCardinalDir()), "east");
  CHECKEQ(getCardinalName(Vec2(1, 1).getBearing().getCardinalDir()), "south-east");
  CHECKEQ(getCardinalName(Vec2(2, 1).getBearing().getCardinalDir()), "south-east");
  CHECKEQ(getCardinalName(Vec2(0, 1).getBearing().getCardinalDir()), "south");
  CHECKEQ(getCardinalName(Vec2(-1, 3).getBearing().getCardinalDir()), "south");
  CHECKEQ(getCardinalName(Vec2(-1, 1).getBearing().getCardinalDir()), "south-west");
  CHECKEQ(getCardinalName(Vec2(-1, 0).getBearing().getCardinalDir()), "west");
  CHECKEQ(getCardinalName(Vec2(-1, -1).getBearing().getCardinalDir()), "north-west");
  CHECKEQ(getCardinalName(Vec2(0, -1).getBearing().getCardinalDir()), "north");
  CHECKEQ(getCardinalName(Vec2(1, -1).getBearing().getCardinalDir()), "north-east");
}

void testConcat() {
  vector<int> v = concat<int>({{1, 2}, {3,4,5}, {}, {7,8,9}});
  vector<int> r = {1,2,3,4,5,7,8,9};
  CHECK(v == r) << v << " != " <<r;
}

void testTable() {
  Table<int> t(10, 10);
  for (Vec2 v : Rectangle(10, 10))
    t[v] = v.x * v.y;
  CHECK(t[9][9] = 81);
  Table<int> t2(15, 20, 25, 30);
  for (int i = 15; i < 40; ++i)
    for (int j = 20; j < 50; ++j)
      t2[i][j] = i * j;
  CHECK(t2[39][49] == 39 * 49);
}

void testProjection() {
/*  Vec2 proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(6, 0));
  CHECKEQ(proj, Vec2(4, 1));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(4, -2));
  CHECKEQ(proj, Vec2(3, 0));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(0, -2));
  CHECKEQ(proj, Vec2(1, 0));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(-2, 0));
  CHECKEQ(proj, Vec2(0, 1));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(-2, 4));
  CHECKEQ(proj, Vec2(0, 3));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(0, 6));
  CHECKEQ(proj, Vec2(1, 4));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(4, 6));
  CHECKEQ(proj, Vec2(3, 4));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(6, 4));
  CHECKEQ(proj, Vec2(4, 3));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(-2, 2));
  CHECKEQ(proj, Vec2(0, 2));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(6, 2));
  CHECKEQ(proj, Vec2(4, 2));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(2, -2));
  CHECKEQ(proj, Vec2(2, 0));
  proj = AllegroView::projectOnBorders(Rectangle(5, 5), Vec2(2, 6));
  CHECKEQ(proj, Vec2(2, 4));*/
}

void testRandomExit() {
  Rectangle r(5, 10, 15, 20);
  for (int i : Range(1000)) {
    Vec2 v = LevelMaker::getRandomExit(Random, r);
    CHECK((v.x == 5) ^ (v.x == 14) ^ (v.y == 10) ^ (v.y == 19));
  }
}

void testCombine() {
  vector<string> words { "pok", "pak", "pik", "puk" };
  CHECKEQ(combine(words), "pok, pak, pik and puk");
  words = { "pok" };
  CHECKEQ(combine(words), "pok");
  words = { "pok", "pik" };
  CHECKEQ(combine(words), "pok and pik");
}

void testTransform2() {
  vector<int> v { 5, 4, 3, 2, 1};
  vector<string> s { "s5", "s4", "s3", "s2", "s1" };
  function<string(const int&)> func = [](const int& a) { return "s" + toString(a); };
  vector<string> res = transform2<string>(v, func);
  CHECKEQ(res, s);
}

void testSectors1() {
  Sectors sectors(Rectangle(7, 7));
  sectors.add(Vec2(0, 0));
  CHECK(!sectors.same(Vec2(0, 0), Vec2(0, 2)));
  sectors.add(Vec2(0, 2));
  CHECK(!sectors.same(Vec2(0, 0), Vec2(0, 2)));
  sectors.add(Vec2(0, 1));
  CHECK(sectors.same(Vec2(0, 0), Vec2(0, 1)));
  CHECK(sectors.same(Vec2(0, 0), Vec2(0, 2)));
  CHECK(sectors.same(Vec2(0, 1), Vec2(0, 2)));
  sectors.remove(Vec2(0, 1));
  CHECK(!sectors.same(Vec2(0, 0), Vec2(0, 1)));
  CHECK(!sectors.same(Vec2(0, 0), Vec2(0, 2)));
  CHECK(!sectors.same(Vec2(0, 1), Vec2(0, 2)));
  CHECK(!sectors.same(Vec2(1, 1), Vec2(1, 2)));
}

void testSectors2() {
  Sectors s(Rectangle(5, 4));
  s.add(Vec2(2, 0));
  s.add(Vec2(3, 0));
  s.add(Vec2(4, 0));
  s.add(Vec2(4, 1));
  s.add(Vec2(0, 2));
  s.add(Vec2(1, 2));
  s.add(Vec2(3, 2));
  s.add(Vec2(4, 2));
  s.add(Vec2(0, 3));

  CHECK(s.same(Vec2(2, 0), Vec2(3, 2)));
  CHECK(!s.same(Vec2(0, 3), Vec2(3, 2)));
  s.add(Vec2(2, 1));
  CHECK(s.same(Vec2(2, 0), Vec2(3, 2)));
  CHECK(s.same(Vec2(0, 3), Vec2(3, 2)));
  s.remove(Vec2(2, 1));
  CHECK(s.same(Vec2(2, 0), Vec2(3, 2)));
  CHECK(!s.same(Vec2(0, 3), Vec2(3, 2)));
}

void testSectors3() {
  Rectangle bounds(250, 250);
  Sectors s(bounds);
  Table<bool> t(bounds, true);
  Rectangle bounds2(15, 15);
  Rectangle bounds3(3, 3);
  for (int i : Range(40)) {
    Vec2 pos(bounds.randomVec2());
    for (Vec2 v : bounds2.translate(pos).intersection(bounds))
      t[v] = false;
  }
  for (int i : Range(100)) {
    Vec2 pos(bounds.randomVec2());
    for (Vec2 v : bounds3.translate(pos).intersection(bounds))
      t[v] = true;
  }
  for (Vec2 v : bounds)
    if (t[v])
      s.add(v);
  for (int i : Range(100000)) {
    Vec2 v = bounds.randomVec2();
    if (Random.roll(3)) {
      s.remove(v);
      t[v] = false;
    } else {
      s.add(v);
      t[v] = true;
    }
  }
  for (Vec2 pos : bounds) {
    CHECK(t[pos] == s.contains(pos));
    if (t[pos])
      for (Vec2 v : pos.neighbors8())
        if (v.inRectangle(bounds))
          CHECK(t[v] == s.same(pos, v));
  }
  std::cout << s.getNumSectors() << " sectors" << endl;
}

void testReverse() {
  vector<int> v1 {1, 2, 3, 4};
  vector<int> v2 {4, 3, 2, 1};
  CHECKEQ(reverse2(v1), v2);
}

void testReverse2() {
  vector<int> v1 {1};
  vector<int> v2 {1};
  CHECKEQ(reverse2(v1), v2);
}

void testReverse3() {
  vector<int> v1;
  vector<int> v2;
  CHECKEQ(reverse2(v1), v2);
}

int testAll() {
  testStringConvertion();
  testTimeQueue();
  testRectangleIterator();
  testValueCheck();
  testSplit();
  testShortestPath();
  testAStar();
  testShortestPath2();
  testShortestPathReverse();
  testRange();
  testRange2();
  testContains();
  testPredicates();
  testOptional();
  testMustInitialize();
  testVec2();
  testConcat();
  testTable();
  testVec2();
  testRectangle();
  testProjection();
  testRandomExit();
  testCombine();
  testSectors1();
  testSectors2();
  testSectors3();
  testReverse();
  testReverse2();
  testReverse3();
  Debug() << "-----===== OK =====-----";
  return 0;
}
