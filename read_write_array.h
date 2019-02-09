#pragma once

#include "util.h"

template <typename Type, typename Param, typename Generator>
class ReadWriteArray {
  public:
  typedef OwnerPointer<Type> PType;
  typedef Type* WType;

  ReadWriteArray(Rectangle bounds) : modified(bounds, -1), readonly(bounds, -1), types(bounds) {}

  const Rectangle& getBounds() const {
    return modified.getBounds();
  }

  WType getWritable(Vec2 pos) {
    if (modified[pos] == -1)
      if (auto type = types[pos])
        putElem(pos, Generator()(*type));
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

  void putElem(Vec2 pos, Param param) {
    if (!readonlyMap.count(param)) {
      allReadonly.push_back(Generator()(param));
      CHECK(allReadonly.size() < 30000);
      readonlyMap.insert(make_pair(param, allReadonly.size() - 1));
    }
    readonly[pos] = readonlyMap.at(param);
    modified[pos] = -1;
    types[pos] = param;
  }

  void putElem(Vec2 pos, PType s) {
    allModified.push_back(std::move(s));
    CHECK(allModified.size() < 30000);
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

  SERIALIZE_ALL(modified, allModified, allReadonly, readonly, types, readonlyMap, numTotal)
  SERIALIZATION_CONSTRUCTOR(ReadWriteArray)

  private:
  vector<PType> SERIAL(allModified);
  Table<short> SERIAL(modified);
  vector<PType> SERIAL(allReadonly);
  Table<short> SERIAL(readonly);
  Table<optional<Param>> SERIAL(types);
  unordered_map<Param, short, CustomHash<Param>> SERIAL(readonlyMap);
  int SERIAL(numTotal) = 0;
};

