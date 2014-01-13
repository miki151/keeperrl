#ifndef _VIEW_INDEX
#define _VIEW_INDEX

#include "view_object.h"

class ViewIndex {
  public:
  ViewIndex();
  void insert(const ViewObject& obj);
  bool hasObject(ViewLayer) const;
  void removeObject(ViewLayer);
  ViewObject getObject(ViewLayer) const;
  Optional<ViewObject> getTopObject(const vector<ViewLayer>&) const;
  bool isEmpty() const;

  void setHighlight(HighlightType, double amount = 1);

  struct HighlightInfo {
    HighlightType type;
    double amount;
  };

  Optional<HighlightInfo> getHighlight() const;

  private:
  vector<int> objIndex;
  vector<ViewObject> objects;
  Optional<HighlightInfo> highlight;
};

#endif
