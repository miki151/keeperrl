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
#include "view_id.h"
#include "unique_entity.h"

class ViewIndex;
class MapMemory;
class MapLayout;
class ViewObject;
class Renderer;
class CreatureView;
class Clock;

class MapGui : public GuiElem {
  public:
  struct Callbacks {
    function<void(Vec2)> leftClickFun;
    function<void(Vec2)> rightClickFun;
    function<void()> refreshFun;
  };
  MapGui(Callbacks, Clock*);

  virtual void render(Renderer&) override;
  virtual bool onLeftClick(Vec2) override;
  virtual bool onRightClick(Vec2) override;
  virtual void onMouseMove(Vec2) override;
  virtual void onMouseRelease() override;
  virtual void onKeyPressed2(Event::KeyEvent) override;

  void updateObjects(const CreatureView*, MapLayout*, bool smoothMovement);
  void setSpriteMode(bool);
  optional<Vec2> getHighlightedTile(Renderer& renderer);
  void setHint(const string&);
  void addAnimation(PAnimation animation, Vec2 position);
  void setCenter(double x, double y);
  void setCenter(Vec2 pos);
  void clearCenter();
  bool isCentered() const;
  Vec2 getScreenPos() const;

  private:
  void drawObjectAbs(Renderer&, int x, int y, const ViewObject&, int sizeX, int sizeY, Vec2 tilePos,
      int currentTimeReal);
  void drawCreatureHighlights(Renderer&, const ViewObject&, int x, int y, int sizeX, int sizeY);
  void drawFloorBorders(Renderer& r, const EnumSet<Dir>& borders, int x, int y);
  void drawHint(Renderer& renderer, Color color, const string& text);
  void drawFoWSprite(Renderer&, Vec2 pos, int sizeX, int sizeY, EnumSet<Dir> dirs);
  void renderExtraBorders(Renderer&, int currentTimeReal);
  Vec2 getMovementOffset(const ViewObject&, Vec2 size, double time, int curTimeReal);
  Vec2 projectOnScreen(Vec2 wpos, int currentTimeReal);
  MapLayout* layout;
  Table<optional<ViewIndex>> objects;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
  Callbacks callbacks;
  Clock* clock;
  optional<Vec2> mouseHeldPos;
  string hint;
  struct AnimationInfo {
    PAnimation animation;
    Vec2 position;
  };
  vector<AnimationInfo> animations;
  DirtyTable<bool> fogOfWar;
  DirtyTable<vector<ViewId>> extraBorderPos;
  bool isFoW(Vec2 pos) const;
  struct {
    double x;
    double y;
  } mouseOffset, center;
  Vec2 lastMousePos;
  bool isScrollingNow = false;
  double currentTimeGame = 0;
  struct ScreenMovement {
    Vec2 from;
    Vec2 to;
    int startTimeReal;
    int endTimeReal;
    double startTimeGame;
    double endTimeGame;
    UniqueEntity<Creature>::Id creatureId;
  };
  optional<ScreenMovement> screenMovement;
  class ViewIdMap {
    public:
    ViewIdMap(Rectangle bounds);
    void add(Vec2 pos, ViewId id);
    bool has(Vec2 pos, ViewId id);
    void clear();

    private:
    DirtyTable<EnumSet<ViewId>> ids;
  };
  bool keyScrolling = false;
  ViewIdMap connectionMap;
};

#endif
