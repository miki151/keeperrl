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
#include "item_counts.h"

class MapMemory;
class MapLayout;
class Renderer;
class CreatureView;
class Clock;
class Creature;
class Options;
class TutorialInfo;
class UserInput;
class FXViewManager;
class Position;
class Tile;
struct PhylacteryInfo;

namespace fx {
  class FXRenderer;
}

class MapGui : public GuiElem {
  public:
  struct Callbacks {
    function<void(Vec2)> continuousLeftClickFun;
    function<void(Vec2)> leftClickFun;
    function<void(Vec2)> rightClickFun;
    function<void(Vec2)> middleClickFun;
    function<void()> refreshFun;
  };
  MapGui(Callbacks, SyncQueue<UserInput>&, Clock*, Options*, GuiFactory*, unique_ptr<fx::FXRenderer>,
      unique_ptr<FXViewManager>);
  ~MapGui() override;

  virtual void render(Renderer&) override;
  virtual bool onClick(MouseButtonId, Vec2) override;
  virtual bool onMouseMove(Vec2, Vec2) override;
  virtual void onMouseGone() override;
  virtual bool onKeyPressed2(SDL::SDL_Keysym) override;

  void updateObjects(CreatureView*, Renderer&, MapLayout*, bool smoothMovement, bool mouseUI, const optional<TutorialInfo>&);
  void setSpriteMode(bool);
  optional<Vec2> getHighlightedTile(Renderer& renderer);
  void addAnimation(PAnimation animation, Vec2 position);
  void addAnimation(const FXSpawnInfo&);
  void setCenter(double x, double y);
  void setCenterRatio(double x, double y);
  void setCenter(Vec2, Level* level);
  void clearCenter();
  void resetScrolling();
  bool isCentered() const;
  Vec2 getScreenPos() const;
  optional<Vec2> projectOnMap(Vec2 screenCoord);
  Vec2 projectOnScreen(Vec2 wpos);
  void highlightTeam(const vector<UniqueEntity<Creature>::Id>&);
  void unhighlightTeam(const vector<UniqueEntity<Creature>::Id>&);
  void setActiveButton(ViewId, int, bool);
  void clearActiveButton();
  static Color getHealthBarColor(double health, bool sprit);
  struct HighlightedInfo {
    optional<Vec2> creaturePos;
    optional<Vec2> tilePos;
    optional<Vec2> tileScreenPos;
    optional<ViewObject> object;
    ViewIndex viewIndex;
  };
  const HighlightedInfo& getLastHighlighted();
  bool isCreatureHighlighted(UniqueEntity<Creature>::Id);
  bool fxesAvailable() const;

  private:
  bool onLeftClick(Vec2);
  bool onRightClick(Vec2);
  bool onMiddleClick(Vec2);
  void onMouseRelease(Vec2);
  void updateObject(Vec2, CreatureView*, Renderer&, milliseconds currentTime);
  void drawObjectAbs(Renderer&, Vec2 pos, const ViewObject&, Vec2 size, Vec2 movement, Vec2 tilePos,
      milliseconds currentTimeReal, const ViewIndex&);
  void drawCreatureHighlights(Renderer&, const ViewObject&, const ViewIndex&, Vec2 pos, Vec2 sz,
      milliseconds currentTimeReal);
  void drawCreatureHighlight(Renderer&, Vec2 pos, Vec2 size, Color, const ViewIndex&);
  void drawSquareHighlight(Renderer&, Vec2 pos, Vec2 size, Color);
  void considerRedrawingSquareHighlight(Renderer&, milliseconds currentTimeReal, Vec2 pos, Vec2 size);
 // void drawFloorBorders(Renderer& r, DirSet borders, int x, int y);
  void drawFoWSprite(Renderer&, Vec2 pos, Vec2 size, DirSet dirs, DirSet diagonalDirs);
  void renderExtraBorders(Renderer&, milliseconds currentTimeReal);
  void renderHighlights(Renderer&, Vec2 size, milliseconds currentTimeReal, bool lowHighlights);
  optional<Vec2> getMousePos();
  void softScroll(double x, double y);
  void setSoftCenter(double x, double y);
  HighlightedInfo lastHighlighted;
  void renderMapObjects(Renderer&, Vec2 size, milliseconds currentTimeReal);
  HighlightedInfo getHighlightedInfo(Vec2 size, milliseconds currentTimeReal);
  void renderAnimations(Renderer&, milliseconds currentTimeReal);
  void renderAndInitFoW(Renderer&, Vec2 size);
  void renderFoWBorders(Renderer&, Vec2 size);
  void renderFloorObjects(Renderer&, Vec2 size, milliseconds currentTimeReal);
  void renderHighObjects(Renderer&, Vec2 size, milliseconds currentTimeReal);
  void renderAsciiObjects(Renderer&, Vec2 size, milliseconds currentTimeReal);
  void renderWalkingJoy(Renderer&, Vec2 size);
  Vec2 getMovementOffset(const ViewObject&, Vec2 size, double time, milliseconds curTimeReal, bool verticalMovement, Vec2 pos);
  bool considerCreatureClick(Vec2 mousePos);
  struct CreatureInfo {
    UniqueEntity<Creature>::Id id;
    ViewId viewId;
    Vec2 position;
  };
  optional<CreatureInfo> getCreature(Vec2 mousePos);
  void considerContinuousLeftClick(Vec2 mousePos);
  MapLayout* layout = nullptr;
  Table<optional<ViewIndex>> objects;
  bool spriteMode;
  Rectangle levelBounds = Rectangle(1, 1);
  Callbacks callbacks;
  SyncQueue<UserInput>& inputQueue;
  optional<milliseconds> lastScrollUpdate;
  optional<milliseconds> lastJoyScrollUpdate;
  Clock* clock;
  optional<Vec2> mouseHeldPos;
  optional<CreatureInfo> draggedCandidate;
  optional<Vec2> lastMapLeftClick;
  vector<vector<Vec2>> shortestPath;
  vector<vector<Vec2>> permaShortestPath;
  vector<PhylacteryInfo> phylacteries;
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
    double SERIAL(x);
    double SERIAL(y);
    COMPARE_ALL(x, y)
  } mouseOffset, center;
  const Level* previousLevel = nullptr;
  optional<CreatureViewCenterType> previousView;
  Table<optional<milliseconds>> lastSquareUpdate;
  optional<Coords> softCenter;
  Vec2 lastMousePos;
  optional<Vec2> lastMouseMove;
  enum class ScrollingState {
    ACTIVE,
    AFTER,
    NONE
  };
  ScrollingState scrollingState = ScrollingState::NONE;
  double currentTimeGame = 0;
  struct ScreenMovement {
    milliseconds startTimeReal;
    milliseconds endTimeReal;
    int moveCounter;
  };
  optional<ScreenMovement> screenMovement;
  Table<unordered_set<ViewId, CustomHash<ViewId>>> connectionMap;
  bool keyScrolling = false;
  bool mouseUI = false;
  optional<milliseconds> lastRightClick;
  EntityMap<Creature, int> teamHighlight;
  struct ActiveButtonInfo {
    ViewId viewId;
    int index;
    bool isBuilding;
  };
  optional<ActiveButtonInfo> activeButton;
  set<Vec2> shadowed;
  bool isRenderedHighlight(const ViewIndex&, HighlightType);
  bool isRenderedHighlightLow(Renderer&, const ViewIndex&, HighlightType);
  optional<ViewId> getHighlightedFurniture();
  Color getHighlightColor(const ViewIndex&, HighlightType);
  void renderHighlight(Renderer& renderer, Vec2 pos, Vec2 size, const ViewIndex& index, HighlightType highlight, Vec2 tilePos);
  void renderTileGas(Renderer& renderer, Vec2 pos, Vec2 size, const ViewIndex& index, Vec2 tilePos);
  void renderTexturedHighlight(Renderer&, Vec2 pos, Vec2 size, Color, ViewId viewId);
  void processScrolling(milliseconds);
  void considerScrollingToCreature();
  GuiFactory* guiFactory;
  vector<Vec2> tutorialHighlightLow;
  vector<Vec2> tutorialHighlightHigh;
  void drawHealthBar(Renderer&, Vec2 tilePos, Vec2 pos, Vec2 size, const ViewObject&, const ViewIndex& index);
  int lastMoveCounter = -1000;
  int currentMoveCounter = -1000;
  double getDistanceToEdgeRatio(Vec2);
  optional<Vec2> centeredCreaturePosition;
  DirSet getConnectionSet(Vec2 tilePos, const ViewId&, const Tile&);
  EntityMap<Creature, milliseconds> woundedInfo;
  EntityMap<Creature, int> furnitureUsageFX;
  void considerWoundedAnimation(const ViewObject&, Color&, milliseconds curTimeReal);

  // For advanced FX time control:
  //bool lastFxTurnBased = false;
  //double lastFxTimeReal = -1.0, lastFxTimeTurn = -1.0;
  unique_ptr<fx::FXRenderer> fxRenderer;
  unique_ptr<FXViewManager> fxViewManager;
  void updateFX(milliseconds currentTimeReal);
  void drawFurnitureCracks(Renderer&, Vec2 tilePos, float state, Vec2 pos, Vec2 size, const ViewIndex& index);
  optional<Vec2> selectionSize;
  void fxHighlight(Renderer&, const FXInfo&, Vec2 tilePos, const ViewIndex&);
  void renderShortestPaths(Renderer&, Vec2 tileSize);
  void renderPhylacteries(Renderer&, Vec2 tileSize, milliseconds currentTimeReal);
  void updateShortestPaths(CreatureView*, Renderer&, Vec2 tileSize, milliseconds curTimeReal);
  bool isDraggedCreature() const;
  void handleJoyScrolling(pair<double, double> dir, milliseconds time);
  Color squareHighlightColor;
  vector<Vec2> redMarks;
  vector<Vec2> greenMarks;
  optional<Vec2> playerPosition;
};
