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


#ifndef _GUI_BUILDER_H
#define _GUI_BUILDER_H

#include "util.h"
#include "gui_elem.h"
#include "game_info.h"
#include "user_input.h"

class Clock;

class GuiBuilder {
  public:
  enum class GameSpeed;
  struct Callbacks {
    function<void(UserInput)> inputCallback;
    function<void(const vector<string>&)> hintCallback;
    function<void(sf::Event::KeyEvent)> keyboardCallback;
    function<void()> refreshScreen;
  };
  GuiBuilder(Renderer&, GuiFactory&, Clock*, Callbacks);
  void reset();
  void setTilesOk(bool);
  
  PGuiElem getSunlightInfoGui(GameInfo::SunlightInfo& sunlightInfo);
  PGuiElem getTurnInfoGui(int turn);
  PGuiElem drawBottomPlayerInfo(GameInfo&);
  PGuiElem drawRightPlayerInfo(GameInfo::PlayerInfo&);
  PGuiElem drawPlayerStats(GameInfo::PlayerInfo&);
  PGuiElem drawPlayerHelp(GameInfo::PlayerInfo&);
  PGuiElem drawPlayerInventory(GameInfo::PlayerInfo&);
  PGuiElem drawBottomBandInfo(GameInfo&);
  PGuiElem drawRightBandInfo(GameInfo::BandInfo&, GameInfo::VillageInfo&);
  PGuiElem drawBuildings(GameInfo::BandInfo&);
  PGuiElem drawTechnology(GameInfo::BandInfo&);
  PGuiElem drawVillages(GameInfo::VillageInfo&);
  PGuiElem drawDeities(GameInfo::BandInfo&);
  PGuiElem drawMinions(GameInfo::BandInfo&);
  PGuiElem drawKeeperHelp();

  struct OverlayInfo {
    PGuiElem elem;
    Vec2 size;
    enum { LEFT, RIGHT, MESSAGES, GAME_SPEED, INVISIBLE } alignment;
  };
  void drawPlayerOverlay(vector<OverlayInfo>&, GameInfo::PlayerInfo&);
  void drawBandOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void drawMessages(vector<OverlayInfo>&, const vector<PlayerMessage>&, int guiLength);
  void drawGameSpeedDialog(vector<OverlayInfo>&);
  
  enum class CollectiveTab {
    BUILDINGS,
    MINIONS,
    TECHNOLOGY,
    WORKSHOP,
    VILLAGES,
    KEY_MAPPING,
  };
  
  void setCollectiveTab(CollectiveTab t);
  CollectiveTab getCollectiveTab() const;

  enum class MinionTab {
    STATS,
    INVENTORY,
    HELP,
  };

  void addFpsCounterTick();
  void addUpsCounterTick();
  void closeOverlayWindows();
  int getActiveBuilding() const;
  int getActiveLibrary() const;
  GameSpeed getGameSpeed() const;
  void setGameSpeed(GameSpeed);
  bool showMorale() const;
  Rectangle getMenuPosition(View::MenuType);
  PGuiElem drawListGui(const string& title, const vector<View::ListElem>& options,
      View::MenuType, int* height, int* highlight, int* choice);

  private:
  Renderer& renderer;
  GuiFactory& gui;
  Clock* clock;
  Callbacks callbacks;
  PGuiElem getHintCallback(const vector<string>&);
  function<void()> getButtonCallback(UserInput);
  int activeBuilding = 0;
  int activeLibrary = -1;
  string chosenCreature;
  bool showTasks = false;
  bool tilesOk;
  double inventoryScroll = 0;
  double playerStatsScroll = 0;
  double buildingsScroll = 0;
  double minionsScroll = 0;
  bool disableTooltip = false;
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::STATS;
  bool gameSpeedDialogOpen = false;
  atomic<GameSpeed> gameSpeed;
  string getGameSpeedName(GameSpeed) const;
  string getCurrentGameSpeedName() const;
  
  class FpsCounter {
    public:
    int getSec();
    
    void addTick();

    int getFps();

    int lastFps = 0;
    int curSec = -1;
    int curFps = 0;
    sf::Clock clock;
  } fpsCounter, upsCounter;

  vector<PGuiElem> drawButtons(vector<GameInfo::BandInfo::Button> buttons, int& active, CollectiveTab);
  PGuiElem getButtonLine(GameInfo::BandInfo::Button, int num, int& active, CollectiveTab);
  void drawMinionsOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void drawTasksOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void drawBuildingsOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void renderMessages(const vector<PlayerMessage>&);
  int getNumMessageLines() const;
  PGuiElem getStandingGui(double standing);
  PGuiElem getItemLine(const GameInfo::PlayerInfo::ItemInfo&, function<void(Rectangle)> onClick);
  vector<string> getItemHint(const GameInfo::PlayerInfo::ItemInfo&);
  bool morale = false;
  void handleItemChoice(const GameInfo::PlayerInfo::ItemInfo&, Vec2);
  vector<PGuiElem> getMultiLine(const string& text, Color, View::MenuType, int maxWidth);
  PGuiElem menuElemMargins(PGuiElem);
  PGuiElem getHighlight(View::MenuType, const string& label, int height);
  vector<string> breakText(const string& text, int maxWidth);
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

#endif
