#include "stdafx.h"

#include "gui_elem.h"

GuiElem::GuiElem(Rectangle b) : bounds(b) {
}

Rectangle GuiElem::getBounds() {
  return bounds;
}

void GuiElem::setBounds(Rectangle b) {
  bounds = b;
}
