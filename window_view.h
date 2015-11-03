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
#include "input_queue.h"
#include "animation.h"
#include "gui_builder.h"
#include "clock.h"

class ViewIndex;
class Options;
class Clock;
class MinimapGui;
class MapGui;

/** See view.h for documentation.*/
class WindowView: public View {
  public:
  struct ViewParams {
    Renderer& renderer;
    GuiFactory& gui;
    bool useTiles;
    Options* options;
    Clock* clock;
  };
  static View* createDefaultView(ViewParams);
  static View* createLoggingView(OutputArchive& of, ViewParams);
  static View* createReplayView(InputArchive& ifs, ViewParams);

  WindowView(ViewParams); 
  virtual void initialize() override;
  virtual void reset() override;
  virtual void displaySplash(const ProgressMeter&, SplashType, function<void()> cancelFun) override;
  virtual void clearSplash() override;

  virtual void close() override;

  virtual void refreshView() override;
  virtual void updateView(const CreatureView*, bool noRefresh) override;
  virtual void drawLevelMap(const CreatureView*) override;
  virtual void setScrollPos(Vec2 pos) override;
  virtual void resetCenter() override;
  virtual optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      MenuType = MenuType::NORMAL, double* scrollPos = nullptr,
      optional<UserInputId> exitAction = none) override;
  virtual GameTypeChoice chooseGameType() override;
  virtual optional<Vec2> chooseDirection(const string& message) override;
  virtual bool yesOrNoPrompt(const string& message, bool defaultNo) override;
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) override;
  virtual void animation(Vec2 pos, AnimationId) override;
  virtual double getGameSpeed() override;

  virtual void presentText(const string& title, const string& text) override;
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      MenuType = MenuType::NORMAL, optional<UserInputId> exitAction = none) override;
  virtual optional<int> getNumber(const string& title, int min, int max, int increments = 1) override;
  virtual optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint) override;
  virtual optional<int> chooseItem(const vector<ItemInfo>& items, double* scrollpos) override;
  virtual optional<UniqueEntity<Creature>::Id> chooseRecruit(const string& title, const string& warning,
      pair<ViewId, int> budget, const vector<CreatureInfo>&, double* scrollPos) override;
  virtual optional<UniqueEntity<Item>::Id> chooseTradeItem(const string& title, pair<ViewId, int> budget,
      const vector<ItemInfo>&, double* scrollPos) override;
  virtual void presentHighscores(const vector<HighscoreList>&) override;
  virtual UserInput getAction() override;
  virtual bool travelInterrupt() override;
  virtual int getTimeMilli() override;
  virtual int getTimeMilliAbsolute() override;
  virtual void stopClock() override;
  virtual bool isClockStopped() override;
  virtual void continueClock() override;
  
  private:

  Renderer& renderer;
  GuiFactory& gui;
  void processEvents();
  void displayMenuSplash2();
  void displayOldSplash();
  void updateMinimap(const CreatureView*);
  void mapLeftClickFun(Vec2);
  void mapCreatureClickFun(UniqueEntity<Creature>::Id);
  void mapCreatureDragFun(UniqueEntity<Creature>::Id, ViewId, Vec2 origin);
  void mapRightClickFun(Vec2);
  Rectangle getTextInputPosition();
  optional<int> chooseFromListInternal(const string& title, const vector<ListElem>& options, int index, MenuType,
      double* scrollPos, optional<UserInputId> exitAction, optional<sf::Event::KeyEvent> exitKey,
      vector<sf::Event::KeyEvent> shortCuts);
  optional<UserInputId> getSimpleInput(sf::Event::KeyEvent key);
  void refreshViewInt(const CreatureView*, bool flipBuffer = true);
  PGuiElem drawGameChoices(optional<GameTypeChoice>& choice, optional<GameTypeChoice>& index);
  PGuiElem getTextContent(const string& title, const string& value, const string& hint);
  void rebuildGui();
  int lastGuiHash = 0;
  void drawMap();
  void propagateEvent(const Event& event, vector<GuiElem*>);
  void keyboardAction(Event::KeyEvent key);

  void drawList(const string& title, const vector<ListElem>& options, int hightlight, int setMousePos = -1);
  void refreshScreen(bool flipBuffer = true);
  void drawAndClearBuffer();
  void displayAutosaveSplash(const ProgressMeter&);

  void zoom(int dir);
  void resize(int width, int height);
  Rectangle getMapGuiBounds() const;
  Rectangle getMinimapBounds() const;
  void resetMapBounds();
  void switchTiles();

  bool considerResizeEvent(sf::Event&);

  int messageInd = 0;
  std::deque<string> currentMessage = std::deque<string>(3, "");
  bool oldMessage = false;

  GameInfo gameInfo;

  MapLayout* mapLayout;
  MapGui* mapGui;
  MinimapGui* minimapGui;
  PGuiElem mapDecoration;
  PGuiElem minimapDecoration;
  vector<PGuiElem> tempGuiElems;
  vector<PGuiElem> blockingElems;
  vector<GuiElem*> getAllGuiElems();
  vector<GuiElem*> getClickableGuiElems();
  SyncQueue<UserInput> inputQueue;

  bool gameReady = false;
  bool uiLock = false;
  atomic<bool> refreshInput;
  atomic<bool> wasRendered;

  typedef std::unique_lock<std::recursive_mutex> RenderLock;

  struct TileLayouts {
    vector<MapLayout> layouts;
    bool sprites;
  };
  TileLayouts asciiLayouts;
  TileLayouts spriteLayouts;
  TileLayouts currentTileLayout;

  function<void()> getButtonCallback(UserInput);

  std::recursive_mutex renderMutex;

  bool lockKeyboard = false;

  thread::id renderThreadId;
  stack<function<void()>> renderDialog;

  void addVoidDialog(function<void()>);

  template <class T>
  void addReturnDialog(SyncQueue<T>& q, function<T()> fun) {
    renderDialog.push([&q, fun, this] {
      q.push(fun());
      renderDialog.pop();
    });
    if (currentThreadId() == renderThreadId)
      renderDialog.top()();
  }
  class TempClockPause {
    public:
    TempClockPause(Clock* c) : clock(c) {
      if (!clock->isPaused()) {
        clock->pause();
        cont = true;
      }
    }

    ~TempClockPause() {
      if (cont)
        clock->cont();
    }

    private:
    Clock* clock;
    bool cont = false;
  };

  void getBlockingGui(Semaphore&, PGuiElem, Vec2 origin);

  template<typename T>
  T getBlockingGui(SyncQueue<T>& queue, PGuiElem elem, Vec2 origin) {
    RenderLock lock(renderMutex);
    TempClockPause pause(clock);
    if (blockingElems.empty()) {
      blockingElems.push_back(gui.darken());
      blockingElems.back()->setBounds(renderer.getSize());
    }
    blockingElems.push_back(std::move(elem));
    blockingElems.back()->setPreferredBounds(origin);
    if (currentThreadId() == renderThreadId) {
      while (queue.isEmpty())
        refreshView();
      blockingElems.clear();
      return *queue.popAsync();
    }
    lock.unlock();
    T ret = queue.pop();
    lock.lock();
    blockingElems.clear();
    return ret;
  }
  atomic<bool> splashDone;
  bool useTiles;
  Options* options;
  Clock* clock;
  GuiBuilder guiBuilder;
  void drawMenuBackground(double barState, double mouthState);
  atomic<int> fullScreenTrigger;
  atomic<int> fullScreenResolution;
  atomic<int> zoomUI;
};


#endif
