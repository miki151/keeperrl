/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _MAP_GUI
#define _MAP_GUI

#include "util.h"
#include "gui_elem.h"

class ViewIndex;
class MapMemory;
class MapLayout;
class ViewObject;
class WindowRenderer;

class MapGui : public GuiElem {
  public:
  MapGui(const Table<Optional<ViewIndex>>& objects, function<void(Vec2)> leftClickFun);

  virtual void render(Renderer&) override;
  virtual void onLeftClick(Vec2) override;
  virtual void onRightClick(Vec2) override;
  virtual void onMouseMove(Vec2) override;
  virtual void onMouseRelease() override;

  void refreshObjects();
  void updateObjects(const MapMemory*);
  void setLevelBounds(Rectangle bounds);
  void setLayout(MapLayout*);
  void setSpriteMode(bool);
  Optional<Vec2> getHighlightedTile(WindowRenderer& renderer);
  PGuiElem getHintCallback(const string&);
  void resetHint();
  void setOptions(const string& title, vector<PGuiElem>);
  void clearOptions();

  private:
  Optional<ViewObject> drawObjectAbs(Renderer& renderer, int x, int y, const ViewIndex& index, int sizeX, int sizeY,
      Vec2 tilePos, bool highlighted);
  void drawHint(Renderer& renderer, Color color, const string& text);
  MapLayout* layout;
  const Table<Optional<ViewIndex>>& objects;
  const MapMemory* lastMemory = nullptr;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
  function<void(Vec2)> leftClickFun;
  Optional<Vec2> mouseHeldPos;
  Optional<Vec2> highlightedPos;
  string hint;
  PGuiElem optionsGui;
  int optionsHeight;
};

#endif
