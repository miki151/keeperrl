#include "stdafx.h"

#include "view_index.h"


template <class Archive> 
void ViewIndex::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(objIndex)
    & BOOST_SERIALIZATION_NVP(objects)
    & BOOST_SERIALIZATION_NVP(highlight);
}

SERIALIZABLE(ViewIndex);


template <class Archive> 
void ViewIndex::HighlightInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(amount);
}

SERIALIZABLE(ViewIndex::HighlightInfo);

ViewIndex::ViewIndex() {
  objIndex = vector<int>(allLayers.size(), -1);
}
void ViewIndex::insert(const ViewObject& obj) {
  int ind = objIndex[int(obj.layer())];
  if (ind > -1)
    objects[ind] = obj;
  else {
    objIndex[int(obj.layer())] = objects.size();
    objects.push_back(obj);
  }
}

bool ViewIndex::hasObject(ViewLayer l) const {
  return objIndex[int(l)] > -1;
}

void ViewIndex::removeObject(ViewLayer l) {
  objIndex[int(l)] = -1;
}

bool ViewIndex::isEmpty() const {
  return objects.empty() && !highlight;
}

ViewObject ViewIndex::getObject(ViewLayer l) const {
  int ind = objIndex[int(l)];
  CHECK(ind > -1) << "No object on layer " << int(l);
  return objects[ind];
}

Optional<ViewObject> ViewIndex::getTopObject(const vector<ViewLayer>& layers) const {
  for (int i = layers.size() - 1; i >= 0; --i)
    if (hasObject(layers[i]))
      return getObject(layers[i]);
  return Nothing();
}

void ViewIndex::setHighlight(HighlightType h, double amount) {
  highlight = {h, amount};
}

Optional<ViewIndex::HighlightInfo> ViewIndex::getHighlight() const {
  return highlight;
}

