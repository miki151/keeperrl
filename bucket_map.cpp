#include "stdafx.h"
#include "bucket_map.h"
#include "creature.h"

template <class T>
SERIALIZE_TMPL(BucketMap<T>, bucketSize, buckets)
SERIALIZABLE(BucketMap<Creature>);

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(BucketMap<T>, BucketMap);

template<class T>
BucketMap<T>::BucketMap(Vec2 size, int bucketSize)
    : bucketSize(bucketSize), buckets((size.x + bucketSize - 1) / bucketSize, (size.y + bucketSize - 1) / bucketSize) {
}

template<class T>
void BucketMap<T>::addElement(Vec2 v, T* elem) {
  CHECK(!buckets[v.x / bucketSize][v.y / bucketSize].contains(elem));
  buckets[v.x / bucketSize][v.y / bucketSize].insert(std::move(elem));
}

template<class T>
void BucketMap<T>::removeElement(Vec2 v, T* elem) {
  CHECK(buckets[v.x / bucketSize][v.y / bucketSize].contains(elem));
  buckets[v.x / bucketSize][v.y / bucketSize].remove(elem);
}

template<class T>
int BucketMap<T>::countElementsInBucket(Vec2 v) const {
  return buckets[v.x / bucketSize][v.y / bucketSize].getElems().size();
}

template<class T>
void BucketMap<T>::moveElement(Vec2 from, Vec2 to, T* elem) {
  removeElement(from, elem);
  addElement(to, elem);
}

template<class T>
vector<T*> BucketMap<T>::getElements(Rectangle area) const {
  vector<T*> ret;
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
