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

#pragma once

#include <iostream>

#include "debug.h"
#include "enums.h"
#include "serialization.h"
#include "hashing.h"
#include "owner_pointer.h"
#include "container_helpers.h"
#include "container_range.h"

template <class T>
string toString(const T& t) {
  stringstream ss;
  ss << t;
  return ss.str();
}

template <class T>
string toString(const optional<T>& t) {
  if (t)
    return toString(*t);
  else
    return "(unknown)";
}

class ParsingException {
};

template <class T>
T fromString(const string& s) {
  std::stringstream ss(s);
  T t;
  ss >> t;
  if (!ss)
    throw ParsingException();
  return t;
}

template <class T>
optional<T> fromStringSafe(const string& s);

#define DEF_UNIQUE_PTR(T) class T;\
  typedef unique_ptr<T> P##T;

#define DEF_SHARED_PTR(T) class T;\
  typedef shared_ptr<T> S##T;

#define DEF_OWNER_PTR(T) class T;\
  typedef OwnerPointer<T> P##T; \
  typedef weak_ptr<T> W##T; \
  template <typename... Args> \
  OwnerPointer<T> make##T(Args... a) { \
    return makeOwner<T>(a...); \
  } \
  template <typename Subclass, typename... Args> \
  OwnerPointer<T> make##T(Args... a) { \
    return makeOwner<T, Subclass>(a...); \
  }

DEF_OWNER_PTR(Item);
DEF_UNIQUE_PTR(LevelMaker);
DEF_UNIQUE_PTR(Creature);
DEF_UNIQUE_PTR(Square);
DEF_UNIQUE_PTR(Furniture);
DEF_UNIQUE_PTR(MonsterAI);
DEF_UNIQUE_PTR(Behaviour);
DEF_UNIQUE_PTR(Task);
DEF_SHARED_PTR(Controller);
DEF_UNIQUE_PTR(Trigger);
DEF_UNIQUE_PTR(Level);
DEF_UNIQUE_PTR(VillageControl);
DEF_SHARED_PTR(GuiElem);
DEF_UNIQUE_PTR(Animation);
DEF_UNIQUE_PTR(ViewObject);
DEF_UNIQUE_PTR(Collective);
DEF_UNIQUE_PTR(CollectiveControl);
DEF_UNIQUE_PTR(Model);
DEF_UNIQUE_PTR(Tribe);
DEF_UNIQUE_PTR(Game);

template <typename T>
T lambdaConstruct(function<void(T&)> fun) {
  T ret {};
  fun(ret);
  return ret;
}

#define CONSTRUCT(Class, Code) lambdaConstruct<Class>([&] (Class& c) { Code })
#define CONSTRUCT_GLOBAL(Class, Code) lambdaConstruct<Class>([] (Class& c) { Code })

#define LIST(...) {__VA_ARGS__}

class Item;
typedef function<bool(const Item*)> ItemPredicate;

template<class T>
vector<T*> extractRefs(vector<unique_ptr<T>>& v) {
  vector<T*> ret;
  ret.reserve(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}

template<class T>
vector<unique_ptr<T>> toUniquePtr(const vector<T*>& v) {
  vector<unique_ptr<T>> ret;
  ret.reserve(v.size());
  for (T* el : v)
    ret.push_back(unique_ptr<T>(el));
  return ret;
}

template<class T>
const vector<T*> extractRefs(const vector<unique_ptr<T>>& v) {
  vector<T*> ret;
  ret.reserve(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}

template<class T>
const vector<T*> extractRefs(const vector<OwnerPointer<T>>& v) {
  vector<T*> ret;
  ret.reserve(v.size());
  for (auto& el : v)
    ret.push_back(el.get());
  return ret;
}

void trim(string& s);
string toUpper(const string& s);
string toLower(const string& s);

bool endsWith(const string&, const string& suffix);

vector<string> split(const string& s, const set<char>& delim);
vector<string> removeEmpty(const vector<string>&);
string combineWithOr(const vector<string>&);



class Rectangle;
class RandomGen;

string getCardinalName(Dir d);

class Vec2 {
  public:
  int SERIAL(x); // HASH(x)
  int SERIAL(y); // HASH(y)
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
  bool isCardinal8() const;
  Dir getCardinalDir() const;
  static Vec2 getCenterOfWeight(vector<Vec2>);

  vector<Vec2> box(int radius, bool shuffle = false);
  vector<Vec2> circle(double radius, bool shuffle = false);
  static vector<Vec2> directions8();
  vector<Vec2> neighbors8() const;
  static vector<Vec2> directions4();
  vector<Vec2> neighbors4() const;
  static vector<Vec2> directions8(RandomGen&);
  vector<Vec2> neighbors8(RandomGen&) const;
  static vector<Vec2> directions4(RandomGen&);
  vector<Vec2> neighbors4(RandomGen&) const;
  vector<Vec2> neighbors(const vector<Vec2>& directions) const;
  static vector<Vec2> corners();
  static vector<set<Vec2>> calculateLayers(set<Vec2>);

  typedef function<Vec2(Vec2)> LinearMap;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
   
  HASH_ALL(x, y);
};

extern string toString(const Vec2& v);

class Range {
  public:
  Range(int start, int end);
  explicit Range(int end);
  static Range singleElem(int);

  bool isEmpty() const;
  Range reverse();
  Range shorten(int r);

  int getStart() const;
  int getEnd() const;
  int getLength() const;
  bool contains(int) const;

  class Iter {
    public:
    Iter(int ind, int min, int max, int increment);

    int operator* () const;
    bool operator != (const Iter& other) const;

    const Iter& operator++ ();

    private:
    int ind;
    int min;
    int max;
    int increment;
  };

  Iter begin();
  Iter end();

  SERIALIZATION_DECL(Range)
  HASH_ALL(start, finish, increment)
  
  private:
  int SERIAL(start) = 0; // HASH(start)
  int SERIAL(finish) = 0; // HASH(finish)
  int SERIAL(increment) = 1; // HASH(increment)
};

class Rectangle {
  public:
  friend class Vec2;
  template<typename T>
  friend class Table;
  Rectangle(int width, int height);
  Rectangle(Vec2 dim);
  Rectangle(int px, int py, int kx, int ky);
  Rectangle(Vec2 p, Vec2 k);
  Rectangle(Range xRange, Range yRange);
  static Rectangle boundingBox(const vector<Vec2>& v);
  static Rectangle centered(Vec2 center, int radius);

  int left() const;
  int top() const;
  int right() const;
  int bottom() const;
  int width() const;
  int height() const;
  Vec2 getSize() const;
  Range getYRange() const;
  Range getXRange() const;
  int area() const;

  Vec2 topLeft() const;
  Vec2 bottomRight() const;
  Vec2 topRight() const;
  Vec2 bottomLeft() const;

  bool intersects(const Rectangle& other) const;
  bool contains(const Rectangle& other) const;
  Rectangle intersection(const Rectangle& other) const;

  Rectangle minusMargin(int margin) const;
  Rectangle translate(Vec2 v) const;
  Rectangle apply(Vec2::LinearMap) const;

  Vec2 randomVec2() const;
  Vec2 middle() const;

  vector<Vec2> getAllSquares() const;

  bool operator == (const Rectangle&) const;
  bool operator != (const Rectangle&) const;

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
  int px = 0, py = 0, kx = 0, ky = 0, w = 0, h = 0;
};

template <class T>
Range All(const T& container) {
  return Range(container.size());
}

template <class T>
Range AllReverse(const T& container) {
  return All(container).reverse();
}

template<typename T>
class EnumInfo {
  public:
  static string getString(T);
};

#define RICH_ENUM(Name, ...) \
enum class Name { __VA_ARGS__ };\
template<> \
class EnumInfo<Name> { \
  public:\
  static string getString(Name e) {\
    static vector<string> names = removeEmpty(split(#__VA_ARGS__, {' ', ','}));\
    return names[int(e)];\
  }\
  enum Tmp { __VA_ARGS__, size};\
  static Name fromString(const string& s) {\
    for (int i : Range(size)) \
      if (getString(Name(i)) == s) \
        return Name(i); \
    throw ParsingException();\
  }\
  static optional<Name> fromStringSafe(const string& s) {\
    for (int i : Range(size)) \
      if (getString(Name(i)) == s) \
        return Name(i); \
    return none;\
  }\
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
    return Iter(EnumInfo<T>::size);
  }
};

#define ENUM_ALL(X) EnumAll<X>()

template<class T, class U>
class EnumMap {
  public:
  EnumMap() {
    clear();
  }

  EnumMap(const EnumMap& o) : elems(o.elems) {}
  EnumMap(EnumMap&& o) : elems(std::move(o.elems)) {}

  EnumMap(function<U(T)> f) {
    for (T t : EnumAll<T>())
      (*this)[t] = f(t);
  }

  bool operator == (const EnumMap<T, U>& other) const {
    return elems == other.elems;
  }

  void clear(U value) {
    for (int i = 0; i < EnumInfo<T>::size; ++i)
      elems[i] = value;
  }

  void clear() {
    for (int i = 0; i < EnumInfo<T>::size; ++i)
      elems[i] = U();
  }

  EnumMap(initializer_list<pair<T, U>> il) : EnumMap() {
    for (auto elem : il)
      elems[int(elem.first)] = elem.second;
  }

  EnumMap(const map<T, U> m) : EnumMap() {
    for (auto elem : m)
      elems[int(elem.first)] = elem.second;
  }

  EnumMap& operator = (initializer_list<pair<T, U>> il) {
    for (auto elem : il)
      elems[int(elem.first)] = elem.second;
    return *this;
  }

  EnumMap& operator = (const EnumMap& other) {
    elems = other.elems;
    return *this;
  }

  const U& operator[](T elem) const {
    CHECK(int(elem) >= 0 && int(elem) < EnumInfo<T>::size);
    return elems[int(elem)];
  }

  U& operator[](T elem) {
    CHECK(int(elem) >= 0 && int(elem) < EnumInfo<T>::size);
    return elems[int(elem)];
  }

  size_t getHash() const {
    return combineHashIter(elems.begin(), elems.end());
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    vector<U> SERIAL(tmp);
    for (int i : All(elems))
      tmp.push_back(std::move(elems[i]));
    ar & SVAR(tmp);
    CHECK(tmp.size() <= elems.size()) << tmp.size() << " " << elems.size();
    for (int i : All(tmp))
      elems[i] = std::move(tmp[i]);
  }

  private:
  std::array<U, EnumInfo<T>::size> elems;
};

std::string operator "" _s(const char* str, size_t);
class RandomGen {
  public:
  RandomGen() {}
  RandomGen(RandomGen&) = delete;
  void init(int seed);
  int get(int max);
  long long getLL();
  int get(int min, int max);
  int get(Range);
  int get(const vector<double>& weights);
  double getDouble();
  double getDouble(double a, double b);
  bool roll(int chance);
  bool chance(double chance);
  template <typename T>
  T choose(const vector<T>& v, const vector<double>& p) {
    CHECK(v.size() == p.size());
    return v.at(get(p));
  }

  template <typename T>
  T choose(const vector<T>& v) {
    vector<double> pi(v.size(), 1);
    return choose(v, pi);
  }

  template <typename T>
  T choose(const set<T>& vi) {
    vector<T> v(vi.size());
    std::copy(vi.begin(), vi.end(), v.begin());
    return choose(v);
  }

  template <typename T>
  T choose(initializer_list<T> vi, initializer_list<double> pi) {
    return choose(vector<T>(vi), vector<double>(pi));
  }

  template <typename T>
  T choose(const EnumMap<T, double>& map) {
    vector<double> weights(EnumInfo<T>::size);
    for (T t : ENUM_ALL(T))
      weights[(int)t] = map[t];
    return (T) get(weights);
  }

  template <typename T, typename... Args>
  const T& choose(T const& first, T const& second, const Args&... rest) {
    return chooseImpl(first, 2, second, rest...);
  }

  template <typename T>
  T choose(vector<pair<T, double>> vi) {
    vector<T> v;
    vector<double> p;
    for (auto elem : vi) {
      v.push_back(elem.first);
      p.push_back(elem.second);
    }
    return choose(v, p);
  }

  template <typename T>
  T choose() {
    return T(get(EnumInfo<T>::size));
  }

  template <typename T>
  vector<T> permutation(vector<T> v) {
    std::shuffle(v.begin(), v.end(), generator);
    return v;
  }

  template <typename Iterator>
  void shuffle(Iterator begin, Iterator end) {
    std::shuffle(begin, end, generator);
  }

  template <typename T>
  vector<T> permutation(const set<T>& vi) {
    vector<T> v(vi.size());
    std::copy(vi.begin(), vi.end(), v.begin());
    return permutation(v);
  }

  template <typename T>
  vector<T> permutation(initializer_list<T> vi) {
    vector<T> v(vi);
    random_shuffle(v.begin(), v.end(), [this](int a) { return get(a);});
    return v;
  }

  vector<int> permutation(Range r) {
    vector<int> v;
    for (int i : r)
      v.push_back(i);
    random_shuffle(v.begin(), v.end(), [this](int a) { return get(a);});
    return v;
  }

  template <typename T>
  vector<T> chooseN(int n, vector<T> v) {
    CHECK(n <= v.size());
    random_shuffle(v.begin(), v.end(), [this](int a) { return get(a);});
    return getPrefix(v, n);
  }

  template <typename T>
  vector<T> chooseN(int n, initializer_list<T> v) {
    return chooseN(n, vector<T>(v));
  }

  private:
  default_random_engine generator;
  std::uniform_real_distribution<double> defaultDist;

  template <typename T>
  const T& chooseImpl(T const& cur, int total) {
    return cur;
  }

  template <typename T, typename... Args>
  const T& chooseImpl(T const& chosen, int total,  T const& next, const Args&... rest) {
    const T& nextChosen = roll(total) ? next : chosen;
    return chooseImpl(nextChosen, total + 1, rest...);
  }
};

extern RandomGen Random;

inline std::ostream& operator <<(std::ostream& d, Rectangle rect) {
  return d << "(" << rect.left() << "," << rect.top() << ") (" << rect.right() << "," << rect.bottom() << ")";
}


template <class T>
class Table {
  public:
  Table(Table&& t) = default;

  Table(const Table& t) : Table(t.bounds) {
    for (int i : Range(bounds.w * bounds.h))
      mem[i] = t.mem[i];
  }

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
  Table& operator = (const Table& other) {
    bounds = other.bounds;
    mem.reset(new T[bounds.w * bounds.h]);
    for (int i : Range(bounds.w * bounds.h))
      mem[i] = other.mem[i];
    return *this;
  }

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
    Vec2 v = vAbs - bounds.topLeft();
    CHECK(vAbs.inRectangle(bounds)) <<
        "Table index out of bounds " << bounds << " " << vAbs;
    return mem[v.x * bounds.h + v.y];
#endif
  }

  const T& operator[](const Vec2& vAbs) const {
#ifdef RELEASE
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
#else
    Vec2 v = vAbs - bounds.topLeft();
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
    mem.reset(new T[bounds.width() * bounds.height()]);
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

  const T& getValue(Vec2 v) const {
    return dirty[v] < counter ? dirtyVal : val[v];
  }

  bool isDirty(Vec2 v) const {
    return dirty[v] == counter;
  }

  T& getDirtyValue(Vec2 v) {
    CHECK(isDirty(v));
    return val[v];
  }

  void setValue(Vec2 v, const T& d) {
    val[v] = d;
    dirty[v] = counter;
  }

  const Rectangle& getBounds() const {
    return val.getBounds();
  }

  void clear() {
    ++counter;
  }

  void clear(Vec2 v) {
    dirty[v] = counter - 1;
  }

  private:
  Table<T> val;
  Table<int> dirty;
  T dirtyVal;
  int counter = 1;
};

template <typename T, typename V>
vector<T> getKeys(const map<T, V>& m) {
  vector<T> ret;
  for (auto elem : m)
    ret.push_back(elem.first);
  return ret;
}

template <typename T, typename V>
vector<V> getValues(const map<T, V>& m) {
  vector<V> ret;
  for (auto elem : m)
    ret.push_back(elem.second);
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
  CHECK(start >= 0 && length >= 0 && start + length <= v.size());
  vector<T> ret;
  ret.reserve(length);
  for (int i : Range(start, start + length))
    ret.push_back(v[i]);
  return ret;
}

template <typename T>
vector<T> getPrefix(const vector<T>& v, int num) {
  return getSubsequence(v, 0, num);
}

template <typename T>
vector<T> getPrefixOrAll(const vector<T>& v, int num) {
  return getSubsequence(v, 0, min((int)v.size(), num));
}

template <typename T>
vector<T> getSuffix(const vector<T>& v, int num) {
  return getSubsequence(v, v.size() - num, num);
}

template <typename Fun>
string transform2(const string& u, Fun fun) {
  string ret(u.size(), ' ');
  transform(u.begin(), u.end(), ret.begin(), fun);
  return ret;
}

template <typename U, typename Fun>
auto transform2(const vector<U>& u, Fun fun) -> vector<decltype(fun(u[0]))> {
  vector<decltype(fun(u[0]))> ret;
  ret.reserve(u.size());
  for (const U& elem : u)
    ret.push_back(fun(elem));
  return ret;
}

template <typename U, typename Fun>
auto transform2(vector<U>& u, Fun fun) -> vector<decltype(fun(u[0]))> {
  vector<decltype(fun(u[0]))> ret;
  ret.reserve(u.size());
  for (U& elem : u)
    ret.push_back(fun(elem));
  return ret;
}

template <typename T, typename U, typename Fun>
auto transform2(const optional<U>& u, Fun fun) -> optional<decltype(fun(*u))> {
  if (u)
    return fun(*u);
  else
    return none;
}

template <typename T>
optional<T> ifTrue(bool cond, const T& t) {
  if (cond)
    return t;
  else
    return none;
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

template <typename Container, typename Elem>
vector<Elem> asVector(const Container& c) {
  vector<Elem> ret;
  for (const auto& elem : c)
    ret.push_back(elem);
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
  v.reserve(v.size() + w.size());
  for (T elem : w)
    v.push_back(elem);
}

template <typename T>
void append(vector<T>& v, vector<T>&& w) {
  v.reserve(v.size() + w.size());
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

template <typename T>
vector<T> concat(vector<T> v, const T& w) {
  v.push_back(w);
  return v;
}

template <typename T>
vector<T> concat(vector<T> v, const vector<T>& w, const vector<T>& u) {
  append(v, w);
  append(v, u);
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

template <typename T, typename V>
bool contains(const vector<T>& v, const optional<V>& elem) {
  return elem && contains(v, *elem);
}

template <typename T, typename V>
bool contains(const initializer_list<T>& v, const optional<V>& elem) {
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
  int sz = 0;
  for (const auto& v : vectors)
    sz += v.size();
  ret.reserve(sz);
  for (const auto& v : vectors)
    for (const T& t : v)
      ret.push_back(t);
  return ret;
}

inline std::ostream& operator <<(std::ostream& d, Vec2 msg) {
  return d << "(" << msg.x << "," << msg.y << ")";
}

template<class T>
std::ostream& operator<<(std::ostream& d, const Table<T>& container){
  for (int i : Range(container.height())) {
    d << i << ":";
    for (int j : Range(container.width()))
      d << container[j][i] << ",";
    d << '\n';
  }
  return d;
}

string addAParticle(const string& s);

string capitalFirst(string s);
string noCapitalFirst(string s);
vector<string> makeSentences(string s);
string makeSentence(string s);
string combineSentences(const vector<string>&);

string lowercase(string s);

string combine(const vector<string>& adj, bool commasOnly = false);

string getPlural(const string& singular, const string& plural, int num);
string getPlural(const string&, int num);
string getPluralText(const string&, int num);

template<class T>
string combine(const vector<T*>& v) {
  return combine(
      transform2(v, [](const T* e) -> string { return e->getName(); }));
}

template<class T>
string combine(const vector<T>& v) {
  return combine(
      transform2(v, [](const T& e) -> string { return e.name; }));
}

template<class T, class U>
vector<T> asVector(const U& u) {
  return vector<T>(u.begin(), u.end());
}

RICH_ENUM(Dir, N, S, E, W, NE, NW, SE, SW );

class DirSet {
  public:
  DirSet();
  DirSet(const vector<Dir>&);
  DirSet(const initializer_list<Dir>&);
  DirSet(bool n, bool s, bool e, bool w, bool ne, bool nw, bool se, bool sw);
  typedef unsigned char ContentType;
  DirSet(ContentType);
  static DirSet fullSet();
  static DirSet oneElement(Dir);

  bool contains(DirSet) const;
 // static DirSet getDirSet(const EnumSet<Dir>&);
  void insert(Dir);
  bool has(Dir) const;
  DirSet intersection(DirSet) const;
  DirSet complement() const;

  operator size_t() const {
    return content;
  }

  class Iter {
    public:
    Iter(const DirSet&, Dir);

    void goForward();
    Dir operator* () const;
    bool operator != (const Iter& other) const;
    const Iter& operator++ ();

    private:
    const DirSet& set;
    Dir ind;
  };

  Iter begin() const {
    return Iter(*this, Dir(0));
  }

  Iter end() const {
    return Iter(*this, Dir(8));
  }

  private:
  ContentType content = 0;
};

template<class T>
class EnumSet {
  public:
  EnumSet() {}

  EnumSet(initializer_list<T> il) {
    for (auto elem : il)
      insert(elem);
  }

  EnumSet(initializer_list<char> il) {
    CHECK(il.size() == EnumInfo<T>::size);
    int cnt = 0;
    for (int i : il) {
      if (i)
        insert(T(cnt));
      ++cnt;
    }
  }

  bool contains(T elem) const {
    return elems.test(size_t(elem));
  }

  void insert(T elem) {
    elems.set(size_t(elem));
  }

  void toggle(T elem) {
    elems.flip(size_t(elem));
  }

  void erase(T elem) {
    elems.reset(size_t(elem));
  }

  void set(T elem, bool state) {
    elems.set(size_t(elem), state);
  }

  void clear() {
    elems.reset();
  }

  bool operator == (const EnumSet& o) const {
    return elems == o.elems;
  }

  bool operator != (const EnumSet& o) const {
    return elems != o.elems;
  }

  typedef std::bitset<EnumInfo<T>::size> Bitset;

  EnumSet sum(const EnumSet& other) const {
    EnumSet<T> ret(other);
    ret.elems |= elems;
    return ret;
  }

  EnumSet intersection(const EnumSet& other) const {
    EnumSet<T> ret(other);
    ret.elems &= elems;
    return ret;
  }

  static EnumSet<T> fullSet() {
    EnumSet<T> ret;
    ret.elems.flip();
    return ret;
  }

  class Iter {
    public:
    Iter(const EnumSet& s, int num) : set(s), ind(num) {
      goForward();
    }

    void goForward() {
      while (ind < EnumInfo<T>::size && !set.contains(T(ind)))
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
    return Iter(*this, EnumInfo<T>::size);
  }

  int getHash() const {
    return hash<Bitset>()(elems);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    vector<T> SERIAL(tmp);
    for (T elem : *this)
      tmp.push_back(elem);
    ar & SVAR(tmp);
    for (T elem : tmp) {
      CHECK(int(elem) >= 0 && int(elem) < EnumInfo<T>::size);
      insert(elem);
    }
  }

  private:
  Bitset elems;
};

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

  const V& get(const U& u) const {
    CHECK(m1.count(u));
    return m1.at(u);
  }

  const U& get(const V& v) const {
    CHECK(m2.count(v));
    return m2.at(v);
  }

  const pair<U, V> getFront1() {
    return *m1.begin();
  }

  const pair<V, U> getFront2() {
    return *m2.begin();
  }

  int getSize() const {
    return m1.size();
  }

  bool isEmpty() const {
    return m1.empty();
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(m1) & SVAR(m2);
  }

  private:
  map<U, V> SERIAL(m1);
  map<V, U> SERIAL(m2);
};

template <class T>
class HeapAllocated {
  public:
  template <typename... Args>
  HeapAllocated(Args... a) : elem(new T(a...)) {}

  HeapAllocated(T&& o) : elem(new T(std::move(o))) {}

  HeapAllocated(const HeapAllocated& o) : elem(new T(*o)) {}

  T* operator -> () {
    return elem.get();
  }

  const T* operator -> () const {
    return elem.get();
  }

  T& operator * () {
    return *elem.get();
  }

  const T& operator * () const {
    return *elem.get();
  }

  const T* get() const {
    return elem.get();
  }

  T* get() {
    return elem.get();
  }

  void reset(T&& t) {
    elem.reset(new T(std::move(t)));
  }

  /*HeapAllocated& operator = (const T& t) {
    *elem.get() = t;
    return *this;
  }*/

  HeapAllocated& operator = (const HeapAllocated& t) {
    *elem.get() = *t;
    return *this;
  }

  SERIALIZE_ALL(elem);

  private:
  unique_ptr<T> SERIAL(elem);
};

class Semaphore {
  public:
  Semaphore(int val = 0);

  void p();
  void v();
  int get();

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

  bool isEmpty() {
    std::unique_lock<std::mutex> lock(mut);
    return q.empty();
  }

  optional<T> popAsync() {
    std::unique_lock<std::mutex> lock(mut);
    if (q.empty())
      return none;
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
  void setDone();
  void finishAndWait();
  ~AsyncLoop();

  private:
  std::atomic<bool> done;
  thread t;
};

template <typename T, typename... Args>
function<void(Args...)> bindMethod(void (T::*ptr) (Args...), T* t) {
  return [=](Args... a) { (t->*ptr)(a...);};
}

template <typename Ret, typename T, typename... Args>
function<Ret(Args...)> bindMethod(Ret (T::*ptr) (Args...), T* t) {
  return [=](Args... a) { return (t->*ptr)(a...);};
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

class DisjointSets {
  public:
  DisjointSets(int size);
  void join(int, int);
  bool same(int, int);
  bool same(const vector<int>&);

  private:
  int getSet(int);
  vector<int> father;
  vector<int> size;
};

template <typename ReturnType, typename... Lambdas>
struct LambdaVisitor;

template <typename ReturnType, typename Lambda1, typename... Lambdas>
struct LambdaVisitor< ReturnType, Lambda1 , Lambdas...> : public Lambda1, public LambdaVisitor<ReturnType, Lambdas...> {
    using Lambda1::operator();
    using LambdaVisitor<ReturnType, Lambdas...>::operator();
    LambdaVisitor(Lambda1 l1, Lambdas... lambdas)
      : Lambda1(l1), LambdaVisitor<ReturnType, Lambdas...> (lambdas...)
    {}
};

template <typename ReturnType, typename Lambda1>
struct LambdaVisitor<ReturnType, Lambda1> : public boost::static_visitor<ReturnType>, public Lambda1 {
    using Lambda1::operator();
    LambdaVisitor(Lambda1 l1)
      : boost::static_visitor<ReturnType>(), Lambda1(l1)
    {}
};

template <typename ReturnType, typename... Lambdas>
LambdaVisitor<ReturnType, Lambdas...> makeVisitor(Lambdas... lambdas) {
  return LambdaVisitor<ReturnType, Lambdas...>(lambdas...);
}

struct DefaultOperator {
  template <typename... T>
  void operator() (const T&...) const {}
};

template <typename... Lambdas>
LambdaVisitor<void, DefaultOperator, Lambdas...> makeDefaultVisitor(Lambdas... lambdas) {
  return LambdaVisitor<void, DefaultOperator, Lambdas...>(DefaultOperator {}, lambdas...);
}

template <typename Visitor, typename Visitable>
typename Visitor::result_type apply_visitor(Visitable& visitable, const Visitor& visitor) {
  return visitable.apply_visitor(visitor);
}

template <typename T, typename ...Args>
optional<T&> getType(variant<Args...>& v) {
  if (auto ptr = boost::get<T>(&v))
    return *ptr;
  else
    return none;
}

template <typename Key, typename Map>
optional<const typename Map::mapped_type&> getReferenceMaybe(const Map& m, const Key& key) {
  auto it = m.find(key);
  if (it != m.end())
    return it->second;
  else
    return none;
}

template <typename Key, typename Map>
optional<typename Map::mapped_type&> getReferenceMaybe(Map& m, const Key& key) {
  auto it = m.find(key);
  if (it != m.end())
    return it->second;
  else
    return none;
}

template <typename Key, typename Map>
optional<typename Map::mapped_type> getValueMaybe(const Map& m, const Key& key) {
  auto it = m.find(key);
  if (it != m.end())
    return it->second;
  else
    return none;
}

extern int getSize(const string&);
extern const char* getString(const string&);
