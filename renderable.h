#pragma once

class ViewObject;

class Renderable {
  public:
  Renderable(const ViewObject&);
  const ViewObject& getViewObject() const;
  ViewObject& modViewObject();

  SERIALIZATION_DECL(Renderable);

  ~Renderable();

  protected:
  void setViewObject(const ViewObject&);

  private:
  HeapAllocated<ViewObject> SERIAL(viewObject);
};

