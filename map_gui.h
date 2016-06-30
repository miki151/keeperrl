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
#include "view_index.h"
#include "entity_map.h"

class MapMemory;
class MapLayout;
class ViewObject;
class Renderer;
class CreatureView;
class Clock;
class Creature;
class Options;

class MapGui : public GuiElem {
  public:
  struct Callbacks {
    function<void(Vec2)> leftClickFun;
    function<void(Vec2)> rightClickFun;
    function<void(UniqueEntity<Creature>::Id)> creatureClickFun;
    function<void()> refreshFun;
    function<void(UniqueEntity<Creature>::Id, ViewId, Vec2)> creatureDragFun;
  };
  MapGui(Callbacks, Clock*, Options*);

  virtual void render(Renderer&) override;
  virtual bool onLeftClick(Vec2) override;
  virtual bool onRightClick(Vec2) override;
  virtual bool onMouseMove(Vec2) override;
  virtual void onMouseGone() override;
  virtual void onMouseRelease(Vec2) override;
  virtual bool onKeyPressed2(SDL::SDL_Keysym) override;

  void updateObjects(const CreatureView*, MapLayout*, bool smoothMovement, bool mouseUI, bool showMorale);
  void setSpriteMode(bool);
  optional<Vec2> getHighlightedTile(Renderer& renderer);
  void setHint(const vector<string>&);
  void addAnimation(PAnimation animation, Vec2 position);
  void setCenter(double x, double y);
  void setCenter(Vec2 pos);
  void clearCenter();
  void resetScrolling();
  bool isCentered() const;
  Vec2 getScreenPos() const;
  optional<Vec2> projectOnMap(Vec2 screenCoord);
  void highlightTeam(const vector<UniqueEntity<Creature>::Id>&);
  void unhighlightTeam(const vector<UniqueEntity<Creature>::Id>&);
  Color getCreatureHighlight(UniqueEntity<Creature>::Id, int curTime);
  void setButtonViewId(ViewId);
  void clearButtonViewId();

  private:
  void drawObjectAbs(Renderer&, Vec2 pos, const ViewObject&, Vec2 size, Vec2 tilePos, int currentTimeReal,
      const EnumMap<HighlightType, double>&);
  void drawCreatureHighlights(Renderer&, const ViewObject&, Vec2 pos, Vec2 sz, int currentTimeReal);
  void drawCreatureHighlight(Renderer&, Vec2 pos, Vec2 size, Color);
  void drawSquareHighlight(Renderer&, Vec2 pos, Vec2 size);
  void considerRedrawingSquareHighlight(Renderer&, int currentTimeReal, Vec2 pos, Vec2 size);
 // void drawFloorBorders(Renderer& r, DirSet borders, int x, int y);
  enum class HintPosition;
  void drawHint(Renderer& renderer, Color color, const vector<string>& text);
  void drawFoWSprite(Renderer&, Vec2 pos, Vec2 size, DirSet dirs);
  void renderExtraBorders(Renderer&, int currentTimeReal);
  void renderHighlights(Renderer&, Vec2 size, int currentTimeReal);
  optional<Vec2> getMousePos();
  void softScroll(double x, double y);
  struct HighlightedInfo {
    optional<Vec2> creaturePos;
    optional<Vec2> tilePos;
    optional<ViewObject> object;
    bool isEnemy;
  };
  void renderMapObjects(Renderer&, Vec2 size, HighlightedInfo&, int currentTimeReal);
  HighlightedInfo getHighlightedInfo(Renderer&, Vec2 size, int currentTimeReal);
  void renderAnimations(Renderer&, int currentTimeReal);

  Vec2 getMovementOffset(const ViewObject&, Vec2 size, double time, int curTimeReal);
  Vec2 projectOnScreen(Vec2 wpos, int currentTimeReal);
  bool considerCreatureClick(Vec2 mousePos);
  struct CreatureInfo {
    Rectangle bounds;
    UniqueEntity<Creature>::Id id;
    ViewId viewId;
  };
  optional<CreatureInfo&> getCreature(Vec2 mousePos);
  void considerMapLeftClick(Vec2 mousePos);
  MapLayout* layout;
  Table<optional<ViewIndex>> objects;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
  Callbacks callbacks;
  double lastRenderTime = 0;
  Clock* clock;
  optional<Vec2> mouseHeldPos;
  bool creatureClick = false;
  optional<Vec2> lastMapLeftClick;
  vector<string> hint;
  struct AnimationInfo {
    PAnimation animation;
    Vec2 position;
  };
  vector<AnimationInfo> animations;
  Options* options;
  DirtyTable<bool> fogOfWar;
  DirtyTable<vector<ViewId>> extraBorderPos;
  bool isFoW(Vec2 pos) const;
  struct Coords {
    double x;
    double y;
  } mouseOffset, center;
  const Level* currentLevel = nullptr;
  optional<Coords> softCenter;
  Vec2 lastMousePos;
  optional<Vec2> lastMouseMove;
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
  bool mouseUI = false;
  vector<CreatureInfo> creatureMap;
  bool showMorale;
  DirtyTable<bool> enemyPositions;
  void updateEnemyPositions(const vector<Vec2>&);
  bool lockedView = true;
  int lastRightClick = -10000;
  bool displayScrollHint = false;
  EntityMap<Creature, int> teamHighlight;
  optional<ViewId> buttonViewId;
};

#endif
