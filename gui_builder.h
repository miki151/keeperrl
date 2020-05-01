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
  SGuiElem drawBottomBandInfo(GameInfo&, int width);
  SGuiElem drawKeeperHelp(const GameInfo&);
  optional<string> getTextInput(const string& title, const string& value, int maxLength, const string& hint);

  struct OverlayInfo {
    SGuiElem elem;
    enum Alignment { LEFT, TOP_LEFT, BOTTOM_LEFT, MESSAGES, GAME_SPEED, MINIONS, IMMIGRATION, VILLAINS, TUTORIAL, MAP_HINT, CENTER } alignment;
  };
  SGuiElem drawPlayerOverlay(const PlayerInfo&);
  void drawOverlays(vector<OverlayInfo>&, GameInfo&);
  SGuiElem drawMessages(const vector<PlayerMessage>&, int guiLength);
  SGuiElem drawGameSpeedDialog();
  SGuiElem drawImmigrationOverlay(const vector<ImmigrantDataInfo>&, const optional<TutorialInfo>&, bool drawHelp);
  SGuiElem drawImmigrationHelp(const CollectiveInfo&);
  typedef function<void(Rectangle, optional<int>)> ItemMenuCallback;
  vector<SGuiElem> drawItemMenu(const vector<ItemInfo>&, ItemMenuCallback, bool doneBut = false);
  typedef function<void(optional<UniqueEntity<Creature>::Id>)> CreatureMenuCallback;
  SGuiElem drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>&, const string& title,
      pair<ViewId, int> budget, const vector<ItemInfo>&, ScrollPosition* scrollPos);
  SGuiElem drawPillageItemMenu(SyncQueue<optional<int>>&, const string& title, const vector<ItemInfo>&,
      ScrollPosition* scrollPos);
  SGuiElem drawCampaignMenu(SyncQueue<CampaignAction>&, View::CampaignOptions, View::CampaignMenuState&);
  SGuiElem drawChooseSiteMenu(SyncQueue<optional<Vec2>>&, const string& message, const Campaign&,
      optional<Vec2>& sitePos);
  SGuiElem drawAvatarMenu(SyncQueue<variant<View::AvatarChoice, AvatarMenuOption>>&, const vector<View::AvatarData>&);
  SGuiElem drawWorldmap(Semaphore&, const Campaign&);
  SGuiElem drawLevelMap(Semaphore&, const CreatureView*);
  SGuiElem drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>&, const string& title,
      const vector<CreatureInfo>&, const string& cancelText);
  SGuiElem drawCreatureInfo(SyncQueue<bool>&, const string& title, bool prompt, const vector<CreatureInfo>& creatures);
  SGuiElem drawModMenu(SyncQueue<optional<ModAction>>&, int highlighted, const vector<ModInfo>&);
  SGuiElem drawCost(pair<ViewId, int>, Color = Color::WHITE);
  SGuiElem drawHighscores(const vector<HighscoreList>&, Semaphore&, int& tabNum, vector<ScrollPosition>& scrollPos,
      bool& online);
  SGuiElem drawMinimapIcons(const GameInfo&);
  SGuiElem drawChooseNumberMenu(SyncQueue<optional<int>>&, const string& title, Range range, int initial, int increments);
  SGuiElem drawCreatureUpgradeMenu(SyncQueue<optional<ExperienceType>>&, const CreatureExperienceInfo&);

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
  void closeOverlayWindowsAndClearButton();
  bool clearActiveButton();
  void setActiveButton(CollectiveTab, int num, ViewId, optional<string> activeGroup, optional<TutorialHighlight>);
  void setActiveGroup(const string&, optional<TutorialHighlight>);
  optional<int> getActiveButton(CollectiveTab) const;
  GameSpeed getGameSpeed() const;
  void setGameSpeed(GameSpeed);
  Rectangle getMenuPosition(MenuType, int numElems);
  Rectangle getTextInputPosition();
  SGuiElem drawListGui(const string& title, const vector<ListElem>& options,
      MenuType, optional<int>* highlight, int* choice, vector<int>* positions);
  SGuiElem getMainMenuLinks(const string& personalMessage, SGuiElem);
  int getScrollPos(int index, int count);
  void setMapGui(shared_ptr<MapGui>);
  void clearHint();
  ~GuiBuilder();
  optional<int> chooseAtMouse(const vector<string>& elems);

  bool disableClickActions = false;
  bool disableTooltip = false;

  private:
  SGuiElem withLine(int, SGuiElem);
  GuiFactory::ListBuilder withLine(int, GuiFactory::ListBuilder);
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
  SGuiElem getTooltip2(SGuiElem, GuiFactory::PositionFun);
  vector<SGuiElem> drawPlayerAttributes(const vector<AttributeInfo>&);
  vector<SGuiElem> drawPlayerAttributes(const ViewObject::CreatureAttributes&);
  SGuiElem drawBestAttack(const BestAttack&);
  SGuiElem drawTrainingInfo(const CreatureExperienceInfo&,
      function<void(optional<ExperienceType>)> increaseCallback = nullptr, bool infoOnly = false);
  //SGuiElem getExpIncreaseLine(const PlayerInfo::LevelInfo&, ExperienceType);
  SGuiElem drawBuildings(const vector<CollectiveInfo::Button>&, const optional<TutorialInfo>&);
  SGuiElem bottomBandCache;
  SGuiElem drawMinionButtons(const vector<PlayerInfo>&, UniqueEntity<Creature>::Id current, optional<TeamId> teamId);
  SGuiElem minionButtonsCache;
  int minionButtonsHash = 0;
  SGuiElem drawMinionPage(const PlayerInfo&, const vector<ViewId>& allQuarters, const optional<TutorialInfo>&);
  SGuiElem drawActivityButton(const PlayerInfo&);
  SGuiElem drawAttributesOnPage(vector<SGuiElem>);
  SGuiElem drawEquipmentAndConsumables(const PlayerInfo&, bool infoOnly = false);
  vector<SGuiElem> drawSkillsList(const PlayerInfo&);
  SGuiElem drawSpellsList(const vector<SpellInfo>&, GenericId creatureId, bool active);
  SGuiElem getSpellIcon(const SpellInfo&, int index, bool active, GenericId creatureId);
  vector<SGuiElem> drawEffectsList(const PlayerInfo&);
  SGuiElem drawMinionActions(const PlayerInfo&, const optional<TutorialInfo>&);
  function<void()> getButtonCallback(UserInput);
  void drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width, bool darkBg);
  void showAttackTriggers(const vector<VillageInfo::Village::TriggerInfo>&, Vec2 pos);
  SGuiElem getTextContent(const string& title, const string& value, const string& hint);
  SGuiElem getVillageActionButton(UniqueEntity<Collective>::Id, VillageInfo::Village::ActionInfo);
  SGuiElem drawHighscorePage(const HighscoreList&, ScrollPosition* scrollPos);
  SGuiElem drawTeams(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease, bool leftRight);
  SGuiElem drawOptionElem(OptionId, function<void()> onChanged, optional<string> defaultString);
  GuiFactory::ListBuilder drawRetiredGames(RetiredGames&, function<void()> reloadCampaign, optional<int> maxActive, string searchString);
  SGuiElem drawImmigrantInfo(const ImmigrantDataInfo&);
  SGuiElem drawSpecialTrait(const ImmigrantDataInfo::SpecialTraitInfo&);
  SGuiElem minionsCache;
  int minionsHash = 0;
  SGuiElem technologyCache;
  int technologyHash = 0;
  int bestiaryIndex = 0;
  int spellSchoolIndex = 0;
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
  CollectiveTab collectiveTab = CollectiveTab::BUILDINGS;
  MinionTab minionTab = MinionTab::INVENTORY;
  bool gameSpeedDialogOpen = false;
  enum BottomWindowId {
    IMMIGRATION_HELP,
    ALL_VILLAINS,
    TASKS,
    BESTIARY,
    SPELL_SCHOOLS,
    ITEMS_HELP
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
  SGuiElem drawMinionsOverlay(const CollectiveInfo::ChosenCreatureInfo&, const vector<ViewId>& allQuarters,
      const optional<TutorialInfo>&);
  SGuiElem minionsOverlayCache;
  int minionsOverlayHash = 0;
  SGuiElem drawWorkshopsOverlay(const CollectiveInfo::ChosenWorkshopInfo&, const optional<TutorialInfo>&);
  SGuiElem workshopsOverlayCache;
  int workshopsOverlayHash = 0;
  SGuiElem drawTasksOverlay(const CollectiveInfo&);
  SGuiElem drawRansomOverlay(const CollectiveInfo::Ransom&);
  SGuiElem drawNextWaveOverlay(const CollectiveInfo::NextWave&);
  SGuiElem drawBuildingsOverlay(const vector<CollectiveInfo::Button>&, bool ransom,
      const optional<TutorialInfo>&);
  void renderMessages(const vector<PlayerMessage>&);
  int getNumMessageLines() const;
  SGuiElem getItemLine(const ItemInfo&, function<void(Rectangle)> onClick,
      function<void()> onMultiClick = nullptr);
  vector<string> getItemHint(const ItemInfo&);
  SGuiElem drawMinionAndLevel(ViewIdList, int level, int iconMult);
  vector<SDL::SDL_Keysym> getConfirmationKeys();
  optional<ItemAction> getItemChoice(const ItemInfo& itemInfo, Vec2 menuPos, bool autoDefault);
  vector<SGuiElem> getMultiLine(const string& text, Color, MenuType, int maxWidth, int fontSize);
  SGuiElem getHighlight(SGuiElem line, MenuType, const string& label, int numActive, optional<int>* highlight);
  string getPlayerTitle(PlayerInfo&);
  SDL::SDL_KeyboardEvent getHotkeyEvent(char);
  shared_ptr<MapGui> mapGui;
  int getImmigrantAnimationOffset(milliseconds initTime);
  HeapAllocated<CallCache<SGuiElem>> cache;
  SGuiElem drawMenuWarning(View::CampaignOptions::WarningType);
  SGuiElem drawTutorialOverlay(const TutorialInfo&);
  unordered_set<pair<int, TutorialHighlight>, CustomHash<pair<int, TutorialHighlight>>> tutorialClicks;
  bool wasTutorialClicked(size_t hash, TutorialHighlight);
  void onTutorialClicked(size_t hash, TutorialHighlight);
  SGuiElem drawLibraryContent(const CollectiveInfo&, const optional<TutorialInfo>&);
  SGuiElem drawMapHintOverlay();
  SGuiElem getClickActions(const ViewObject&);
  vector<string> hint;
  SGuiElem getExpIncreaseLine(const CreatureExperienceInfo&, ExperienceType,
      function<void()> increaseCallback = nullptr, bool infoOnly = false);
  optional<int> highlightedTeamMember;
  SGuiElem drawQuartersButton(const PlayerInfo&, const vector<ViewId>& allQuarters);
  SGuiElem drawWarningWindow(const CollectiveInfo::RebellionChance&);
  SGuiElem drawRebellionChanceText(CollectiveInfo::RebellionChance);
  SGuiElem drawVillainsOverlay(const VillageInfo&);
  SGuiElem drawAllVillainsOverlay(const VillageInfo&);
  SGuiElem drawVillainInfoOverlay(const VillageInfo::Village&, bool showDismissHint);
  SGuiElem drawVillainType(VillainType);
  SGuiElem drawLyingItemsList(const string& title, const ItemCounts&, int maxWidth);
  SGuiElem drawTickBox(shared_ptr<bool> value, const string& title);
  SGuiElem drawItemUpgradeButton(const CollectiveInfo::QueuedItemInfo&);
  SGuiElem drawGenderButtons(const vector<View::AvatarData>&, shared_ptr<int> gender, shared_ptr<int> chosenAvatar);
  SGuiElem drawFirstNameButtons(const vector<View::AvatarData>&, shared_ptr<int> gender, shared_ptr<int> chosenAvatar,
      shared_ptr<int> chosenName);
  SGuiElem drawRoleButtons(shared_ptr<PlayerRole> chosenRole, shared_ptr<int> chosenAvatar, shared_ptr<int> avatarPage,
      const vector<View::AvatarData>&);
  SGuiElem drawChosenCreatureButtons(PlayerRole, shared_ptr<int> chosenAvatar, shared_ptr<int> gender, int page,
      const vector<View::AvatarData>&);
  SGuiElem drawCreatureList(const vector<CreatureInfo>&, function<void(UniqueEntity<Creature>::Id)> button);
  Color getElemColor(ListElem::ElemMod);
  SGuiElem drawAvatarsForRole(const vector<View::AvatarData>&, shared_ptr<int> avatarPage, shared_ptr<int> chosenAvatar,
                              shared_ptr<int> gender, shared_ptr<PlayerRole> chosenRole);
  SGuiElem drawScreenshotOverlay();
  SGuiElem drawTitleButton(const PlayerInfo& minion);
  SGuiElem drawKillsLabel(const PlayerInfo& minion);
  function<void(Rectangle)> getActivityButtonFun(const PlayerInfo&);
  SGuiElem drawSpellSchoolLabel(const SpellSchoolInfo&);
  SGuiElem drawResources(const vector<CollectiveInfo::Resource>&, const optional<TutorialInfo>&, int width);
  SGuiElem drawBiomeMenu(SyncQueue<CampaignAction>&, const vector<View::CampaignOptions::BiomeInfo>&, int chosen);
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
  SGuiElem drawKeeperDangerOverlay(const string&);
  SGuiElem drawBoolOptionElem(OptionId, string name);
};

RICH_ENUM(GuiBuilder::GameSpeed,
  SLOW,
  NORMAL,
  FAST,
  VERY_FAST
);

