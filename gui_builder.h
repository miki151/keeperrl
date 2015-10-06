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
class MinionAction;
class ListElem;
struct HighscoreList;
class Options;

class GuiBuilder {
  public:
  enum class GameSpeed;
  struct Callbacks {
    function<void(UserInput)> inputCallback;
    function<void(const vector<string>&)> hintCallback;
    function<void(sf::Event::KeyEvent)> keyboardCallback;
    function<void()> refreshScreen;
  };
  GuiBuilder(Renderer&, GuiFactory&, Clock*, Options*, Callbacks);
  void reset();
  void setTilesOk(bool);
  int getStandardLineHeight() const;
  
  PGuiElem getSunlightInfoGui(GameSunlightInfo& sunlightInfo);
  PGuiElem getTurnInfoGui(int turn);
  PGuiElem drawBottomPlayerInfo(GameInfo&);
  PGuiElem drawRightPlayerInfo(PlayerInfo&);
  PGuiElem drawPlayerHelp(PlayerInfo&);
  PGuiElem drawPlayerInventory(PlayerInfo&);
  PGuiElem drawBottomBandInfo(GameInfo&);
  PGuiElem drawRightBandInfo(CollectiveInfo&, VillageInfo&);
  PGuiElem drawBuildings(CollectiveInfo&);
  PGuiElem drawTechnology(CollectiveInfo&);
  PGuiElem drawVillages(VillageInfo&);
  PGuiElem drawDeities(CollectiveInfo&);
  PGuiElem drawMinions(CollectiveInfo&);
  PGuiElem drawKeeperHelp();
  optional<string> getTextInput(const string& title, const string& value, int maxLength, const string& hint);

  struct OverlayInfo {
    PGuiElem elem;
    Vec2 size;
    enum { LEFT, TOP_RIGHT, BOTTOM_RIGHT, MESSAGES, GAME_SPEED, INVISIBLE } alignment;
  };
  void drawPlayerOverlay(vector<OverlayInfo>&, PlayerInfo&);
  void drawBandOverlay(vector<OverlayInfo>&, CollectiveInfo&);
  void drawMessages(vector<OverlayInfo>&, const vector<PlayerMessage>&, int guiLength);
  void drawGameSpeedDialog(vector<OverlayInfo>&);
  typedef function<void(optional<MinionAction>)> MinionMenuCallback;
  PGuiElem drawMinionMenu(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id& current,
      MinionMenuCallback);
  typedef function<void(Rectangle, optional<int>)> ItemMenuCallback;
  vector<PGuiElem> drawItemMenu(const vector<ItemInfo>&, ItemMenuCallback, bool doneBut = false);
  typedef function<void(optional<int>)> CreatureMenuCallback;
  PGuiElem drawRecruitMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const string& warning, pair<ViewId, int> budget, const vector<CreatureInfo>&, double* scrollPos);
  PGuiElem drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>&, const string& title,
      pair<ViewId, int> budget, const vector<ItemInfo>&, double* scrollPos);
  PGuiElem drawCost(pair<ViewId, int>, ColorId = ColorId::WHITE);
  PGuiElem drawHighscores(const vector<HighscoreList>&, Semaphore&, int& tabNum, vector<double>& scrollPos,
      bool& online);
  
  enum class CollectiveTab {
    BUILDINGS,
    MINIONS,
    TECHNOLOGY,
    VILLAGES,
    KEY_MAPPING,
  };
  
  void setCollectiveTab(CollectiveTab t);
  CollectiveTab getCollectiveTab() const;

  enum class MinionTab {
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
  Rectangle getMenuPosition(MenuType);
  Rectangle getMinionMenuPosition();
  Rectangle getEquipmentMenuPosition(int height);
  Rectangle getTextInputPosition();
  PGuiElem drawListGui(const string& title, const vector<ListElem>& options,
      MenuType, int* height, int* highlight, int* choice);
  int getScrollPos(int index, int count);

  private:
  Renderer& renderer;
  GuiFactory& gui;
  Clock* clock;
  Options* options;
  Callbacks callbacks;
  PGuiElem getHintCallback(const vector<string>&);
  PGuiElem getTooltip(const vector<string>&);
  vector<PGuiElem> drawPlayerAttributes(const vector<PlayerInfo::AttributeInfo>&);
  PGuiElem drawMinionButtons(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id& current);
  PGuiElem drawMinionPage(const PlayerInfo&, MinionMenuCallback);
  PGuiElem drawActivityButton(const PlayerInfo&, MinionMenuCallback);
  vector<PGuiElem> drawAttributesOnPage(vector<PGuiElem>&&);
  vector<PGuiElem> drawEquipmentAndConsumables(const vector<ItemInfo>&, MinionMenuCallback);
  vector<PGuiElem> drawSkillsList(const PlayerInfo&);
  vector<PGuiElem> drawSpellsList(const PlayerInfo&, bool active);
  PGuiElem getSpellIcon(const PlayerInfo::Spell&, bool active);
  vector<PGuiElem> drawEffectsList(const PlayerInfo&);
  vector<PGuiElem> drawMinionActions(const PlayerInfo&, MinionMenuCallback);
  vector<PGuiElem> joinLists(vector<PGuiElem>&&, vector<PGuiElem>&&);
  function<void()> getButtonCallback(UserInput);
  void drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width);
  void showAttackTriggers(const vector<TriggerInfo>&, Vec2 pos);
  PGuiElem getTextContent(const string& title, const string& value, const string& hint);
  PGuiElem getVillageActionButton(int villageIndex, VillageAction);
  PGuiElem getVillageStateLabel(VillageInfo::Village::State);
  vector<PGuiElem> drawRecruitList(const vector<CreatureInfo>&, CreatureMenuCallback, int budget);
  PGuiElem drawHighscorePage(const HighscoreList&, double *scrollPos);
  int activeBuilding = 0;
  bool hideBuildingOverlay = false;
  int activeLibrary = -1;
  string chosenCreature;
  bool showTasks = false;
  bool tilesOk;
  double inventoryScroll = 0;
  double playerStatsScroll = 0;
  double buildingsScroll = 0;
  double minionsScroll = 0;
  double lyingItemsScroll = 0;
  double villagesScroll = 0;
  int itemIndex = -1;
  bool playerOverlayFocused = false;
  optional<int> lastPlayerPositionHash;
  int scrollbarsHeld = GuiFactory::getHeldInitValue();
  bool disableTooltip = false;
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::INVENTORY;
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

  vector<PGuiElem> drawButtons(vector<CollectiveInfo::Button> buttons, int& active, CollectiveTab);
  PGuiElem getButtonLine(CollectiveInfo::Button, int num, int& active, CollectiveTab);
  void drawMinionsOverlay(vector<OverlayInfo>&, CollectiveInfo&);
  void drawTasksOverlay(vector<OverlayInfo>&, CollectiveInfo&);
  void drawRansomOverlay(vector<OverlayInfo>& ret, const CollectiveInfo::Ransom&);
  void drawBuildingsOverlay(vector<OverlayInfo>&, CollectiveInfo&);
  void renderMessages(const vector<PlayerMessage>&);
  int getNumMessageLines() const;
  PGuiElem getStandingGui(double standing);
  PGuiElem getItemLine(const ItemInfo&, function<void(Rectangle)> onClick,
      function<void()> onMultiClick = nullptr);
  vector<string> getItemHint(const ItemInfo&);
  bool morale = true;
  optional<ItemAction> getItemChoice(const ItemInfo& itemInfo, Vec2 menuPos, bool autoDefault);
  vector<PGuiElem> getMultiLine(const string& text, Color, MenuType, int maxWidth);
  PGuiElem menuElemMargins(PGuiElem);
  PGuiElem getHighlight(MenuType, const string& label, int height);
  vector<string> breakText(const string& text, int maxWidth);
  string getPlayerTitle(PlayerInfo&);
  Event::KeyEvent getHotkeyEvent(char);
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

#endif
