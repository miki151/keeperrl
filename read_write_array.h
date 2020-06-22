#pragma once

#include "util.h"

template <typename Type, typename Param>
class ReadWriteArray {
  public:
  typedef unique_ptr<Type> PType;
  typedef Type* WType;

  ReadWriteArray(Rectangle bounds) : modified(bounds, -1), readonly(bounds, -1), types(bounds) {}

  const Rectangle& getBounds() const {
    return modified.getBounds();
  }

  WType getWritable(Vec2 pos) {
    if (modified[pos] == -1)
      if (auto type = types[pos])
        putElem(pos, unique<Type>(*getReadonly(pos)));
    if (modified[pos] > -1)
      return allModified[modified[pos]].get();
    else
      return nullptr;
  }

  const WType getReadonly(Vec2 pos) const {
    if (readonly[pos] > -1)
      return allReadonly[readonly[pos]].get();
    else if (modified[pos] > -1)
      return allModified[modified[pos]].get();
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
    modified[pos] = -1;
    types[pos] = param;
  }

  void putElem(Vec2 pos, PType s) {
    allModified.push_back(std::move(s));
    modified[pos] = allModified.size() - 1;
    readonly[pos] = -1;
  }

  void clearElem(Vec2 pos) {
    types[pos] = none;
    modified[pos] = -1;
    readonly[pos] = -1;
  }

  int getNumGenerated() const {
    return allModified.size() + readonlyMap.size();
  }

  int getNumTotal() const {
    return 0;
  }

  void shrinkToFit() {
    allModified.shrink_to_fit();
    allReadonly.shrink_to_fit();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    if (version < 1) {
      CHECK(Archive::is_loading::value);
      Table<short> SERIAL(modifiedTmp);
      ar(modifiedTmp);
      modified = Table<int>(modifiedTmp.getBounds());
      for (auto v : modifiedTmp.getBounds())
        modified[v] = modifiedTmp[v];
    } else
      ar(modified);
    ar(allModified, allReadonly, readonly, types, readonlyMap, numTotal);
  }
  SERIALIZATION_CONSTRUCTOR(ReadWriteArray)

  private:
  vector<PType> SERIAL(allModified);
  Table<int> SERIAL(modified);
  vector<PType> SERIAL(allReadonly);
  Table<short> SERIAL(readonly);
  Table<optional<Param>> SERIAL(types);
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
