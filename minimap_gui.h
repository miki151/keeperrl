#ifndef _MINIMAP_GUI_H
#define _MINIMAP_GUI_H

#include "util.h"
#include "gui_elem.h"
#include "texture_renderer.h"

class Level;
class CreatureView;
class WindowRenderer;

class MinimapGui : public GuiElem {
  public:

  MinimapGui(function<void()> clickFun);

  void update(const Level* level, Rectangle levelPart, const CreatureView* creature, bool printLocations = false);
  void update(const Level* level, Rectangle levelPart, Rectangle bounds,
      const CreatureView* creature, bool printLocations = false);
  void presentMap(const CreatureView* creature, Rectangle bounds, WindowRenderer& renderer,
      function<void(double, double)> clickFun);

  virtual void render(Renderer&) override;
  virtual void onLeftClick(Vec2) override;

  static void initialize();

  private:
  bool initialized = false;
  function<void()> clickFun;
};

#endif
