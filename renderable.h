#ifndef _RENDERABLE_H
#define _RENDERABLE_H

class ViewObject;

class Renderable {
  public:
  Renderable(const ViewObject&);
  const ViewObject& getViewObject() const;

  SERIALIZATION_DECL(Renderable);

  ~Renderable();

  protected:
  ViewObject& modViewObject();
  void setViewObject(const ViewObject&);

  private:
  PViewObject SERIAL(viewObject);
};

#endif
