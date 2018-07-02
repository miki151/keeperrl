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
#include "view.h"
#include "scroll_position.h"
#include "fps_counter.h"
#include "view_object.h"
#include "item_counts.h"

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
class MinimapGui;

RICH_ENUM(CollectiveTab,
  BUILDINGS,
  MINIONS,
  TECHNOLOGY,
  KEY_MAPPING
);

class GuiBuilder {
  public:
  enum class GameSpeed;
  struct Callbacks {
    function<void(UserInput)> input;
    function<void(SDL::SDL_Keysym)> keyboard;
    function<void()> refreshScreen;
    function<void(const string&)> info;
  };
  GuiBuilder(Renderer&, GuiFactory&, Clock*, Options*, Callbacks);
  void reset();
  int getStandardLineHeight() const;
  int getImmigrationBarWidth() const;
  int getItemLineOwnerMargin();

  SGuiElem getSunlightInfoGui(const GameSunlightInfo& sunlightInfo);
  SGuiElem getTurnInfoGui(const GlobalTime& turn);
  SGuiElem drawBottomPlayerInfo(const GameInfo&);
  SGuiElem drawRightPlayerInfo(const PlayerInfo&);
  SGuiElem drawPlayerInventory(const PlayerInfo&);
  SGuiElem drawRightBandInfo(GameInfo&);
  SGuiElem drawTechnology(CollectiveInfo&);
  SGuiElem drawMinions(CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawBottomBandInfo(GameInfo&);
  SGuiElem drawKeeperHelp();
  optional<string> getTextInput(const string& title, const string& value, int maxLength, const string& hint);

  struct OverlayInfo {
    SGuiElem elem;
    enum Alignment { LEFT, TOP_LEFT, BOTTOM_LEFT, MESSAGES, GAME_SPEED, MINIONS, IMMIGRATION, VILLAINS, TUTORIAL, MAP_HINT } alignment;
  };
  SGuiElem drawPlayerOverlay(const PlayerInfo&);
  void drawOverlays(vector<OverlayInfo>&, GameInfo&);
  SGuiElem drawMessages(const vector<PlayerMessage>&, int guiLength);
  SGuiElem drawGameSpeedDialog();
  SGuiElem drawImmigrationOverlay(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawImmigrationHelp(const CollectiveInfo&);
  typedef function<void(Rectangle, optional<int>)> ItemMenuCallback;
  vector<SGuiElem> drawItemMenu(const vector<ItemInfo>&, ItemMenuCallback, bool doneBut = false);
  typedef function<void(optional<UniqueEntity<Creature>::Id>)> CreatureMenuCallback;
  SGuiElem drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>&, const string& title,
      pair<ViewId, int> budget, const vector<ItemInfo>&, ScrollPosition* scrollPos);
  SGuiElem drawPillageItemMenu(SyncQueue<optional<int>>&, const string& title, const vector<ItemInfo>&,
      ScrollPosition* scrollPos);
  SGuiElem drawCampaignMenu(SyncQueue<CampaignAction>&, View::CampaignOptions, Options*,
      View::CampaignMenuState&);
  SGuiElem drawChooseSiteMenu(SyncQueue<optional<Vec2>>&, const string& message, const Campaign&,
      optional<Vec2>& sitePos);
  SGuiElem drawWorldmap(Semaphore&, const Campaign&);
  SGuiElem drawLevelMap(Semaphore&, const CreatureView*);
  SGuiElem drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const vector<CreatureInfo>&, const string& cancelText);
  /*SGuiElem drawTeamLeaderMenu(SyncQueue<vector<UniqueEntity<Creature>::Id>>&, const string& title,
      const vector<CreatureInfo>&);*/
  SGuiElem drawCreatureInfo(SyncQueue<bool>&, const string& title, bool prompt, const vector<CreatureInfo>& creatures);
  SGuiElem drawCost(pair<ViewId, int>, Color = Color::WHITE);
  SGuiElem drawHighscores(const vector<HighscoreList>&, Semaphore&, int& tabNum, vector<ScrollPosition>& scrollPos,
      bool& online);
  SGuiElem drawMinimapIcons(const optional<TutorialInfo>&);
  SGuiElem drawChooseNumberMenu(SyncQueue<optional<int>>&, const string& title, Range range, int initial, int increments);

  void setCollectiveTab(CollectiveTab t);
  CollectiveTab getCollectiveTab() const;

  enum class MinionTab {
    INVENTORY,
    HELP,
  };

  void addFpsCounterTick();
  void addUpsCounterTick();
  void closeOverlayWindows();
  void closeOverlayWindowsAndClearButton();
  bool clearActiveButton();
  void setActiveButton(CollectiveTab, int num, ViewId, optional<string> activeGroup, optional<TutorialHighlight>);
  void setActiveGroup(const string&, optional<TutorialHighlight>);
  optional<int> getActiveButton(CollectiveTab) const;
  GameSpeed getGameSpeed() const;
  void setGameSpeed(GameSpeed);
  bool showMorale() const;
  Rectangle getMenuPosition(MenuType, int numElems);
  Rectangle getTextInputPosition();
  SGuiElem drawListGui(const string& title, const vector<ListElem>& options,
      MenuType, optional<int>* highlight, int* choice, vector<int>* positions);
  int getScrollPos(int index, int count);
  void setMapGui(shared_ptr<MapGui>);
  void clearHint();
  ~GuiBuilder();
  optional<int> chooseAtMouse(const vector<string>& elems);

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
  vector<SGuiElem> drawPlayerAttributes(const vector<AttributeInfo>&);
  vector<SGuiElem> drawPlayerAttributes(const ViewObject::CreatureAttributes&);
  SGuiElem drawBestAttack(const BestAttack&);
  SGuiElem drawTrainingInfo(const PlayerInfo&);
  //SGuiElem getExpIncreaseLine(const PlayerInfo::LevelInfo&, ExperienceType);
  SGuiElem drawBuildings(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem bottomBandCache;
  SGuiElem drawMinionButtons(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id current, optional<TeamId> teamId);
  SGuiElem minionButtonsCache;
  int minionButtonsHash = 0;
  SGuiElem drawMinionPage(const PlayerInfo&, const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawActivityButton(const PlayerInfo&);
  SGuiElem drawAttributesOnPage(vector<SGuiElem>&&);
  SGuiElem drawEquipmentAndConsumables(const PlayerInfo&);
  vector<SGuiElem> drawSkillsList(const PlayerInfo&);
  SGuiElem drawSpellsList(const PlayerInfo&, bool active);
  SGuiElem getSpellIcon(const PlayerInfo::Spell&, bool active);
  vector<SGuiElem> drawEffectsList(const PlayerInfo&);
  vector<SGuiElem> drawMinionActions(const PlayerInfo&, const optional<TutorialInfo>&);
  function<void()> getButtonCallback(UserInput);
  void drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width, bool darkBg);
  void showAttackTriggers(const vector<VillageInfo::Village::TriggerInfo>&, Vec2 pos);
  SGuiElem getTextContent(const string& title, const string& value, const string& hint);
  SGuiElem getVillageActionButton(UniqueEntity<Collective>::Id, VillageInfo::Village::ActionInfo);
  SGuiElem drawHighscorePage(const HighscoreList&, ScrollPosition* scrollPos);
  SGuiElem drawTeams(CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease);
  SGuiElem drawOptionElem(Options*, OptionId, function<void()> onChanged, optional<string> defaultString);
  GuiFactory::ListBuilder drawRetiredGames(RetiredGames&, function<void()> reloadCampaign, optional<int> maxActive);
  SGuiElem drawImmigrantInfo(const ImmigrantDataInfo&);
  SGuiElem drawSpecialTrait(const SpecialTrait&);
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
  ScrollPosition libraryScroll;
  ScrollPosition minionPageScroll;
  optional<int> itemIndex;
  bool playerOverlayFocused = false;
  optional<int> lastPlayerPositionHash;
  int scrollbarsHeld = GuiFactory::getHeldInitValue();
  int scrollbarsHeld2 = GuiFactory::getHeldInitValue();
  bool disableTooltip = false;
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::INVENTORY;
  bool gameSpeedDialogOpen = false;
  enum BottomWindowId {
    IMMIGRATION_HELP,
    ALL_VILLAINS,
    TASKS
  };
  optional<BottomWindowId> bottomWindow;
  void toggleBottomWindow(BottomWindowId);
  atomic<GameSpeed> gameSpeed;
  const char* getGameSpeedName(GameSpeed) const;
  const char* getCurrentGameSpeedName() const;

  FpsCounter fpsCounter, upsCounter;
  enum class CounterMode { FPS, LAT, SMOD };
  CounterMode counterMode = CounterMode::FPS;

  SGuiElem getButtonLine(CollectiveInfo::Button, int num, CollectiveTab, const optional<TutorialInfo>&);
  SGuiElem drawMinionsOverlay(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem minionsOverlayCache;
  int minionsOverlayHash = 0;
  SGuiElem drawWorkshopsOverlay(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem workshopsOverlayCache;
  int workshopsOverlayHash = 0;
  SGuiElem drawTasksOverlay(const CollectiveInfo&);
  SGuiElem drawRansomOverlay(const optional<CollectiveInfo::Ransom>&);
  SGuiElem drawNextWaveOverlay(const optional<CollectiveInfo::NextWave>&);
  SGuiElem drawBuildingsOverlay(const CollectiveInfo&, const optional<TutorialInfo>&);
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
  SGuiElem getHighlight(SGuiElem line, MenuType, const string& label, int numActive, optional<int>* highlight);
  string getPlayerTitle(PlayerInfo&);
  SDL::SDL_KeyboardEvent getHotkeyEvent(char);
  shared_ptr<MapGui> mapGui;
  vector<SGuiElem> getSettingsButtons();
  int getImmigrantAnimationOffset(milliseconds initTime);
  HeapAllocated<CallCache<SGuiElem>> cache;
  SGuiElem drawMenuWarning(View::CampaignOptions::WarningType);
  SGuiElem drawTutorialOverlay(const TutorialInfo&);
  unordered_set<pair<int, TutorialHighlight>, CustomHash<pair<int, TutorialHighlight>>> tutorialClicks;
  bool wasTutorialClicked(size_t hash, TutorialHighlight);
  void onTutorialClicked(size_t hash, TutorialHighlight);
  SGuiElem drawLibraryOverlay(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawMapHintOverlay();
  SGuiElem getClickActions(const ViewObject&);
  vector<string> hint;
  SGuiElem getExpIncreaseLine(const PlayerInfo::LevelInfo&, ExperienceType);
  optional<int> highlightedTeamMember;
  SGuiElem drawQuartersButton(const PlayerInfo&, const CollectiveInfo&);
  SGuiElem drawWarningWindow(const optional<CollectiveInfo::RebellionChance>&,
      const optional<CollectiveInfo::NextWave>&);
  SGuiElem drawRebellionChanceText(CollectiveInfo::RebellionChance);
  SGuiElem drawVillainsOverlay(const VillageInfo&);
  SGuiElem drawAllVillainsOverlay(const VillageInfo&);
  SGuiElem drawVillainInfoOverlay(const VillageInfo::Village&, bool showDismissHint);
  SGuiElem drawVillainType(VillainType);
  SGuiElem drawLyingItemsList(const ItemCounts&, int maxWidth);
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

