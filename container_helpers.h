#pragma once

#include "stdafx.h"

template<class T>
void removeIndex(vector<T>& v, int index) {
  v[index] = std::move(v.back());
  v.pop_back();
}

template<class T>
optional<int> findAddress(const vector<T>& v, const T* ptr) {
  for (int i : All(v))
    if (&v[i] == ptr)
      return i;
  return none;
}

template<class T>
optional<int> findElement(const vector<T>& v, const T& element) {
  for (int i : All(v))
    if (v[i] == element)
      return i;
  return none;
}

template<class T>
optional<int> findElement(const vector<T*>& v, const T* element) {
  for (int i : All(v))
    if (v[i] == element)
      return i;
  return none;
}

template<class T>
optional<int> findElement(const vector<unique_ptr<T>>& v, const T* element) {
  for (int i : All(v))
    if (v[i].get() == element)
      return i;
  return none;
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
  CHECK(v.size() == 1) << v.size();
  return v[0];
}

template<class T>
T& getOnlyElement(vector<T>& v) {
  CHECK(v.size() == 1) << v.size();
  return v[0];
}

// TODO: write a template that works with all containers
template<class T>
T getOnlyElement(const set<T>& v) {
  CHECK(v.size() == 1) << v.size();
  return *v.begin();
}

template<class T>
T getOnlyElement(vector<T>&& v) {
  CHECK(v.size() == 1) << v.size();
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
vector<T> makeVec(T&& f, Args&&... args) {
  vector<T> ret;
  ret.reserve(sizeof...(Args) + 1);
  ret.push_back(std::move(f));
  emplaceBack(ret, args...);
  return ret;
}

