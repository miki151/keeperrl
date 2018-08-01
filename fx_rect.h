#pragma once

#include "fx_vec.h"
#include <memory>

namespace fx {

// Axis-aligned rect
// Invariant: min <= max (use validRange)
// TODO: specialize with scalar
template <class T> class Rect {
  Rect(T min, T max, NoAssertsTag) : m_min(min), m_max(max) {}

public:
  using Scalar = typename T::Scalar;
  using Vector = T;
  using Point = Vector;

  enum { dim_size = 2, num_corners = 4 };

  // min <= max in all dimensions; can be empty
  static bool validRange(const Point &min, const Point &max) {
    for(int i = 0; i < dim_size; i++)
      if(!(min[i] <= max[i]))
        return false;
    return true;
  }

  // min >= max in any dimension
  static bool emptyRange(const Point &min, const Point &max) {
    for(int n = 0; n < dim_size; n++)
      if(!(min[n] < max[n]))
        return true;
    return false;
  }

  Rect(Scalar min_x, Scalar min_y, Scalar max_x, Scalar max_y) : Rect({min_x, min_y}, {max_x, max_y}) {}

  Rect(Point min, Point max) : m_min(min), m_max(max) { PASSERT(validRange(min, max)); }
  explicit Rect(T size) : Rect(T(), size) {}
  Rect() : m_min(), m_max() {}

  Rect(const Rect &) = default;
  Rect &operator=(const Rect &) = default;

  template <class U> explicit Rect(const Rect<U> &rhs) : Rect(T(rhs.min()), T(rhs.max())) {}

  Scalar min(int i) const { return m_min[i]; }
  Scalar max(int i) const { return m_max[i]; }

  const Point &min() const { return m_min; }
  const Point &max() const { return m_max; }

  void setMin(const T &min) {
    m_min = min;
    PASSERT(validRange(m_min, m_max));
  }
  void setMax(const T &max) {
    m_max = max;
    PASSERT(validRange(m_min, m_max));
  }
  void setSize(const T &size) {
    m_max = m_min + size;
    PASSERT(validRange(m_min, m_max));
  }

  Scalar x() const { return m_min[0]; }
  Scalar y() const { return m_min[1]; }

  Scalar ex() const { return m_max[0]; }
  Scalar ey() const { return m_max[1]; }

  Scalar width() const { return size(0); }
  Scalar height() const { return size(1); }

  auto surfaceArea() const { return width() * height(); }

  Scalar size(int axis) const { return m_max[axis] - m_min[axis]; }
  T size() const { return m_max - m_min; }
  Point center() const { return (m_max + m_min) / Scalar(2); }

  Rect operator+(const T &offset) const { return Rect(m_min + offset, m_max + offset); }
  Rect operator-(const T &offset) const { return Rect(m_min - offset, m_max - offset); }
  Rect operator*(const T &scale) const {
    T tmin = m_min * scale, tmax = m_max * scale;
    for(int n = 0; n < dim_size; n++)
      if(scale[n] < Scalar(0))
        swap(tmin[n], tmax[n]);
    return {tmin, tmax, no_asserts_tag};
  }

  Rect operator*(Scalar scale) const {
    auto tmin = m_min * scale, tmax = m_max * scale;
    if(scale < Scalar(0))
      swap(tmin, tmax);
    return {tmin, tmax, no_asserts_tag};
  }

  bool empty() const { return emptyRange(m_min, m_max); }

  bool contains(const Point &point) const {
    for(int i = 0; i < dim_size; i++)
      if(!(point[i] >= m_min[i] && point[i] <= m_max[i]))
        return false;
    return true;
  }

  bool contains(const Rect &Rect) const { return Rect == intersection(Rect); }

  bool containsCell(const T &pos) const {
    for(int i = 0; i < dim_size; i++)
      if(!(pos[i] >= m_min[i] && pos[i] + Scalar(1) <= m_max[i]))
        return false;
    return true;
  }

  std::array<Point, 4> corners() const { return {{m_min, {m_min[0], m_max[1]}, m_max, {m_max[0], m_min[1]}}}; }

  Rect intersectionOrEmpty(const Rect &rhs) const {
    auto tmin = vmax(m_min, rhs.m_min);
    auto tmax = vmin(m_max, rhs.m_max);

    if(!emptyRange(tmin, tmax))
      return Rect{tmin, tmax, no_asserts_tag};
    return Rect{};
  }

  Point closestPoint(const Point &point) const { return vmin(vmax(point, m_min), m_max); }

  Scalar distanceSq(const Point &point) const { return fx::distanceSq(point, closestPoint(point)); }

  auto distanceSq(const Rect &rhs) const {
    Point p1 = vclamp(rhs.center(), m_min, m_max);
    Point p2 = vclamp(p1, rhs.m_min, rhs.m_max);
    return fx::distanceSq(p1, p2);
  }

  auto distance(const Point &point) const { return std::sqrt(distanceSq(point)); }
  auto distance(const Rect &Rect) const { return std::sqrt(distanceSq(Rect)); }

  Rect inset(const T &val_min, const T &val_max) const {
    auto new_min = m_min + val_min, new_max = m_max - val_max;
    return {vmin(new_min, new_max), vmax(new_min, new_max), no_asserts_tag};
  }
  Rect inset(const T &value) const { return inset(value, value); }
  Rect inset(Scalar value) const { return inset(T(value)); }

  Rect enlarge(const T &val_min, const T &val_max) const { return inset(-val_min, -val_max); }
  Rect enlarge(const T &value) const { return inset(-value); }
  Rect enlarge(Scalar value) const { return inset(T(-value)); }

  bool operator==(const Rect &rhs) const { return m_min == rhs.m_min && m_max == rhs.m_max; }

private:
  union {
    struct {
      Point m_min, m_max;
    };
    Point m_v[2];
  };
};

using FRect = Rect<FVec2>;
using IRect = Rect<IVec2>;
}
