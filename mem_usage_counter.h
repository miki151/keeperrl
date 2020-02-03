#pragma once

#include <cereal/cereal.hpp>
#include "my_containers.h"

class MemUsageArchive : public cereal::OutputArchive<MemUsageArchive> {
  public:
  MemUsageArchive() : OutputArchive<MemUsageArchive>(this) {}

  void addUsage(long long cnt) {
    for (auto& t : types)
      typeUsage[t] += cnt;
    totalUsage += cnt;
  }
  long long getUsage() const {
    return totalUsage;
  }

  template <typename T>
  void addType() {
    types.push_back(typeid(T).name());
  }

  template <typename T>
  void finishType() {
    CHECK(types.back() == typeid(T).name());
    types.pop_back();
  }

  void dumpUsage(ostream& o) {
    vector<pair<string, long long>> v;
    for (auto& elem : typeUsage)
      v.push_back(elem);
    sort(v.begin(), v.end(), [](auto& e1, auto& e2) { return e1.second > e2.second; });
    for (auto& elem : v)
      o << elem.first << " " << elem.second << "\n";
  }

  private:
  long long totalUsage = 0;
  vector<string> types;
  map<string, long long> typeUsage;
  vector<string> buffer;
};

CEREAL_REGISTER_ARCHIVE(MemUsageArchive)

template <class T>
void prologue(MemUsageArchive& ar, T const & ) {
  ar.addType<T>();
}

template <class T>
void epilogue(MemUsageArchive& ar, T const & ) {
  ar.finishType<T>();
}

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value, void>::type
serialize(MemUsageArchive & ar, T& t) {
  //ar.addUsage(sizeof(T));
}

template <class T> inline
void serialize(MemUsageArchive & ar, cereal::NameValuePair<T>& t )
{
  //ar.addUsage(sizeof(T));
}

template <class T> inline
void serialize(MemUsageArchive& ar, cereal::SizeTag<T> & t )
{
  //ar.addUsage(sizeof(cereal::SizeTag<T>));
}

template <class T> inline
void serialize(MemUsageArchive & ar, cereal::BinaryData<T> & bd) {
  //ar.addUsage(bd.size);
}

template <class T> inline
void save(MemUsageArchive & ar1, vector<T> const & bd) {
  ar1.addUsage(bd.capacity() * sizeof(T));
  for (auto& elem : bd)
    ar1(elem);
}

template <class T, size_t N> inline
void save(MemUsageArchive & ar1, std::array<T, N> const & bd) {
  for (auto& elem : bd)
    ar1(elem);
}

template <class A, class B, class C> inline
void save(MemUsageArchive & ar, std::basic_string<A, B, C> const & bd) {
  ar.addUsage(bd.capacity() * sizeof(A));
}

template <class T, class U> inline
void save(MemUsageArchive & ar1, map<T, U> const& bd) {
  ar1.addUsage((sizeof(T) + sizeof(U)) * bd.size());
  for (auto& elem : bd) {
    ar1(elem);
  }
}

template <class T, class U, class V> inline
void save(MemUsageArchive & ar1, unordered_map<T, U, V> const & bd) {
  ar1.addUsage((sizeof(T) + sizeof(U)) * bd.size());
  for (auto& elem : bd) {
    ar1(elem);
  }
}

template <class T> inline
void save(MemUsageArchive & ar1, set<T> const& bd) {
  ar1.addUsage(sizeof(T) * bd.size());
  for (auto& elem : bd) {
    ar1(elem);
  }
}

template <class T, class U> inline
void save(MemUsageArchive & ar1, unordered_set<T, U> const & bd) {
  ar1.addUsage(sizeof(T) * bd.size());
  for (auto& elem : bd) {
    ar1(elem);
  }
}

template <class T, class U> inline
void serialize(MemUsageArchive & ar1, pair<T, U> & bd) {
  ar1(bd.first, bd.second);
}

template <class T> inline
void save(MemUsageArchive & ar1, optional<T>const& bd) {
  if (!!bd)
    ar1(*bd);
}

template <class T> inline
void save(MemUsageArchive & ar1, unique_ptr<T>const& bd) {
  if (!!bd) {
    ar1.addUsage(sizeof(T));
    ar1(*bd);
  }
}
