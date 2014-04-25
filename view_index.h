#ifndef _VIEW_INDEX
#define _VIEW_INDEX

#include "view_object.h"

class ViewIndex {
  public:
  ViewIndex();
  void insert(const ViewObject& obj);
  bool hasObject(ViewLayer) const;
  void removeObject(ViewLayer);
  const ViewObject& getObject(ViewLayer) const;
  ViewObject& getObject(ViewLayer);
  Optional<ViewObject> getTopObject(const vector<ViewLayer>&) const;
  bool isEmpty() const;

  void addHighlight(HighlightType, double amount = 1);

  struct HighlightInfo {
    HighlightType type;
    double amount;

    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };

  vector<HighlightInfo> getHighlight() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;
  private:
  vector<int> SERIAL(objIndex);
  vector<ViewObject> SERIAL(objects);
  vector<HighlightInfo> SERIAL(highlight);
};

#endif
