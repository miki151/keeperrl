#include "stdafx.h"

void ViewIndex::insert(const ViewObject& obj) {
  objects.erase(obj.layer());
  objects.emplace(obj.layer(), obj);
}

bool ViewIndex::hasObject(ViewLayer l) const {
  return objects.count(l);
}

ViewObject ViewIndex::getObject(ViewLayer l) {
  TRY(return objects.at(l), "no object on layer " << (int)l);
}

Optional<ViewObject> ViewIndex::getTopObject(vector<ViewLayer> layers) {
  for (auto it = allLayers.rbegin(); it != allLayers.rend(); ++it)
    if (contains(layers, *it) && hasObject(*it))
      return getObject(*it);
  return Nothing();
}

void ViewIndex::setHighlight(HighlightType h, double amount) {
  highlight = {h, amount};
}

Optional<ViewIndex::HighlightInfo> ViewIndex::getHighlight() const {
  return highlight;
}

