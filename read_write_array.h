#pragma once

#include "util.h"

template <typename Type, typename Param, typename Generator>
class ReadWriteArray {
  public:
  typedef OwnerPointer<Type> PType;
  typedef WeakPointer<Type> WType;

  ReadWriteArray(Rectangle bounds) : modified(bounds), readonly(bounds, nullptr), types(bounds) {}

  SERIALIZE_ALL(modified, readonly, types, readonlyMap, numModified, numTotal)
  SERIALIZATION_CONSTRUCTOR(ReadWriteArray)

  const Rectangle& getBounds() const {
    return modified.getBounds();
  }

  WType getWritable(Vec2 pos) {
    if (!modified[pos])
      if (auto type = types[pos])
        putElem(pos, Generator()(*type));
    return modified[pos].get();
  }

  const WType getReadonly(Vec2 pos) const {
    return readonly[pos];
  }

  void putElem(Vec2 pos, Param param) {
    if (!readonlyMap.count(param))
      readonlyMap.insert(make_pair(param, Generator()(param)));
    if (!readonly[pos])
      ++numTotal;
    readonly[pos] = readonlyMap.at(param).get();
    modified[pos].clear();
    types[pos] = param;
  }

  void putElem(Vec2 pos, PType s) {
    if (!readonly[pos])
      ++numTotal;
    ++numModified;
    modified[pos] = std::move(s);
    readonly[pos] = modified[pos].get();
  }

  void clearElem(Vec2 pos) {
    if (readonly[pos]) {
      --numTotal;
      if (modified[pos])
        --numModified;
      types[pos] = none;
      modified[pos].clear();
      readonly[pos] = nullptr;
    }
  }

  int getNumGenerated() const {
    return numModified + readonlyMap.size();
  }

  int getNumTotal() const {
    return numTotal;
  }

  private:
  Table<PType> SERIAL(modified);
  Table<WType> SERIAL(readonly);
  Table<optional<Param>> SERIAL(types);
  unordered_map<Param, PType, CustomHash<Param>> SERIAL(readonlyMap);
  int SERIAL(numModified) = 0;
  int SERIAL(numTotal) = 0;
};

