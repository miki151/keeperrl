#pragma once

#include <vector>
#include <set>
#include "extern/optional.h"
#include "debug.h"

using std::experimental::optional;
using std::experimental::none;

template <typename T>
class vector : public std::vector<T> {
  public:
  using std::vector<T>::vector;

  using base = std::vector<T>;

  int size() const {
    return (int) base::size();
  }

  template <typename V>
  bool contains(const V& elem) const {
    return std::find(base::begin(), base::end(), elem) != base::end();
  }

  template <typename V>
  bool contains(const optional<V>& elem) const {
    return elem && contains(*elem);
  }

  void removeIndex(int index) {
    base::at(index) = std::move(base::back());
    base::pop_back();
  }

  optional<int> findAddress(const T* ptr) const {
    for (int i = 0; i < size(); ++i)
      if (&base::at(i) == ptr)
        return i;
    return none;
  }

  optional<int> findElement(const T& element) const {
    for (int i = 0; i < size(); ++i)
      if (base::at(i) == element)
        return i;
    return none;
  }

  bool removeElementMaybe(const T& element) {
    if (auto ind = findElement(element)) {
      removeIndex(*ind);
      return true;
    }
    return false;
  }

  void removeElement(const T& element) {
    auto ind = findElement(element);
    removeIndex(*ind);
  }

  const T& getOnlyElement() const& {
    CHECK(size() == 1);
    return base::at(0);
  }

  T& getOnlyElement() & {
    CHECK(size() == 1);
    return base::at(0);
  }

  T getOnlyElement() && {
    CHECK(size() == 1);
    return std::move(base::at(0));
  }

  vector<T> reverse() const {
    return vector<T>(base::begin(), base::end());
  }

  template <typename Predicate>
  vector<T> filter(Predicate predicate) const& {
    vector<T> ret;
    for (const T& t : *this)
      if (predicate(t))
        ret.push_back(t);
    return ret;
  }

  template <typename Predicate>
  vector<T> filter(Predicate predicate) && {
    vector<T> ret;
    for (T& t : *this)
      if (predicate(t))
        ret.push_back(std::move(t));
    return ret;
  }

  template <typename Fun>
  auto transform(Fun fun) const -> vector<decltype(fun(*base::begin()))> {
    vector<decltype(fun(*base::begin()))> ret;
    ret.reserve(size());
    for (const auto& elem : *this)
      ret.push_back(fun(elem));
    return ret;
  }
};

template <typename T, typename Compare = std::less<T>>
class set : public std::set<T, Compare> {
  public:
  using std::set<T, Compare>::set;

  using base = std::set<T, Compare>;

  int size() const {
    return (int) base::size();
  }

  template <typename Fun>
  auto transform(Fun fun) const -> vector<decltype(fun(*base::begin()))> {
    vector<decltype(fun(*base::begin()))> ret;
    ret.reserve(size());
    for (const auto& elem : *this)
      ret.push_back(fun(elem));
    return ret;
  }
};

template<class T>
std::ostream& operator<<(std::ostream& d, const vector<T>& container){
  d << "{";
  for (const T& elem : container) {
    d << elem << ",";
  }
  d << "}";
  return d;
}
