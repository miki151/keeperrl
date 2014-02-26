#ifndef _GUI_ELEM
#define _GUI_ELEM

#include "renderer.h"

class GuiElem {
  public:
  GuiElem(Rectangle bounds);

  virtual void render(Renderer&) = 0;
  virtual void onLeftClick(Vec2) = 0;
  virtual void onRightClick(Vec2) = 0;
  virtual void onMouseMove(Vec2) = 0;
  virtual void onRefreshBounds() {}

  void setBounds(Rectangle);
  Rectangle getBounds();

  private:
  Rectangle bounds;
};

#endif
