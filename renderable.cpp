#include "stdafx.h"
#include "util.h"
#include "renderable.h"
#include "view_object.h"

Renderable::Renderable(const ViewObject& obj) : viewObject(new ViewObject(obj)) {
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
  CHECK_SERIAL;
}

SERIALIZABLE(Renderable);

void Renderable::setViewObject(const ViewObject& obj) {
  viewObject.reset(new ViewObject(obj));
}

