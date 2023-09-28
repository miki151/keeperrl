#pragma once

#include "stdafx.h"
#include "hashing.h"
#include "util.h"

template <typename Value>
class CallCache {
  public:
  CallCache(int size) : maxSize(size) {}

  typedef pair<int, int> Key;

  template <typename... Args, typename Generator>
  Value get(Generator gen, int id, Args&&...args) {
    Key key = {id, combineHash(args...)};
    if (auto elem = getValue(key))
      return *elem;
    else
      return insertValue(key, gen(std::forward<Args>(args)...));
  }

  int getSize() const {
    return cache.size();
  }

  template <typename... Args>
  bool contains(int id, Args...args) {
    return cache.count({id, combineHash(args...)});
  }

  private:
  Value& insertValue(Key key, Value value) {
    if (cache.size() >= maxSize) {
      CHECK(!lastUsed.isEmpty());
      pair<const int, Key> lru = lastUsed.getFront2();
      lastUsed.erase(lru.first);
      cache.erase(lru.second);
    }
    if (lastUsed.contains(key))
      lastUsed.erase(key);
    lastUsed.insert(key, ++cnt);
    return cache[key] = value;
  }

  optional<Value&> getValue(Key key) {
    if (auto elem = getReferenceMaybe(cache, key)) {
      lastUsed.erase(key);
      lastUsed.insert(key, ++cnt);
      return *elem;
    } else
      return none;
  }
  const int maxSize;
  HashMap<Key, Value> cache;
  BiMap<Key, int> lastUsed;
  int cnt = 0;
};
