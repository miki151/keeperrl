#include "stdafx.h"
#include "drag_and_drop.h"
#include "gui_elem.h"

void DragContainer::put(DragContent c, PGuiElem g, Vec2 o) {
  content = c;
  gui = std::move(g);
  origin = o;
}

Vec2 DragContainer::getOrigin() {
  CHECK(!!content);
  return origin;
}

optional<DragContent> DragContainer::pop() {
  auto c = content;
  content = none;
  gui.reset();
  return c;
}

GuiElem* DragContainer::getGui() {
  return gui.get();
}

const optional<DragContent>& DragContainer::getElement() const {
  return content;
}

bool DragContainer::hasElement() {
  return !!content;
}
