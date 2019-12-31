#pragma once

#include "util.h"
#include "entity_set.h"
#include "indexed_vector.h"

template <typename T>
class BucketMap {
  public:
  BucketMap(Vec2 size, int bucketSize);

  void addElement(Vec2, T*);
  void removeElement(Vec2, T*);
  void moveElement(Vec2 from, Vec2 to, T*);
  int countElementsInBucket(Vec2) const;

  vector<T*> getElements(Rectangle area) const;

  SERIALIZATION_DECL(BucketMap)

  private:
  int SERIAL(bucketSize);
  Table<IndexedVector<T*, typename UniqueEntity<T>::Id>> SERIAL(buckets);
};

class Creature;
class CreatureBucketMap : public BucketMap<Creature> {
  public:
  using BucketMap::BucketMap;
};
