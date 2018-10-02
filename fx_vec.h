#pragma once

#include "fx_base.h"
#include "fx_math.h"

namespace fx {

#ifndef FWK_VEC2_CONVERSIONS
#define FWK_VEC2_CONVERSIONS
#endif

#ifndef FWK_VEC3_CONVERSIONS
#define FWK_VEC3_CONVERSIONS
#endif

// TODO: better name to differentiate from Vec2 ?
template <class T> struct vec2 {
  using Scalar = T;
  enum { vec_size = 2 };

  constexpr vec2(T x, T y) : x(x), y(y) {}
  constexpr vec2() : x(0), y(0) {}
  FWK_VEC2_CONVERSIONS

  vec2(const vec2 &) = default;
  vec2 &operator=(const vec2 &) = default;

  explicit vec2(T t) : x(t), y(t) {}
  template <class U> explicit vec2(const vec2<U> &rhs) : vec2(T(rhs.x), T(rhs.y)) {}

  vec2 operator*(const vec2 &rhs) const { return vec2(x * rhs.x, y * rhs.y); }
  vec2 operator/(const vec2 &rhs) const { return vec2(x / rhs.x, y / rhs.y); }
  vec2 operator+(const vec2 &rhs) const { return vec2(x + rhs.x, y + rhs.y); }
  vec2 operator-(const vec2 &rhs) const { return vec2(x - rhs.x, y - rhs.y); }
  vec2 operator*(T s) const { return vec2(x * s, y * s); }
  vec2 operator/(T s) const { return vec2(x / s, y / s); }
  vec2 operator-() const { return vec2(-x, -y); }

  T &operator[](int idx) { return v[idx]; }
  const T &operator[](int idx) const { return v[idx]; }

  bool operator==(const vec2 &rhs) const { return x == rhs.x && y == rhs.y; }

  union {
    struct {
      T x, y;
    };
    T v[2];
  };
};

template <class T> struct vec3 {
  using Scalar = T;
  enum { vec_size = 3 };

  constexpr vec3(T x, T y, T z) : x(x), y(y), z(z) {}
  constexpr vec3(const vec2<T> &xy, T z) : x(xy.x), y(xy.y), z(z) {}
  constexpr vec3() : x(0), y(0), z(0) {}
  explicit vec3(T t) : x(t), y(t), z(t) {}
  FWK_VEC3_CONVERSIONS

  vec3(const vec3 &) = default;
  vec3 &operator=(const vec3 &) = default;

  template <class U> explicit vec3(const vec3<U> &rhs) : vec3(T(rhs.x), T(rhs.y), T(rhs.z)) {}

  vec3 operator*(const vec3 &rhs) const { return vec3(x * rhs.x, y * rhs.y, z * rhs.z); }
  vec3 operator/(const vec3 &rhs) const { return vec3(x / rhs.x, y / rhs.y, z / rhs.z); }
  vec3 operator+(const vec3 &rhs) const { return vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
  vec3 operator-(const vec3 &rhs) const { return vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
  vec3 operator*(T s) const { return vec3(x * s, y * s, z * s); }
  vec3 operator/(T s) const { return vec3(x / s, y / s, z / s); }
  vec3 operator-() const { return vec3(-x, -y, -z); }

  T &operator[](int idx) { return v[idx]; }
  const T &operator[](int idx) const { return v[idx]; }

  vec2<T> xy() const { return {x, y}; }
  vec2<T> xz() const { return {x, z}; }
  vec2<T> yz() const { return {y, z}; }

  bool operator==(const vec3 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

  union {
    struct {
      T x, y, z;
    };
    T v[3];
  };
};

template <class T, class U> void operator+=(vec2<T> &a, const U &b) { a = a + b; }
template <class T, class U> void operator-=(vec2<T> &a, const U &b) { a = a - b; }
template <class T, class U> void operator*=(vec2<T> &a, const U &b) { a = a * b; }
template <class T, class U> void operator/=(vec2<T> &a, const U &b) { a = a / b; }

template <class T, class U> void operator+=(vec3<T> &a, const U &b) { a = a + b; }
template <class T, class U> void operator-=(vec3<T> &a, const U &b) { a = a - b; }
template <class T, class U> void operator*=(vec3<T> &a, const U &b) { a = a * b; }
template <class T, class U> void operator/=(vec3<T> &a, const U &b) { a = a / b; }

template <class T> vec2<T> operator*(T s, const vec2<T> &v) { return v * s; }
template <class T> vec3<T> operator*(T s, const vec3<T> &v) { return v * s; }

template <class T> vec2<T> vmin(const vec2<T> &lhs, const vec2<T> &rhs) {
  return {min(lhs[0], rhs[0]), min(lhs[1], rhs[1])};
}

template <class T> vec3<T> vmin(const vec3<T> &lhs, const vec3<T> &rhs) {
  return {min(lhs[0], rhs[0]), min(lhs[1], rhs[1]), min(lhs[2], rhs[2])};
}

template <class T> vec2<T> vmax(const vec2<T> &lhs, const vec2<T> &rhs) {
  return {max(lhs[0], rhs[0]), max(lhs[1], rhs[1])};
}

template <class T> vec3<T> vmax(const vec3<T> &lhs, const vec3<T> &rhs) {
  return {max(lhs[0], rhs[0]), max(lhs[1], rhs[1]), max(lhs[2], rhs[2])};
}

template <class T> auto dot(const vec2<T> &lhs, const vec2<T> &rhs) { return lhs[0] * rhs[0] + lhs[1] * rhs[1]; }

template <class T> auto dot(const vec3<T> &lhs, const vec3<T> &rhs) {
  return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

template <class T> auto length(const vec2<T> &vec) { return std::sqrt(dot(vec, vec)); }
template <class T> auto length(const vec3<T> &vec) { return std::sqrt(dot(vec, vec)); }

template <class T> auto lengthSq(const vec2<T> &vec) { return dot(vec, vec); }
template <class T> auto lengthSq(const vec3<T> &vec) { return dot(vec, vec); }

template <class T> auto distance(const vec2<T> &lhs, const vec2<T> &rhs) { return length(lhs - rhs); }

template <class T> auto distanceSq(const vec2<T> &lhs, const vec2<T> &rhs) { return lengthSq(lhs - rhs); }

template <class T> vec2<T> normalize(const vec2<T> &v) { return v / length(v); }
template <class T> vec3<T> normalize(const vec3<T> &v) { return v / length(v); }

template <class T> vec3<T> asXZ(const vec2<T> &v) { return {v[0], T(0), v[1]}; }
template <class T> vec3<T> asXY(const vec2<T> &v) { return {v[0], v[1], T(0)}; }
template <class T> vec3<T> asXZY(const vec2<T> &xz, T y) { return {xz[0], y, xz[1]}; }
template <class T> vec3<T> asXZY(const vec3<T> &v) { return {v[0], v[2], v[1]}; }

template <class T> vec2<T> vinv(const vec2<T> &vec) { return {T(1) / vec[0], T(1) / vec[1]}; }
template <class T> vec3<T> vinv(const vec3<T> &vec) { return {T(1) / vec[0], T(1) / vec[1], T(1) / vec[2]}; }

// Right-handed coordinate system
template <class T> T cross(const vec3<T> &a, const vec3<T> &b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

// Default orientation in all vector-related operations
// (cross products, rotations, etc.) is counter-clockwise.

template <class T> auto cross(const vec2<T> &a, const vec2<T> &b) { return a[0] * b[1] - a[1] * b[0]; }

template <class T> vec2<T> perpendicular(const vec2<T> &v) { return {-v[1], v[0]}; }

float vectorToAngle(const FVec2 &normalizedVector);
FVec2 angleToVector(float radians);
FVec2 rotateVector(const FVec2 &vec, float radians);
}
