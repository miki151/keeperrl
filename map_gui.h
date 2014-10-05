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
  typedef function<void(Vec2)> ClickFun;
  MapGui(const Table<Optional<ViewIndex>>& objects, ClickFun leftClickFun, ClickFun rightClickFun);

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
  void addAnimation(PAnimation animation, Vec2 position);

  private:
  void drawObjectAbs(Renderer& renderer, int x, int y, const ViewObject&, int sizeX, int sizeY, Vec2 tilePos);
  void drawFloorBorders(Renderer& r, const EnumSet<Dir>& borders, int x, int y);
  void drawHint(Renderer& renderer, Color color, const string& text);
  void drawFoWSprite(Renderer&, Vec2 pos, int sizeX, int sizeY, EnumSet<Dir> dirs);
  MapLayout* layout;
  const Table<Optional<ViewIndex>>& objects;
  const MapMemory* lastMemory = nullptr;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
  ClickFun leftClickFun;
  ClickFun rightClickFun;
  Optional<Vec2> mouseHeldPos;
  Optional<Vec2> highlightedPos;
  string hint;
  PGuiElem optionsGui;
  int optionsHeight;
  struct AnimationInfo {
    PAnimation animation;
    Vec2 position;
  };
  vector<AnimationInfo> animations;
  DirtyTable<bool> fogOfWar;
  bool isFoW(Vec2 pos) const;
};

#endif
