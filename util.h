#ifndef _UTIL_H
#define _UTIL_H

#include <iostream>

#include "debug.h"
#include "enums.h"

template <class T>
string convertToString(const T& t);

template <class T>
T convertFromString(const string& s);

class Item;
typedef unique_ptr<Item> PItem;

class Creature;
typedef unique_ptr<Creature> PCreature;

class Effect;
typedef unique_ptr<Effect> PEffect;

class Square;
typedef unique_ptr<Square> PSquare;

class MonsterAI;
typedef unique_ptr<MonsterAI> PMonsterAI;

class Behaviour;
typedef unique_ptr<Behaviour> PBehaviour;

class Obstacle;
typedef unique_ptr<Obstacle> PObstacle;

class Task;
typedef unique_ptr<Task> PTask;

class Controller;
typedef unique_ptr<Controller> PController;

class Trigger;
typedef unique_ptr<Trigger> PTrigger;

void trim(string& s);
string toUpper(const string& s);
string toLower(const string& s);

vector<string> split(const string& s, char delim);

class Rectangle;


class Vec2 {
  public:
  int x, y;
  Vec2() : x(0), y(0) {}
  Vec2(int x, int y);
  bool inRectangle(int px, int py, int kx, int ky) const;
  bool inRectangle(const Rectangle&) const;
  bool operator == (const Vec2& v) const;
  bool operator != (const Vec2& v) const;
  Vec2 operator + (const Vec2& v) const;
  Vec2 operator * (int) const;
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
  double lengthD() const;
  Vec2 shorten() const;
  pair<Vec2, Vec2> approxL1() const;
  string getBearing() const;
  bool isCardinal4() const;
  Dir getCardinalDir() const;

  vector<Vec2> box(int radius, bool shuffle = false);
  static vector<Vec2> directions8(bool shuffle = false);
  vector<Vec2> neighbors8(bool shuffle = false) const;
  static vector<Vec2> directions4(bool shuffle = false);
  vector<Vec2> neighbors4(bool shuffle = false) const;
  static vector<Vec2> corners();
};

namespace std {

template <> struct hash<Vec2> {
  size_t operator()(const Vec2& obj) const {
    return hash<int>()(obj.x) * 10000 + hash<int>()(obj.y);
  }
};

template <class T> struct hash<set<T>> {
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

  Rectangle minusMargin(int margin) const;

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
  int getRandom(int max);
  int getRandom(int min, int max);
  double getDouble();
  double getDouble(double a, double b);
  bool roll(int chance);

  private:
  default_random_engine generator;
  std::uniform_real_distribution<double> defaultDist;
};

extern RandomGen Random;

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
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
    /*Vec2 v(vAbs.x - px, vAbs.y - py);
    CHECK(v.x >= 0 && v.y >= 0 && v.x < width && v.y < height) <<
        "Table index out of bounds " << getBounds() << " " << vAbs;
    return mem[v.x * height + v.y];*/
  }

  const T& operator[](const Vec2& vAbs) const {
    return mem[(vAbs.x - bounds.px) * bounds.h + vAbs.y - bounds.py];
/*    Vec2 v(vAbs.x - px, vAbs.y - py);
    CHECK(v.x >= 0 && v.y >= 0 && v.x < width && v.y < height) <<
        "Table index out of bounds " << getBounds() << " " << vAbs;
    return mem[v.x * height + v.y];*/
  }

  private:
  Rectangle bounds;
  unique_ptr<T[]> mem;
};

template <typename T>
T chooseRandom(const vector<T>& vi, const vector<double>& pi, double r = -1) {
  vector<T> v(vi);
  vector<double> p(pi);
  CHECK(v.size() == p.size());
  double sum = 0;
  for (double elem : p)
    sum += elem;
  if (r == -1)
    r = Random.getDouble(0, sum);
  sum = 0;
  for (int i : All(p)) {
    sum += p[i];
    if (sum >= r)
      return v[i];
  }
  return v.back();
}


template <typename T>
T chooseRandom(const vector<T>& vi, double r = -1) {
  vector<T> v(vi);
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
T chooseRandom(initializer_list<pair<T, double>> vi, double r = -1) {
  return chooseRandom(vector<pair<T, double>>(vi), r);
}

template <typename T>
vector<T> randomPermutation(vector<T> v) {
  random_shuffle(v.begin(), v.end(), [](int a) { return Random.getRandom(a);});
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
  random_shuffle(v.begin(), v.end(), [](int a) { return Random.getRandom(a);});
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
map<V, vector<T> > groupBy(const vector<T>& values, function<V (const T&)> getKey) {
  map<V, vector<T> > ret;
  for (const T& elem : values)
    ret[getKey(elem)].push_back(elem);
  return ret;
}

template <typename T>
vector<T> getPrefix(const vector<T>& v, int start, int length) {
  CHECK(start >= 0 && length > 0 && start + length <= v.size());
  vector<T> ret;
  for (int i : Range(start, start + length))
    ret.push_back(v[i]);
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
  return contains(vector<T>(v), elem);
}

template <typename T>
vector<T> concat(const vector<T>& v, const vector<T>& w) {
  vector<T> ret(v);
  for (T elem : w)
    ret.push_back(elem);
  return ret;
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

class Nothing {
};

template <class T>
class Optional {
  public:
  Optional(const T& t) : elem(new T(t)) {}
  Optional(const Optional<T>& t) {
    if (t)
      elem.reset(new T(*t));
  }
  Optional(Optional<T>&&) = default;
  Optional(Nothing) {}
  Optional() {}

  Optional<T>& operator = (const Optional<T>& t) {
    if (t)
      elem.reset(new T(*t));
    else
      elem.reset();
    return *this;
  }

  T& operator = (const T& t) {
    elem.reset(new T(t));
    return *elem.get();
  }

  Optional<T>& operator = (Optional<T>&& t) = default;

  void operator = (Nothing) {
    elem.reset();
  }

  T& operator = (T&& t) {
    if (elem)
      *elem = std::move(t);
    else
      elem.reset(new T(std::move(t)));
    return *elem.get();
  }

  T* operator -> () {
    CHECK(elem.get() != nullptr);
    return elem.get();
  }

  const T* operator -> () const {
    CHECK(elem.get() != nullptr);
    return elem.get();
  }

  bool operator == (const T& t) const {
    return elem.get() && *elem.get() == t;
  }

  bool operator != (const T& t) const {
    return !elem.get() || *elem.get() != t;
  }

  operator bool() const {
    return elem.get() != nullptr;
  }

  T& operator * () {
    CHECK(elem.get());
    return *elem.get();
  }

  const T& operator * () const {
    CHECK(elem.get());
    return *elem.get();
  }

  private:
  unique_ptr<T> elem;
};

template <typename T, typename V>
bool contains(const initializer_list<T>& v, const Optional<V>& elem) {
  return elem && contains(vector<T>(v), *elem);
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
  vector<string> s = split(in.substr(1, in.size() - 2), ',');
  CHECKEQ((int)s.size(), 2);
  msg = Vec2(convertFromString<int>(s[0]), convertFromString<int>(s[1]));
  return d;
}

inline Debug& operator <<(Debug& d, Rectangle rect) {
  return d << "(" << rect.getPX() << "," << rect.getPY() << ") (" << rect.getKX() << "," << rect.getKY() << ")";
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
Optional<int> findElement(vector<T>& v, const T& element) {
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
void removeElement(vector<T>& v, const T& element) {
  auto ind = findElement(v, element);
  CHECK(ind) << "Element not found";
  removeIndex(v, *ind);
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

inline string addAParticle(const string& s) {
  if (isupper(s[0]))
    return s;
  if (contains({'a', 'e', 'u', 'i', 'o'}, s[0]))
    return string("an ") + s;
  else
    return string("a ") + s;
}

inline string capitalFirst(string s) {
  if (islower(s[0]))
    s[0] = toupper(s[0]);
  return s;
}

string combine(const vector<string>& adj);

#endif
