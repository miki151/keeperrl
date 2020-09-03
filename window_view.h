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
#include "view.h"
#include "map_layout.h"
#include "input_queue.h"
#include "animation.h"
#include "gui_builder.h"
#include "clock.h"
#include "sound.h"

class SoundLibrary;
class ViewIndex;
class Options;
class Clock;
class MinimapGui;
class MapGui;
class FileSharing;

/** See view.h for documentation.*/
class WindowView: public View {
  public:
  struct ViewParams {
    Renderer& renderer;
    GuiFactory& gui;
    bool useTiles;
    Options* options;
    Clock* clock;
    SoundLibrary* soundLibrary;
    FileSharing* bugreportSharing;
    DirectoryPath bugreportDir;
    string installId;
  };
  static View* createDefaultView(ViewParams);
  static View* createLoggingView(OutputArchive& of, ViewParams);
  static View* createReplayView(InputArchive& ifs, ViewParams);

  WindowView(ViewParams); 
  virtual void initialize(unique_ptr<fx::FXRenderer>, unique_ptr<FXViewManager>) override;
  virtual void reset() override;
  virtual void displaySplash(const ProgressMeter*, const string&, function<void()> cancelFun) override;
  virtual void clearSplash() override;

  virtual void close() override;

  virtual void refreshView() override;
  virtual void updateView(CreatureView*, bool noRefresh) override;
  virtual void drawLevelMap(const CreatureView*) override;
  virtual void setScrollPos(Position) override;
  virtual void resetCenter() override;
  virtual optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      MenuType = MenuType::NORMAL, ScrollPosition* scrollPos = nullptr,
      optional<UserInputId> exitAction = none) override;
  virtual optional<Vec2> chooseDirection(Vec2 playerPos, const string& message) override;
  virtual optional<Vec2> chooseTarget(Vec2 playerPos, TargetType, Table<PassableInfo> passable, const string& message) override;
  virtual bool yesOrNoPrompt(const string& message, bool defaultNo) override;
  virtual bool yesOrNoPromptBelow(const string& message, bool defaultNo) override;
  virtual void animateObject(Vec2 begin, Vec2 end, optional<ViewId> object, optional<FXInfo> fx) override;
  virtual void animation(Vec2 pos, AnimationId, Dir orientation) override;
  virtual void animation(const FXSpawnInfo&) override;
  virtual double getGameSpeed() override;
  virtual optional<int> chooseAtMouse(const vector<string>& elems) override;

  virtual void presentText(const string& title, const string& text) override;
  virtual void presentTextBelow(const string& title, const string& text) override;
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      MenuType = MenuType::NORMAL) override;
  virtual optional<int> getNumber(const string& title, Range range, int initial, int increments = 1) override;
  virtual optional<string> getText(const string& title, const string& value, int maxLength,
      const string& hint) override;
  virtual optional<int> chooseItem(const vector<ItemInfo>& items, ScrollPosition* scrollpos) override;
  virtual optional<UniqueEntity<Item>::Id> chooseTradeItem(const string& title, pair<ViewId, int> budget,
      const vector<ItemInfo>&, ScrollPosition* scrollPos) override;
  virtual optional<int> choosePillageItem(const string& title, const vector<ItemInfo>&, ScrollPosition* scrollPos) override;
  virtual optional<ExperienceType> getCreatureUpgrade(const CreatureExperienceInfo&) override;
  virtual void presentHighscores(const vector<HighscoreList>&) override;
  virtual UserInput getAction() override;
  virtual bool travelInterrupt() override;
  virtual milliseconds getTimeMilli() override;
  virtual milliseconds getTimeMilliAbsolute() override;
  virtual void stopClock() override;
  virtual bool isClockStopped() override;
  virtual void continueClock() override;
  virtual void addSound(const Sound&) override;
  virtual optional<Vec2> chooseSite(const string& message, const Campaign&, optional<Vec2> current) override;
  virtual void presentWorldmap(const Campaign&) override;
  virtual variant<AvatarChoice, AvatarMenuOption> chooseAvatar(const vector<AvatarData>&) override;
  virtual CampaignAction prepareCampaign(CampaignOptions, CampaignMenuState&) override;
  virtual bool chooseRetiredDungeon(RetiredGames&) override;
  virtual optional<UniqueEntity<Creature>::Id> chooseCreature(const string& title, const vector<CreatureInfo>&,
      const string& cancelText) override;
  virtual bool creatureInfo(const string& title, bool prompt, const vector<CreatureInfo>&) override;
  virtual optional<ModAction> getModAction(int highlighted, const vector<ModInfo>&) override;
  virtual void logMessage(const string&) override;
  virtual void setBugReportSaveCallback(BugReportSaveCallback) override;
  virtual void dungeonScreenshot(Vec2 size) override;
  virtual bool zoomUIAvailable() const override;

  private:

  Renderer& renderer;
  GuiFactory& gui;
  void processEvents();
  void displayMenuSplash2();
  void displayOldSplash();
  void updateMinimap(const CreatureView*);
  void mapContinuousLeftClickFun(Vec2);
  void mapRightClickFun(Vec2);
  Rectangle getTextInputPosition();
  optional<int> chooseFromListInternal(const string& title, const vector<ListElem>& options, optional<int> index,
      MenuType, ScrollPosition*);
  void refreshViewInt(const CreatureView*, bool flipBuffer = true);
  SGuiElem getTextContent(const string& title, const string& value, const string& hint);
  void rebuildGui();
  int lastGuiHash = 0;
  void drawMap();
  void propagateEvent(const Event& event, vector<SGuiElem>);
  void keyboardAction(const SDL::SDL_Keysym&);

  void drawList(const string& title, const vector<ListElem>& options, int hightlight, int setMousePos = -1);
  void refreshScreen(bool flipBuffer = true);
  void drawAndClearBuffer();
  void getSmallSplash(const ProgressMeter*, const string& text, function<void()> cancelFun);

  void zoom(int dir);
  void resize(int width, int height);
  Rectangle getMapGuiBounds() const;
  void switchTiles();

  bool considerResizeEvent(Event&, bool withBugReportEvent = true);

  int messageInd = 0;
  std::deque<string> currentMessage = std::deque<string>(3, "");
  bool oldMessage = false;

  GameInfo gameInfo;

  MapLayout* mapLayout;
  shared_ptr<MapGui> mapGui;
  shared_ptr<MinimapGui> minimapGui;
  SGuiElem mapDecoration;
  SGuiElem minimapDecoration;
  vector<SGuiElem> tempGuiElems;
  vector<SGuiElem> blockingElems;
  vector<SGuiElem> getAllGuiElems();
  vector<SGuiElem> getClickableGuiElems();
  SyncQueue<UserInput> inputQueue;

  bool gameReady = false;
  atomic<bool> uiLock;
  atomic<bool> refreshInput;
  atomic<bool> wasRendered;

  struct TileLayouts {
    vector<MapLayout> layouts;
    bool sprites;
  };
  TileLayouts asciiLayouts;
  TileLayouts spriteLayouts;
  TileLayouts currentTileLayout;

  function<void()> getButtonCallback(UserInput);

  recursive_mutex logMutex;

  bool lockKeyboard = false;

  thread::id renderThreadId;
  stack<function<void()>> renderDialog;

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

  void getBlockingGui(Semaphore&, SGuiElem, optional<Vec2> origin = none);
  bool isKeyPressed(SDL::SDL_Scancode);

  template<typename T>
  T getBlockingGui(SyncQueue<T>& queue, SGuiElem elem, optional<Vec2> origin = none, bool darkenBackground = true) {
    TempClockPause pause(clock);
    if (darkenBackground && blockingElems.empty()) {
      blockingElems.push_back(gui.darken());
      blockingElems.back()->setBounds(Rectangle(renderer.getSize()));
    }
    if (!origin)
      origin = (renderer.getSize() - Vec2(*elem->getPreferredWidth(), *elem->getPreferredHeight())) / 2;
    origin->y = max(0, origin->y);
    Vec2 size(*elem->getPreferredWidth(), min(renderer.getSize().y - origin->y, *elem->getPreferredHeight()));
    elem->setBounds(Rectangle(*origin, *origin + size));
    propagateMousePosition({elem});
    blockingElems.push_back(std::move(elem));
    if (currentThreadId() == renderThreadId) {
      while (queue.isEmpty())
        refreshView();
      blockingElems.clear();
      return *queue.popAsync();
    }
    T ret = queue.pop();
    blockingElems.clear();
    return ret;
  }
  atomic<bool> splashDone;
  bool useTiles;
  Options* options;
  Clock* clock;
  GuiBuilder guiBuilder;
  void drawMenuBackground(double barState, double mouthState);
  atomic<int> zoomUI;
  void playSounds(const CreatureView*);
  vector<Sound> soundQueue;
  EnumMap<SoundId, optional<milliseconds>> lastPlayed;
  SoundLibrary* soundLibrary;
  deque<string> messageLog;
  void propagateMousePosition(const vector<SGuiElem>&);
  Rectangle getEquipmentMenuPosition(int height);
  Vec2 getOverlayPosition(GuiBuilder::OverlayInfo::Alignment, int height, int width, int rightBarWidth, int bottomBarHeight);
  bool considerBugReportEvent(Event&);
  BugReportSaveCallback bugReportSaveCallback;
  FileSharing* bugreportSharing;
  DirectoryPath bugreportDir;
  string installId;
  void rebuildMinimapGui();
  fx::FXRenderer* fxRenderer;
  Vec2 getMinimapOrigin() const;
  int getMinimapWidth() const;
};
