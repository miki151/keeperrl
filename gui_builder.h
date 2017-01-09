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
#include "game_info.h"
#include "user_input.h"
#include "clock.h"
#include "scroll_position.h"

class Clock;
class MinionAction;
class ListElem;
struct HighscoreList;
class Options;
class MapGui;
class CampaignAction;
class Campaign;
class RetiredGames;
template<typename Value>
class CallCache;

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
    function<void(SDL::SDL_Keysym)> keyboard;
    function<void()> refreshScreen;
    function<void(const string&)> info;
  };
  GuiBuilder(Renderer&, GuiFactory&, Clock*, Options*, Callbacks);
  void reset();
  int getStandardLineHeight() const;
  int getImmigrationBarWidth() const;

  SGuiElem getSunlightInfoGui(GameSunlightInfo& sunlightInfo);
  SGuiElem getTurnInfoGui(int& turn);
  SGuiElem drawBottomPlayerInfo(GameInfo&);
  SGuiElem drawRightPlayerInfo(PlayerInfo&);
  SGuiElem drawPlayerInventory(PlayerInfo&);
  SGuiElem drawRightBandInfo(GameInfo&);
  SGuiElem drawTechnology(CollectiveInfo&);
  SGuiElem drawMinions(CollectiveInfo&);
  SGuiElem drawBottomBandInfo(GameInfo&);
  SGuiElem drawKeeperHelp();
  optional<string> getTextInput(const string& title, const string& value, int maxLength, const string& hint);

  struct OverlayInfo {
    SGuiElem elem;
    enum { LEFT, TOP_LEFT, BOTTOM_LEFT, MESSAGES, GAME_SPEED, INVISIBLE, MINIONS, IMMIGRATION } alignment;
  };
  SGuiElem drawPlayerOverlay(const PlayerInfo&);
  void drawOverlays(vector<OverlayInfo>&, GameInfo&);
  SGuiElem drawMessages(const vector<PlayerMessage>&, int guiLength);
  SGuiElem drawGameSpeedDialog();
  SGuiElem drawImmigrationOverlay(const CollectiveInfo&);
  SGuiElem drawImmigrationHelp(const CollectiveInfo&);
  typedef function<void(Rectangle, optional<int>)> ItemMenuCallback;
  vector<SGuiElem> drawItemMenu(const vector<ItemInfo>&, ItemMenuCallback, bool doneBut = false);
  typedef function<void(optional<UniqueEntity<Creature>::Id>)> CreatureMenuCallback;
  SGuiElem drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>&, const string& title,
      pair<ViewId, int> budget, const vector<ItemInfo>&, ScrollPosition* scrollPos);
  SGuiElem drawPillageItemMenu(SyncQueue<optional<int>>&, const string& title, const vector<ItemInfo>&,
      ScrollPosition* scrollPos);
  SGuiElem drawCampaignMenu(SyncQueue<CampaignAction>&, const Campaign&, Options*, RetiredGames&,
      optional<Vec2>& embark, bool& retiredMenu, bool& helpText);
  SGuiElem drawChooseSiteMenu(SyncQueue<optional<Vec2>>&, const string& message, const Campaign&,
      optional<Vec2>& sitePos);
  SGuiElem drawWorldmap(Semaphore& sem, const Campaign&);
  SGuiElem drawTeamLeaderMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const vector<CreatureInfo>&, const string& cancelText);
  SGuiElem drawCreaturePrompt(SyncQueue<bool>&, const string& title, const vector<CreatureInfo>& creatures);
  SGuiElem drawCost(pair<ViewId, int>, ColorId = ColorId::WHITE);
  SGuiElem drawHighscores(const vector<HighscoreList>&, Semaphore&, int& tabNum, vector<ScrollPosition>& scrollPos,
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
  void setActiveButton(CollectiveTab, int num, ViewId, optional<string> activeGroup);
  void setActiveGroup(const string&);
  optional<int> getActiveButton(CollectiveTab) const;
  GameSpeed getGameSpeed() const;
  void setGameSpeed(GameSpeed);
  bool showMorale() const;
  Rectangle getMenuPosition(MenuType, int numElems);
  Rectangle getMinionMenuPosition();
  Rectangle getEquipmentMenuPosition(int height);
  Rectangle getTextInputPosition();
  SGuiElem drawListGui(const string& title, const vector<ListElem>& options,
      MenuType, int* highlight, int* choice, vector<int>* positions);
  int getScrollPos(int index, int count);
  void setMapGui(shared_ptr<MapGui>);
  ~GuiBuilder();

  private:
  SGuiElem getImmigrationHelpText();
  SGuiElem drawCampaignGrid(const Campaign&, optional<Vec2>* markedPos, function<bool(Vec2)> activeFun,
      function<void(Vec2)> clickFun);
  Renderer& renderer;
  GuiFactory& gui;
  Clock* clock;
  Options* options;
  Callbacks callbacks;
  SGuiElem getHintCallback(const vector<string>&);
  SGuiElem getTooltip(const vector<string>&, int id);
  vector<SGuiElem> drawPlayerAttributes(const vector<PlayerInfo::AttributeInfo>&);
  SGuiElem drawPlayerLevelButton(const PlayerInfo&);
  SGuiElem getExpIncreaseLine(const PlayerInfo::LevelInfo&, ExperienceType);
  SGuiElem drawBuildings(const CollectiveInfo&);
  SGuiElem buildingsCache;
  int buildingsHash = 0;
  vector<OverlayInfo> buildingsOverlayCache;
  int buildingsOverlayHash = 0;
  SGuiElem bottomBandCache;
  SGuiElem drawMinionButtons(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id current, optional<TeamId> teamId);
  SGuiElem minionButtonsCache;
  int minionButtonsHash = 0;
  SGuiElem drawMinionPage(const PlayerInfo&);
  SGuiElem drawActivityButton(const PlayerInfo&);
  SGuiElem drawVillages(VillageInfo&);
  SGuiElem villagesCache;
  int villagesHash = 0;
  vector<SGuiElem> drawAttributesOnPage(vector<SGuiElem>&&);
  vector<SGuiElem> drawEquipmentAndConsumables(const PlayerInfo&);
  vector<SGuiElem> drawSkillsList(const PlayerInfo&);
  vector<SGuiElem> drawSpellsList(const PlayerInfo&, bool active);
  SGuiElem getSpellIcon(const PlayerInfo::Spell&, bool active);
  vector<SGuiElem> drawEffectsList(const PlayerInfo&);
  vector<SGuiElem> drawMinionActions(const PlayerInfo&);
  vector<SGuiElem> joinLists(vector<SGuiElem>&&, vector<SGuiElem>&&);
  function<void()> getButtonCallback(UserInput);
  void drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width);
  void showAttackTriggers(const vector<VillageInfo::Village::TriggerInfo>&, Vec2 pos);
  SGuiElem getTextContent(const string& title, const string& value, const string& hint);
  SGuiElem getVillageActionButton(int villageIndex, VillageInfo::Village::ActionInfo);
  SGuiElem getVillageStateLabel(VillageInfo::Village::State);
  SGuiElem drawHighscorePage(const HighscoreList&, ScrollPosition* scrollPos);
  SGuiElem drawTeams(CollectiveInfo&);
  SGuiElem drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease);
  SGuiElem drawOptionElem(Options*, OptionId, function<void()> onChanged);
  GuiFactory::ListBuilder drawRetiredGames(RetiredGames&, function<void()> reloadCampaign, bool active);
  SGuiElem drawImmigrantInfo(const ImmigrantDataInfo&);
  SGuiElem minionsCache;
  int minionsHash = 0;
  SGuiElem technologyCache;
  int technologyHash = 0;
  SGuiElem keeperHelp;
  optional<OverlayInfo> speedDialog;
  int rightBandInfoHash = 0;
  SGuiElem rightBandInfoCache;
  SGuiElem immigrationCache;
  int immigrationHash = 0;
  optional<string> activeGroup;
  struct ActiveButton {
    CollectiveTab tab;
    int num;
  };
  optional<ActiveButton> activeButton;
  bool showTasks = false;
  ScrollPosition inventoryScroll;
  ScrollPosition playerStatsScroll;
  ScrollPosition buildingsScroll;
  ScrollPosition minionButtonsScroll;
  ScrollPosition minionsScroll;
  ScrollPosition lyingItemsScroll;
  ScrollPosition villagesScroll;
  ScrollPosition tasksScroll;
  ScrollPosition workshopsScroll;
  ScrollPosition workshopsScroll2;
  ScrollPosition minionPageScroll;
  int itemIndex = -1;
  int numSeenVillains = -1;
  bool playerOverlayFocused = false;
  optional<int> lastPlayerPositionHash;
  int scrollbarsHeld = GuiFactory::getHeldInitValue();
  bool disableTooltip = false;
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::INVENTORY;
  bool gameSpeedDialogOpen = false;
  bool immigrantHelpOpen = false;
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
    Clock clock;
  } fpsCounter, upsCounter;

  SGuiElem drawButtons(vector<CollectiveInfo::Button> buttons, CollectiveTab);
  SGuiElem getButtonLine(CollectiveInfo::Button, int num, CollectiveTab);
  SGuiElem drawMinionsOverlay(const CollectiveInfo&);
  SGuiElem minionsOverlayCache;
  int minionsOverlayHash = 0;
  SGuiElem drawWorkshopsOverlay(const CollectiveInfo&);
  SGuiElem workshopsOverlayCache;
  int workshopsOverlayHash = 0;
  SGuiElem drawTasksOverlay(const CollectiveInfo&);
  SGuiElem drawRansomOverlay(const optional<CollectiveInfo::Ransom>&);
  SGuiElem drawBuildingsOverlay(const CollectiveInfo&);
  void renderMessages(const vector<PlayerMessage>&);
  int getNumMessageLines() const;
  SGuiElem getItemLine(const ItemInfo&, function<void(Rectangle)> onClick,
      function<void()> onMultiClick = nullptr);
  vector<string> getItemHint(const ItemInfo&);
  SGuiElem drawMinionAndLevel(ViewId viewId, int level, int iconMult);
  vector<SDL::SDL_Keysym> getConfirmationKeys();
  optional<ItemAction> getItemChoice(const ItemInfo& itemInfo, Vec2 menuPos, bool autoDefault);
  vector<SGuiElem> getMultiLine(const string& text, Color, MenuType, int maxWidth);
  SGuiElem menuElemMargins(SGuiElem);
  SGuiElem getHighlight(MenuType, const string& label, int height);
  string getPlayerTitle(PlayerInfo&);
  SDL::SDL_KeyboardEvent getHotkeyEvent(char);
  shared_ptr<MapGui> mapGui = nullptr;
  vector<SGuiElem> getSettingsButtons();
  int getImmigrantAnimationOffset(std::chrono::milliseconds initTime);
  HeapAllocated<CallCache<SGuiElem>> cache;
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

