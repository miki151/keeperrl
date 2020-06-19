#pragma once

#include "util.h"

template <typename Type, typename Param>
class ReadWriteArray {
  public:
  typedef unique_ptr<Type> PType;
  typedef Type* WType;

  ReadWriteArray(Rectangle bounds) : modified(bounds, -1), readonly(bounds, -1) {}

  const Rectangle& getBounds() const {
    return modified.getBounds();
  }

  WType getWritable(Vec2 pos) {
    if (modified[pos] == -1)
      if (readonly[pos] > -1)
        putElem(pos, *getReadonly(pos));
    if (modified[pos] > -1)
      return &allModified[modified[pos]];
    else
      return nullptr;
  }

  const WType getReadonly(Vec2 pos) {
    if (readonly[pos] > -1)
      return &allReadonly[readonly[pos]];
    else if (modified[pos] > -1)
      return &allModified[modified[pos]];
    else
      return nullptr;
  }

  template <typename Generator>
  void putElem(Vec2 pos, Param param, const Generator& generator) {
    if (!readonlyMap.count(param)) {
      allReadonly.push_back(generator(param));
      CHECK(allReadonly.size() < 30000);
      readonlyMap.insert(make_pair(param, allReadonly.size() - 1));
    }
    readonly[pos] = readonlyMap.at(param);
    clearModified(pos);
  }

  void clearModified(Vec2 pos) {
    if (modified[pos] > -1) {
      const auto index = modified[pos];
      modified[pos] = -1;
      if (index < allModified.size() - 1) {
        allModified[index] = std::move(allModified.back());
        modifiedPositions[index] = modifiedPositions.back();
      }
      allModified.pop_back();
      modifiedPositions.pop_back();
      if (index < allModified.size())
        modified[modifiedPositions[index]] = index;
    }
  }

  Type* putElem(Vec2 pos, Type s) {
    clearModified(pos);
    allModified.push_back(std::move(s));
    modifiedPositions.push_back(pos);
    modified[pos] = allModified.size() - 1;
    readonly[pos] = -1;
    return &allModified.back();
  }

  void clearElem(Vec2 pos) {
    clearModified(pos);
    readonly[pos] = -1;
  }

  int getNumGenerated() const {
    return allModified.size() + readonlyMap.size();
  }

  int getNumTotal() const {
    return 0;
  }

  void shrinkToFit() {
    allReadonly.shrink_to_fit();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    if (version < 1) {
      Table<short> SERIAL(prevModified);
      vector<PType> SERIAL(prevAllModified);
      Table<short> SERIAL(prevReadonly);
      vector<PType> SERIAL(prevAllReadonly);
      Table<optional<Param>> SERIAL(types);
      ar(prevModified, prevAllModified, prevAllReadonly, prevReadonly, types);
      modified = Table<int>(prevModified.getBounds(), -1);
      readonly = Table<int>(prevModified.getBounds());
      for (auto v : prevModified.getBounds()) {
        readonly[v] = prevReadonly[v];
        if (prevModified[v] > -1) {
          allModified.push_back(*prevAllModified[prevModified[v]]);
          modified[v] = allModified.size() - 1;
          modifiedPositions.push_back(v);
        }
      }
      CHECK(allModified.size() == modifiedPositions.size());
      for (auto& elem : prevAllReadonly)
        allReadonly.push_back(std::move(*elem));
    } else
      ar(allReadonly, readonly, modified, allModified, modifiedPositions);
    ar(readonlyMap, numTotal);
  }
  SERIALIZATION_CONSTRUCTOR(ReadWriteArray)

  private:
  vector<Type> SERIAL(allModified);
  vector<Vec2> SERIAL(modifiedPositions);
  Table<int> SERIAL(modified);
  vector<Type> SERIAL(allReadonly);
  Table<int> SERIAL(readonly);
  unordered_map<Param, short, CustomHash<Param>> SERIAL(readonlyMap);
  int SERIAL(numTotal) = 0;
};

namespace cereal { namespace detail {
  template <typename T, typename U> struct Version<ReadWriteArray<T, U>>
  {
    static const std::uint32_t version;
    static std::uint32_t registerVersion()
    {
      ::cereal::detail::StaticObject<Versions>::getInstance().mapping.emplace(
           std::type_index(typeid(ReadWriteArray<T, U>)).hash_code(), 1);
      return 1;
    }
    static void unused() { (void)version; }
  }; /* end Version */
  template <typename T, typename U> const std::uint32_t Version<ReadWriteArray<T, U>>::version =
    Version<ReadWriteArray<T, U>>::registerVersion();
} } // end namespaces
