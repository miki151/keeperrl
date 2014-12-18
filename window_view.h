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

#ifndef _WINDOW_VIEW
#define _WINDOW_VIEW

#include "util.h"
#include "view.h"
#include "map_layout.h"
#include "map_gui.h"
#include "minimap_gui.h"
#include "input_queue.h"
#include "animation.h"
#include "gui_builder.h"

class ViewIndex;

/** See view.h for documentation.*/
class WindowView: public View {
  public:
  WindowView(); 
  virtual void initialize(Options*) override;
  virtual void reset() override;
  virtual void displaySplash(const ProgressMeter&, View::SplashType) override;
  virtual void clearSplash() override;

  virtual void close() override;

  virtual void refreshView() override;
  virtual void updateView(const CreatureView*) override;
  virtual void drawLevelMap(const CreatureView*) override;
  virtual void resetCenter() override;
  virtual Optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      MenuType = View::NORMAL_MENU, int* scrollPos = nullptr,
      Optional<UserInputId> exitAction = Nothing()) override;
  virtual Optional<Vec2> chooseDirection(const string& message) override;
  virtual bool yesOrNoPrompt(const string& message) override;
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) override;
  virtual void animation(Vec2 pos, AnimationId) override;

  virtual void presentText(const string& title, const string& text) override;
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      MenuType = NORMAL_MENU, Optional<UserInputId> exitAction = Nothing()) override;
  virtual Optional<int> getNumber(const string& title, int min, int max, int increments = 1) override;
  virtual Optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint) override;

  virtual UserInput getAction() override;
  virtual bool travelInterrupt() override;
  virtual int getTimeMilli() override;
  virtual int getTimeMilliAbsolute() override;
  virtual void stopClock() override;
  virtual bool isClockStopped() override;
  virtual void continueClock() override;
  
  static Color getFireColor();
  static bool areTilesOk();


  private:

  void processEvents();
  void displayMenuSplash2();
  void displayOldSplash();
  void updateMinimap(const CreatureView*);
  void mapLeftClickFun(Vec2);
  void mapRightClickFun(Vec2);
  Rectangle getMenuPosition(View::MenuType type);
  Optional<int> chooseFromListInternal(const string& title, const vector<ListElem>& options, int index, MenuType,
      int* scrollPos, Optional<UserInputId> exitAction, Optional<sf::Event::KeyEvent> exitKey,
      vector<sf::Event::KeyEvent> shortCuts);
  Optional<UserInputId> getSimpleInput(sf::Event::KeyEvent key);
  void refreshViewInt(const CreatureView*, bool flipBuffer = true);
  void rebuildGui();
  void drawMap();
  Optional<sf::Event::KeyEvent> getEventFromMenu();
  void propagateEvent(const Event& event, vector<GuiElem*>);
  void keyboardAction(Event::KeyEvent key);

  PGuiElem drawListGui(const string& title, const vector<ListElem>& options, MenuType, int& height,
      int* highlight = nullptr, int* choice = nullptr);
  void drawList(const string& title, const vector<ListElem>& options, int hightlight, int setMousePos = -1);
  void refreshScreen(bool flipBuffer = true);
  void drawAndClearBuffer();
  Optional<Vec2> getHighlightedTile();

  Optional<ViewObject> drawObjectAbs(int x, int y, const ViewIndex&, int sizeX, int sizeY, Vec2 tilePos);
  void darkenObjectAbs(int x, int y);
  void clearMessageBox();
  void switchZoom();
  void zoom(bool out);
  void resize(int width, int height, vector<GuiElem*> gui);
  Rectangle getMapGuiBounds() const;
  Rectangle getMinimapBounds() const;
  void resetMapBounds();
  void switchTiles();

  bool considerScrollEvent(sf::Event&);
  bool considerResizeEvent(sf::Event&, vector<GuiElem*> gui);

  int messageInd = 0;
  std::deque<string> currentMessage = std::deque<string>(3, "");
  bool oldMessage = false;

  Table<Optional<ViewIndex>> objects;
  GameInfo gameInfo;

  MapLayout* mapLayout;
  MapGui* mapGui;
  MinimapGui* minimapGui;
  PGuiElem mapDecoration;
  PGuiElem minimapDecoration;
  vector<PGuiElem> tempGuiElems;
  vector<GuiElem*> getAllGuiElems();
  vector<GuiElem*> getClickableGuiElems();
  SyncQueue<UserInput> inputQueue;

  bool gameReady = false;
  std::atomic<bool> refreshInput;

  typedef std::unique_lock<std::recursive_mutex> RenderLock;

  struct TileLayouts {
    MapLayout normalLayout;
    MapLayout unzoomLayout;
    bool sprites;
  };
  TileLayouts asciiLayouts;
  TileLayouts spriteLayouts;
  TileLayouts currentTileLayout;

  function<void()> getButtonCallback(UserInput);

  std::recursive_mutex renderMutex;

  bool lockKeyboard = false;

  thread::id renderThreadId;
  function<void()> renderDialog;

  void addVoidDialog(function<void()>);

  template <class T>
  void addReturnDialog(SyncQueue<T>& q, function<T()> fun) {
    renderDialog = [&q, fun, this] {
      q.push(fun());
      renderDialog = nullptr;
    };
    if (currentThreadId() == renderThreadId)
      renderDialog();
  }
  atomic<bool> splashDone;
  GuiBuilder guiBuilder;
  Texture menuCore;
  Texture menuMouth;
  void drawMenuBackground(double barState, double mouthState);
  Texture splash1;
  Texture splash2;
  Texture loadingSplash;
  Options* options;
};


#endif
