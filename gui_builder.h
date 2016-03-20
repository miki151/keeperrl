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
class MapGui;
class CampaignAction;
class Campaign;
struct CampaignSetupInfo;

RICH_ENUM(CollectiveTab,
  BUILDINGS,
  MINIONS,
  TECHNOLOGY,
  VILLAGES,
  KEY_MAPPING
);

class GuiBuilder {
  public:
  enum class GameSpeed;
  struct Callbacks {
    function<void(UserInput)> input;
    function<void(const vector<string>&)> hint;
    function<void(sf::Event::KeyEvent)> keyboard;
    function<void()> refreshScreen;
    function<void(const string&)> info;
  };
  GuiBuilder(Renderer&, GuiFactory&, Clock*, Options*, Callbacks);
  void reset();
  int getStandardLineHeight() const;
  
  PGuiElem getSunlightInfoGui(GameSunlightInfo& sunlightInfo);
  PGuiElem getTurnInfoGui(int& turn);
  PGuiElem drawBottomPlayerInfo(GameInfo&);
  PGuiElem drawRightPlayerInfo(PlayerInfo&);
  PGuiElem drawPlayerHelp(PlayerInfo&);
  PGuiElem drawPlayerInventory(PlayerInfo&);
  PGuiElem drawRightBandInfo(CollectiveInfo&, VillageInfo&);
  PGuiElem drawTechnology(CollectiveInfo&);
  PGuiElem drawVillages(VillageInfo&);
  PGuiElem drawMinions(CollectiveInfo&);
  PGuiElem drawBottomBandInfo(GameInfo&);
  PGuiElem drawKeeperHelp();
  optional<string> getTextInput(const string& title, const string& value, int maxLength, const string& hint);

  struct OverlayInfo {
    PGuiElem elem;
    Vec2 size;
    enum { LEFT, TOP_RIGHT, BOTTOM_RIGHT, MESSAGES, GAME_SPEED, INVISIBLE, MINIONS } alignment;
  };
  void drawPlayerOverlay(vector<OverlayInfo>&, PlayerInfo&);
  void drawBandOverlay(vector<OverlayInfo>&, CollectiveInfo&);
  void drawMessages(vector<OverlayInfo>&, const vector<PlayerMessage>&, int guiLength);
  void drawGameSpeedDialog(vector<OverlayInfo>&);
  typedef function<void(Rectangle, optional<int>)> ItemMenuCallback;
  vector<PGuiElem> drawItemMenu(const vector<ItemInfo>&, ItemMenuCallback, bool doneBut = false);
  typedef function<void(optional<int>)> CreatureMenuCallback;
  PGuiElem drawRecruitMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const string& warning, pair<ViewId, int> budget, const vector<CreatureInfo>&, double* scrollPos);
  PGuiElem drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>&, const string& title,
      pair<ViewId, int> budget, const vector<ItemInfo>&, double* scrollPos);
  PGuiElem drawCampaignMenu(SyncQueue<CampaignAction>&, const Campaign&, const CampaignSetupInfo&,
      optional<Vec2>& embarkPos);
  PGuiElem drawChooseSiteMenu(SyncQueue<optional<Vec2>>&, const string& message, const Campaign&,
      optional<Vec2>& sitePos);
  PGuiElem drawCampaignGrid(const Campaign&, optional<Vec2>& markedPos, function<bool(Vec2)> activeFun);
  PGuiElem drawTeamLeaderMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const vector<CreatureInfo>&, const string& cancelText);
  PGuiElem drawCost(pair<ViewId, int>, ColorId = ColorId::WHITE);
  PGuiElem drawHighscores(const vector<HighscoreList>&, Semaphore&, int& tabNum, vector<double>& scrollPos,
      bool& online);
  
  void setCollectiveTab(CollectiveTab t);
  CollectiveTab getCollectiveTab() const;

  enum class MinionTab {
    INVENTORY,
    HELP,
  };

  void addFpsCounterTick();
  void addUpsCounterTick();
  void closeOverlayWindows();
  bool clearActiveButton();
  void setActiveButton(CollectiveTab, int num, ViewId);
  optional<int> getActiveButton(CollectiveTab) const;
  GameSpeed getGameSpeed() const;
  void setGameSpeed(GameSpeed);
  bool showMorale() const;
  Rectangle getMenuPosition(MenuType, int numElems);
  Rectangle getMinionMenuPosition();
  Rectangle getEquipmentMenuPosition(int height);
  Rectangle getTextInputPosition();
  PGuiElem drawListGui(const string& title, const vector<ListElem>& options,
      MenuType, int* height, int* highlight, int* choice);
  int getScrollPos(int index, int count);
  void setMapGui(MapGui*);

  private:
  Renderer& renderer;
  GuiFactory& gui;
  Clock* clock;
  Options* options;
  Callbacks callbacks;
  PGuiElem getHintCallback(const vector<string>&);
  PGuiElem getTooltip(const vector<string>&);
  vector<PGuiElem> drawPlayerAttributes(const vector<PlayerInfo::AttributeInfo>&);
  PGuiElem drawBuildings(CollectiveInfo&);
  PGuiElem buildingsCache;
  int buildingsHash = 0;
  PGuiElem bottomBandCache;
  PGuiElem drawMinionButtons(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id current, optional<TeamId> teamId);
  PGuiElem minionButtonsCache;
  int minionButtonsHash = 0;
  PGuiElem drawMinionPage(const PlayerInfo&);
  PGuiElem drawActivityButton(const PlayerInfo&);
  vector<PGuiElem> drawAttributesOnPage(vector<PGuiElem>&&);
  vector<PGuiElem> drawEquipmentAndConsumables(const PlayerInfo&);
  vector<PGuiElem> drawSkillsList(const PlayerInfo&);
  vector<PGuiElem> drawSpellsList(const PlayerInfo&, bool active);
  PGuiElem getSpellIcon(const PlayerInfo::Spell&, bool active);
  vector<PGuiElem> drawEffectsList(const PlayerInfo&);
  vector<PGuiElem> drawMinionActions(const PlayerInfo&);
  vector<PGuiElem> joinLists(vector<PGuiElem>&&, vector<PGuiElem>&&);
  function<void()> getButtonCallback(UserInput);
  void drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width);
  void showAttackTriggers(const vector<TriggerInfo>&, Vec2 pos);
  PGuiElem getTextContent(const string& title, const string& value, const string& hint);
  PGuiElem getVillageActionButton(int villageIndex, VillageAction);
  PGuiElem getVillageStateLabel(VillageInfo::Village::State);
  vector<PGuiElem> drawRecruitList(const vector<CreatureInfo>&, CreatureMenuCallback, int budget);
  PGuiElem drawHighscorePage(const HighscoreList&, double *scrollPos);
  PGuiElem drawTeams(CollectiveInfo&);
  PGuiElem drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease);
  PGuiElem teamCache;
  int teamHash = 0;
  optional<string> activeGroup;
  struct ActiveButton {
    CollectiveTab tab;
    int num;
  };
  optional<ActiveButton> activeButton;
  bool showTasks = false;
  double inventoryScroll = 0;
  double playerStatsScroll = 0;
  double buildingsScroll = 0;
  double minionButtonsScroll = 0;
  double minionsScroll = 0;
  double lyingItemsScroll = 0;
  double villagesScroll = 0;
  int itemIndex = -1;
  int numSeenVillains = -1;
  bool playerOverlayFocused = false;
  optional<int> lastPlayerPositionHash;
  int scrollbarsHeld = GuiFactory::getHeldInitValue();
  bool disableTooltip = false;
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::INVENTORY;
  bool gameSpeedDialogOpen = false;
  atomic<GameSpeed> gameSpeed;
  const char* getGameSpeedName(GameSpeed) const;
  const char* getCurrentGameSpeedName() const;
  
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

  vector<PGuiElem> drawButtons(vector<CollectiveInfo::Button> buttons, CollectiveTab);
  PGuiElem getButtonLine(CollectiveInfo::Button, int num, CollectiveTab);
  void drawMinionsOverlay(vector<OverlayInfo>&, CollectiveInfo&);
  PGuiElem minionsOverlayCache;
  int minionsOverlayHash = 0;
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
  MapGui* mapGui = nullptr;
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

#endif
