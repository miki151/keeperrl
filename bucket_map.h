#pragma once

#include "util.h"
#include "entity_set.h"
#include "indexed_vector.h"

template <typename T>
class BucketMap {
  public:
  BucketMap(int width, int height, int bucketSize);

  void addElement(Vec2, WeakPointer<T>);
  void removeElement(Vec2, WeakPointer<T>);
  void moveElement(Vec2 from, Vec2 to, WeakPointer<T>);

  vector<WeakPointer<T>> getElements(Rectangle area) const;

  SERIALIZATION_DECL(BucketMap);

  private:
  int SERIAL(bucketSize);
  Table<IndexedVector<WeakPointer<T>, typename UniqueEntity<T>::Id>> SERIAL(buckets);
};

class Creature;
class CreatureBucketMap : public BucketMap<Creature> {
  public:
  using BucketMap::BucketMap;
};
