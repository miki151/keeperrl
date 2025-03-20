#pragma once

#include "util.h"

template <typename T, typename Id>
class IndexedVector {
  public:
  IndexedVector() {}
  IndexedVector(vector<T>&& e) : elems(std::move(e)) {
    for (int i: All(elems))
      indexes.emplace(elems[i]->getUniqueId(), i);
  }

  IndexedVector(const IndexedVector&) = default;
  IndexedVector& operator=(const IndexedVector&) = default;

  IndexedVector(IndexedVector&&) = default;

  IndexedVector& operator = (IndexedVector&&) = default;

  IndexedVector(const vector<T>& e) : elems(e) {
    for (int i: All(elems))
      indexes.emplace(elems[i]->getUniqueId(), i);
  }

  const vector<T>& getElems() const {
    return elems;
  }

  void insertIfDoesntContain(T t) {
    if (!contains(t)) {
      indexes.emplace(t->getUniqueId(), elems.size());
      elems.push_back(std::move(t));
    }
  }

  void insert(T t) {
    CHECK(!contains(t));
    indexes.emplace(t->getUniqueId(), elems.size());
    elems.push_back(std::move(t));
  }

  bool contains(const T& t) const {
    return indexes.count(t->getUniqueId());
  }

  void removeMaybe(const T& t) {
    if (contains(t))
      remove(t);
  }

  void remove(const T& t) {
    remove(t->getUniqueId());
  }

  T remove(Id id) {
    int index = indexes.at(id);
    T ret = std::move(elems[index]);
    indexes.erase(ret->getUniqueId());
    elems[index] = std::move(elems.back());
    elems.pop_back();
    if (index < elems.size()) // checks if we haven't just removed the last element
      indexes[elems[index]->getUniqueId()] = index;
    return ret;
  }

  const T& operator[] (int index) const {
    return elems[index];
  }

  T& operator[] (int index) {
    return elems[index];
  }

  optional<const T&> fetch(Id id) const {
    auto iter = indexes.find(id);
    if (iter == indexes.end())
      return none;
    else
      return elems[iter->second];
  }

  optional<T&> fetch(Id id) {
    auto iter = indexes.find(id);
    if (iter == indexes.end())
      return none;
    else
      return elems[iter->second];
  }

  vector<T> removeAll() {
    indexes.clear();
    return std::move(elems);
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar(elems);
    if (version == 0)
      ar(indexes);
    else
      for (int i : All(elems))
        indexes.emplace(elems[i]->getUniqueId(), i);
  }

  private:
  vector<T> SERIAL(elems);
  HashMap<Id, int> SERIAL(indexes);
};
