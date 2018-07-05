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
#include "minion_equipment.h"
#include "item_factory.h"
#include "item_type.h"
#include "creature.h"
#include "item.h"
#include "attr_type.h"
#include "body.h"
#include "call_cache.h"
#include "container_range.h"
#include "serialization.h"
#include "text_serialization.h"
#include "creature_factory.h"
#include "level_builder.h"
#include "model.h"
#include "position_matching.h"

class Test {
  public:
  void testStringConvertion() {
    CHECK(toString(1234) == "1234");
    CHECK(fromString<int>("1234") == 1234);
  }

  void testTimeQueue() {
   /* CreatureAttributes attr = CATTR(c.name = ""; c.speed = 5; c.size = CreatureSize::SMALL; c.strength = 1; c.dexterity = 3; c.humanoid = false; c.weight = 0.1;);
    PCreature b(new Creature(ViewObject(ViewId::JACKAL, ViewLayer::CREATURE, ""), nullptr, attr));
    PCreature a(new Creature(ViewObject(ViewId::PLAYER, ViewLayer::CREATURE, ""), nullptr, attr));
    PCreature c(new Creature(ViewObject(ViewId::JACKAL, ViewLayer::CREATURE, ""), nullptr, attr));
    WCreature rb = b.get(), *ra = a.get(), *rc = c.get();
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
  void testRectangleDistance() {
    Rectangle r(-3, -6, 4, 3);
    CHECKEQ(r.getDistance(Rectangle(5, 5, 7, 7)), 2);
    CHECKEQ(r.getDistance(Rectangle(4, 4, 7, 7)), 1);
    CHECKEQ(r.getDistance(Rectangle(4, 3, 7, 7)), 0);
    CHECKEQ(r.getDistance(Rectangle(3, 2, 7, 7)), -1);
    CHECKEQ(r.getDistance(Rectangle(2, 3, 7, 7)), 0);
    CHECKEQ(r.getDistance(Rectangle(2, 4, 7, 7)), 1);
    CHECKEQ(r.getDistance(Rectangle(2, 2, 7, 7)), -1);
    CHECKEQ(r.getDistance(Rectangle(0, 5, 2, 7)), 2);
    CHECKEQ(r.getDistance(Rectangle(6, -2, 8, 0)), 2);
    CHECKEQ(r.getDistance(Rectangle(4, -2, 6, 0)), 0);
    CHECKEQ(r.getDistance(Rectangle(3, -2, 5, 0)), -1);
    CHECKEQ(r.getDistance(Rectangle(2, -2, 4, 0)), -2);
    CHECKEQ(r.getDistance(Rectangle(1, -2, 3, 0)), -3);
    CHECKEQ(r.getDistance(Rectangle(0, -2, 2, 0)), -4);
    CHECKEQ(r.getDistance(Rectangle(-1, -2, 1, 0)), -4);
    CHECKEQ(r.getDistance(Rectangle(-2, -2, 0, 0)), -3);
    CHECKEQ(r.getDistance(Rectangle(-2, -3, 0, -1)), -3);
    CHECKEQ(r.getDistance(Rectangle(-2, -4, 0, -2)), -3);
    CHECKEQ(r.getDistance(Rectangle(-2, -5, 0, -3)), -3);
    CHECKEQ(r.getDistance(Rectangle(-2, -6, 0, -4)), -2);
    CHECKEQ(r.getDistance(Rectangle(-2, -7, 0, -5)), -1);
    CHECKEQ(r.getDistance(Rectangle(-3, -7, -1, -5)), -1);
    CHECKEQ(r.getDistance(Rectangle(-4, -7, -2, -5)), -1);
    CHECKEQ(r.getDistance(Rectangle(-5, -7, -3, -5)), 0);
    CHECKEQ(r.getDistance(Rectangle(-5, -8, -3, -6)), 0);
    CHECKEQ(r.getDistance(Rectangle(-5, -9, -3, -7)), 1);
    CHECKEQ(r.getDistance(Rectangle(-6, -8, -4, -6)), 1);
    CHECKEQ(r.getDistance(Rectangle(-6, -2, -4, 0)), 1);
    CHECKEQ(r.getDistance(Rectangle(-6, 3, -4, 5)), 1);
    CHECKEQ(r.getDistance(Rectangle(-5, 3, -3, 5)), 0);
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

  void testSplitIncludeDelim() {
    vector<string> v = { "pok", ";", "pokpok", ";", ";", "pok", ";" };
    vector<string> w = { "po", "k", ";po", "k", "po", "k", ";;po","k", ";" };
    CHECK(splitIncludeDelim("pok;pokpok;;pok;", {';'}) == v) << splitIncludeDelim("pok;pokpok;;pok;", {';'});
    CHECK(splitIncludeDelim("pok;pokpok;;pok;", {'k'}) == w) << splitIncludeDelim("pok;pokpok;;pok;", {'k'});
  }

  void testShortestPath() {
    vector<vector<double> > table { { 2, 1, 2, 18, 1}, { 1, 1, 18, 1, 2}, {2, 6, 10, 1,1}, {1, 2, 1, 8, 1}, {5, 3, 1, 1, 2}};
    ShortestPath path(Rectangle(5, 5),
        [table](Vec2 pos) { return table[pos.y][pos.x];},
        [] (Vec2 from, Vec2 to) { return from.dist4(to); },
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
        [] (Vec2 from, Vec2 to) { return from.dist4(to); },
        Vec2::directions4(), Vec2(4, 0), Vec2(0, 0));
  }

  void testShortestPath2() {
    vector<vector<double> > table { { 2, 1, 2, ShortestPath::infinity, 1}, { 1, 1, 18, 1, ShortestPath::infinity}, {2, 6, 10, 1,1}, {1, 2, 1, 8, 1}, {5, 3, 1, 1, 2}};
    ShortestPath path(Rectangle(5, 5),
        [table](Vec2 pos) { return table[pos.y][pos.x];},
        [] (Vec2 from, Vec2 to) { return from.dist4(to); },
        Vec2::directions4(), Vec2(4, 0), Vec2(1, 0));
    CHECK(!path.isReachable(Vec2(1, 0)));
  }

  void testShortestPathReverse() {
    vector<vector<double> > table { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, { 1, 1, 1, ShortestPath::infinity, ShortestPath::infinity, 1, 1, 1, 1, 1, 1}, { 1, 1, 1, ShortestPath::infinity, ShortestPath::infinity, 1, 1, 1, 1, 1, 1}};
    ShortestPath path(Rectangle(11, 3),
        [table](Vec2 pos) { return table[pos.y][pos.x];},
        [] (Vec2 from, Vec2 to) { return from.dist4(to); },
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
    CHECK(a.getOnlyElement() == 4);
  }

  void testRange3() {
    CHECK(!Range(1, 5).intersects(Range(5, 10)));
    CHECK(!Range(1, 5).intersects(Range(0, 1)));
    CHECK(!Range(1, 5).intersects(Range(-1, 0)));
    CHECK(!Range(1, 5).intersects(Range(5, 10).reverse()));
    CHECK(!Range(1, 5).intersects(Range(0, 1).reverse()));
    CHECK(!Range(1, 5).intersects(Range(-1, 0).reverse()));
    CHECK(Range(1, 5).intersects(Range(4, 10)));
    CHECK(Range(1, 5).intersects(Range(-1, 2)));
    CHECK(Range(1, 5).intersects(Range(-1, 2).reverse()));
    CHECK(Range(1, 5).intersects(Range(4, 10).reverse()));
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
    vector<string> res = v.transform(func);
    CHECKEQ(res, s);
  }

  void testSetTransform() {
    set<int> v { 5, 4, 3, 2, 1};
    vector<string> s { "s5", "s4", "s3", "s2", "s1" };
    function<string(const int&)> func = [](const int& a) { return "s" + toString(a); };
    vector<string> res = v.transform(func);
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
    INFO << s.getNumSectors() << " sectors";
  }

  void testSectorsWithPortals() {
    Sectors s(Rectangle(7, 7));
    s.add(Vec2(2, 1));
    s.add(Vec2(3, 1));
    s.add(Vec2(2, 4));
    s.add(Vec2(3, 4));
    s.add(Vec2(3, 3));
    CHECK(!s.same(Vec2(3, 1), Vec2(3, 4)));
    s.add(Vec2(3, 2));
    CHECK(s.same(Vec2(3, 1), Vec2(3, 4)));
    s.remove(Vec2(3, 2));
    CHECK(!s.same(Vec2(3, 1), Vec2(3, 4)));
    s.addExtraConnection(Vec2(2, 1), Vec2(2, 4));
    CHECK(s.same(Vec2(3, 1), Vec2(3, 4)));
    s.removeExtraConnection(Vec2(2, 1), Vec2(2, 4));
    CHECK(!s.same(Vec2(3, 1), Vec2(3, 4)));
    s.add(Vec2(3, 2));
    CHECK(s.same(Vec2(3, 1), Vec2(3, 4)));
    s.addExtraConnection(Vec2(2, 1), Vec2(2, 4));
    CHECK(s.same(Vec2(3, 1), Vec2(3, 4)));
    s.removeExtraConnection(Vec2(2, 1), Vec2(2, 4));
    s.remove(Vec2(3, 2));
    CHECK(!s.same(Vec2(3, 1), Vec2(3, 4)));
    s.add(Vec2(5, 1));
    s.addExtraConnection(Vec2(2, 1), Vec2(5, 1));
    CHECK(s.same(Vec2(3, 1), Vec2(5, 1)));
    CHECK(!s.same(Vec2(3, 4), Vec2(5, 1)));
    s.add(Vec2(3, 2));
    CHECK(s.same(Vec2(3, 4), Vec2(5, 1)));
    s.remove(Vec2(2, 1));
    CHECK(!s.same(Vec2(3, 4), Vec2(5, 1)));
    s.addExtraConnection(Vec2(0, 0), Vec2(5, 5));
    CHECK(!s.same(Vec2(0, 0), Vec2(5, 5)));
  }

  void testReverse() {
    vector<int> v1 {1, 2, 3, 4};
    vector<int> v2 {4, 3, 2, 1};
    CHECKEQ(v1.reverse(), v2);
  }

  void testReverse2() {
    vector<int> v1 {1};
    vector<int> v2 {1};
    CHECKEQ(v1.reverse(), v2);
  }

  void testReverse3() {
    vector<int> v1;
    vector<int> v2;
    CHECKEQ(v1.reverse(), v2);
  }


  void testOwnerPointer() {
    static int numRef = 0;
    numRef = 0;
    struct tmp : public OwnedObject<tmp> {
      tmp(int a) : x(a) { ++numRef;}
      ~tmp() { --numRef; }
      int x;
    };
    WeakPointer<tmp> w1;
    WeakPointer<tmp> w2;
    {
      OwnerPointer<tmp> t1 = makeOwner<tmp>(1);
      OwnerPointer<tmp> t2 = std::move(t1);
      OwnerPointer<tmp> t3 = makeOwner<tmp>(2);
      w1 = t3.get();
      w2 = t2.get();
      CHECK(w1 != w2);
      auto w4 = t3.get();
      CHECK(w1 == w4);
      CHECK(!!w1);
      CHECKEQ(w1->x, 2);
      CHECK(!!w2);
      CHECKEQ(w2->x, 1);
      t3 = std::move(t2);
      CHECK(!w1);
      CHECK(!!w2);
    }
    CHECK(!w1);
    CHECK(!w2);
    CHECKEQ(numRef, 0);
  }

  void testMinionEquipment1() {
    PItem bow1 = ItemType(ItemType::Bow{}).get();
    PItem bow2 = ItemType(ItemType::Bow{}).get();
    PItem bow3 = ItemType(ItemType::Bow{}).get();
    PCreature human = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    CHECK(equipment.needsItem(human.get(), bow1.get(), false));
    CHECK(equipment.tryToOwn(human.get(), bow1.get()));
    CHECK(equipment.needsItem(human.get(), bow1.get(), false));
    CHECK(equipment.isOwner(bow1.get(), human.get()));
    CHECK(equipment.getItemsOwnedBy(human.get()).contains(bow1.get()));
    equipment.discard(bow1.get());
    CHECK(!equipment.isOwner(bow1.get(), human.get()));
    CHECK(!equipment.getItemsOwnedBy(human.get()).contains(bow1.get()));
    CHECK(equipment.tryToOwn(human.get(), bow1.get()));
    bow2->addModifier(AttrType::DAMAGE, -10);
    CHECK(!equipment.needsItem(human.get(), bow2.get(), false));
    CHECK(equipment.needsItem(human.get(), bow2.get(), true));
    CHECK(equipment.tryToOwn(human.get(), bow2.get()));
    CHECK(equipment.getOwner(bow1.get()) == none);
    CHECK(equipment.isOwner(bow2.get(), human.get()));
    CHECK(equipment.tryToOwn(human.get(), bow1.get()));
    CHECK(equipment.isOwner(bow1.get(), human.get()));
    CHECK(equipment.getItemsOwnedBy(human.get()).contains(bow1.get()));
    CHECK(equipment.getOwner(bow2.get()) == none);
    equipment.setLocked(human.get(), bow1->getUniqueId(), true);
    CHECK(!equipment.tryToOwn(human.get(), bow3.get()));
    CHECK(equipment.isOwner(bow1.get(), human.get()));
    CHECK(equipment.getItemsOwnedBy(human.get()).contains(bow1.get()));
  }

  void testMinionEquipmentItemDestroyed() {
    PItem sword = ItemType(ItemType::Sword{}).get();
    PItem sword2 = ItemType(ItemType::Sword{}).get();
    sword2->addModifier(AttrType::DAMAGE, -5);
    PCreature human = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    CHECK(equipment.tryToOwn(human.get(), sword.get()));
    CHECK(equipment.getItemsOwnedBy(human.get()).size() == 1);
    CHECK(!equipment.needsItem(human.get(), sword2.get()));
    sword.clear();
    CHECK(equipment.getItemsOwnedBy(human.get()).size() == 0);
    CHECK(equipment.needsItem(human.get(), sword2.get()));
  }

  void testMinionEquipmentUpdateItems() {
    PItem sword = ItemType(ItemType::Sword{}).get();
    PItem sword2 = ItemType(ItemType::Sword{}).get();
    sword2->addModifier(AttrType::DAMAGE, -5);
    PCreature human = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    PCreature human2 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    CHECK(equipment.tryToOwn(human.get(), sword.get()));
    CHECK(equipment.tryToOwn(human2.get(), sword2.get()));
    CHECK(equipment.getItemsOwnedBy(human.get()).size() == 1);
    CHECK(!equipment.needsItem(human.get(), sword2.get()));
    equipment.updateItems({sword2.get()});
    CHECK(equipment.getItemsOwnedBy(human.get()).size() == 0);
    CHECK(equipment.needsItem(human.get(), sword2.get()));
    CHECK(equipment.isOwner(sword2.get(), human2.get()));
  }

  void testMinionEquipmentUpdateOwners() {
    PItem sword1 = ItemType(ItemType::Sword{}).get();
    PItem sword2 = ItemType(ItemType::Sword{}).get();
    PCreature human1 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    PCreature human2 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    CHECK(equipment.tryToOwn(human1.get(), sword1.get()));
    CHECK(equipment.isOwner(sword1.get(), human1.get()));
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 1);
    CHECK(equipment.tryToOwn(human2.get(), sword1.get()));
    CHECK(!equipment.isOwner(sword1.get(), human1.get()));
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 0);
    CHECK(equipment.tryToOwn(human1.get(), sword2.get()));
    CHECK(equipment.getOwner(sword1.get()) == human2->getUniqueId());
    CHECK(equipment.getItemsOwnedBy(human2.get()).size() == 1);
    equipment.updateOwners({human1.get()});
    CHECK(equipment.getOwner(sword1.get()) == none);
    CHECK(equipment.getItemsOwnedBy(human2.get()).size() == 0);
    CHECK(equipment.isOwner(sword2.get(), human1.get()));
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 1);
  }

  void testMinionEquipmentAutoAssign() {
    PItem sword1 = ItemType(ItemType::Sword{}).get();
    PItem sword2 = ItemType(ItemType::Sword{}).get();
    PItem sword3 = ItemType(ItemType::Sword{}).get();
    sword1->addModifier(AttrType::DAMAGE, 12);
    PCreature human1 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    PCreature human2 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    PCreature human3 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    equipment.autoAssign(human1.get(), {sword2.get(), sword1.get(), sword3.get()});
    CHECK(equipment.isOwner(sword1.get(), human1.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human1.get()), makeVec(sword1.get()));
    equipment.autoAssign(human2.get(), {sword1.get(), sword2.get()});
    CHECK(equipment.isOwner(sword2.get(), human2.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human2.get()), makeVec(sword2.get()));
    equipment.autoAssign(human3.get(), {sword1.get(), sword2.get()});
    CHECK(equipment.getItemsOwnedBy(human3.get()).size() == 0);
    equipment.updateOwners({human2.get()});
    equipment.autoAssign(human2.get(), {sword3.get(), sword1.get(), sword2.get()});
    CHECK(equipment.isOwner(sword1.get(), human2.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human2.get()), makeVec(sword1.get()));
    CHECK(!equipment.getOwner(sword2.get()));
    PItem bow = ItemType(ItemType::Bow{}).get();
    PItem bow2 = ItemType(ItemType::Bow{}).get();
    bow2->addModifier(AttrType::DAMAGE, 30);
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 0);
    equipment.autoAssign(human1.get(), {bow.get()});
    CHECKEQ(equipment.getItemsOwnedBy(human1.get()), makeVec(bow.get()));
    equipment.updateOwners({human1.get()});
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 1);
    equipment.autoAssign(human1.get(), {bow2.get()});
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 1);
    CHECK(equipment.getItemsOwnedBy(human1.get()).contains(bow2.get()));
    equipment.updateOwners({human1.get()});
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 1);
    human1->getBody().looseBodyPart(BodyPart::ARM);
    for (int i : Range(5))
      equipment.updateOwners({human1.get()});
    CHECK(!equipment.isOwner(bow.get(), human1.get()));
    CHECK(equipment.getItemsOwnedBy(human1.get()).size() == 0);
  }

  void testMinionEquipmentLocking() {
    PItem sword1 = ItemType(ItemType::Sword{}).get();
    PItem sword2 = ItemType(ItemType::Sword{}).get();
    sword1->addModifier(AttrType::DAMAGE, 12);
    PCreature human1 = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    equipment.autoAssign(human1.get(), {sword2.get(), sword1.get()});
    CHECK(equipment.isOwner(sword1.get(), human1.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human1.get()), makeVec(sword1.get()));
    equipment.discard(sword1.get());
    equipment.autoAssign(human1.get(), {sword2.get()});
    equipment.setLocked(human1.get(), sword2->getUniqueId(), true);
    CHECK(equipment.isOwner(sword2.get(), human1.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human1.get()), makeVec(sword2.get()));
    equipment.autoAssign(human1.get(), {sword2.get(), sword1.get()});
    CHECK(!equipment.needsItem(human1.get(), sword1.get()));
    CHECK(equipment.needsItem(human1.get(), sword1.get(), true));
    CHECK(equipment.isOwner(sword2.get(), human1.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human1.get()), makeVec(sword2.get()));
    equipment.updateItems({sword1.get()});
    equipment.autoAssign(human1.get(), {sword1.get()});
    CHECK(equipment.isOwner(sword1.get(), human1.get()));
    CHECKEQ(equipment.getItemsOwnedBy(human1.get()), makeVec(sword1.get()));
  }

  void testMinionEquipment123() {
    PItem sword = ItemType(ItemType::Sword{}).get();
    PItem boots = ItemType(ItemType::LeatherBoots{}).get();
    PItem gloves = ItemType(ItemType::LeatherGloves{}).get();
    PItem helmet = ItemType(ItemType::LeatherHelm{}).get();
    vector<WItem> items = {sword.get(), boots.get(), gloves.get(), helmet.get()};
    PCreature human = CreatureFactory::fromId(CreatureId::BANDIT, TribeId::getBandit());
    MinionEquipment equipment;
    for (int i : Range(30))
      equipment.autoAssign(human.get(), items);
    for (auto item : items) {
      CHECK(equipment.isOwner(item, human.get()));
      CHECK(equipment.needsItem(human.get(), item));
      equipment.setLocked(human.get(), item->getUniqueId(), true);
    }
    equipment.updateOwners({human.get()});
    for (auto item : items) {
      CHECK(equipment.isOwner(item, human.get()));
      CHECK(equipment.isLocked(human.get(), item->getUniqueId()));
    }
    CHECK(equipment.getItemsOwnedBy(human.get()).size() == items.size());
  }

  void testContainerRange() {
    vector<string> v { "abc", "def", "ghi" };
    int i = 0;
    for (auto elem : Iter(v)) {
      CHECK(elem.index() == i);
      CHECK(*elem == v[i]);
      CHECK(elem->size() == v[i].size());
      ++i;
    }
  }

  void testContainerRangeMap() {
    map<int, string> v { {0, "abc"}, {1, "def"}, {2, "ghi"} };
    int i = 0;
    for (auto elem : Iter(v)) {
      CHECK(elem.index() == i);
      CHECK(elem->second == v[i]);
      CHECK(elem->second.size() == v[i].size());
      ++i;
    }
  }

  void testContainerRangeErase() {
    vector<int> v {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    for (auto elem : Iter(v))
      if (*elem % 3 == 1)
        elem.markToErase();
    std::sort(v.begin(), v.end());
    CHECKEQ(v, makeVec(2, 3, 5, 6, 8, 9, 11, 12));
  }

  void testContainerRangeConst() {
    const vector<int> v {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    vector<int> o;
    int cnt = 0;
    for (auto elem : Iter(v))
      if (*elem % 3 == 1)
        o.push_back(*elem);
    CHECKEQ(o, makeVec(1, 4, 7, 10));
  }

  void testContainerRangeTemp() {
    vector<int> o;
    int cnt = 0;
    for (auto elem : Iter(vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12})))
      if (*elem % 3 == 1)
        o.push_back(*elem);
    CHECKEQ(o, makeVec(1, 4, 7, 10));
  }

  void testContainerRangeMapConst() {
    const map<int, string> v { {0, "abc"}, {1, "def"}, {2, "ghi"} };
    int i = 0;
    for (auto elem : Iter(v)) {
      CHECK(elem.index() == i);
      CHECK(elem->second == v.at(i));
      CHECK(elem->second.size() == v.at(i).size());
      ++i;
    }
  }

  void testContainerRangeMapErase() {
    map<int, int> v {{1, -1}, {2, -2}, {3, -3}, {4, -4}, {5, -5}, {6, -6}, {7, -7}, {8, -8}, {9, -9}, {10, -10},
        {11, -11}, {12, -12}};
    for (auto elem : Iter(v))
      if (elem->first % 3 == 1)
        elem.markToErase();
    map<int, int> result {{2, -2}, {3, -3}, {5, -5}, {6, -6}, {8, -8}, {9, -9}, {11, -11}, {12, -12}};
    CHECK(v == result);
  }

  using TestCache = CallCache<string>;

  int cnt1 = 0;

  string genString1(int a) {
    ++cnt1;
    return toString(a);
  }

  void testCacheTemplate() {
    TestCache cache(10);
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 15), "15");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 15), "15");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 15), "15");
    CHECKEQ(cnt1, 1);
  }

  void testCacheTemplate2() {
    TestCache cache(3);
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 1), "1");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 1), "1");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 2), "2");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 2), "2");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 3), "3");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 3), "3");
    CHECKEQ(cnt1, 3);
    CHECKEQ(cache.getSize(), 3);
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 4), "4");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 4), "4");
    CHECKEQ(cnt1, 4);
    CHECKEQ(cache.getSize(), 3);
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 2), "2");
    CHECKEQ(cnt1, 4);
    CHECKEQ(cache.getSize(), 3);
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 1), "1");
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 1), "1");
    CHECKEQ(cnt1, 5);
    CHECKEQ(cache.getSize(), 3);
    CHECKEQ(cache.get(bindMethod<string>(&Test::genString1, this), 123, 3), "3");
    CHECKEQ(cnt1, 6);
    CHECKEQ(cache.getSize(), 3);
  }

  struct Tmp123 {
    int SERIAL(a);
    char SERIAL(a1);
    double SERIAL(b);
    string SERIAL(c);
    float SERIAL(d);

    bool operator == (const Tmp123& o) const {
      return a == o.a && a1 == o.a1 && b == o.b && c == o.c && d == o.d;
    }
    bool operator != (const Tmp123& o) const {
      return !(*this == o);
    }
    SERIALIZE_ALL(a, a1, b, c, d)
  };

  struct Tmp456 {
    char SERIAL(a);
    Tmp123 SERIAL(b);
    char SERIAL(c);
    char SERIAL(d);
    char SERIAL(e);

    bool operator == (const Tmp456& o) const {
      return a == o.a && b == o.b && c == o.c && d == o.d && e == o.e;
    }
    bool operator != (const Tmp456& o) const {
      return !(*this == o);
    }
    SERIALIZE_ALL(a, b, c, d, e)
  };

  void testTextSerialization() {
    Tmp123 a1 {323, 'o', 43.1, "pok\" \\pak", 3.1415};
    Tmp456 a {'z', a1, 'n', '"', ' '};
    TextOutput output;
    output.getArchive() << a;
    std::cout << "Serialized to " << output.getStream().str() << std::endl;
    TextInput input(output.getStream().str());
    Tmp456 b;
    input.getArchive() >> b;
    CHECK(a == b);
  }

  struct MatchingTest {
    auto get(int x, int y) {
      return Position(Vec2(x, y), level.get());
    };
    void free(Position pos) {
      pos.removeFurniture(pos.getFurniture(FurnitureLayer::MIDDLE));
      matching.updateMovement(pos);
    };
    PositionMatching matching;
    PModel model = Model::create();
    LevelBuilder builder = LevelBuilder(nullptr, Random, 10, 10, "", false, none);
    PLevelMaker levelMaker = LevelMaker::emptyLevel(FurnitureType::MOUNTAIN);
    PLevel level = builder.build(model.get(), levelMaker.get(), 1234);
  };

  void testPositionMatching1() {
    MatchingTest t;
    auto pos1 = t.get(5, 5);
    t.matching.addTarget(pos1);
    CHECK(!t.matching.getMatch(pos1));
    auto pos2 = t.get(4, 5);
    t.free(pos2);
    CHECK(t.matching.getMatch(pos1) == pos2);
    auto pos3 = t.get(3, 5);
    t.matching.addTarget(pos3);
    CHECK(!t.matching.getMatch(pos3));
    t.matching.releaseTarget(pos1);
    CHECK(t.matching.getMatch(pos3) == pos2);
  }

  void testPositionMatching2() {
    MatchingTest t;
    auto pos1 = t.get(5, 5);
    t.matching.addTarget(pos1);
    CHECK(!t.matching.getMatch(pos1));
    auto pos2 = t.get(4, 5);
    t.free(pos2);
    CHECK(t.matching.getMatch(pos1) == pos2);
    auto pos3 = t.get(3, 5);
    t.matching.addTarget(pos3);
    CHECK(!t.matching.getMatch(pos3));
    auto pos4 = t.get(3, 6);
    t.free(pos4);
    CHECK(t.matching.getMatch(pos3) == pos4);
    auto pos5 = t.get(3, 7);
    t.matching.addTarget(pos5);
    CHECK(!t.matching.getMatch(pos5));
    auto pos6 = t.get(6, 5);
    t.free(pos6);
    CHECK(t.matching.getMatch(pos5) == pos4);
    CHECK(t.matching.getMatch(pos3) == pos2);
    CHECK(t.matching.getMatch(pos1) == pos6);
  }

  void testPositionMatching3() {
    MatchingTest t;
    for (auto v : Rectangle(10, 10))
      t.free(t.get(v.x, v.y));
  }

  void testPositionMatching4() {
    MatchingTest t;
    for (auto v : Rectangle(10, 10))
      t.matching.addTarget(t.get(v.x, v.y));
  }
};

void testAll() {
  Test().testStringConvertion();
  Test().testTimeQueue();
  Test().testRectangleIterator();
  Test().testValueCheck();
  Test().testSplit();
  Test().testSplitIncludeDelim();
  Test().testShortestPath();
  Test().testAStar();
  Test().testShortestPath2();
  Test().testShortestPathReverse();
  Test().testRange();
  Test().testRange2();
  Test().testRange3();
  Test().testContains();
  Test().testPredicates();
  Test().testOptional();
  Test().testMustInitialize();
  Test().testVec2();
  Test().testConcat();
  Test().testTable();
  Test().testVec2();
  Test().testRectangle();
  Test().testRectangleDistance();
  Test().testProjection();
  Test().testRandomExit();
  Test().testCombine();
  Test().testSectors1();
  Test().testSectors2();
  Test().testSectors3();
  Test().testSectorsWithPortals();
  Test().testReverse();
  Test().testReverse2();
  Test().testReverse3();
  Test().testOwnerPointer();
  Test().testMinionEquipment1();
  Test().testMinionEquipmentItemDestroyed();
  Test().testMinionEquipmentUpdateItems();
  Test().testMinionEquipmentUpdateOwners();
  Test().testMinionEquipmentAutoAssign();
  Test().testMinionEquipmentLocking();
  Test().testMinionEquipment123();
  Test().testContainerRange();
  Test().testContainerRangeMap();
  Test().testContainerRangeErase();
  Test().testContainerRangeMapErase();
  Test().testContainerRangeConst();
  Test().testContainerRangeTemp();
  Test().testContainerRangeMapConst();
  Test().testCacheTemplate();
  Test().testCacheTemplate2();
  Test().testTextSerialization();
  Test().testPositionMatching1();
  Test().testPositionMatching2();
  Test().testPositionMatching3();
  Test().testPositionMatching4();
  INFO << "-----===== OK =====-----";
}
