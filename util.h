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

#include "stdafx.h"
#include "my_containers.h"
#include "debug.h"
#include "enums.h"
#include "hashing.h"
#include "container_helpers.h"
#include "serialization.h"
#include "owner_pointer.h"
#include "hashing.h"
#include "extern/variant.h"
#include "extern/optional.h"
#include "profiler.h"
#include "mem_usage_counter.h"

template <class T>
string toString(const T& t) {
  PROFILE;
  stringstream ss;
  ss << t;
  return ss.str();
}

string toStringWithSign(int);
string toPercentage(double);

template <class T>
string toString(const optional<T>& t) {
  if (t)
    return toString(*t);
  else
    return "(unknown)";
}

class ParsingException {};

class GameExitException {};

template <class T>
T fromString(const string& s) {
  PROFILE;
  std::stringstream ss(s);
  T t;
  ss >> t;
  if (!ss)
    throw ParsingException();
  return t;
}

template <class T>
optional<T> fromStringSafe(const string& s);


string toStringRounded(double value, double precision);

template <typename T, typename Fun>
T lambdaConstruct(Fun fun) {
  T ret {};
  fun(ret);
  return ret;
}

#define CONSTRUCT(Class, Code) lambdaConstruct<Class>([&] (Class& c) { Code })
#define CONSTRUCT_GLOBAL(Class, Code) lambdaConstruct<Class>([] (Class& c) { Code })

#define LIST(...) {__VA_ARGS__}

#define REQUIRE(...) \
    typename int_<decltype(__VA_ARGS__)>::type = 0
#define TVALUE(NAME) std::declval<NAME>()

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

void trim(string& s);
string toUpper(const string& s);
string toLower(const string& s);

string stripFilename(string);

bool endsWith(const string&, const string& suffix);
bool startsWith(const string&, const string& prefix);
bool contains(const string&, const string& substring, unsigned long index);


vector<string> split(const string& s, const std::initializer_list<char>& delim);
vector<string> splitIncludeDelim(const string& s, const std::initializer_list<char>& delim);
string combineWithOr(const vector<string>&);



class Rectangle;
class RandomGen;

string getCardinalName(Dir d);

struct SVec2 {
  short SERIAL(x);
  short SERIAL(y);
  SERIALIZE_ALL(x, y)
};

class Vec2 {
  public:
  int SERIAL(x); // HASH(x)
  int SERIAL(y); // HASH(y)
  Vec2() : x(0), y(0) {}
  Vec2(int x, int y);
  Vec2(Dir);
  Vec2(SVec2);
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
  int dist4(Vec2) const;
  double distD(Vec2) const;
  double lengthD() const;
  Vec2 shorten() const;
  pair<Vec2, Vec2> approxL1() const;
  Vec2 getBearing() const;
  bool isCardinal4() const;
  bool isCardinal8() const;
  Dir getCardinalDir() const;
  static Vec2 getCenterOfWeight(const vector<Vec2>&);

  static const vector<Vec2>& directions8();
  vector<Vec2> neighbors8() const;
  static const vector<Vec2>& directions4();
  vector<Vec2> neighbors4() const;
  static vector<Vec2> directions8(RandomGen&);
  vector<Vec2> neighbors8(RandomGen&) const;
  static vector<Vec2> directions4(RandomGen&);
  vector<Vec2> neighbors4(RandomGen&) const;
  static vector<Vec2> corners();
  static vector<set<Vec2>> calculateLayers(set<Vec2>);

  typedef function<Vec2(Vec2)> LinearMap;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  HASH_ALL(x, y)
};

extern string toString(const Vec2&);

class PrettyInputArchive;

struct Vec2Range {
  Vec2 SERIAL(begin);
  Vec2 SERIAL(end);
  Vec2 get(RandomGen&) const;
  template <class Archive>
  void serialize(Archive&, const unsigned int version);
  void serialize(PrettyInputArchive&, const unsigned int version);
};

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
  int clamp(int) const;
  bool contains(int) const;
  bool intersects(Range) const;
  Range intersection(Range) const;

  bool operator == (const Range&) const;
  Range operator + (int) const;
  Range operator - (int) const;

  class Iter {
    public:
    Iter(int ind, int min, int max, int increment);

    int operator* () const;
    bool operator != (const Iter& other) const;

    const Iter& operator++ ();

    private:
    int ind;
    //int min;
    //int max;
    int increment;
  };

  Iter begin();
  Iter end();

  SERIALIZATION_DECL(Range)
  HASH_ALL(start, finish, increment)

  private:
  Range(int start, int end, int increment);
  int SERIAL(start) = 0; // HASH(start)
  int SERIAL(finish) = 0; // HASH(finish)
  int SERIAL(increment) = 1; // HASH(increment)
};

extern string toString(const Range&);

class Rectangle {
  public:
  friend class Vec2;
  template<typename T>
  friend class Table;
  Rectangle(int width, int height);
  explicit Rectangle(Vec2 dim);
  Rectangle(int px, int py, int kx, int ky);
  Rectangle(Vec2 p, Vec2 k);
  Rectangle(Range xRange, Range yRange);
  static Rectangle boundingBox(const vector<Vec2>& v);
  static Rectangle centered(Vec2 center, int radius);
  static Rectangle centered(int radius);

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
  // can be negative if rectangles intersect
  int getDistance(const Rectangle& other) const;

  Rectangle minusMargin(int margin) const;
  Rectangle translate(Vec2 v) const;
  Rectangle apply(Vec2::LinearMap) const;

  Vec2 random(RandomGen&) const;
  Vec2 middle() const;
  bool empty() const;

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
    int /*px, */py, /*kx, */ky;
  };

  Iter begin() const;
  Iter end() const;

  SERIALIZATION_DECL(Rectangle);

  private:
  int SERIAL(px) = 0;
  int SERIAL(py) = 0;
  int SERIAL(kx) = 0;
  int SERIAL(ky) = 0;
  int SERIAL(w) = 0;
  int SERIAL(h) = 0;
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
};

#define RICH_ENUM2(Type, Name, ...) \
enum class Name : Type { __VA_ARGS__ };\
template<> \
class EnumInfo<Name> { \
  public:\
  static const char* getName() {\
    return #Name;\
  }\
  static string getString(Name e) {\
    static vector<string> names = split(#__VA_ARGS__, {' ', ','}).filter([](const string& s){ return !s.empty(); });\
    return names[int(e)];\
  }\
  enum Tmp { __VA_ARGS__, size};\
  static Name fromStringWithException(const string& s) {\
    for (int i : Range(size)) \
      if (getString(Name(i)) == s) \
        return Name(i); \
    throw ParsingException();\
  } \
  static Name fromString(const string& s) { \
    for (int i : Range(size)) \
      if (getString(Name(i)) == s) \
        return Name(i); \
    FATAL << "Error parsing " << #Name << " from " << s; \
    return Name(0);\
  }\
  static optional<Name> fromStringSafe(const string& s) {\
    for (int i : Range(size)) \
      if (getString(Name(i)) == s) \
        return Name(i); \
    return none;\
  }\
}
#define RICH_ENUM(Name, ...) RICH_ENUM2(int, Name, __VA_ARGS__)

template <class T>
class EnumAll {
  public:
  EnumAll() : b(0, 1), e(EnumInfo<T>::size, 1) {}
  struct Reverse {};
  EnumAll(Reverse) : b(EnumInfo<T>::size - 1, -1), e(-1, -1) {}

  class Iter {
    public:
    Iter(int num, int d) : ind(num), dir(d) {
    }

    T operator* () const {
      return T(ind);
    }

    bool operator != (const Iter& other) const {
      return ind != other.ind;
    }

    const Iter& operator++ () {
      ind += dir;
      return *this;
    }

    private:
    int ind;
    int dir;
  };

  Iter begin() {
    return b;
  }

  Iter end() {
    return e;
  }

  private:
  Iter b;
  Iter e;
};

#define ENUM_STRING(e) (EnumInfo<std::remove_cv<std::remove_reference<decltype(e)>::type>::type>::getString(e))
#define ENUM_CSTRING(e) (EnumInfo<std::remove_cv<std::remove_reference<decltype(e)>::type>::type>::getString(e).c_str())

#define ENUM_ALL(X) EnumAll<X>()
#define ENUM_ALL_REVERSE(X) EnumAll<X>(EnumAll<X>::Reverse{})

template<class T, class U>
class EnumMap {
  public:
  EnumMap() {
    clear();
  }

  EnumMap(const EnumMap& o) = default;
  EnumMap(EnumMap&& o) = default;

  template <typename Fun>
  explicit EnumMap(Fun f) {
    for (T t : EnumAll<T>())
      (*this)[t] = f(t);
  }

  bool operator == (const EnumMap<T, U>& other) const {
    return elems == other.elems;
  }

  bool operator != (const EnumMap<T, U>& other) const {
    return elems != other.elems;
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

  EnumMap& operator = (EnumMap&& other) {
    elems = std::move(other.elems);
    return *this;
  }

  const U& operator[](T elem) const {
    CHECK(int(elem) >= 0 && int(elem) < EnumInfo<T>::size) << int(elem);
    return elems[int(elem)];
  }

  U& operator[](T elem) {
    CHECK(int(elem) >= 0 && int(elem) < EnumInfo<T>::size) << int(elem);
    return elems[int(elem)];
  }

  size_t getHash() const {
    return combineHashIter(elems.begin(), elems.end());
  }

  //private:
  std::array<U, EnumInfo<T>::size> elems;
};

template <class Archive, typename Enum, typename U>
void serialize(Archive& ar1, EnumMap<Enum, U>& m) {
  int size = EnumInfo<Enum>::size;
  ar1(size);
  if (size > EnumInfo<Enum>::size)
    throw ::cereal::Exception("EnumMap larger than legal enum range");
  for (int i : Range(size))
    ar1(m[Enum(i)]);
}

#ifdef MEM_USAGE_TEST
template <class T, class U> inline
void serialize(MemUsageArchive & ar1, EnumMap<T, U> & bd) {
  ar1(bd.elems);
}
#endif

template<class T>
class EnumSet {
  public:
  EnumSet() {}

  template <typename Fun>
  explicit EnumSet(Fun f) {
    for (T t : EnumAll<T>())
      if (f(t))
        insert(t);
  }

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

  bool isEmpty() const {
    return elems.none();
  }

  int getSize() const {
    return elems.count();
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

  EnumSet ex_or(const EnumSet& other) const {
    EnumSet<T> ret(other);
    ret.elems ^= elems;
    return ret;
  }

  void sumWith(const EnumSet& other) {
    elems |= other.elems;
  }

  void intersectWith(const EnumSet& other) {
    elems &= other.elems;
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
    ar(tmp);
    for (T elem : tmp) {
      if (int(elem) < 0 || int(elem) >= EnumInfo<T>::size)
        throw ::cereal::Exception("EnumSet element outside of legal enum range");
      insert(elem);
    }
  }

#ifdef MEM_USAGE_TEST
  void serialize(MemUsageArchive& ar, const unsigned int version) {
  }
#endif

  private:
  Bitset elems;
};

std::string operator "" _s(const char* str, size_t);

class RandomGen {
  public:
  RandomGen();
  RandomGen(RandomGen&) = delete;
  void init(int seed);
  int get(int max);
  long long getLL();
  int get(int min, int max);
  int get(Range);
  int get(const vector<double>& weights);
  double getDouble();
  double getDouble(double a, double b);
  pair<float, float> getFloat2Fast();
  float getFloat(float a, float b);
  float getFloatFast(float a, float b);
  bool roll(int chance);
  bool chance(double chance);
  bool chance(float chance);
  template <typename T>
  const T& choose(const vector<T>& v, const vector<double>& p) {
    CHECK(v.size() == p.size());
    return v[get(p)];
  }

  template <typename T>
  const T& choose(const vector<T>& v) {
    vector<double> pi(v.size(), 1);
    return choose(v, pi);
  }

  template <typename T>
  T choose(vector<T>&& v, const vector<double>& p) {
    CHECK(v.size() == p.size());
    return std::move(v[get(p)]);
  }

  template <typename T>
  T choose(vector<T>&& v) {
    vector<double> pi(v.size(), 1);
    return choose(std::move(v), pi);
  }

  template <typename T>
  T choose(const set<T>& vi) {
    vector<T> v(vi.size());
    std::copy(vi.begin(), vi.end(), v.begin());
    return choose(v);
  }

  template <typename T, typename Hash>
  T choose(const unordered_set<T, Hash>& vi) {
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

  template <typename T>
  T choose(const EnumSet<T>& set) {
    auto it = set.begin();
    for (int i : Range(get(set.getSize())))
      ++it;
    return *it;
  }

  template <typename T>
  T choose(vector<pair<int, T>> vi) {
    vector<T> v;
    vector<double> p;
    for (auto elem : vi) {
      v.push_back(elem.second);
      p.push_back(elem.first);
    }
    return choose(v, p);
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

  template <typename T>
  vector<T> permutation() {
    vector<T> ret;
    ret.reserve(EnumInfo<T>::size);
    for (auto e : ENUM_ALL(T))
      ret.push_back(e);
    shuffle(ret.begin(), ret.end());
    return ret;
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

  template <typename T, typename Hash>
  vector<T> permutation(const unordered_set<T, Hash>& vi) {
    vector<T> v(vi.size());
    std::copy(vi.begin(), vi.end(), v.begin());
    return permutation(v);
  }

  template <typename T>
  vector<T> permutation(initializer_list<T> vi) {
    vector<T> v(vi);
    std::shuffle(v.begin(), v.end(), generator);
    return v;
  }

  vector<int> permutation(Range r) {
    vector<int> v;
    for (int i : r)
      v.push_back(i);
    std::shuffle(v.begin(), v.end(), generator);
    return v;
  }

  template <typename T>
  vector<T> chooseN(int n, vector<T> v) {
    CHECK(n <= v.size());
    std::shuffle(v.begin(), v.end(), generator);
    return v.getPrefix(n);
  }

  template <typename T>
  vector<T> chooseN(int n, initializer_list<T> v) {
    return chooseN(n, vector<T>(v));
  }

  template <typename T, typename... Args>
  T&& choose(T&& first, T&& second, Args&&... rest) {
    return chooseImpl(std::forward<T>(first), 2, std::forward<T>(second), std::forward<Args>(rest)...);
  }

  private:
  std::mt19937 generator;
  std::uniform_real_distribution<double> defaultDist;

  template <typename T>
  T&& chooseImpl(T&& cur, int total) {
    return std::forward<T>(cur);
  }

  template <typename T, typename... Args>
  T&& chooseImpl(T&& chosen, int total,  T&& next, Args&&... rest) {
    return chooseImpl(roll(total) ? std::forward<T>(next) : std::forward<T>(chosen), total + 1, std::forward<Args>(rest)...);
  }
};

extern RandomGen Random;

inline std::ostream& operator <<(std::ostream& d, Rectangle rect) {
  return d << "(" << rect.left() << "," << rect.top() << ") (" << rect.right() << "," << rect.bottom() << ")";
}


template <class T>
class Table {
  public:
  Table(Table&& t) noexcept = default;

  Table(const Table& t) : Table(t.bounds) {
    for (int i : Range(bounds.w * bounds.h))
      mem[i] = t.mem[i];
  }

  Table(int x, int y, int w, int h) : Table(Rectangle(x, y, x + w, y + h)) {
  }

  Table(const Rectangle& rect) : bounds(rect), mem(new T[rect.w * rect.h]){
    for (int i : Range(bounds.w * bounds.h))
      mem[i] = T();
  }

  Table(const Rectangle& rect, const T& value) : Table(rect) {
    for (int i : Range(bounds.w * bounds.h))
      mem[i] = value;
  }

  Table(int w, int h) : Table(0, 0, w, h) {
  }

  Table(Vec2 size) : Table(size.x, size.y) {
  }

  Table(int x, int y, int width, int height, const T& value) : Table(Rectangle(x, y, x + width, y + height), value) {
  }

  Table(int width, int height, const T& value) : Table(0, 0, width, height, value) {
  }

  Table(Vec2 size, const T& value) : Table(size.x, size.y, value) {
  }

  const Rectangle& getBounds() const {
    return bounds;
  }

  Table& operator = (Table&& other) noexcept = default;
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
    RowAccess(T* m, int p, int w) : px(p), width(w), mem(m) {}
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
    CHECK(vAbs.inRectangle(bounds)) <<
        "Table index out of bounds " << bounds << " " << vAbs;
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
  }

  const T& operator[](const Vec2& vAbs) const {
    CHECK(vAbs.inRectangle(bounds)) <<
        "Table index out of bounds " << bounds << " " << vAbs;
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
  }

  template <class Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << bounds;
    for (Vec2 vAbs : bounds)
      ar << mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
  }

#ifdef MEM_USAGE_TEST
  void save(MemUsageArchive& ar, const unsigned int version) const {
    ar.addUsage(bounds.width() * bounds.height() * sizeof(T));
    for (Vec2 v : bounds)
      ar << (*this)[v];
  }
#endif

  template <class Archive>
  void load(Archive& ar, const unsigned int version) {
    ar >> bounds;
    mem.reset(new T[bounds.width() * bounds.height()]);
    for (Vec2 vAbs : bounds)
      ar >> mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
  }

  SERIALIZATION_CONSTRUCTOR(Table)

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
  for (auto& elem : m)
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

template <typename T, typename V, typename Hash>
vector<T> getKeys(const unordered_map<T, V, Hash>& m) {
  vector<T> ret;
  for (auto& elem : m)
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

template <typename Fun>
string transform2(const string& u, Fun fun) {
  string ret(u.size(), ' ');
  transform(u.begin(), u.end(), ret.begin(), fun);
  return ret;
}

template <typename U, typename Fun>
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

template <typename Container, typename Elem>
vector<Elem> asVector(const Container& c) {
  vector<Elem> ret;
  for (const auto& elem : c)
    ret.push_back(elem);
  return ret;
}

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

template <typename T>
struct IterateVectors {
  IterateVectors(const vector<T>& v1, const vector<T>& v2) : v1(v1), v2(v2) {}
  IterateVectors(vector<T>&& v1, const vector<T>& v2) = delete;
  IterateVectors(const vector<T>& v1, vector<T>&& v2) = delete;
  IterateVectors(vector<T>&& v1, vector<T>&& v2) = delete;

  struct Iter {
    using SubIt = typename vector<T>::const_iterator;
    Iter(SubIt cur1, SubIt v1End1, SubIt v2Begin1, bool first1)
        : cur(cur1), v1End(v1End1), v2Begin(v2Begin1), firstRange(first1) {
      if (firstRange && cur == v1End) {
        cur = v2Begin;
        firstRange = false;
      }
    }
    SubIt cur, v1End, v2Begin;
    bool firstRange = true;
    const T& operator* () const {
      return *cur;
    }
    bool operator != (const Iter& other) const {
      return cur != other.cur;
    }
    const Iter& operator++ () {
      ++cur;
      if (firstRange && cur == v1End) {
        cur = v2Begin;
        firstRange = false;
      }
      return *this;
    }
  };
  Iter begin() {
    return Iter(v1.begin(), v1.end(), v2.begin(), true);
  }
  Iter end() {
    return Iter(v2.end(), v1.end(), v2.begin(), false);
  }
  const vector<T>& v1;
  const vector<T>& v2;
};

template <typename T, typename U>
auto iterateVectors(T&& v1, U&& v2) {
  return IterateVectors<typename std::remove_reference<T>::type::value_type>(std::forward<T>(v1), std::forward<U>(v2));
}

class OnExit {
  public:
  OnExit(function<void()> f) : fun(f) {}

  ~OnExit() { fun(); }

  private:
  function<void()> fun;
};

template <typename T, typename V>
bool contains(const initializer_list<T>& v, const optional<V>& elem) {
  return std::find(v.begin(), v.end(), elem) != v.end();
}

template <typename T, typename V>
bool contains(const deque<T>& v, const V& elem) {
  return std::find(v.begin(), v.end(), elem) != v.end();
}

template <class T>
class MustInitialize {
  public:
  MustInitialize(const MustInitialize& o) noexcept : elem(o.elem) {
    CHECK(!!elem) << "Element not initialized";
  }

  MustInitialize(MustInitialize&& o) noexcept = default;

  MustInitialize() {}

  MustInitialize& operator = (MustInitialize&& t) = default;

  MustInitialize& operator = (const T& t) {
    elem = t;
    return *this;
  }

  MustInitialize& operator = (T&& t) noexcept {
    elem = std::move(t);
    return *this;
  }

  T& operator += (const T& t) {
    CHECK(!!elem) << "Element not initialized";
    return *elem += t;
  }

  T& operator -= (const T& t) {
    CHECK(!!elem) << "Element not initialized";
    return *elem -= t;
  }

  T* operator -> () {
    CHECK(!!elem) << "Element not initialized";
    return &*elem;
  }

  const T* operator -> () const {
    CHECK(!!elem) << "Element not initialized";
    return &*elem;
  }

  bool operator == (const T& t) const {
    CHECK(!!elem) << "Element not initialized";
    return *elem == t;
  }

  bool operator != (const T& t) const {
    CHECK(!!elem) << "Element not initialized";
    return *elem != t;
  }

  T& operator * () {
    CHECK(!!elem) << "Element not initialized";
    return *elem;
  }

  const T& operator * () const {
    CHECK(!!elem) << "Element not initialized";
    return *elem;
  }

  SERIALIZE_ALL(elem)

  private:
  optional<T> SERIAL(elem);
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

string capitalFirst(string s);
vector<string> makeSentences(string s);
string makeSentence(string s);
string combineSentences(const vector<string>&);

string lowercase(string s);

string combine(const vector<string>& adj, bool commasOnly = false);
string combine(const vector<string>& adj, const string& delimiter);

template<class T, class U>
vector<T> asVector(const U& u) {
  return vector<T>(u.begin(), u.end());
}

RICH_ENUM(Dir, N, S, E, W, NE, NW, SE, SW );

extern Dir rotate(Dir);

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

  operator ContentType() const {
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

template <typename Map>
optional<typename Map::mapped_type> getValueMaybe(const Map& m, const typename Map::key_type& key) {
  auto it = m.find(key);
  if (it != m.end())
    return it->second;
  else
    return none;
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
    if (auto other = getReferenceMaybe(m1, u)) {
      m2.erase(*other);
      m1.erase(u);
    }
  }

  void erase(const V& v) {
    if (auto other = getReferenceMaybe(m2, v)) {
      m1.erase(*other);
      m2.erase(v);
    }
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
    ar(m1, m2);
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

  HeapAllocated(const HeapAllocated& o) noexcept : elem(new T(*o)) {}
  HeapAllocated(HeapAllocated&& o) noexcept : elem(std::move(o.elem)) {}

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

  HeapAllocated& operator = (const HeapAllocated& t) {
    *elem.get() = *t;
    return *this;
  }

  HeapAllocated& operator = (HeapAllocated&& t) noexcept {
    elem = std::move(t.elem);
    return *this;
  }

  bool operator == (const HeapAllocated& o) const {
    return *elem == *o.elem;
  }

  template <class Archive>
  void serialize(Archive& ar1) {
    if (!elem && Archive::is_loading::value) {
      elem = make_unique<T>();
    }
    CHECK(!!elem);
    ar1(*elem);
  }

#ifdef MEM_USAGE_TEST
  void serialize(MemUsageArchive& ar1) {
    ar1.addUsage(sizeof(T));
    CHECK(!!elem);
    ar1(*elem);
  }
#endif

  protected:
  unique_ptr<T> elem;
};

template <class T>
class heap_optional {
  public:
  heap_optional() {}

  heap_optional(T&& o) noexcept : elem(new T(std::move(o))) {}
  heap_optional(const T& o) noexcept : elem(new T(std::move(o))) {}

  heap_optional(const heap_optional& o) noexcept : elem(o.elem ? new T(*o.elem) : nullptr) {}
  heap_optional(heap_optional&& o) noexcept : elem(std::move(o.elem)) {}

  T* operator -> () {
    CHECK(!!elem);
    return elem.get();
  }

  const T* operator -> () const {
    CHECK(!!elem);
    return elem.get();
  }

  T& operator * () {
    CHECK(!!elem);
    return *elem;
  }

  const T& operator * () const {
    CHECK(!!elem);
    return *elem;
  }

  explicit operator bool () const {
    return !!elem;
  }

  void reset(T&& t) {
    elem.reset(new T(std::move(t)));
  }

  void clear() {
    elem.reset();
  }

  heap_optional& operator = (const T& t) {
    elem = make_unique<T>(t);
    return *this;
  }

  heap_optional& operator = (T&& t) noexcept {
    elem = make_unique<T>(std::move(t));
    return *this;
  }

  heap_optional& operator = (const heap_optional& t) noexcept {
    if (t.elem)
      elem = make_unique<T>(*t.elem);
    else
      clear();
    return *this;
  }

  heap_optional& operator = (heap_optional&& t) noexcept {
    elem = std::move(t.elem);
    return *this;
  }

  heap_optional& operator = (none_t) noexcept {
    clear();
    return *this;
  }

  int getHash() const {
    if (!elem)
      return 0;
    return combineHash(*elem);
  }

  SERIALIZE_ALL(elem)

  private:
  unique_ptr<T> SERIAL(elem);
};

template <typename T>
auto to_heap_optional(const optional<T>& o) {
  if (o)
    return heap_optional<T>(*o);
  else
    return heap_optional<T>();
}

template <typename T>
auto to_heap_optional(optional<T>&& o) {
  if (o)
    return heap_optional<T>(*std::move(o));
  else
    return heap_optional<T>();
}

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

thread makeThread(function<void()> fun);

class scoped_thread {
  public:
  scoped_thread(thread t_): t(std::move(t_)) {
    CHECK(t.joinable());
  }
  ~scoped_thread(){
    t.join();
  }
  scoped_thread(scoped_thread&&) = default;
  scoped_thread& operator = (scoped_thread&&) = default;
  scoped_thread(const scoped_thread&) = delete;
  scoped_thread& operator = (scoped_thread const &) = delete;
  private:
  std::thread t;
};

scoped_thread makeScopedThread(function<void()> fun);

void openUrl(const string& url);

template <typename T, typename... Args>
auto bindMethod(void (T::*ptr) (Args...), T* t) {
  return [=](Args... a) { (t->*ptr)(a...);};
}

template <typename Ret, typename T, typename... Args>
auto bindMethod(Ret (T::*ptr) (Args...), T* t) {
  return [=](Args... a) { return (t->*ptr)(a...);};
}

template <typename... Args>
auto bindFunction(void (*ptr) (Args...)) {
  return [=](Args... a) { (*ptr)(a...);};
}

template <typename Ret, typename... Args>
auto bindFunction(Ret (*ptr) (Args...)) {
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

extern int getSize(const string&);
extern const char* getString(const string&);


template <const char* getNames(bool), typename... Types>
class NamedVariant : public variant<Types...> {
  public:
  using variant<Types...>::variant;
  const char* getName() const {
    return getName(this->index());
  }
  static const char* getVariantName() {
    return getNames(false);
  }
  static const char* getName(int num) {
    static const auto names = split(getNames(true), {' ', ','}).filter([](const string& s){ return !s.empty(); });
    return names[num].c_str();
  }
};

#define MAKE_VARIANT(NAME, ...)\
constexpr static inline const char* get##NAME##Names(bool b) { if (b) return #__VA_ARGS__; else return #NAME;}\
using NAME = NamedVariant<get##NAME##Names, __VA_ARGS__>;\
using NAME##_impl = variant<__VA_ARGS__>

#define MAKE_VARIANT2(NAME, ...)\
constexpr inline const char* get##NAME##Names(bool b) { if (b) return #__VA_ARGS__; else return #NAME;}\
class NAME : public NamedVariant<get##NAME##Names, __VA_ARGS__> { \
  using NamedVariant::NamedVariant;\
};\
using NAME##_impl = variant<__VA_ARGS__>


#define COMPARE_ALL(...) \
auto getElems() const { \
  return std::forward_as_tuple(__VA_ARGS__); \
} \
auto getElems() { \
  return std::forward_as_tuple(__VA_ARGS__); \
} \
template <typename T> \
bool operator == (const T& o) const { \
  return o.getElems() == getElems(); \
} \
template <typename T> \
bool operator != (const T& o) const { \
  return o.getElems() != getElems(); \
} \
template <class Archive> \
void serialize(Archive& ar1, const unsigned int) { \
  auto elems = getElems();\
  ar1(elems); \
}


template <typename Phantom>
struct EmptyStruct {
  // Work around a check_serial.sh bug
  #define Nothing
  // SERIAL(Nothing)
  COMPARE_ALL(Nothing)
  #undef Nothing
};

#define EMPTY_STRUCT(Name) \
struct _Tag123##Name {};\
using Name = EmptyStruct<_Tag123##Name>

template <class T> constexpr bool isOneOf(const T& value) {
  return false;
}
template <class T, class Arg1, class... Args>
constexpr bool isOneOf(const T& value, const Arg1& arg1, const Args&... args) {
  return value == arg1 || isOneOf(value, args...);
}
template <class T, int size> constexpr int arraySize(T (&)[size]) {
  return size;
}

#define STRUCT_DECLARATIONS(TYPE) \
  ~TYPE(); \
  TYPE(TYPE&&) noexcept;  \
  TYPE& operator = (TYPE&&); \
  TYPE(const TYPE&); \
  TYPE& operator = (const TYPE&);

#define STRUCT_IMPL(TYPE) \
  TYPE::~TYPE() {} \
  TYPE::TYPE(TYPE&&) noexcept = default; \
  TYPE::TYPE(const TYPE&) = default; \
  TYPE& TYPE::operator =(TYPE&&) = default; \
  TYPE& TYPE::operator = (const TYPE&) = default;
