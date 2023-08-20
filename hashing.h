#pragma once

#include "stdafx.h"
#include "my_containers.h"

struct general_ {};
struct special_ : general_ {};
template<typename> struct int_ { typedef int type; };

template<typename T, typename int_<decltype(std::declval<T&>().getHash())>::type = 0>
int getHashImpl(const T& obj, special_) {
  return obj.getHash();
}

template<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>
int getHashImpl(const T& obj, special_) {
  return int(obj);
}

template<typename T>
int getHashImpl(const T& obj, general_) {
  return hash<T>()(obj);
}

template <typename T, typename U>
size_t combineHash(const pair<T, U>& arg);

template <typename T>
size_t combineHash(const T& arg) {
  return getHashImpl(arg, special_());
}

template <typename T>
size_t combineHash(const optional<T>& arg) {
  if (!arg)
    return 1236543;
  else
    return combineHash(*arg);
}

template <typename Iter>
size_t combineHashIter(Iter begin, Iter end) {
  size_t seed = 43216789;
  for (; begin != end; ++begin) {
    seed ^= combineHash(*begin) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

template <typename T>
size_t combineHash(const vector<T>& v) {
  return combineHashIter(v.begin(), v.end());
}

template <typename T, typename U, typename V>
size_t combineHash(const tuple<T, U, V>& v) {
  return combineHash(std::get<0>(v), std::get<1>(v), std::get<2>(v));
}

template <typename T>
size_t combineHash(const set<T>& v) {
  return combineHashIter(v.begin(), v.end());
}

template <typename Arg1, typename... Args>
size_t combineHash(const Arg1& arg1, const Args&... args) {
  size_t hash = combineHash(args...);
  return hash ^ (combineHash(arg1) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}

inline size_t combineHash() {
  return 0x9e3779b9;
}

template<>
inline size_t combineHash(const milliseconds& m) {
  return m.count();
}

template <typename T, typename U>
size_t combineHash(const pair<T, U>& arg) {
  return combineHash(arg.first, arg.second);
}

#define HASH_ALL(...)\
size_t getHash() const {\
  return combineHash(__VA_ARGS__);\
}

#define HASH_DEF(Type, ...)\
size_t Type::getHash() const {\
  return combineHash(__VA_ARGS__);\
}

#define HASH(X) X

template <typename T>
struct CustomHash {
  size_t operator() (const T& t) const {
    return combineHash(t);
  }
};

template <typename Key, typename Value>
using HashMap = unordered_map<Key, Value, CustomHash<Key>>;

template <typename Key>
using HashSet = unordered_set<Key, CustomHash<Key>>;