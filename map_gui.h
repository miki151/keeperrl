#ifndef _MAP_GUI
#define _MAP_GUI

#include "util.h"
#include "gui_elem.h"

class ViewIndex;
class MapMemory;
class MapLayout;
class ViewObject;

class MapGui : public GuiElem {
  public:
  MapGui(Rectangle bounds, const Table<Optional<ViewIndex>>& objects);

  virtual void render(Renderer&) override;
  virtual void onLeftClick(Vec2) override;
  virtual void onRightClick(Vec2) override;
  virtual void onMouseMove(Vec2) override;

  void refreshObjects();
  void updateObjects(const MapMemory*);
  void setLevelBounds(Rectangle bounds);
  void setLayout(MapLayout*);
  void setSpriteMode(bool);
  Optional<Vec2> getHighlightedTile(Renderer& renderer);
  void drawHint(Renderer& renderer, Color color, const string& text);

  private:
  Optional<ViewObject> drawObjectAbs(Renderer& renderer, int x, int y, const ViewIndex& index, int sizeX, int sizeY,
      Vec2 tilePos, bool highlighted);
  MapLayout* layout;
  const Table<Optional<ViewIndex>>& objects;
  const MapMemory* lastMemory = nullptr;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
};

#endif
