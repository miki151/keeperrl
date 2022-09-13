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

#include "util.h"
#include <time.h>

RandomGen::RandomGen() {
  PROFILE;
}

void RandomGen::init(int seed) {
  PROFILE;
  generator.seed(seed);
}

int RandomGen::get(int max) {
  return get(0, max);
}

long long RandomGen::getLL() {
  return uniform_int_distribution<long long>(-(1LL << 62), 1LL << 62)(generator);
}

int RandomGen::get(Range r) {
  return get(r.getStart(), r.getEnd());
}

int RandomGen::get(int min, int max) {
  CHECK(max > min);
  return uniform_int_distribution<int>(min, max - 1)(generator);
}

std::string operator "" _s(const char* str, size_t) { 
  return std::string(str); 
}

string getCardinalName(Dir d) {
  switch (d) {
    case Dir::N: return "north";
    case Dir::S: return "south";
    case Dir::E: return "east";
    case Dir::W: return "west";
    case Dir::NE: return "north-east";
    case Dir::NW: return "north-west";
    case Dir::SE: return "south-east";
    case Dir::SW: return "south-west";
  }
}

int RandomGen::get(const vector<double>& weights) {
  double sum = 0;
  for (double elem : weights)
    sum += elem;
  CHECK(sum > 0);
  double r = getDouble(0, sum);
  sum = 0;
  for (int i : All(weights)) {
    sum += weights[i];
    if (sum >= r)
      return i;
  }
  return weights.size() - 1;
}

bool RandomGen::roll(int chance) {
  return get(chance) == 0;
}

bool RandomGen::chance(double v) {
  return getDouble(0, 1) <= v;
}

bool RandomGen::chance(float v) {
  return getFloat(0, 1) <= v;
}

double RandomGen::getDouble() {
  return defaultDist(generator);
}

double RandomGen::getDouble(double a, double b) {
  return uniform_real_distribution<double>(a, b)(generator);
}

pair<float, float> RandomGen::getFloat2Fast() {
  auto v = get(0, 1 << 30);
  int v1 = v >> 15;
  int v2 = v & 0x7fff;
  const float mul = 1.0f / float(0x7fff);
  return make_pair(float(v1) * mul, float(v2) * mul);
}

float RandomGen::getFloat(float a, float b) {
  return uniform_real_distribution<float>(a, b)(generator);
}

float RandomGen::getFloatFast(float a, float b) {
  auto v = get(0, INT_MAX);
  return a + (b - a) * float(v) * (1.0f / float(INT_MAX - 1));
}

RandomGen Random;

template string toString<int>(const int&);
template string toString<unsigned int>(const unsigned int&);
//template string toString<size_t>(const size_t&);
template string toString<char>(const char&);
template string toString<double>(const double&);
//template string toString<Vec2>(const Vec2&);

template int fromString<int>(const string&);
template double fromString<double>(const string&);

template optional<int> fromStringSafe<int>(const string&);
template optional<double> fromStringSafe<double>(const string&);
template optional<SteamId> fromStringSafe<SteamId>(const string&);
template optional<string> fromStringSafe<string>(const string&);

string toStringRounded(double value, double precision) {
  return toString(precision * round(value / precision));
}

template <class T>
optional<T> fromStringSafe(const string& s){
  std::stringstream ss(s);
  T t;
  ss >> t;
  if (!ss)
    return none;
  return t;
}

void trim(string& s) {
  while (!s.empty() && isspace(s[0]))
    s.erase(s.begin());
  while (!s.empty() && isspace(*s.rbegin()))
    s.erase(s.size() - 1);
}

string toUpper(const string& s) {
  string ret(s);
  transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
  return ret;
}

string toLower(const string& s) {
  string ret(s);
  transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
  return ret;
}

static bool isNotFilename(char c) {
  return !(tolower(c) >= 'a' && tolower(c) <= 'z') && !isdigit(c) && c != '_';
}

string stripFilename(string s) {
  s.erase(remove_if(s.begin(),s.end(), isNotFilename), s.end());
  return s;
}

bool endsWith(const string& s, const string& suffix) {
  return s.size() >= suffix.size() && contains(s, suffix, s.size() - suffix.size());
}

bool startsWith(const string& s, const string& prefix) {
  return contains(s, prefix, 0);
}

bool contains(const string& a, const string& substring, unsigned long index) {
  if (a.size() - index < substring.size())
    return false;
  for (auto i = index; i < index + substring.size(); ++i)
    if (a[i] != substring[i - index])
      return false;
  return true;
}

vector<string> split(const string& s, const std::initializer_list<char>& delim) {
  if (s.empty())
    return {};
  int begin = 0;
  vector<string> ret;
  for (int i : Range(s.size() + 1))
    if (i == s.size() || std::find(delim.begin(), delim.end(), s[i]) != delim.end()) {
      ret.push_back(s.substr(begin, i - begin));
      begin = i + 1;
    }
  return ret;
}

vector<string> splitIncludeDelim(const string& s, const std::initializer_list<char>& delim) {
  if (s.empty())
    return {};
  int begin = 0;
  vector<string> ret;
  for (int i : Range(s.size() + 1))
    if (i == s.size() || std::find(delim.begin(), delim.end(), s[i]) != delim.end()) {
      if (i > begin)
        ret.push_back(s.substr(begin, i - begin));
      if (i < s.size() && std::find(delim.begin(), delim.end(), s[i]) != delim.end())
        ret.push_back(string(1, s[i]));
      begin = i + 1;
    }
  return ret;
}

bool contains(const string& s, const string& p) {
  return s.find(p) != string::npos;
}

string toString(const Vec2& v) {
  stringstream ss;
  ss << "(" << v.x << ", " << v.y << ")";
  return ss.str();
}

Vec2::Vec2(int _x, int _y) : x(_x), y(_y) {
}

Vec2::Vec2(SVec2 v) : x(v.x), y(v.y) {
}

Vec2::Vec2(Dir dir) {
  switch (dir) {
    case Dir::N: x = 0; y = -1; break;
    case Dir::S: x = 0; y = 1; break;
    case Dir::E: x = 1; y = 0; break;
    case Dir::W: x = -1; y = 0; break;
    case Dir::NE: x = 1; y = -1; break;
    case Dir::SE: x = 1; y = 1; break;
    case Dir::NW: x = -1; y = -1; break;
    case Dir::SW: x = -1; y = 1; break;
  }
}

Vec2 Vec2::mult(const Vec2& v) const {
  return Vec2(x * v.x, y * v.y);
}

Vec2 Vec2::div(const Vec2& v) const {
  return Vec2(x / v.x, y / v.y);
}

int Vec2::dotProduct(Vec2 a, Vec2 b) {
  return a.x * b.x + a.y * b.y;
}

static const vector<Vec2> dir8 {
  Vec2(0, -1), Vec2(0, 1), Vec2(1, 0), Vec2(-1, 0), Vec2(1, -1), Vec2(-1, -1), Vec2(1, 1), Vec2(-1, 1)
};

const vector<Vec2>& Vec2::directions8() {
  return dir8;
}

vector<Vec2> Vec2::neighbors8() const {
  return {Vec2(x, y + 1), Vec2(x + 1, y), Vec2(x, y - 1), Vec2(x - 1, y), Vec2(x + 1, y + 1), Vec2(x + 1, y - 1),
      Vec2(x - 1, y - 1), Vec2(x - 1, y + 1)};
}

static const vector<Vec2> dir4 {
  Vec2(0, -1), Vec2(0, 1), Vec2(1, 0), Vec2(-1, 0)
};

const vector<Vec2>& Vec2::directions4() {
  return dir4;
}

vector<Vec2> Vec2::neighbors4() const {
  return { Vec2(x, y + 1), Vec2(x + 1, y), Vec2(x, y - 1), Vec2(x - 1, y)};
}

vector<Vec2> Vec2::directions8(RandomGen& random) {
  return random.permutation(directions8());
}

vector<Vec2> Vec2::neighbors8(RandomGen& random) const {
  return random.permutation(neighbors8());
}

vector<Vec2> Vec2::directions4(RandomGen& random) {
  return random.permutation(directions4());
}

vector<Vec2> Vec2::neighbors4(RandomGen& random) const {
  return random.permutation(neighbors4());
}

bool Vec2::isCardinal4() const {
  return abs(x) + abs(y) == 1;
}

bool Vec2::isCardinal8() const {
  return max(abs(x), abs(y)) == 1;
}

Dir Vec2::getCardinalDir() const {
  switch (x) {
    case 0:
      switch (y) {
        case -1: return Dir::N;
        case 1: return Dir::S;
      }
      break;
    case 1:
      switch (y) {
        case 0: return Dir::E;
        case -1: return Dir::NE;
        case 1: return Dir::SE;
      }
      break;
    case -1:
      switch (y) {
        case 0: return Dir::W;
        case -1: return Dir::NW;
        case 1: return Dir::SW;
      }
      break;
  }
  FATAL << "Not cardinal dir " << *this;
  return {};
}

vector<Vec2> Vec2::corners() {
  return { Vec2(1, 1), Vec2(1, -1), Vec2(-1, -1), Vec2(-1, 1)};
}

vector<set<Vec2>> Vec2::calculateLayers(set<Vec2> elems) {
  vector<set<Vec2>> ret;
  while (1) {
    ret.emplace_back();
    set<Vec2> curElems(elems);
    for (Vec2 v : curElems)
      for (Vec2 v2 : v.neighbors4())
        if (!curElems.count(v2)) {
          ret.back().insert(v);
          elems.erase(v);
          break;
        }
    if (elems.empty())
      break;
  }
  return ret;
}

SERIALIZE_DEF(Rectangle, px, py, kx, ky, w, h)

SERIALIZATION_CONSTRUCTOR_IMPL(Rectangle);

Rectangle Rectangle::boundingBox(const vector<Vec2>& verts) {
  CHECK(!verts.empty());
  int infinity = 1000000;
  int minX = infinity, maxX = -infinity, minY = infinity, maxY = -infinity;
  for (Vec2 v : verts) {
    minX = min(minX, v.x);
    maxX = max(maxX, v.x);
    minY = min(minY, v.y);
    maxY = max(maxY, v.y);
  }
  return Rectangle(minX, minY, maxX + 1, maxY + 1);
}

Rectangle Rectangle::centered(Vec2 center, int radius) {
  return Rectangle(center - Vec2(radius, radius), center + Vec2(radius + 1, radius + 1));
}

Rectangle Rectangle::centered(int radius) {
  return Rectangle(-Vec2(radius, radius), Vec2(radius + 1, radius + 1));
}

vector<Vec2> Rectangle::getAllSquares() const {
  vector<Vec2> ret;
  for (Vec2 v : (*this))
    ret.push_back(v);
  return ret;
}

Rectangle Rectangle::apply(Vec2::LinearMap map) const {
  Vec2 v1 = map(Vec2(px, py));
  Vec2 v2 = map(Vec2(kx - 1, ky - 1));
  return Rectangle(min(v1.x, v2.x), min(v1.y, v2.y), max(v1.x, v2.x) + 1, max(v1.y, v2.y) + 1);
}

bool Rectangle::operator == (const Rectangle& r) const {
  return px == r.px && py == r.py && kx == r.kx && ky == r.ky;
}

bool Rectangle::operator != (const Rectangle& r) const {
  return !(*this == r);
}

template <class Archive>
void Vec2::serialize(Archive& ar, const unsigned int) {
  ar(NAMED(x), NAMED(y));
}

SERIALIZABLE(Vec2);

bool Vec2::inRectangle(int px, int py, int kx, int ky) const {
  return x >= px && x < kx && y >= py && y < ky;
}

bool Vec2::inRectangle(const Rectangle& r) const {
  return x >= r.px && x < r.kx && y >= r.py && y < r.ky;
}

bool Vec2::operator== (const Vec2& v) const {
  return v.x == x && v.y == y;
}

bool Vec2::operator!= (const Vec2& v) const {
  return v.x != x || v.y != y;
}

Vec2& Vec2::operator +=(const Vec2& v) {
  x += v.x;
  y += v.y;
  return *this;
}

Vec2 Vec2::operator + (const Vec2& v) const {
  return Vec2(x + v.x, y + v.y);
}

Vec2 Vec2::operator * (int a) const {
  return Vec2(x * a, y * a);
}

Vec2 Vec2::operator * (double a) const {
  return Vec2(x * a, y * a);
}

Vec2 Vec2::operator / (int a) const {
  return Vec2(x / a, y / a);
}

Vec2& Vec2::operator -=(const Vec2& v) {
  x -= v.x;
  y -= v.y;
  return *this;
}

Vec2 Vec2::operator - (const Vec2& v) const {
  return Vec2(x - v.x, y - v.y);
}

Vec2 Vec2::operator - () const {
  return Vec2(-x, -y);
}

bool Vec2::operator < (Vec2 v) const {
  return x < v.x || (x == v.x && y < v.y);
}

int Vec2::length8() const {
  return max(abs(x), abs(y));
}

int Vec2::dist8(Vec2 v) const {
  return (v - *this).length8();
}

int Vec2::dist4(Vec2 v) const {
  return (v - *this).length4();
}

double Vec2::distD(Vec2 v) const {
  return (v - *this).lengthD();
}

int Vec2::length4() const {
  return abs(x) + abs(y);
}

double Vec2::lengthD() const {
  return sqrt(x * x + y * y);
}

Vec2 Vec2::shorten() const {
  CHECK(x == 0 || y == 0 || abs(x) == abs(y));
  int d = length8();
  return Vec2(x / d, y / d);
}

SERIALIZE_DEF(Vec2Range, begin, end)

static int sgn(int a) {
  if (a == 0)
    return 0;
  if (a < 0)
    return -1;
  else
    return 1;
}

static int sgn(int a, int b) {
  if (abs(a) >= abs(b))
    return sgn(a);
  else
    return 0;
}

pair<Vec2, Vec2> Vec2::approxL1() const {
  return make_pair(Vec2(sgn(x, x), sgn(y,y)), Vec2(sgn(x, y), sgn(y, x)));
}

Vec2 Vec2::getBearing() const {
  double ang = atan2(y, x) / 3.14159265359 * 180 / 45;
  if (ang < 0)
    ang += 8;
  if (ang < 0.5 || ang >= 7.5)
    return Vec2(1, 0);
  if (ang >= 0.5 && ang < 1.5)
    return Vec2(1, 1);
  if (ang >= 1.5 && ang < 2.5)
    return Vec2(0, 1);
  if (ang >= 2.5 && ang < 3.5)
    return Vec2(-1, 1);
  if (ang >= 3.5 && ang < 4.5)
    return Vec2(-1, 0);
  if (ang >= 4.5 && ang < 5.5)
    return Vec2(-1, -1);
  if (ang >= 5.5 && ang < 6.5)
    return Vec2(0, -1);
  if (ang >= 6.5 && ang < 7.5)
    return Vec2(1, -1);
  FATAL << ang;
  return Vec2(0, 0);
}

Vec2 Vec2::getCenterOfWeight(const vector<Vec2>& vs) {
  Vec2 ret;
  for (Vec2 v : vs)
    ret += v;
  return ret / vs.size();
}

Rectangle::Rectangle(int _w, int _h) : px(0), py(0), kx(_w), ky(_h), w(_w), h(_h) {
  if (w <= 0 || h <= 0) {
    kx = ky = 0;
    w = h = 0;
  }
}

Rectangle::Rectangle(Vec2 d) : Rectangle(d.x, d.y) {
}

Rectangle::Rectangle(int px1, int py1, int kx1, int ky1) : px(px1), py(py1), kx(kx1), ky(ky1), w(kx1 - px1),
    h(ky1 - py1) {
  if (kx <= px || ky <= py) {
    kx = px;
    ky = py;
    w = h = 0;
  }
}

Rectangle::Rectangle(Vec2 p, Vec2 k) : Rectangle(min(p.x, k.x), min(p.y, k.y), max(p.x, k.x), max(p.y, k.y)) {
}

Rectangle::Rectangle(Range xRange, Range yRange)
    : Rectangle(xRange.getStart(), yRange.getStart(), xRange.getEnd(), yRange.getEnd()) {
}

Rectangle::Iter::Iter(int x1, int y1, int px1, int py1, int kx1, int ky1) : pos(x1, y1), py(py1), ky(ky1) {}

Vec2 Rectangle::random(RandomGen& r) const {
  return Vec2(r.get(px, kx), r.get(py, ky));
}

Vec2 Rectangle::middle() const {
  return Vec2((px + kx) / 2, (py + ky) / 2);
}

bool Rectangle::empty() const {
  return area() <= 0;
}

int Rectangle::left() const {
  return px;
}

int Rectangle::top() const {
  return py;
}

Range Rectangle::getXRange() const {
  return Range(px, kx);
}

Range Rectangle::getYRange() const {
  return Range(py, ky);
}

int Rectangle::right() const {
  return kx;
}

int Rectangle::bottom() const {
  return ky;
}

int Rectangle::width() const {
  return w;
}

int Rectangle::height() const {
  return h;
}

int Rectangle::area() const {
  return w * h;
}

Vec2 Rectangle::getSize() const {
  return Vec2(w, h);
}

Vec2 Rectangle::topLeft() const {
  return Vec2(px, py);
}

Vec2 Rectangle::bottomRight() const {
  return Vec2(kx, ky);
}

Vec2 Rectangle::topRight() const {
  return Vec2(kx, py);
}

Vec2 Rectangle::bottomLeft() const {
  return Vec2(px, ky);
}

bool Rectangle::intersects(const Rectangle& other) const {
  return max(px, other.px) < min(kx, other.kx) && max(py, other.py) < min(ky, other.ky);
}

bool Rectangle::contains(const Rectangle& other) const {
  return px <= other.px && py <= other.py && kx >= other.kx && ky >= other.ky;
}

Rectangle Rectangle::intersection(const Rectangle& other) const {
  return Rectangle(max(px, other.px), max(py, other.py), min(kx, other.kx), min(ky, other.ky));
}

int Rectangle::getDistance(const Rectangle& other) const {
  int ret = min(
      min(bottomRight().dist8(other.topLeft()), other.bottomRight().dist8(topLeft())),
      min(bottomLeft().dist8(other.topRight()), other.bottomLeft().dist8(topRight())));
  if (getXRange().intersects(other.getXRange()))
    ret = min(ret, min(abs(top() - other.bottom()), abs(other.top() - bottom())));
  if (getYRange().intersects(other.getYRange()))
    ret = min(ret, min(abs(left()- other.right()), abs(other.left()- right())));
  if (intersects(other))
    ret = -ret;
  return ret;
}

Rectangle Rectangle::translate(Vec2 v) const {
  return Rectangle(topLeft() + v, bottomRight() + v);
}

Rectangle Rectangle::minusMargin(int margin) const {
  return Rectangle(px + margin, py + margin, kx - margin, ky - margin);
}

Vec2 Rectangle::Iter::operator* () const {
  return pos;
}
bool Rectangle::Iter::operator != (const Iter& other) const {
  return pos != other.pos;
}

const Rectangle::Iter& Rectangle::Iter::operator++ () {
  ++pos.y;
  if (pos.y >= ky) {
    pos.y = py;
    ++pos.x;
  }
  return *this;
}

Rectangle::Iter Rectangle::begin() const {
  return Iter(px, py, px, py, kx, ky);
}

Rectangle::Iter Rectangle::end() const {
  return Iter(kx, py, px, py, kx, ky);
}

Range::Range(int a, int b) : start(a), finish(b) {
}
Range::Range(int a) : Range(0, a) {}

Range Range::singleElem(int a) {
  return Range(a, a + 1);
}

Range Range::reverse() {
  Range r(finish - 1, start - 1);
  r.increment = -1;
  return r;
}

bool Range::isEmpty() const {
  return (increment == 1 && start >= finish) || (increment == -1 && start <= finish);
}

Range Range::shorten(int r) {
  if (start < finish) {
    if (finish - start >= 2 * r)
      return Range(start + r, finish - r);
    else
      return Range(0, 0);
  } else {
    if (start - finish >= 2 * r)
      return Range(start - r, finish + r);
    else
      return Range(0, 0);
  }
}

int Range::getStart() const {
  return start;
}

int Range::getEnd() const {
  return finish;
}

int Range::getLength() const {
  return finish - start;
}

bool Range::contains(int p) const {
  return (increment > 0 && p >= start && p < finish) || (increment < 0 && p <= start && p > finish);
}

int Range::clamp(int a) const {
  CHECK(increment == 1);
  return max(start, min(a, finish - 1));
}

bool Range::intersects(Range r) const {
  return contains(r.start) || contains(r.finish - r.increment) || r.contains(start);
}

Range Range::intersection(Range r) const {
  CHECK(increment == 1 && r.increment == 1);
  return Range(max(start, r.start), min(finish, r.finish));
}

bool Range::operator == (const Range& r) const {
  return start == r.start && finish == r.finish && increment == r.increment;
}

Range Range::operator + (int x) const {
  return Range(start + x, finish + x, increment);
}

Range Range::operator - (int x) const {
  return Range(start - x, finish - x, increment);
}

Range::Iter Range::begin() {
  if ((increment > 0 && start < finish) || (increment < 0 && start > finish))
    return Iter(start, start, finish, increment);
  else
    return end();
}

Range::Iter Range::end() {
  return Iter(finish, start, finish, increment);
}

Range::Range(int start, int end, int increment) : start(start), finish(end), increment(increment) {}

Range::Iter::Iter(int i, int a, int b, int inc) : ind(i), /*min(a), max(b), */increment(inc) {}

int Range::Iter::operator* () const {
  return ind;
}

bool Range::Iter::operator != (const Iter& other) const {
  return other.ind != ind;
}

const Range::Iter& Range::Iter::operator++ () {
  ind += increment;
  //CHECK(ind <= max && ind >= min) << ind << " " << min << " " << max;
  return *this;
}

SERIALIZE_DEF(Range, NAMED(start), NAMED(finish), OPTION(increment))
SERIALIZATION_CONSTRUCTOR_IMPL(Range);

string combine(const vector<string>& adj, bool commasOnly) {
  string res;
  for (int i : All(adj)) {
    if (i == adj.size() - 1 && i > 0 && !commasOnly)
      res.append(" and ");
    else if (i > 0)
      res.append(", ");
    res.append(adj[i]);
  }
  return res;
}

string combine(const vector<string>& adj, const string& delimiter) {
  string res;
  for (auto& elem : adj) {
    if (!res.empty())
      res += delimiter;
    res += elem;
  }
  return res;
}

bool hasSentenceEnding(const string& s) {
  return s.back() == '.' || s.back() == '?' || s.back() == '!' || s.back() == '\"';
}

string combineSentences(const vector<string>& v) {
  if (v.empty())
    return "";
  string ret;
  for (string s : v) {
    if (s.empty())
      continue;
    if (!ret.empty()) {
      if (!hasSentenceEnding(ret))
        ret += ".";
      ret += " ";
    }
    ret += s;
  }
  return ret;
}

string addAParticle(const string& s) {
  if (isupper(s[0]))
    return s;
  if (contains({'a', 'e', 'u', 'i', 'o'}, s[0]))
    return string("an ") + s;
  else
    return string("a ") + s;
}

string capitalFirst(string s) {
  if (!s.empty() && islower(s[0]))
    s[0] = toupper(s[0]);
  return s;
}

string noCapitalFirst(string s) {
  if (!s.empty() && isupper(s[0]))
    s[0] = tolower(s[0]);
  return s;
}

string makeSentence(string s) {
  if (s.empty())
    return s;
  s = capitalFirst(s);
  if (s.size() > 1 && s[0] == '\"' && islower(s[1]))
    s[1] = toupper(s[1]);
  if (!hasSentenceEnding(s))
    s.append(".");
  return s;
}

vector<string> makeSentences(string s) {
  vector<string> ret = split(s, {'.'});
  for (auto& elem : ret) {
    trim(elem);
    elem = makeSentence(elem);
  }
  return ret;
}

string lowercase(string s) {
  for (int i : All(s))
    if (isupper(s[i]))
      s[i] = tolower(s[i]);
  return s;
}

string getPlural(const string& a, const string&b, int num) {
  if (num == 1)
    return "1 " + a;
  else
    return toString(num) + " " + b;
}

string getPlural(const string& a, int num) {
  if (num == 1)
    return "1 " + a;
  else
    return toString(num) + " " + makePlural(a);
}

string makePlural(const string& s) {
  if (s.empty())
    return "";
  if (s.back() == 'y')
    return s.substr(0, s.size() - 1) + "ies";
  if (endsWith(s, "ph"))
    return s + "s";
  if (s.back() == 'h')
    return s + "es";
  if (s.back() == 's')
    return s;
  if (endsWith(s, "shelf"))
    return s.substr(0, s.size() - 5) + "shelves";
  return s + "s";
}

static string toText(int num) {
  switch (num) {
    case 0: return "zero";
    case 1: return "one";
    case 2: return "two";
    case 3: return "three";
    case 4: return "four";
    case 5: return "five";
    case 6: return "six";
    case 7: return "seven";
    case 8: return "eight";
    case 9: return "nine";
    case 10: return "ten";
    case 11: return "eleven";
    case 12: return "twelve";
    default: return toString(num);
  }
}

string getPluralText(const string& a, int num) {
  if (num == 1)
    return "a " + a;
  else
    return toText(num) + " " + a + "s";
}

Semaphore::Semaphore(int v) : value(v) {}

void Semaphore::p() {
  std::unique_lock<std::mutex> lock(mut);
  while (!value) {
    cond.wait(lock);
  }
  --value;
}

int Semaphore::get() {
  std::unique_lock<std::mutex> lock(mut);
  return value;
}

void Semaphore::v() {
  std::unique_lock<std::mutex> lock(mut);
  ++value;
  lock.unlock();
  cond.notify_one();
}

AsyncLoop::AsyncLoop(function<void()> f) : AsyncLoop([]{}, f) {
}

AsyncLoop::AsyncLoop(function<void()> init, function<void()> loop)
    : done(false), t([=] { init(); while (!done) { loop(); }}) {
}

void AsyncLoop::setDone() {
  done = true;
}

void AsyncLoop::finishAndWait() {
  setDone();
  if (t.joinable())
    t.join();
}

AsyncLoop::~AsyncLoop() {
  finishAndWait();
}


/*#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 4000);
  return attr;
}

thread makeThread(function<void()> fun) {
  return thread(getAttributes(), fun);
}

#else*/

thread makeThread(function<void()> fun) {
  return thread(fun);
}

scoped_thread makeScopedThread(function<void()> fun) {
  return scoped_thread(makeThread(std::move(fun)));
}

//#endif

ConstructorFunction::ConstructorFunction(function<void()> fun) {
  fun();
}

DestructorFunction::DestructorFunction(function<void()> dest) : destFun(dest) {
}

DestructorFunction::~DestructorFunction() {
  destFun();
}

bool DirSet::contains(DirSet dirSet) const {
  return intersection(dirSet) == dirSet;
}

DirSet::DirSet(const initializer_list<Dir>& dirs) {
  for (Dir dir : dirs)
    content |= (1 << int(dir));
}

DirSet::DirSet(const vector<Dir>& dirs) {
  for (Dir dir : dirs)
    content |= (1 << int(dir));
}

DirSet::DirSet(unsigned char c) : content(c) {
}

DirSet::DirSet() {
}

DirSet::DirSet(bool n, bool s, bool e, bool w, bool ne, bool nw, bool se, bool sw) {
  content = n | (s << 1) | (e << 2) | (w << 3) | (ne << 4) | (nw << 5) | (se << 6) | (sw << 7);
}

void DirSet::insert(Dir dir) {
  content |= (1 << int(dir));
}

bool DirSet::has(Dir dir) const {
  return content & (1 << int(dir));
}

DirSet DirSet::oneElement(Dir dir) {
  return DirSet(1 << int(dir));
}

DirSet DirSet::fullSet() {
  return 255;
}

DirSet DirSet::intersection(DirSet s) const {
  s.content &= content;
  return s;
}

DirSet DirSet::complement() const {
  return ~content;
}

DirSet::Iter::Iter(const DirSet& s, Dir num) : set(s), ind(num) {
  goForward();
}

void DirSet::Iter::goForward() {
  while (ind < Dir(8) && !set.has(ind))
    ind = Dir(int(ind) + 1);
}

Dir DirSet::Iter::operator* () const {
  return ind;
}

bool DirSet::Iter::operator != (const Iter& other) const {
  return ind != other.ind;
}

const DirSet::Iter& DirSet::Iter::operator++ () {
  ind = Dir(int(ind) + 1);
  goForward();
  return *this;
}

DisjointSets::DisjointSets(int s) : father(s), size(s, 1) {
  for (int i : Range(s))
    father[i] = i;
}

void DisjointSets::join(int i, int j) {
  i = getSet(i);
  j = getSet(j);
  if (size[i] < size[j])
    swap(i, j);
  father[j] = i;
  size[i] += size[j];
}

bool DisjointSets::same(int i, int j) {
  return getSet(i) == getSet(j);
}

bool DisjointSets::same(const vector<int>& v) {
  for (int i : All(v))
    if (!same(v[i], v[0]))
      return false;
  return true;
}

int DisjointSets::getSet(int i) {
  int ret = i;
  while (father[ret] != ret)
    ret = father[ret];
  while (i != ret) {
    int j = father[i];
    father[i] = ret;
    i = j;
  }
  return ret;
}

int getSize(const std::string& s) {
  return s.size();
}

const char* getString(const std::string& s) {
  return s.c_str();
}

string combineWithOr(const vector<string>& elems) {
  string ret;
  for (auto& elem : elems) {
    if (!ret.empty())
      ret += " or ";
    ret += elem;
  }
  return ret;
}

string toStringWithSign(int v) {
  return (v > 0 ? "+" : "") + toString(v);
}

Dir rotate(Dir dir) {
  switch (dir) {
    case Dir::N:
      return Dir::NE;
    case Dir::NE:
      return Dir::E;
    case Dir::E:
      return Dir::SE;
    case Dir::SE:
      return Dir::S;
    case Dir::S:
      return Dir::SW;
    case Dir::SW:
      return Dir::W;
    case Dir::W:
      return Dir::NW;
    case Dir::NW:
      return Dir::N;
  }
}

#include "pretty_archive.h"
template void Vec2::serialize(PrettyInputArchive&, unsigned);
template<>
void Range::serialize(PrettyInputArchive& ar1, unsigned) {
  if (ar1.readMaybe(start)) {
    finish = start + 1;
    increment = 1;
    return;
  }
  optional_no_none<int> finish;
  ar1(NAMED(start), OPTION(finish), OPTION(increment));
  ar1(endInput());
  this->finish = finish.value_or(start + 1);
  if (this->start >= this->finish)
    ar1.error("Range is empty: (" + toString(start) + ", " + toString(this->finish) + ")");
}

Vec2 Vec2Range::get(RandomGen& r) const {
  return Vec2(r.get(begin.x, end.x), r.get(begin.y, end.y));
}

void Vec2Range::serialize(PrettyInputArchive& ar1, const unsigned int) {
  pair<int, int> b;
  optional<pair<int, int>> e;
  ar1(NAMED(b), NAMED(e), endInput());
  begin = Vec2(b.first, b.second);
  if (!e)
    e = make_pair(b.first + 1, b.second + 1);
  end = Vec2(e->first, e->second);
}

string toString(const Range& r) {
  return "[" + toString(r.getStart()) + ", " + toString(r.getEnd()) + "]";
}

string toPercentage(double v) {
  return toString<int>(round(v * 100)) + "%";
}

void openUrl(const string& url) {
#if defined(WINDOWS) || defined(OSX)
  system(("cmd /c start " + url).data());
#else
  system(("xdg-open " + url).data());
#endif
}
