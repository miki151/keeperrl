#include "stdafx.h"
#include "bucket_map.h"
#include "creature.h"

template <class T>
template <class Archive>
void BucketMap<T>::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(bucketSize)
    & SVAR(buckets);
}

SERIALIZABLE(BucketMap<Creature>);

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(BucketMap<T>, BucketMap);

template<class T>
BucketMap<T>::BucketMap(int w, int h, int size)
    : bucketSize(size), buckets((w + size - 1) / size, (h + size - 1) / size) {
}

template<class T>
void BucketMap<T>::addElement(Vec2 v, WeakPointer<T> elem) {
  CHECK(!buckets[v.x / bucketSize][v.y / bucketSize].contains(elem));
  buckets[v.x / bucketSize][v.y / bucketSize].insert(std::move(elem));
}

template<class T>
void BucketMap<T>::removeElement(Vec2 v, WeakPointer<T> elem) {
  CHECK(buckets[v.x / bucketSize][v.y / bucketSize].contains(elem));
  buckets[v.x / bucketSize][v.y / bucketSize].remove(elem);
}

template<class T>
void BucketMap<T>::moveElement(Vec2 from, Vec2 to, WeakPointer<T> elem) {
  removeElement(from, elem);
  addElement(to, elem);
}

template<class T>
vector<WeakPointer<T>> BucketMap<T>::getElements(Rectangle area) const {
  vector<WeakPointer<T>> ret;
  Rectangle bArea(
      area.left() / bucketSize, area.top() / bucketSize,
      (area.right() - 1) / bucketSize + 1, (area.bottom() - 1) / bucketSize + 1);
  if (bArea.intersects(buckets.getBounds()))
    for (Vec2 v : bArea.intersection(buckets.getBounds()))
      for (auto elem : buckets[v].getElems())
        ret.push_back(elem);
  return ret;
}

template class BucketMap<Creature>;
