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

#include "gui_elem.h"
#include "game_info.h"
#include "user_input.h"

class GuiBuilder {
  public:
  typedef function<void(UserInput)> InputCallback;
  typedef function<void(const string&)> HintCallback;
  typedef function<void(sf::Event::KeyEvent)> KeyboardCallback;
  GuiBuilder(Renderer&, InputCallback, HintCallback, KeyboardCallback);
  void reset();
  void setTilesOk(bool);
  
  PGuiElem getSunlightInfoGui(GameInfo::SunlightInfo& sunlightInfo);
  PGuiElem getTurnInfoGui(int turn);
  PGuiElem drawBottomPlayerInfo(GameInfo&);
  PGuiElem drawRightPlayerInfo(GameInfo::PlayerInfo&);
  PGuiElem drawPlayerStats(GameInfo::PlayerInfo&);
  PGuiElem drawPlayerHelp(GameInfo::PlayerInfo&);
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
    enum { LEFT, RIGHT, MESSAGES } alignment;
  };
  void drawPlayerOverlay(vector<OverlayInfo>&, GameInfo::PlayerInfo&);
  void drawBandOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void drawMessages(vector<OverlayInfo>&, const vector<PlayerMessage>&, int guiLength);
  
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
    HELP,
  };

  void addFpsCounterTick();
  void closeOverlayWindows();
  int getActiveBuilding() const;
  int getActiveLibrary() const;  

  private:
  Renderer& renderer;
  InputCallback inputCallback;
  HintCallback hintCallback;
  KeyboardCallback keyboardCallback;
  PGuiElem getHintCallback(const string&);
  function<void()> getButtonCallback(UserInput);
  int activeBuilding = 0;
  int activeLibrary = -1;
  string chosenCreature;
  bool tilesOk;
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::STATS;
  
  class FpsCounter {
    public:
    int getSec();
    
    void addTick();

    int getFps();

    int lastFps = 0;
    int curSec = -1;
    int curFps = 0;
    sf::Clock clock;
  } fpsCounter;

  vector<PGuiElem> drawButtons(vector<GameInfo::BandInfo::Button> buttons, int& active, CollectiveTab);
  PGuiElem getButtonLine(GameInfo::BandInfo::Button, int num, int& active, CollectiveTab);
  void drawMinionsOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void drawBuildingsOverlay(vector<OverlayInfo>&, GameInfo::BandInfo&);
  void renderMessages(const vector<PlayerMessage>&);
  int getNumMessageLines() const;
};

#endif
