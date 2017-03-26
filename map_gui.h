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

#pragma once

#include "util.h"
#include "gui_elem.h"
#include "view_id.h"
#include "unique_entity.h"
#include "view_index.h"
#include "entity_map.h"
#include "view_object.h"

class MapMemory;
class MapLayout;
class Renderer;
class CreatureView;
class Clock;
class Creature;
class Options;

class MapGui : public GuiElem {
  public:
  struct Callbacks {
    function<void(Vec2)> continuousLeftClickFun;
    function<void(Vec2)> leftClickFun;
    function<void(Vec2)> rightClickFun;
    function<void(UniqueEntity<Creature>::Id)> creatureClickFun;
    function<void()> refreshFun;
    function<void(UniqueEntity<Creature>::Id, ViewId, Vec2)> creatureDragFun;
    function<void(UniqueEntity<Creature>::Id, Vec2)> creatureDroppedFun;
    function<void(TeamId, Vec2)> teamDroppedFun;
  };
  MapGui(Callbacks, Clock*, Options*, GuiFactory*);

  virtual void render(Renderer&) override;
  virtual bool onLeftClick(Vec2) override;
  virtual bool onRightClick(Vec2) override;
  virtual bool onMouseMove(Vec2) override;
  virtual void onMouseGone() override;
  virtual void onMouseRelease(Vec2) override;
  virtual bool onKeyPressed2(SDL::SDL_Keysym) override;

  void updateObjects(CreatureView*, MapLayout*, bool smoothMovement, bool mouseUI, const vector<Vec2>& tutorial);
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
  bool isCreatureHighlighted(UniqueEntity<Creature>::Id);
  void setButtonViewId(ViewId);
  void clearButtonViewId();
  bool highlightMorale();
  bool highlightEnemies();
  void setHighlightMorale(bool);
  void setHighlightEnemies(bool);

  private:
  void updateObject(Vec2, CreatureView*, milliseconds currentTime);
  void drawObjectAbs(Renderer&, Vec2 pos, const ViewObject&, Vec2 size, Vec2 tilePos, milliseconds currentTimeReal,
      const EnumMap<HighlightType, double>&);
  void drawCreatureHighlights(Renderer&, const ViewObject&, Vec2 pos, Vec2 sz, milliseconds currentTimeReal);
  void drawCreatureHighlight(Renderer&, Vec2 pos, Vec2 size, Color);
  void drawSquareHighlight(Renderer&, Vec2 pos, Vec2 size);
  void considerRedrawingSquareHighlight(Renderer&, milliseconds currentTimeReal, Vec2 pos, Vec2 size);
 // void drawFloorBorders(Renderer& r, DirSet borders, int x, int y);
  enum class HintPosition;
  void drawHint(Renderer& renderer, Color color, const vector<string>& text);
  void drawFoWSprite(Renderer&, Vec2 pos, Vec2 size, DirSet dirs);
  void renderExtraBorders(Renderer&, milliseconds currentTimeReal);
  void renderHighlights(Renderer&, Vec2 size, milliseconds currentTimeReal, bool lowHighlights);
  optional<Vec2> getMousePos();
  void softScroll(double x, double y);
  struct HighlightedInfo {
    optional<Vec2> creaturePos;
    optional<Vec2> tilePos;
    optional<ViewObject> object;
    bool isEnemy;
  };
  void renderMapObjects(Renderer&, Vec2 size, HighlightedInfo&, milliseconds currentTimeReal);
  HighlightedInfo getHighlightedInfo(Vec2 size, milliseconds currentTimeReal);
  void renderAnimations(Renderer&, milliseconds currentTimeReal);

  Vec2 getMovementOffset(const ViewObject&, Vec2 size, double time, milliseconds curTimeReal);
  Vec2 projectOnScreen(Vec2 wpos, milliseconds currentTimeReal);
  bool considerCreatureClick(Vec2 mousePos);
  struct CreatureInfo {
    UniqueEntity<Creature>::Id id;
    ViewId viewId;
  };
  optional<CreatureInfo> getCreature(Vec2 mousePos);
  void considerContinuousLeftClick(Vec2 mousePos);
  MapLayout* layout;
  Table<optional<ViewIndex>> objects;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
  Callbacks callbacks;
  optional<milliseconds> lastRenderTime;
  Clock* clock;
  optional<Vec2> mouseHeldPos;
  optional<CreatureInfo> draggedCandidate;
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
  const Level* previousLevel = nullptr;
  const CreatureView* previousView = nullptr;
  Table<optional<milliseconds>> lastSquareUpdate;
  optional<Coords> softCenter;
  Vec2 lastMousePos;
  optional<Vec2> lastMouseMove;
  bool isScrollingNow = false;
  double currentTimeGame = 0;
  struct ScreenMovement {
    Vec2 from;
    Vec2 to;
    milliseconds startTimeReal;
    milliseconds endTimeReal;
    double startTimeGame;
    double endTimeGame;
    UniqueEntity<Creature>::Id creatureId;
  };
  optional<ScreenMovement> screenMovement;
  class ViewIdMap {
    public:
    ViewIdMap(Rectangle bounds);
    void add(Vec2 pos, ViewId);
    void remove(Vec2 pos);
    bool has(Vec2 pos, ViewId);
    void clear();

    private:
    DirtyTable<EnumSet<ViewId>> ids;
  };
  bool keyScrolling = false;
  ViewIdMap connectionMap;
  bool mouseUI = false;
  bool morale = true;
  bool enemies = true;
  DirtyTable<bool> enemyPositions;
  void updateEnemyPositions(const vector<Vec2>&);
  bool lockedView = true;
  optional<milliseconds> lastRightClick;
  bool displayScrollHint = false;
  EntityMap<Creature, int> teamHighlight;
  optional<ViewId> buttonViewId;
  set<Vec2> shadowed;
  bool isRenderedHighlight(const ViewIndex&, HighlightType);
  bool isRenderedHighlightLow(const ViewIndex&, HighlightType);
  optional<ViewId> getHighlightedFurniture();
  Color getHighlightColor(const ViewIndex&, HighlightType);
  void renderHighlight(Renderer& renderer, Vec2 pos, Vec2 size, const ViewIndex& index, HighlightType highlight);
  void renderTexturedHighlight(Renderer&, Vec2 pos, Vec2 size, Color);
  void processScrolling(milliseconds);
  GuiFactory* guiFactory;
  optional<UniqueEntity<Creature>::Id> getDraggedCreature() const;
  void setDraggedCreature(UniqueEntity<Creature>::Id, ViewId, Vec2 origin);
  vector<Vec2> tutorialHighlight;
};
