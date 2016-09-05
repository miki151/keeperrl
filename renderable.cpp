#include "stdafx.h"
#include "util.h"
#include "renderable.h"
#include "view_object.h"

Renderable::Renderable(const ViewObject& obj) : viewObject(obj) {
}

Renderable::~Renderable() {
}

SERIALIZATION_CONSTRUCTOR_IMPL(Renderable);

const ViewObject& Renderable::getViewObject() const {
  return *viewObject.get();
}

ViewObject& Renderable::modViewObject() {
  return *viewObject.get();
}

template <class Archive>
void Renderable::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(viewObject);
}

SERIALIZABLE(Renderable);

void Renderable::setViewObject(const ViewObject& obj) {
  viewObject = HeapAllocated<ViewObject>(obj);
}
