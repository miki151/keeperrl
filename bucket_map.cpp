#include "stdafx.h"
#include "bucket_map.h"
#include "creature.h"

template <class T>
template <class Archive>
void BucketMap<T>::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(bucketSize)
    & SVAR(buckets);
}

SERIALIZABLE(BucketMap<Creature*>);

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(BucketMap<T>, BucketMap);

template<class T>
BucketMap<T>::BucketMap(int w, int h, int size)
    : bucketSize(size), buckets((w + size - 1) / size, (h + size - 1) / size) {
}

template<class T>
void BucketMap<T>::addElement(Vec2 v, T elem) {
  CHECK(!buckets[v.x / bucketSize][v.y / bucketSize].count(elem));
  buckets[v.x / bucketSize][v.y / bucketSize].insert(elem);
}

template<class T>
void BucketMap<T>::removeElement(Vec2 v, T elem) {
  CHECK(buckets[v.x / bucketSize][v.y / bucketSize].count(elem));
  buckets[v.x / bucketSize][v.y / bucketSize].erase(elem);
}

template<class T>
void BucketMap<T>::moveElement(Vec2 from, Vec2 to, T elem) {
  removeElement(from, elem);
  addElement(to, elem);
}

template<class T>
vector<T> BucketMap<T>::getElements(Rectangle area) const {
  vector<T> ret;
  Rectangle bArea(
      area.left() / bucketSize, area.top() / bucketSize,
      (area.right() - 1) / bucketSize + 1, (area.bottom() - 1) / bucketSize + 1);
  if (bArea.intersects(buckets.getBounds()))
    for (Vec2 v : bArea.intersection(buckets.getBounds()))
      for (T elem : buckets[v])
        ret.push_back(elem);
  return ret;
}

template class BucketMap<Creature*>;
