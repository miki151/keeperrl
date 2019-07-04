#pragma once

#include "stdafx.h"
#include "debug.h"
#include "my_containers.h"

template<class Container>
auto getOnlyElement(const Container& v) {
  CHECK(v.size() == 1) << v.size();
  return *v.begin();
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
  ret.push_back(std::forward<T>(f));
  emplaceBack(ret, std::forward<Args>(args)...);
  return ret;
}

template <typename Map>
void mergeMap(Map from, Map& to) {
  for (auto&& elem : from)
    if (!to.count(elem.first))
      to.insert(std::move(elem));
};

