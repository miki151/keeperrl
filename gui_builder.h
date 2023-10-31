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
#include "scripted_ui_data.h"
#include "minion_page_index.h"

class Clock;
class MinionAction;
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
  SGuiElem drawPlayerInventory(const PlayerInfo&, bool withKeys);
  SGuiElem drawRightBandInfo(GameInfo&);
  SGuiElem drawMinions(const CollectiveInfo&, optional<int> minionIndexDummy, const optional<TutorialInfo>&);
  SGuiElem drawBottomBandInfo(GameInfo&, int width);
  SGuiElem drawKeeperHelp(const GameInfo&);
  SGuiElem drawTextInput(SyncQueue<optional<string>>&, const string& title, const string& value, int maxLength);

  struct OverlayInfo {
    SGuiElem elem;
    enum Alignment { LEFT, TOP_LEFT, BOTTOM_LEFT, MESSAGES, MINIONS, IMMIGRATION, VILLAINS, TUTORIAL, MAP_HINT, CENTER } alignment;
  };
  SGuiElem drawPlayerOverlay(const PlayerInfo&, bool dummy);
  void drawOverlays(vector<OverlayInfo>&, const GameInfo&);
  SGuiElem drawMessages(const vector<PlayerMessage>&, int guiLength);
  SGuiElem getGameSpeedHotkeys();
  SGuiElem drawImmigrationOverlay(const vector<ImmigrantDataInfo>&, const optional<TutorialInfo>&, bool drawHelp);
  SGuiElem drawImmigrationHelp(const CollectiveInfo&);
  typedef function<void(Rectangle, optional<int>)> ItemMenuCallback;
  vector<SGuiElem> drawItemMenu(const vector<ItemInfo>&, ItemMenuCallback, bool doneBut = false);
  typedef function<void(optional<UniqueEntity<Creature>::Id>)> CreatureMenuCallback;
  SGuiElem drawCampaignMenu(SyncQueue<CampaignAction>&, View::CampaignOptions, View::CampaignMenuState&);
  SGuiElem drawRetiredDungeonMenu(SyncQueue<variant<string, bool, none_t> >&, RetiredGames&, string searchString, int maxGames);
  SGuiElem drawWarlordMinionsMenu(SyncQueue<variant<int, bool>>& queue, const vector<PlayerInfo>&,
      vector<int>& chosen, int maxCount);
  SGuiElem drawChooseSiteMenu(SyncQueue<optional<Vec2>>&, const string& message, const Campaign&, Vec2 initialPos);
  SGuiElem drawWorldmap(Semaphore&, const Campaign&, Vec2 current);
  SGuiElem drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const vector<PlayerInfo>&, const string& cancelText);
  SGuiElem drawCreatureInfo(SyncQueue<bool>&, const string& title, bool prompt, const vector<PlayerInfo>& creatures);
  SGuiElem drawCost(pair<ViewId, int>, Color = Color::WHITE);
  SGuiElem drawMinimapIcons(const GameInfo&);
  optional<int> getNumber(const string& title, Vec2 position, Range, int initial);

  struct BugReportInfo {
    string text;
    bool includeSave;
    bool includeScreenshot;
  };
  SGuiElem drawBugreportMenu(bool withSaveFile, function<void(optional<BugReportInfo>)>);

  void setCollectiveTab(CollectiveTab t);
  CollectiveTab getCollectiveTab() const;

  enum class MinionTab {
    INVENTORY,
    HELP,
  };

  void addFpsCounterTick();
  void addUpsCounterTick();
  void closeOverlayWindows();
  bool isEnlargedMinimap() const;
  void toggleEnlargedMinimap();
  void closeOverlayWindowsAndClearButton();
  bool clearActiveButton();
  void setActiveButton(int num, ViewId, optional<string> activeGroup, optional<TutorialHighlight>, bool buildingSelected);
  void setActiveGroup(const string& group, optional<TutorialHighlight>);
  optional<int> getActiveButton() const;
  GameSpeed getGameSpeed() const;
  void setGameSpeed(GameSpeed);
  Rectangle getMenuPosition(int numElems);
  int getScrollPos(int index, int count);
  void setMapGui(shared_ptr<MapGui>);
  void clearHint();
  ~GuiBuilder();
  optional<int> chooseAtMouse(const vector<string>& elems);

  bool disableClickActions = false;
  bool disableTooltip = false;
  bool playerInventoryFocused() const;
  bool mouseGone = false;

  private:
  SGuiElem withLine(int, SGuiElem);
  GuiFactory::ListBuilder withLine(int, GuiFactory::ListBuilder);
  SGuiElem getImmigrationHelpText();
  SGuiElem drawCampaignGrid(const Campaign&, optional<Vec2> initialPos, function<bool(Vec2)> selectable,
      function<void(Vec2)> selectCallback);
  void moveCampaignGridPointer(const Campaign&, int iconSize, Dir);
  Renderer& renderer;
  GuiFactory& gui;
  Clock* clock;
  Options* options;
  Callbacks callbacks;
  SGuiElem getHintCallback(const vector<string>&);
  SGuiElem getTooltip(const vector<string>&, int id, milliseconds delay = milliseconds{700}, bool forceEnableTooltip = false);
  SGuiElem getTooltip2(SGuiElem, GuiFactory::PositionFun);
  SGuiElem drawImmigrantCreature(const ImmigrantCreatureInfo&);
  vector<SGuiElem> drawPlayerAttributes(const vector<AttributeInfo>&);
  vector<SGuiElem> drawPlayerAttributes(const ViewObject::CreatureAttributes&);
  SGuiElem drawBestAttack(const BestAttack&);
  SGuiElem drawTrainingInfo(const CreatureExperienceInfo&, bool infoOnly = false);
  SGuiElem drawExperienceInfo(const CreatureExperienceInfo&);
  SGuiElem drawBuildings(const vector<CollectiveInfo::Button>&, const optional<TutorialInfo>&);
  SGuiElem bottomBandCache;
  SGuiElem drawMinionButtons(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id current, optional<TeamId> teamId);
  SGuiElem minionButtonsCache;
  int minionButtonsHash = 0;
  SGuiElem drawMinionPage(const PlayerInfo&, const optional<TutorialInfo>&);
  void updateMinionPageScroll();
  SGuiElem drawAttributesOnPage(vector<SGuiElem>);
  SGuiElem drawEquipmentAndConsumables(const PlayerInfo&, bool infoOnly = false);
  function<void(Rectangle)> getEquipmentGroupsFun(const PlayerInfo&);
  SGuiElem drawSpellsList(const vector<SpellInfo>&, GenericId creatureId, bool active);
  SGuiElem getSpellIcon(const SpellInfo&, int index, bool active, GenericId creatureId);
  vector<SGuiElem> drawEffectsList(const PlayerInfo&, bool withTooltip = true);
  SGuiElem drawMinionActions(const PlayerInfo&, const optional<TutorialInfo>&);
  function<void()> getButtonCallback(UserInput);
  void drawMiniMenu(SGuiElem, function<bool()> done, Vec2 menuPos, int width, bool darkBg);
  void drawMiniMenu(SGuiElem, bool& exit, Vec2 menuPos, int width, bool darkBg, bool resetScrolling = true);
  void drawMiniMenu(vector<SGuiElem>, vector<function<void()>>, vector<SGuiElem> tooltips, Vec2 menuPos,
      int width, bool darkBg, bool exitOnCallback = true, int* selected = nullptr, bool* exit = nullptr);
  SGuiElem getMiniMenuScrolling(const vector<SGuiElem>& activeElems, int& selected);
  SGuiElem drawTeams(const CollectiveInfo&, const optional<TutorialInfo>&, int& buttonCnt);
  SGuiElem drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease, bool leftRight);
  SGuiElem drawOptionElem(OptionId, function<void()> onChanged, function<bool()> focused);
  pair<GuiFactory::ListBuilder, vector<SGuiElem>> drawRetiredGames(RetiredGames&,
      function<void()> reloadCampaign, optional<int> maxActive,
      string searchString, function<bool(int)> isFocused);
  SGuiElem drawRetiredDungeonsButton(SyncQueue<CampaignAction>&, View::CampaignOptions, View::CampaignMenuState&);
  SGuiElem drawCampaignSettingsButton(SyncQueue<CampaignAction>&, View::CampaignOptions, View::CampaignMenuState&);
  SGuiElem drawGameModeButton(SyncQueue<CampaignAction>&, View::CampaignOptions, View::CampaignMenuState&);

  SGuiElem drawImmigrantInfo(const ImmigrantDataInfo&);
  SGuiElem drawSpecialTrait(const ImmigrantDataInfo::SpecialTraitInfo&);
  SGuiElem technologyCache;
  int technologyHash = 0;
  int bestiaryIndex = 0;
  int spellSchoolIndex = 0;
  MinionPageIndex minionPageIndex;
  optional<int> minionsIndex;
  optional<Vec2> workshopIndex;
  optional<int> helpIndex;
  optional<int> villainsIndex;
  optional<Vec2> creatureListIndex;
  int rightBandInfoHash = 0;
  SGuiElem rightBandInfoCache;
  SGuiElem immigrationCache;
  int immigrationHash = 0;
  optional<string> activeGroup;
  optional<int> activeButton;
  ScrollPosition inventoryScroll;
  ScrollPosition playerStatsScroll;
  ScrollPosition buildingsScroll;
  ScrollPosition minionButtonsScroll;
  ScrollPosition minionsScroll;
  ScrollPosition lyingItemsScroll;
  ScrollPosition miniMenuScroll;
  ScrollPosition tasksScroll;
  ScrollPosition workshopsScroll;
  ScrollPosition workshopsScroll2;
  ScrollPosition libraryScroll;
  ScrollPosition minionPageScroll;
  pair<double, double> scrollAreaScrollPos;
  void scrollWorldMap(int iconSize, Vec2, Rectangle worldMapBounds);
  optional<int> itemIndex;
  optional<Vec2> campaignGridPointer;
  bool playerOverlayFocused = false;
  optional<int> inventoryIndex;
  optional<int> abilityIndex;
  optional<int> lastPlayerPositionHash;
  optional<int> techIndex;
  int commandsIndex = -1;
  int scrollbarsHeld = GuiFactory::getHeldInitValue();
  int scrollbarsHeld2 = GuiFactory::getHeldInitValue();
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::INVENTORY;
  enum BottomWindowId {
    IMMIGRATION_HELP,
    TASKS,
    BESTIARY,
    SPELL_SCHOOLS,
    ITEMS_HELP,
    SCRIPTED_HELP,
    ENLARGED_MINIMAP
  };
  optional<BottomWindowId> bottomWindow;
  string scriptedHelpId;
  void toggleBottomWindow(BottomWindowId);
  void openScriptedHelp(string);
  GameSpeed gameSpeed = GameSpeed(0);
  const char* getGameSpeedName(GameSpeed) const;
  const char* getCurrentGameSpeedName() const;

  FpsCounter fpsCounter, upsCounter;
  enum class CounterMode { FPS, LAT, SMOD };
  CounterMode counterMode = CounterMode::FPS;

  SGuiElem getButtonLine(CollectiveInfo::Button, int num, const optional<TutorialInfo>&);
  SGuiElem drawMinionsOverlay(const CollectiveInfo::ChosenCreatureInfo&, const optional<TutorialInfo>&);
  SGuiElem minionsOverlayCache;
  int minionsOverlayHash = 0;
  SGuiElem drawWorkshopsOverlay(const CollectiveInfo::ChosenWorkshopInfo&, const optional<TutorialInfo>&,
      optional<Vec2> dummyIndex);
  void updateWorkshopIndex(const CollectiveInfo::ChosenWorkshopInfo&);
  SGuiElem workshopsOverlayCache;
  int workshopsOverlayHash = 0;
  SGuiElem drawTasksOverlay(const CollectiveInfo&);
  SGuiElem drawBuildingsOverlay(const vector<CollectiveInfo::Button>&, const optional<TutorialInfo>&);
  void renderMessages(const vector<PlayerMessage>&);
  int getNumMessageLines() const;
  SGuiElem getItemLine(const ItemInfo&, function<void(Rectangle)> onClick,
      function<void()> onMultiClick = nullptr, bool forceEnableTooltip = false, bool renderInBounds = true);
  vector<string> getItemHint(const ItemInfo&);
  SGuiElem drawMinionAndLevel(ViewIdList, int level, int iconMult);
  optional<ItemAction> getItemChoice(const ItemInfo& itemInfo, Vec2 menuPos, bool autoDefault);
  vector<SGuiElem> getMultiLine(const string& text, Color, int maxWidth, int fontSize);
  string getPlayerTitle(PlayerInfo&);
  shared_ptr<MapGui> mapGui;
  int getImmigrantAnimationOffset(milliseconds initTime);
  HeapAllocated<CallCache<SGuiElem>> cache;
  SGuiElem drawTutorialOverlay(const TutorialInfo&);
  HashSet<pair<int, TutorialHighlight>> tutorialClicks;
  bool wasTutorialClicked(size_t hash, TutorialHighlight);
  void onTutorialClicked(size_t hash, TutorialHighlight);
  SGuiElem drawLibraryContent(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawMapHintOverlay();
  SGuiElem getClickActions(const ViewObject&);
  vector<string> hint;
  SGuiElem getExpIncreaseLine(const CreatureExperienceInfo::TrainingInfo&, bool infoOnly = false);
  optional<int> highlightedTeamMember;
  SGuiElem drawRebellionOverlay(const CollectiveInfo::RebellionChance&);
  SGuiElem drawVillainsOverlay(const VillageInfo&, const optional<CollectiveInfo::NextWave>&,
      const optional<CollectiveInfo::RebellionChance>&, optional<int> villainsIndexDummy);
  void drawAllVillainsMenu(Vec2 pos, const VillageInfo&);
  SGuiElem drawVillainInfoOverlay(const VillageInfo::Village&, bool showDismissHint);
  SGuiElem drawNextWaveOverlay(const CollectiveInfo::NextWave&);
  SGuiElem getVillainDismissHint(optional<VillageAction>);
  SGuiElem drawVillainType(VillainType);
  SGuiElem drawLyingItemsList(const string& title, const ItemCounts&, int maxWidth);
  SGuiElem drawTickBox(shared_ptr<bool> value, const string& title);
  function<void(Rectangle)> getItemUpgradeCallback(const CollectiveInfo::QueuedItemInfo&);
  SGuiElem drawItemUpgradeButton(const CollectiveInfo::QueuedItemInfo&);
  function<void(Rectangle)> getItemCountCallback(const CollectiveInfo::QueuedItemInfo&);
  SGuiElem drawCreatureList(const vector<PlayerInfo>&, function<void(UniqueEntity<Creature>::Id)> button,
      int zoom = 2);
  SGuiElem drawScreenshotOverlay();
  SGuiElem drawTitleButton(const PlayerInfo& minion);
  SGuiElem drawKillsLabel(const PlayerInfo& minion);
  function<void(Rectangle)> getActivityButtonFun(const PlayerInfo&);
  function<void(Rectangle)> getAIButtonFun(const PlayerInfo&);
  SGuiElem drawSpellSchoolLabel(const SpellSchoolInfo&);
  SGuiElem drawResources(const vector<CollectiveInfo::Resource>&, const optional<TutorialInfo>&, int width);
  SGuiElem drawBestiaryOverlay(const vector<PlayerInfo>&, int index);
  SGuiElem drawBestiaryButtons(const vector<PlayerInfo>&, int index);
  SGuiElem drawBestiaryPage(const PlayerInfo&);
  SGuiElem drawSpellSchoolsOverlay(const vector<SpellSchoolInfo>&, int index);
  SGuiElem drawSpellSchoolButtons(const vector<SpellSchoolInfo>&, int index);
  SGuiElem drawSpellSchoolPage(const SpellSchoolInfo&);
  SGuiElem drawSpellLabel(const SpellInfo&);
  SGuiElem drawItemsHelpOverlay(const vector<ItemInfo>&);
  SGuiElem drawItemsHelpButtons(const vector<ItemInfo>&);
  SGuiElem drawTechUnlocks(const CollectiveInfo::LibraryInfo::TechInfo&);
  SGuiElem drawZLevelButton(const CurrentLevelInfo&, Color textColor);
  SGuiElem drawBoolOptionElem(OptionId, string name);
  SGuiElem drawCreatureTooltip(const PlayerInfo&);
  function<void(Rectangle)> getCommandsCallback(const vector<PlayerInfo::CommandInfo>&);
  ScriptedUIState scriptedUIState;
  ScriptedUIData scriptedUIData;
  string leftClickText();
  string rightClickText();
  bool hasController() const;
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

