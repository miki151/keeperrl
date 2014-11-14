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

#ifndef _UTIL_H
#define _UTIL_H

#include <iostream>

#include "debug.h"
#include "enums.h"
#include "serialization.h"

template <class T>
string toString(const T& t);

template <class T>
T fromString(const string& s);

#define DEF_UNIQUE_PTR(T) class T;\
  typedef unique_ptr<T> P##T;

DEF_UNIQUE_PTR(Item);
DEF_UNIQUE_PTR(Creature);
DEF_UNIQUE_PTR(Square);
DEF_UNIQUE_PTR(MonsterAI);
DEF_UNIQUE_PTR(Behaviour);
DEF_UNIQUE_PTR(Obstacle);
DEF_UNIQUE_PTR(Task);
DEF_UNIQUE_PTR(Controller);
DEF_UNIQUE_PTR(Trigger);
DEF_UNIQUE_PTR(Level);
DEF_UNIQUE_PTR(VillageControl);
DEF_UNIQUE_PTR(GuiElem);
DEF_UNIQUE_PTR(Animation);
DEF_UNIQUE_PTR(ViewObject);
DEF_UNIQUE_PTR(Collective);
DEF_UNIQUE_PTR(CollectiveControl);

template <typename T>
T lambdaConstruct(function<void(T&)> fun) {
  T ret {};
  fun(ret);
  return ret;
}

#define CONSTRUCT(Class, Code) lambdaConstruct<Class>([&] (Class& c) { Code })
#define LIST(...) {__VA_ARGS__}

class Item;
typedef function<bool(const Item*)> ItemPredicate;

template<class T>
vector<T*> extractRefs(vector<unique_ptr<T>>& v) {
  vector<T*> ret;
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}

template<class T>
const vector<T*> extractRefs(const vector<unique_ptr<T>>& v) {
  vector<T*> ret;
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}

struct GameOverException {
};

enum class GameType { ADVENTURER, KEEPER, RETIRED_KEEPER };

struct SaveGameException {
  SaveGameException(GameType t) : type(t) {
  }
  GameType type;
};

void trim(string& s);
string toUpper(const string& s);
string toLower(const string& s);

bool endsWith(const string&, const string& suffix);

vector<string> split(const string& s, const set<char>& delim);

class Rectangle;


class Vec2 {
  public:
  int x, y;
  Vec2() : x(0), y(0) {}
  Vec2(int x, int y);
  Vec2(Dir);
  bool inRectangle(int px, int py, int kx, int ky) const;
  bool inRectangle(const Rectangle&) const;
  bool operator == (const Vec2& v) const;
  bool operator != (const Vec2& v) const;
  Vec2 operator + (const Vec2& v) const;
  Vec2 operator * (int) const;
  Vec2 operator * (double) const;
  Vec2 operator / (int) const;
  Vec2& operator += (const Vec2& v);
  Vec2 operator - (const Vec2& v) const;
  Vec2& operator -= (const Vec2& v);
  Vec2 operator - () const;
  bool operator < (Vec2) const;
  Vec2 mult(const Vec2& v) const;
  Vec2 div(const Vec2& v) const;
  static int dotProduct(Vec2 a, Vec2 b);
  int length8() const;
  int length4() const;
  int dist8(Vec2) const;
  double distD(Vec2) const;
  double lengthD() const;
  Vec2 shorten() const;
  pair<Vec2, Vec2> approxL1() const;
  Vec2 getBearing() const;
  bool isCardinal4() const;
  Dir getCardinalDir() const;
  static Vec2 getCenterOfWeight(vector<Vec2>);

  vector<Vec2> box(int radius, bool shuffle = false);
  vector<Vec2> circle(double radius, bool shuffle = false);
  static vector<Vec2> directions8(bool shuffle = false);
  vector<Vec2> neighbors8(bool shuffle = false) const;
  static vector<Vec2> directions4(bool shuffle = false);
  vector<Vec2> neighbors4(bool shuffle = false) const;
  static vector<Vec2> corners();
  static vector<set<Vec2>> calculateLayers(set<Vec2>);

  typedef function<Vec2(Vec2)> LinearMap;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);
};

string getCardinalName(Dir d);

namespace std {

template <> struct hash<Vec2> {
  size_t operator()(const Vec2& obj) const {
    return hash<int>()(obj.x) * 10000 + hash<int>()(obj.y);
  }
};

#ifdef DEBUG_STL
/*template <> struct hash<__gnu_debug::string> {
  size_t operator()(const string& v) const {
    return hash<std::string>()(v);
  }
};*/

template <class T> struct hash<__gnu_debug::set<T>> {
#else
template <class T> struct hash<std::set<T>> {
#endif
  size_t operator()(const set<T>& v) const {
    size_t ret = 0;
    for (const T& elem : v)
      ret = ret * 79146198 + hash<T>()(elem);
    return ret;
  }
};

}


class Rectangle {
  public:
  friend class Vec2;
  template<typename T>
  friend class Table;
  Rectangle(int width, int height);
  Rectangle(Vec2 dim);
  Rectangle(int px, int py, int kx, int ky);
  Rectangle(Vec2 p, Vec2 k);
  static Rectangle boundingBox(const vector<Vec2>& v);

  int getPX() const;
  int getPY() const;
  int getKX() const;
  int getKY() const;
  int getW() const;
  int getH() const;

  Vec2 getTopLeft() const;
  Vec2 getBottomRight() const;
  Vec2 getTopRight() const;
  Vec2 getBottomLeft() const;

  bool intersects(const Rectangle& other) const;
  bool contains(const Rectangle& other) const;
  Rectangle intersection(const Rectangle& other) const;

  Rectangle minusMargin(int margin) const;
  Rectangle translate(Vec2 v) const;
  Rectangle apply(Vec2::LinearMap) const;

  Vec2 randomVec2() const;
  Vec2 middle() const;

  vector<Vec2> getAllSquares();

  class Iter {
    public:
    Iter(int x, int y, int px, int py, int kx, int ky);

    Vec2 operator* () const;
    bool operator != (const Iter& other) const;

    const Iter& operator++ ();

    private:
    Vec2 pos;
    int px, py, kx, ky;
  };

  Iter begin() const;
  Iter end() const;

  SERIALIZATION_DECL(Rectangle);

  private:
  int px, py, kx, ky, w, h;
};

class Range {
  public:
  Range(int start, int end);
  Range(int end);

  class Iter {
    public:
    Iter(int ind, int min, int max);

    int operator* () const;
    bool operator != (const Iter& other) const;

    const Iter& operator++ ();

    private:
    int ind;
    int min;
    int max;
  };

  Iter begin();
  Iter end();

  private:
  int start;
  int finish;
};

template <class T>
Range All(const T& container) {
  return Range(container.size());
}

extern const vector<Vec2> neighbors;

class RandomGen {
  public:
  void init(int seed);
  int get(int max);
  int get(int min, int max);
  int get(const vector<double>& weights, double r = -1);
  double getDouble();
  double getDouble(double a, double b);
  bool roll(int chance);
  bool rollD(double chance);

  private:
  default_random_engine generator;
  std::uniform_real_distribution<double> defaultDist;
};

extern RandomGen Random;

inline Debug& operator <<(Debug& d, Rectangle rect) {
  return d << "(" << rect.getPX() << "," << rect.getPY() << ") (" << rect.getKX() << "," << rect.getKY() << ")";
}


template <class T>
class Table {
  public:
  Table(Table&& t) = default;

  Table(int x, int y, int w, int h) : bounds(x, y, x + w, y + h), mem(new T[w * h]) {
  }

  Table(const Rectangle& rect) : bounds(rect), mem(new T[rect.w * rect.h]){
  }

  Table(const Rectangle& rect, const T& value) : Table(rect) {
    for (int i : Range(bounds.w * bounds.h))
      mem[i] = value;
  }

  Table(int w, int h) : Table(0, 0, w, h) {
  }

  Table(int x, int y, int width, int height, const T& value) : Table(Rectangle(x, y, x + width, y + height), value) {
  }

  Table(int width, int height, const T& value) : Table(0, 0, width, height, value) {
  }

  const Rectangle& getBounds() const {
    return bounds;
  }

  Table& operator = (Table&& other) = default;

  int getWidth() const {
    return bounds.w;
  }
  int getHeight() const {
    return bounds.h;
  }

  class RowAccess {
    public:
    RowAccess(T* m, int p, int w) : px(p), width(w), mem(m) {};
    T& operator[](int ind) {
      CHECK(ind >= px && ind < px + width);
      return mem[ind - px];
    }

    const T& operator[](int ind) const {
      CHECK(ind >= px && ind < px + width);
      return mem[ind - px];
    }

    private:
    int px;
    int width;
    T* mem;
  };

  RowAccess operator[](int ind) {
    return RowAccess(mem.get() + (ind - bounds.px) * bounds.h, bounds.py, bounds.h);
  }
 
  RowAccess operator[](int ind) const {
    return RowAccess(mem.get() + (ind - bounds.px) * bounds.h, bounds.py, bounds.h);
  }
 
  T& operator[](const Vec2& vAbs) {
#ifdef RELEASE
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
#else
    Vec2 v = vAbs - bounds.getTopLeft();
    CHECK(vAbs.inRectangle(bounds)) <<
        "Table index out of bounds " << bounds << " " << vAbs;
    return mem[v.x * bounds.h + v.y];
#endif
  }

  const T& operator[](const Vec2& vAbs) const {
#ifdef RELEASE
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
#else
    Vec2 v = vAbs - bounds.getTopLeft();
    CHECK(vAbs.inRectangle(bounds)) <<
        "Table index out of bounds " << bounds << " " << vAbs;
    return mem[v.x * bounds.h + v.y];
#endif
  }

  template <class Archive> 
  void save(Archive& ar, const unsigned int version) const {
    ar << BOOST_SERIALIZATION_NVP(bounds);
    for (Vec2 v : bounds)
      ar << boost::serialization::make_nvp("Elem", (*this)[v]);
  }

  template <class Archive> 
  void load(Archive& ar, const unsigned int version) {
    ar >> BOOST_SERIALIZATION_NVP(bounds);
    mem.reset(new T[bounds.getW() * bounds.getH()]);
    for (Vec2 v : bounds)
      ar >> boost::serialization::make_nvp("Elem", (*this)[v]);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

  SERIALIZATION_CONSTRUCTOR(Table);

  private:
  Rectangle bounds;
  unique_ptr<T[]> mem;
};

template<typename T>
class DirtyTable {
  public:
  DirtyTable(Rectangle bounds, T dirty) : val(bounds), dirty(bounds, 0), dirtyVal(dirty) {} 

  double getValue(Vec2 v) const {
    return dirty[v] < counter ? dirtyVal : val[v];
  }

  void setValue(Vec2 v, const T& d) {
    val[v] = d;
    dirty[v] = counter;
  }

  void clear() {
    ++counter;
  }

  private:
  Table<T> val;
  Table<int> dirty;
  T dirtyVal;
  int counter = 1;
};

template <typename T>
T chooseElem(const vector<T>& v, int ind) {
  return v[ind];
}

template <typename T>
T chooseRandom(const vector<T>& v, const vector<double>& p, double r = -1) {
  CHECK(v.size() == p.size());
  return v[Random.get(p, r)];
}


template <typename T>
T chooseRandom(const vector<T>& v, double r = -1) {
  vector<double> pi(v.size(), 1);
  return chooseRandom(v, pi, r);
}

template <typename T>
T chooseRandom(const set<T>& vi, double r = -1) {
  vector<T> v(vi.size());
  std::copy(vi.begin(), vi.end(), v.begin());
  return chooseRandom(v, r);
}

template <typename T>
T chooseRandom(initializer_list<T> vi, initializer_list<double> pi, double r = -1) {
  return chooseRandom(vector<T>(vi), vector<double>(pi), r);
}

template <typename T>
T chooseRandom(initializer_list<T> vi, double r = -1) {
  vector<T> v(vi);
  vector<double> pi(v.size(), 1);
  return chooseRandom(v, pi, r);
}

template <typename T>
T chooseRandom(vector<pair<T, double>> vi, double r = -1) {
  vector<T> v;
  vector<double> p;
  for (auto elem : vi) {
    v.push_back(elem.first);
    p.push_back(elem.second);
  }
  return chooseRandom(v, p);
}

template <typename T>
vector<T> randomPermutation(vector<T> v) {
  random_shuffle(v.begin(), v.end(), [](int a) { return Random.get(a);});
  return v;
}

template <typename T>
vector<T> randomPermutation(const set<T>& vi) {
  vector<T> v(vi.size());
  std::copy(vi.begin(), vi.end(), v.begin());
  return randomPermutation(v);
}

template <typename T>
vector<T> randomPermutation(initializer_list<T> vi) {
  vector<T> v(vi);
  random_shuffle(v.begin(), v.end(), [](int a) { return Random.get(a);});
  return v;
}

template <typename T, typename V>
vector<T> getKeys(const map<T, V>& m) {
  vector<T> ret;
  for (auto elem : m)
    ret.push_back(elem.first);
  return ret;
}

template <typename T, typename V>
vector<T> getKeys(const unordered_map<T, V>& m) {
  vector<T> ret;
  for (auto elem : m)
    ret.push_back(elem.first);
  return ret;
}

template <typename T>
T copyOf(const T& t) {
  return t;
}

template <typename T, typename V>
map<V, vector<T> > groupBy(const vector<T>& values, function<V (const T&)> getKey) {
  map<V, vector<T> > ret;
  for (const T& elem : values)
    ret[getKey(elem)].push_back(elem);
  return ret;
}

template <typename T>
vector<T> getSubsequence(const vector<T>& v, int start, int length) {
  CHECK(start >= 0 && length > 0 && start + length <= v.size());
  vector<T> ret;
  for (int i : Range(start, start + length))
    ret.push_back(v[i]);
  return ret;
}

template <typename T>
vector<T> getPrefix(const vector<T>& v, int num) {
  return getSubsequence(v, 0, num);
}

template <typename T>
vector<T> getSuffix(const vector<T>& v, int num) {
  return getSubsequence(v, v.size() - num, num);
}

template <typename T, typename U, typename Fun>
vector<T> transform2(const vector<U>& u, Fun fun) {
  vector<T> ret(u.size());
  transform(u.begin(), u.end(), ret.begin(), fun);
  return ret;
}

template <typename T, typename U, typename Fun>
vector<T> transform2(vector<U>& u, Fun fun) {
  vector<T> ret(u.size());
  transform(u.begin(), u.end(), ret.begin(), fun);
  return ret;
}

template <typename T>
vector<T> reverse2(vector<T> v) {
  reverse(v.begin(), v.end());
  return v;
}

template <typename T, typename Predicate>
vector<T> filter(const vector<T>& v, Predicate predicate) {
  vector<T> ret;
  for (const T& t : v)
    if (predicate(t))
      ret.push_back(t);
  return ret;
}

template <typename T, typename Predicate>
vector<T> filter(vector<T>&& v, Predicate predicate) {
  vector<T> ret;
  for (T& t : v)
    if (predicate(t))
      ret.push_back(std::move(t));
  return ret;
}

template <typename T, typename V>
bool contains(const T& v, const V& elem) {
  return std::find(v.begin(), v.end(), elem) != v.end();
}

template<>
bool contains(const string& s, const string& pattern);

template <typename T, typename V>
bool contains(const initializer_list<T>& v, const V& elem) {
  return std::find(v.begin(), v.end(), elem) != v.end();
}

template <typename T>
void append(vector<T>& v, const vector<T>& w) {
  for (T elem : w)
    v.push_back(elem);
}

template <typename T>
void append(vector<T>& v, vector<T>&& w) {
  for (T& elem : w)
    v.push_back(std::move(elem));
}

template <typename T, typename U>
void append(vector<T>& v, const U& u) {
  for (T elem : u)
    v.push_back(elem);
}

template <typename T>
vector<T> concat(vector<T> v, const vector<T>& w) {
  append(v, w);
  return v;
}

template <class T>
function<bool(const T&)> alwaysTrue() {
  return [](T) { return true; };
}

template <class T>
function<bool(const T&)> alwaysFalse() {
  return [](T) { return false; };
}

template <class T>
function<bool(const T&)> andFun(function<bool(const T&)> x, const function<bool(const T&)> y) {
  return [x, y](T t) { return x(t) && y(t); };
}

class OnExit {
  public:
  OnExit(function<void()> f) : fun(f) {}

  ~OnExit() { fun(); }

  private:
  function<void()> fun;
};

class Nothing {
};

template <class T>
class Optional {
  public:
  Optional(const T& t) {
    elem.push_back(t);
  }
  Optional(const Optional<T>& t) {
    if (t)
      elem.push_back(*t);
  }
  Optional(Optional<T>&&) = default;
  Optional(Nothing) {}
  Optional() {}

  Optional<T>& operator = (const Optional<T>& t) {
    if (t) {
      elem.clear();
      elem.push_back(*t);
    }
    else
      elem.clear();
    return *this;
  }

  T& operator = (const T& t) {
    elem.clear();
    elem.push_back(t);
    return elem.front();
  }

  Optional<T>& operator = (Optional<T>&& t) = default;

  void operator = (Nothing) {
    elem.clear();
  }

  T& operator = (T&& t) {
    if (!elem.empty())
      elem[0] = std::move(t);
    else {
      elem.push_back(std::move(t));
    }
    return elem.front();
  }

  T* operator -> () {
    CHECK(!elem.empty());
    return &elem.front();
  }

  const T* operator -> () const {
    CHECK(!elem.empty());
    return &elem.front();
  }

  bool operator == (const T& t) const {
    return !elem.empty() && elem.front() == t;
  }

  bool operator != (const T& t) const {
    return elem.empty() || elem.front() != t;
  }

  operator bool() const {
    return !elem.empty();
  }

  T& operator * () {
    CHECK(!elem.empty());
    return elem.front();
  }

  const T& operator * () const {
    CHECK(!elem.empty());
    return elem.front();
  }

  T getOr(const T& other) {
    if (!elem.empty())
      return elem.front();
    else
      return other;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_NVP(elem);
  }

  private:
  vector<T> elem;
};

template <typename T, typename V>
bool contains(const vector<T>& v, const Optional<V>& elem) {
  return elem && contains(v, *elem);
}

template <typename T, typename V>
bool contains(const initializer_list<T>& v, const Optional<V>& elem) {
  return contains(vector<T>(v), elem);
}

template <class T>
class MustInitialize {
  public:
  MustInitialize(const MustInitialize& o) : elem(o.elem) {
    CHECK(!elem.empty()) << "Element not initialized";
  }

  MustInitialize() {}

  T& operator = (const T& t) {
    if (!elem.empty())
      elem.pop();
    CHECK(elem.empty());
    elem.push(t);
    return elem.front();
  }

  T& operator += (const T& t) {
    CHECK(!elem.empty()) << "Element not initialized";
    return elem.front() += t;
  }

  T& operator -= (const T& t) {
    CHECK(!elem.empty()) << "Element not initialized";
    return elem.front() -= t;
  }

  T* operator -> () {
    CHECK(!elem.empty());
    return &elem.front();
  }

  const T* operator -> () const {
    CHECK(!elem.empty()) << "Element not initialized";
    return &elem.front();
  }

  bool operator == (const T& t) const {
    CHECK(!elem.empty()) << "Element not initialized";
    return elem.front() == t;
  }

  bool operator != (const T& t) const {
    CHECK(!elem.empty()) << "Element not initialized";
    return elem.front() != t;
  }

  T& operator * () {
    CHECK(!elem.empty()) << "Element not initialized";
    return elem.front();
  }

  const T& operator * () const {
    CHECK(!elem.empty()) << "Element not initialized";
    return elem.front();
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_NVP(elem);
  }

  private:
  queue<T> elem;
};

template<class T>
vector<T> concat(const vector<vector<T>>& vectors) {
  vector<T> ret;
  for (const vector<T>& v : vectors)
    for (const T& t : v)
      ret.push_back(t);
  return ret;
}

template<class T>
inline T& operator <<(T& d, Vec2 msg) {
  return d << "(" << msg.x << "," << msg.y << ")";
}

inline std::istream& operator >>(std::istream& d, Vec2& msg) {
  string in;
  d >> in;
  vector<string> s = split(in.substr(1, in.size() - 2), {','});
  CHECKEQ((int)s.size(), 2);
  msg = Vec2(fromString<int>(s[0]), fromString<int>(s[1]));
  return d;
}

template<class T>
Debug& operator<<(Debug& d, const Table<T>& container){
  for (int i : Range(container.height())) {
    d << i << ":";
    for (int j : Range(container.width()))
      d << container[j][i] << ",";
    d << '\n';
  }
  return d;
}

inline NoDebug& operator <<(NoDebug& d, Vec2 msg) {
  return d;
}

inline NoDebug& operator <<(NoDebug& d, Rectangle msg) {
  return d;
}

template<class T>
NoDebug& operator<<(NoDebug& d, const Table<T>& container){
  return d;
}

template<class T>
void removeIndex(vector<T>& v, int index) {
  v[index] = std::move(v.back());
  v.pop_back();
}

template<class T>
Optional<int> findElement(const vector<T>& v, const T& element) {
  for (int i : All(v))
    if (v[i] == element)
      return i;
  return Nothing();
}

template<class T>
Optional<int> findElement(const vector<T*>& v, const T* element) {
  for (int i : All(v))
    if (v[i] == element)
      return i;
  return Nothing();
}

template<class T>
Optional<int> findElement(const vector<unique_ptr<T>>& v, const T* element) {
  for (int i : All(v))
    if (v[i].get() == element)
      return i;
  return Nothing();
}

template<class T>
bool removeElementMaybe(vector<T>& v, const T& element) {
  if (auto ind = findElement(v, element)) {
    removeIndex(v, *ind);
    return true;
  }
  return false;
}

template<class T>
void removeElement(vector<T>& v, const T& element) {
  auto ind = findElement(v, element);
  CHECK(ind) << "Element not found";
  removeIndex(v, *ind);
}

template<class T>
unique_ptr<T> removeElement(vector<unique_ptr<T>>& v, const T* element) {
  auto ind = findElement(v, element);
  CHECK(ind) << "Element not found";
  unique_ptr<T> ret = std::move(v[*ind]);
  removeIndex(v, *ind);
  return ret;
}
template<class T>
void removeElement(vector<T*>& v, const T* element) {
  auto ind = findElement(v, element);
  CHECK(ind) << "Element not found";
  removeIndex(v, *ind);
}

template<class T>
T getOnlyElement(const vector<T>& v) {
  CHECK(v.size() == 1);
  return v[0];
}

// TODO: write a template that works with all containers
template<class T>
T getOnlyElement(const set<T>& v) {
  CHECK(v.size() == 1);
  return *v.begin();
}

template<class T>
T getOnlyElement(vector<T>&& v) {
  CHECK(v.size() == 1);
  return std::move(v[0]);
}

template <typename T>
void emplaceBack(vector<T>&) {}

template <typename T, typename First, typename... Args>
void emplaceBack(vector<T>& v, First&& first, Args&&... args) {
  v.emplace_back(std::move(std::forward<First>(first)));
  emplaceBack(v, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
vector<T> makeVec(Args&&... args) {
  vector<T> ret;
  emplaceBack(ret, args...);
  return ret;
}

string addAParticle(const string& s);

string capitalFirst(string s);
vector<string> makeSentences(string s);
string makeSentence(string s);

string lowercase(string s);

string combine(const vector<string>& adj);

string getPlural(const string& singular, const string& plural, int num);
string getPlural(const string&, int num);

template<class T>
string combine(const vector<T*>& v) {
  return combine(
      transform2<string>(v, [](const T* e) { return e->getName(); }));
}

template<class T>
string combine(const vector<T>& v) {
  return combine(
      transform2<string>(v, [](const T& e) { return e.name; }));
}

template<typename T>
class EnumInfo {
  public:
  static string getString(T);
  static constexpr int getSize() { return 0;}
};

#define RICH_ENUM(Name, ...) \
enum class Name { __VA_ARGS__ };\
namespace std {\
template <>\
struct hash<Name> {\
  size_t operator()(Name obj) const {\
    return int(obj);\
  }\
};\
}\
template<> \
class EnumInfo<Name> { \
  public:\
  static string getString(Name e) {\
    static vector<string> names = split(#__VA_ARGS__, {' ', ','});\
    return names[int(e)];\
  }\
  enum class Tmp { __VA_ARGS__, END_TMP};\
  static constexpr int getSize() {\
    return (int)Tmp::END_TMP;\
  }\
  static Name fromString(const string& s) {\
    for (int i : Range(getSize())) \
      if (getString(Name(i)) == s) \
        return Name(i); \
    FAIL << #Name << " value not found " << s;\
    return Name(0);\
  }\
}

RICH_ENUM(Dir, N, S, E, W, NE, NW, SE, SW );

template<class T, class U>
class EnumMap {
  public:
  EnumMap() : elems() {
  }

  EnumMap(const EnumMap& o) : elems(o.elems) {}

  bool operator == (const EnumMap<T, U>& other) const {
    return elems == other.elems;
  }

  void clear(U value) {
    for (int i = 0; i < EnumInfo<T>::getSize(); ++i)
      elems[i] = value;
  }

  void clear() {
    for (int i = 0; i < EnumInfo<T>::getSize(); ++i)
      elems[i] = U();
  }

  EnumMap(initializer_list<pair<T, U>> il) : EnumMap() {
    for (auto elem : il)
      elems[int(elem.first)] = elem.second;
  }

  void operator = (initializer_list<pair<T, U>> il) {
    for (auto elem : il)
      elems[int(elem.first)] = elem.second;
  }

  void operator = (const EnumMap& other) {
    elems = other.elems;
  }

  const U& operator[](T elem) const {
    return elems[int(elem)];
  }

  U& operator[](T elem) {
    return elems[int(elem)];
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & elems;
  }

  private:
  std::array<U, EnumInfo<T>::getSize()> elems;
};

template<class T>
class EnumSet : public EnumMap<T, char> {
  public:
  EnumSet() {}

  EnumSet(initializer_list<T> il) {
    for (auto elem : il)
      insert(elem);
  }

  EnumSet(initializer_list<char> il) {
    CHECK(il.size() == EnumInfo<T>::getSize());
    int cnt = 0;
    for (int i : il)
      (*this)[T(cnt++)] = i;
  }

  void insert(T elem) {
    (*this)[elem] = 1;
  }

  void erase(T elem) {
    (*this)[elem] = 0;
  }

  EnumSet sum(const EnumSet& other) const {
    EnumSet ret(other);
    for (T elem : *this)
      ret.insert(elem);
    return ret;
  }

  EnumSet intersection(const EnumSet& other) const {
    EnumSet ret;
    for (T elem : *this)
      if (other[elem])
        ret.insert(elem);
    return ret;
  }

  static EnumSet<T> fullSet() {
    EnumSet<T> ret;
    ret.clear(1);
    return ret;
  }

  class Iter {
    public:
    Iter(const EnumSet& s, int num) : set(s), ind(num) {
      goForward();
    }

    void goForward() {
      while (ind < EnumInfo<T>::getSize() && !set[T(ind)])
        ++ind;
    }

    T operator* () const {
      return T(ind);
    }

    bool operator != (const Iter& other) const {
      return ind != other.ind;
    }

    const Iter& operator++ () {
      ++ind;
      goForward();
      return *this;
    }

    private:
    const EnumSet& set;
    int ind;
  };

  Iter begin() const {
    return Iter(*this, 0);
  }

  Iter end() const {
    return Iter(*this, EnumInfo<T>::getSize());
  }
};

namespace std {

template <typename T>
struct hash<EnumSet<T>> {
  size_t operator()(const EnumSet<T>& obj) const {
    size_t ret = 0;
    for (const T& elem : obj) {
      ret = ret * 79146198 + hash<T>()(elem);
    }
    return ret;
  }
};
}

template <class T>
class EnumAll {
  public:
  class Iter {
    public:
    Iter(int num) : ind(num) {
    }

    T operator* () const {
      return T(ind);
    }

    bool operator != (const Iter& other) const {
      return ind != other.ind;
    }

    const Iter& operator++ () {
      ++ind;
      return *this;
    }

    private:
    int ind;
  };

  Iter begin() {
    return Iter(0);
  }

  Iter end() {
    return Iter(EnumInfo<T>::getSize());
  }
};

#define ENUM_ALL(X) EnumAll<X>()

template <typename T>
T chooseRandom(EnumMap<T, double> vi, double r = -1) {
  vector<T> v;
  vector<double> p;
  for (T elem : ENUM_ALL(T)) {
    v.push_back(elem);
    p.push_back(vi[elem]);
  }
  return chooseRandom(v, p);
}

template <typename T>
T chooseRandom() {
  return T(Random.get(EnumInfo<T>::getSize()));
}

template <typename U, typename V>
class BiMap {
  public:
  bool contains(const U& u) const {
    return m1.count(u);
  }

  bool contains(const V& v) const {
    return m2.count(v);
  }

  void insert(const U& u, const V& v) {
    CHECK(!m1.count(u));
    CHECK(!m2.count(v));
    m1[u] = v;
    m2[v] = u;
  }

  void erase(const U& u) {
    CHECK(m1.count(u));
    m2.erase(m1.at(u));
    m1.erase(u);
  }

  void erase(const V& v) {
    CHECK(m2.count(v));
    m1.erase(m2.at(v));
    m2.erase(v);
  }

  V& get(const U& u) {
    CHECK(m1.count(u));
    return m1.at(u);
  }

  U& get(const V& v) {
    CHECK(m2.count(v));
    return m2.at(v);
  }

  const V& get(const U& u) const {
    CHECK(m1.count(u));
    return m1.at(u);
  }

  const U& get(const V& v) const {
    CHECK(m2.count(v));
    return m2.at(v);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & m1 & m2;
  }

  private:
  map<U, V> m1;
  map<V, U> m2;
};

class Semaphore {
  public:
  Semaphore(int val = 0);

  void p();
  void v();

  private:
  int value;
  std::mutex mut;
  std::condition_variable cond;
};

template <class T>
class SyncQueue {
  public:
  T pop() {
    std::unique_lock<std::mutex> lock(mut);
    while (q.empty()) {
      cond.wait(lock);
    }
    OnExit o([=] { q.pop(); });
    return q.front();
  }

  Optional<T> popAsync() {
    std::unique_lock<std::mutex> lock(mut);
    if (q.empty())
      return Nothing();
    else {
      OnExit o([=] { q.pop(); });
      return q.front();
    }
  }

  void push(const T& t) {
    std::unique_lock<std::mutex> lock(mut);
    q.push(t);
    lock.unlock();
    cond.notify_one();
  }

  private:
  std::mutex mut;
  std::condition_variable cond;
  queue<T> q;
};

class AsyncLoop {
  public:
  AsyncLoop(function<void()> init, function<void()> loop);
  AsyncLoop(function<void()>);
  ~AsyncLoop();

  private:
  thread t;
  std::atomic<bool> done;
};

template <typename T, typename... Args>
function<void(Args...)> bindMethod(void (T::*ptr) (Args...), T* t) {
  return [=](Args... a) { (t->*ptr)(a...);};
}

template <typename... Args>
function<void(Args...)> bindFunction(void (*ptr) (Args...)) {
  return [=](Args... a) { (*ptr)(a...);};
}

template <typename Ret, typename... Args>
function<Ret(Args...)> bindFunction(Ret (*ptr) (Args...)) {
  return [=](Args... a) { return (*ptr)(a...);};
}

struct ConstructorFunction {
  ConstructorFunction(function<void()> inConstructor);
};

struct DestructorFunction {
  DestructorFunction(function<void()> inDestructor);
  ~DestructorFunction();
    
  private:
  function<void()> destFun;
};


template<typename... Args>
struct NamedTupleBase : public tuple<Args...> {
  typedef tuple<Args...> BaseTuple;
  NamedTupleBase(Args... a) : BaseTuple(a...) {}
  NamedTupleBase() {}
  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    boost::serialization::serialize<Archive, Args...>(ar, *this, version);
  }
};

#define NAMED_TUPLE_STUFF(name)\
  using NamedTupleBase::NamedTupleBase;\
  name& operator = (const name& a) { NamedTupleBase::operator = (a); return *this; }

#define NAME_ELEM(num, name)\
  std::tuple_element<num, BaseTuple>::type& name() { return get<num>(*this); }\
  const std::tuple_element<num, BaseTuple>::type& name() const { return get<num>(*this); }


#endif
