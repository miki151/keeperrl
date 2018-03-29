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

#include "stdafx.h"
#include "gui_builder.h"
#include "clock.h"
#include "renderer.h"
#include "view_id.h"
#include "player_message.h"
#include "view.h"
#include "options.h"
#include "map_gui.h"
#include "campaign.h"
#include "retired_games.h"
#include "call_cache.h"
#include "creature_factory.h"
#include "creature.h"
#include "creature_name.h"
#include "campaign_type.h"
#include "villain_type.h"
#include "attr_type.h"
#include "minimap_gui.h"
#include "creature_view.h"
#include "level.h"
#include "quarters.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

#define THIS_LINE __LINE__

GuiBuilder::GuiBuilder(Renderer& r, GuiFactory& g, Clock* c, Options* o, Callbacks call)
    : renderer(r), gui(g), clock(c), options(o), callbacks(call), gameSpeed(GameSpeed::NORMAL),
      fpsCounter(60), upsCounter(60), cache(100) {
}

void GuiBuilder::reset() {
  gameSpeed = GameSpeed::NORMAL;
  numSeenVillains = -1;
  tutorialClicks.clear();
}

void GuiBuilder::setMapGui(shared_ptr<MapGui> g) {
  mapGui = g;
}

void GuiBuilder::clearHint() {
  hint.clear();
}

GuiBuilder::~GuiBuilder() {}

const int legendLineHeight = 30;
const int titleLineHeight = legendLineHeight + 8;

int GuiBuilder::getStandardLineHeight() const {
  return legendLineHeight;
}

SGuiElem GuiBuilder::getHintCallback(const vector<string>& s) {
  return gui.mouseOverAction([this, s]() { hint = s; });
}

function<void()> GuiBuilder::getButtonCallback(UserInput input) {
  return [this, input]() {
    callbacks.input(input);
/*#ifndef RELEASE // adding the input twice uncovers bugs sometimes
    callbacks.input(input);
#endif*/
  };
}

void GuiBuilder::setCollectiveTab(CollectiveTab t) {
  if (collectiveTab != t) {
    collectiveTab = t;
    clearActiveButton();
    if (t != CollectiveTab::MINIONS)
      callbacks.input({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()});
    callbacks.input({UserInputId::WORKSHOP, -1});
    callbacks.input({UserInputId::LIBRARY_CLOSE});
    showTasks = false;
  }
}

CollectiveTab GuiBuilder::getCollectiveTab() const {
  return collectiveTab;
}

void GuiBuilder::closeOverlayWindows() {
  // send a random id which wont be found
  callbacks.input({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()});
  callbacks.input({UserInputId::WORKSHOP, -1});
  callbacks.input({UserInputId::LIBRARY_CLOSE});
  showTasks = false;
}

void GuiBuilder::closeOverlayWindowsAndClearButton() {
  closeOverlayWindows();
  clearActiveButton();
}

optional<int> GuiBuilder::getActiveButton(CollectiveTab tab) const {
  if (activeButton && activeButton->tab == tab)
    return activeButton->num;
  else
    return none;
}

void GuiBuilder::setActiveGroup(const string& group, optional<TutorialHighlight> tutorial) {
  closeOverlayWindowsAndClearButton();
  activeGroup = group;
  if (tutorial)
    onTutorialClicked(combineHash(group), *tutorial);
}

void GuiBuilder::setActiveButton(CollectiveTab tab, int num, ViewId viewId, optional<string> group,
    optional<TutorialHighlight> tutorial) {
  closeOverlayWindowsAndClearButton();
  activeButton = ActiveButton {tab, num};
  mapGui->setButtonViewId(viewId);
  activeGroup = group;
  if (tutorial) {
    onTutorialClicked(num, *tutorial);
    if (group)
      onTutorialClicked(combineHash(*group), *tutorial);
  }
}

bool GuiBuilder::clearActiveButton() {
  if (activeButton || activeGroup) {
    activeButton = none;
    activeGroup = none;
    mapGui->clearButtonViewId();
    callbacks.input(UserInputId::RECT_CANCEL);
    return true;
  }
  return false;
}

SGuiElem GuiBuilder::drawCost(pair<ViewId, int> cost, Color color) {
  string costText = toString(cost.second);
  return GuiFactory::ListBuilder(gui)
      .addElemAuto(gui.rightMargin(5, gui.label(costText, color)))
      .addElem(gui.viewObject(cost.first), 25)
      .buildHorizontalList();
}

bool GuiBuilder::wasTutorialClicked(size_t hash, TutorialHighlight state) {
  return tutorialClicks.count({hash, state});
}

void GuiBuilder::onTutorialClicked(size_t hash, TutorialHighlight state) {
  tutorialClicks.insert({hash, state});
}

SGuiElem GuiBuilder::getButtonLine(CollectiveInfo::Button button, int num, CollectiveTab tab,
    const optional<TutorialInfo>& tutorial) {
  GuiFactory::ListBuilder line(gui);
  line.addElem(gui.viewObject(button.viewId), 35);
  auto tutorialHighlight = button.tutorialHighlight;
  if (button.state != CollectiveInfo::Button::ACTIVE)
    line.addElemAuto(gui.label(button.name + " " + button.count, Color::GRAY, button.hotkey));
  else {
    line.addElemAuto(gui.label(button.name + " " + button.count, Color::WHITE, button.hotkey));
  }
  if (button.cost)
    line.addBackElemAuto(drawCost(*button.cost));
  function<void()> buttonFun;
  ViewId viewId = button.viewId;
  if (button.state != CollectiveInfo::Button::INACTIVE)
    buttonFun = [=] {
      if (getActiveButton(tab) == num)
        clearActiveButton();
      else {
        setCollectiveTab(tab);
        setActiveButton(tab, num, viewId, none, tutorialHighlight);
      }
    };
  else {
    buttonFun = [] {};
  }
  auto tutorialElem = gui.empty();
  if (tutorial && tutorialHighlight && tutorial->highlights.contains(*tutorialHighlight))
    tutorialElem = gui.conditional(gui.tutorialHighlight(),
        [=]{ return !wasTutorialClicked(num, *tutorialHighlight); });
  return gui.setHeight(legendLineHeight, gui.stack(makeVec(
      getHintCallback({capitalFirst(button.help)}),
      gui.buttonChar(buttonFun, !button.hotkeyOpensGroup ? button.hotkey : 0, true, true),
      gui.uiHighlightConditional([=] { return getActiveButton(tab) == num; }),
      tutorialElem,
      line.buildHorizontalList())));
}

static optional<int> getFirstActive(const vector<CollectiveInfo::Button>& buttons, int begin) {
  for (int i = begin; i < buttons.size() && buttons[i].groupName == buttons[begin].groupName; ++i)
    if (buttons[i].state == CollectiveInfo::Button::ACTIVE)
      return i;
  return none;
}

static optional<int> getNextActive(const vector<CollectiveInfo::Button>& buttons, int begin,
    optional<int> current) {
  if (!current)
    return getFirstActive(buttons, begin);
  CHECK(*current >= begin);
  int i = *current;
  do {
    ++i;
    if (buttons[i].groupName != buttons[begin].groupName)
      i = begin;
    if (current == i)
      return none;
  } while (buttons[i].state != CollectiveInfo::Button::ACTIVE);
  return i;
}

SGuiElem GuiBuilder::drawBuildings(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> keypressOnly;
  auto& buttons = info.buildings;
  auto tab = CollectiveTab::BUILDINGS;
  auto elems = gui.getListBuilder(legendLineHeight);
  elems.addSpace(5);
  string lastGroup;
  for (int i : All(buttons)) {
    if (!buttons[i].groupName.empty() && buttons[i].groupName != lastGroup) {
      lastGroup = buttons[i].groupName;
      optional<TutorialHighlight> tutorialHighlight;
      if (tutorial)
        for (int j = i; j < buttons.size() && buttons[i].groupName == buttons[j].groupName; ++j)
          if (auto highlight = buttons[j].tutorialHighlight)
            if (tutorial->highlights.contains(*highlight))
              tutorialHighlight = *highlight;
      function<void()> buttonFunHotkey = [=] {
        if (activeGroup != lastGroup) {
          setCollectiveTab(tab);
          if (auto firstBut = getNextActive(buttons, i, none))
            setActiveButton(tab, *firstBut, buttons[*firstBut].viewId, lastGroup, tutorialHighlight);
          else
            setActiveGroup(lastGroup, tutorialHighlight);
        } else
        if (auto newBut = getNextActive(buttons, i, getActiveButton(tab)))
          setActiveButton(tab, *newBut, buttons[*newBut].viewId, lastGroup, tutorialHighlight);
        else
          clearActiveButton();
      };
      function<void()> labelFun = [=] {
        if (activeGroup != lastGroup) {
          setCollectiveTab(tab);
          setActiveGroup(lastGroup, tutorialHighlight);
        } else {
          clearActiveButton();
        }
      };
      auto line = gui.getListBuilder();
      line.addElem(gui.viewObject(buttons[i].viewId), 35);
      char hotkey = buttons[i].hotkeyOpensGroup ? buttons[i].hotkey : 0;
      SGuiElem tutorialElem = gui.empty();
      if (tutorialHighlight)
        tutorialElem = gui.conditional(gui.tutorialHighlight(),
             [=]{ return !wasTutorialClicked(combineHash(lastGroup), *tutorialHighlight); });
      line.addElemAuto(gui.label(lastGroup, Color::WHITE, hotkey));
      elems.addElem(gui.stack(makeVec(
          gui.keyHandlerChar(buttonFunHotkey, hotkey, true, true),
          gui.button(labelFun),
          tutorialElem,
          gui.uiHighlightConditional([=] { return activeGroup == lastGroup;}),
          line.buildHorizontalList())));
    }
    if (buttons[i].groupName.empty())
      elems.addElem(getButtonLine(buttons[i], i, tab, tutorial));
    else
      keypressOnly.push_back(gui.invisible(getButtonLine(buttons[i], i, tab, tutorial)));
  }
  keypressOnly.push_back(elems.buildVerticalList());
  return gui.scrollable(gui.stack(std::move(keypressOnly)), &buildingsScroll, &scrollbarsHeld);
}

SGuiElem GuiBuilder::drawTechnology(CollectiveInfo& info) {
  int hash = combineHash(info.techButtons, info.workshopButtons);
  if (hash != technologyHash) {
    technologyHash = hash;
    auto lines = gui.getListBuilder(legendLineHeight);
    lines.addSpace(legendLineHeight / 2);
    for (int i : All(info.techButtons)) {
      auto line = gui.getListBuilder();
      line.addElem(gui.viewObject(info.techButtons[i].viewId), 35);
      line.addElemAuto(gui.label(info.techButtons[i].name, Color::WHITE, info.techButtons[i].hotkey));
      lines.addElem(gui.stack(
            gui.buttonChar([this, i] {
              closeOverlayWindowsAndClearButton();
              getButtonCallback(UserInput(UserInputId::TECHNOLOGY, i))();
            }, info.techButtons[i].hotkey, true, true),
            line.buildHorizontalList()));
    }
    lines.addSpace(legendLineHeight / 2);
    for (int i : All(info.workshopButtons)) {
      auto& button = info.workshopButtons[i];
      auto line = gui.getListBuilder();
      line.addElem(gui.viewObject(button.viewId), 35);
      line.addElemAuto(gui.label(button.name, button.unavailable ? Color::GRAY : Color::WHITE));
      SGuiElem elem = line.buildHorizontalList();
      if (button.active)
        elem = gui.stack(
            gui.uiHighlightLine(Color::GREEN),
            std::move(elem));
      if (!button.unavailable)
        elem = gui.stack(
          gui.button([this, i] {
            workshopsScroll2.reset();
            workshopsScroll.reset();
            getButtonCallback(UserInput(UserInputId::WORKSHOP, i))(); }),
          std::move(elem));
      lines.addElem(std::move(elem));
    }
    technologyCache = lines.buildVerticalList();
  }
  return gui.external(technologyCache.get());
}

SGuiElem GuiBuilder::drawKeeperHelp() {
  if (!keeperHelp) {
    vector<string> helpText {
      "use mouse to dig and build",
        "shift selects rectangles",
        "control deselects",
        "",
        "scroll with arrows",
        "or right mouse button",
        "shift with arrows scrolls faster",
        "",
        "right click on minion",
        "to control or show information",
        "",
        "[space] pause",
        "[a] and [z] or mouse wheel: zoom",
        "press mouse wheel: level map",
        "",
        "follow the orange hints :-)"};
    auto lines = gui.getListBuilder(legendLineHeight);
    for (string line : helpText)
      lines.addElem(gui.label(line, Color::LIGHT_BLUE));
    keeperHelp = lines.buildVerticalList();
  }
  return gui.external(keeperHelp.get());
}

void GuiBuilder::addFpsCounterTick() {
  fpsCounter.addTick();
}

void GuiBuilder::addUpsCounterTick() {
  upsCounter.addTick();
}

const int resourceSpace = 110;

SGuiElem GuiBuilder::drawBottomBandInfo(GameInfo& gameInfo) {
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<CollectiveInfo>();
  GameSunlightInfo& sunlightInfo = gameInfo.sunlightInfo;
  if (!bottomBandCache) {
    auto topLine = gui.getListBuilder(resourceSpace);
    for (int i : All(info.numResource)) {
      auto res = gui.getListBuilder();
      res.addElem(gui.viewObject(info.numResource[i].viewId), 30);
      res.addElemAuto(gui.labelFun([&info, i] { return toString<int>(info.numResource[i].count); },
            [&info, i] { return info.numResource[i].count >= 0 ? Color::WHITE : Color::RED; }));
      auto tutorialHighlight = info.numResource[i].tutorialHighlight;
      auto tutorialElem = gui.conditional(gui.tutorialHighlight(), [tutorialHighlight, &gameInfo] {
          return gameInfo.tutorial && tutorialHighlight && gameInfo.tutorial->highlights.contains(*tutorialHighlight);
      });
      topLine.addElem(gui.stack(
          tutorialElem,
          getHintCallback({info.numResource[i].name}),
          res.buildHorizontalList()));
    }
    auto bottomLine = gui.getListBuilder(140);
    bottomLine.addElem(getTurnInfoGui(gameInfo.time));
    bottomLine.addElem(getSunlightInfoGui(sunlightInfo));
    bottomLine.addElem(gui.labelFun([&info] {
          return "population: " + toString(info.minionCount) + " / " +
          toString(info.minionLimit); }));
    bottomBandCache = gui.getListBuilder(28)
          .addElem(gui.centerHoriz(topLine.buildHorizontalList()))
          .addElem(gui.centerHoriz(bottomLine.buildHorizontalList()))
          .buildVerticalList();
  }
  return gui.external(bottomBandCache.get());
}

const char* GuiBuilder::getGameSpeedName(GuiBuilder::GameSpeed gameSpeed) const {
  switch (gameSpeed) {
    case GameSpeed::SLOW: return "slow";
    case GameSpeed::NORMAL: return "normal";
    case GameSpeed::FAST: return "fast";
    case GameSpeed::VERY_FAST: return "very fast";
  }
}

const char* GuiBuilder::getCurrentGameSpeedName() const {
  if (clock->isPaused())
    return "paused";
  else
    return getGameSpeedName(gameSpeed);
}

SGuiElem GuiBuilder::drawRightBandInfo(GameInfo& info) {
  auto getIconHighlight = [&] (Color c) { return gui.topMargin(-1, gui.uiHighlight(c)); };
  auto& collectiveInfo = *info.playerInfo.getReferenceMaybe<CollectiveInfo>();
  int hash = combineHash(collectiveInfo, info.villageInfo, info.modifiedSquares, info.totalSquares, info.tutorial);
  if (hash != rightBandInfoHash) {
    rightBandInfoHash = hash;
    VillageInfo& villageInfo = info.villageInfo;
    vector<SGuiElem> buttons = makeVec(
        gui.icon(gui.BUILDING),
        gui.icon(gui.MINION),
        gui.icon(gui.LIBRARY),
        gui.stack(gui.conditional(getIconHighlight(Color::YELLOW),
                      [=] { return numSeenVillains < villageInfo.numTotalVillains;}),
                  gui.icon(gui.DIPLOMACY)),
        gui.icon(gui.HELP)
    );
    for (int i : All(buttons)) {
      buttons[i] = gui.stack(
          gui.conditional(getIconHighlight(Color::GREEN), [this, i] { return int(collectiveTab) == i;}),
          std::move(buttons[i]),
          gui.button([this, i]() { setCollectiveTab(CollectiveTab(i)); }));
    }
    if (info.tutorial) {
      for (auto& building : collectiveInfo.buildings)
        if (auto& highlight = building.tutorialHighlight)
          if (info.tutorial->highlights.contains(*highlight)) {
            buttons[0] = gui.stack(
                gui.conditional(
                    gui.blink(getIconHighlight(Color::YELLOW)),
                    [this] { return collectiveTab != CollectiveTab::BUILDINGS;}),
                buttons[0]);
            break;
          }
      EnumSet<TutorialHighlight> minionHighlights = { TutorialHighlight::NEW_TEAM, TutorialHighlight::CONTROL_TEAM};
      if (!info.tutorial->highlights.intersection(minionHighlights).isEmpty())
        buttons[1] = gui.stack(
            gui.conditional(
                gui.blink(getIconHighlight(Color::YELLOW)),
                [this] { return collectiveTab != CollectiveTab::MINIONS;}),
            buttons[1]);
    }
    vector<pair<CollectiveTab, SGuiElem>> elems = makeVec(
        make_pair(CollectiveTab::MINIONS, drawMinions(collectiveInfo, info.tutorial)),
        make_pair(CollectiveTab::BUILDINGS, cache->get(bindMethod(
            &GuiBuilder::drawBuildings, this), THIS_LINE, collectiveInfo, info.tutorial)),
        make_pair(CollectiveTab::KEY_MAPPING, drawKeeperHelp()),
        make_pair(CollectiveTab::TECHNOLOGY, drawTechnology(collectiveInfo)),
        make_pair(CollectiveTab::VILLAGES, drawVillages(villageInfo)));
    vector<SGuiElem> tabs;
    for (auto& elem : elems) {
      auto tab = elem.first;
      tabs.push_back(gui.conditional(std::move(elem.second), [tab, this] { return tab == collectiveTab;}));
    }
    SGuiElem main = gui.stack(std::move(tabs));
    main = gui.margins(std::move(main), 15, 15, 15, 5);
    int numButtons = buttons.size();
    auto buttonList = gui.getListBuilder(50);
    for (auto& elem : buttons)
      buttonList.addElem(std::move(elem));
    SGuiElem butGui = gui.margins(
        gui.centerHoriz(buttonList.buildHorizontalList()), 0, 5, 9, 5);
    auto bottomLine = gui.getListBuilder();
    bottomLine.addElem(gui.stack(
        gui.getListBuilder()
            .addElem(gui.label("speed:"), 60)
            .addElemAuto(gui.labelFun([this] { return getCurrentGameSpeedName();},
              [this] { return clock->isPaused() ? Color::RED : Color::WHITE; })).buildHorizontalList(),
        gui.button([&] { gameSpeedDialogOpen = !gameSpeedDialogOpen; })), 160);
    int modifiedSquares = info.modifiedSquares;
    int totalSquares = info.totalSquares;
    bottomLine.addElemAuto(gui.stack(
        gui.labelFun([=]()->string {
          switch (counterMode) {
            case CounterMode::FPS:
              return "FPS " + toString(fpsCounter.getFps()) + " / " + toString(upsCounter.getFps());
            case CounterMode::LAT:
              return "LAT " + toString(fpsCounter.getMaxLatency()) + "ms / " + toString(upsCounter.getMaxLatency()) + "ms";
            case CounterMode::SMOD:
              return "SMOD " + toString(modifiedSquares) + "/" + toString(totalSquares);
          }
        }, Color::WHITE),
        gui.button([=]() { counterMode = (CounterMode) ( ((int) counterMode + 1) % 3); })));
    main = gui.margin(gui.leftMargin(25, bottomLine.buildHorizontalList()),
        std::move(main), 18, gui.BOTTOM);
    rightBandInfoCache = gui.margin(std::move(butGui), std::move(main), 55, gui.TOP);
  }
  return gui.external(rightBandInfoCache.get());
}

GuiBuilder::GameSpeed GuiBuilder::getGameSpeed() const {
  return gameSpeed;
}

void GuiBuilder::setGameSpeed(GameSpeed speed) {
  gameSpeed = speed;
}

static char getHotkeyChar(GuiBuilder::GameSpeed speed) {
  return '1' + int(speed);
}

static SDL_Keycode getHotkey(GuiBuilder::GameSpeed speed) {
  switch (speed) {
    case GuiBuilder::GameSpeed::SLOW: return SDL::SDLK_1;
    case GuiBuilder::GameSpeed::NORMAL: return SDL::SDLK_2;
    case GuiBuilder::GameSpeed::FAST: return SDL::SDLK_3;
    case GuiBuilder::GameSpeed::VERY_FAST: return SDL::SDLK_4;
  }
}

SGuiElem GuiBuilder::drawGameSpeedDialog() {
  int keyMargin = 95;
  auto pauseFun = [=] {
    if (clock->isPaused())
      clock->cont();
    else
      clock->pause();
    gameSpeedDialogOpen = false;
  };
  vector<SGuiElem> lines;
  vector<SGuiElem> hotkeys;
  lines.push_back(gui.stack(
        gui.getListBuilder(keyMargin)
            .addElem(gui.label("pause"))
            .addElem(gui.label("[space]")).buildHorizontalList(),
        gui.button(pauseFun)));
  hotkeys.push_back(gui.keyHandler(pauseFun, {gui.getKey(SDL::SDLK_SPACE)}));
  for (GameSpeed speed : ENUM_ALL(GameSpeed)) {
    auto speedFun = [=] { gameSpeed = speed; gameSpeedDialogOpen = false; clock->cont();};
    auto colorFun = [this, speed] { return speed == gameSpeed ? Color::GREEN : Color::WHITE; };
    lines.push_back(gui.stack(gui.getListBuilder(keyMargin)
              .addElem(gui.label(getGameSpeedName(speed), colorFun))
              .addElem(gui.label("'" + string(1, getHotkeyChar(speed)) + "' ", colorFun)).buildHorizontalList(),
          gui.button(speedFun)));
    hotkeys.push_back(gui.keyHandler(speedFun, {gui.getKey(getHotkey(speed))}));
  }
  reverse(lines.begin(), lines.end());
  int margin = 20;
  Vec2 size(150 + 2 * margin, legendLineHeight * lines.size() - 10 + 2 * margin);
  SGuiElem dialog = gui.miniWindow(
      gui.margins(gui.verticalList(std::move(lines), legendLineHeight), margin));
  return gui.stack(gui.stack(std::move(hotkeys)),
      gui.preferredSize(size, gui.conditional(std::move(dialog), [this] { return gameSpeedDialogOpen; })));
}

SGuiElem GuiBuilder::drawImmigrantInfo(const ImmigrantDataInfo& info) {
  auto lines = gui.getListBuilder(legendLineHeight);
  if (info.autoState)
    switch (*info.autoState) {
      case ImmigrantAutoState::AUTO_ACCEPT:
        lines.addElem(gui.label("(Immigrant will be accepted automatically)", Color::GREEN));
        break;
      case ImmigrantAutoState::AUTO_REJECT:
        lines.addElem(gui.label("(Immigrant will be rejected automatically)", Color::RED));
        break;
    }
  lines.addElem(
      gui.getListBuilder()
          .addElemAuto(gui.label(capitalFirst(info.name)))
          .addSpace(100)
          .addBackElemAuto(info.cost ? drawCost(*info.cost) : gui.empty())
          .buildHorizontalList());
  lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(info.attributes)));
  if (info.timeLeft)
    lines.addElem(gui.label("Turns left: " + toString(info.timeLeft)));
  for (auto& req : info.info)
    lines.addElem(gui.label(req, Color::WHITE));
  for (auto& req : info.requirements)
    lines.addElem(gui.label(req, Color::ORANGE));
  return gui.miniWindow(gui.margins(lines.buildVerticalList(), 15));
}

int GuiBuilder::getImmigrantAnimationOffset(milliseconds initTime) {
  const milliseconds delay = clock->getRealMillis() - initTime;
  const milliseconds fallingTime {1000};
  const int fallHeight = 2000;
  if (delay < fallingTime)
    return fallHeight - fallHeight * delay.count() / fallingTime.count();
  else
    return 0;
}

int GuiBuilder::getImmigrationBarWidth() const {
  return 40;
}

SGuiElem GuiBuilder::drawTutorialOverlay(const TutorialInfo& info) {
  auto continueButton = gui.stack(
      gui.button([this] {
          callbacks.input(UserInputId::TUTORIAL_CONTINUE);
          tutorialClicks.clear();
          closeOverlayWindowsAndClearButton();
      }),
      gui.setHeight(20, gui.labelHighlightBlink("[Continue]", Color::LIGHT_BLUE, Color::WHITE)));
  auto backButton = gui.stack(
      gui.button(getButtonCallback(UserInputId::TUTORIAL_GO_BACK)),
      gui.setHeight(20, gui.labelHighlight("[Go back]", Color::LIGHT_BLUE)));
  SGuiElem warning;
  if (info.warning)
    warning = gui.label(*info.warning, Color::RED);
  else
    warning = gui.label("Press [Space] to unpause the game.",
        [this]{ return clock->isPaused() ? Color::RED : Color::TRANSPARENT;});
  return gui.preferredSize(520, 250, gui.stack(gui.darken(), gui.rectangleBorder(Color::GRAY),
      gui.margins(gui.stack(
        gui.labelMultiLine(info.message, legendLineHeight),
        gui.alignment(GuiFactory::Alignment::BOTTOM_CENTER, gui.setHeight(20, warning)),
        gui.alignment(GuiFactory::Alignment::BOTTOM_RIGHT, info.canContinue ? continueButton : gui.empty()),
        gui.alignment(GuiFactory::Alignment::BOTTOM_LEFT, info.canGoBack ? backButton : gui.empty())
      ), 20)));
}

SGuiElem GuiBuilder::drawImmigrationOverlay(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  const int elemWidth = getImmigrationBarWidth();
  auto makeHighlight = [=] (Color c) { return gui.margins(gui.rectangle(c), 4); };
  auto lines = gui.getListBuilder(elemWidth);
  auto getAcceptButton = [=] (int immigrantId, optional<Keybinding> keybinding) {
    return gui.stack(
        gui.releaseLeftButton(getButtonCallback({UserInputId::IMMIGRANT_ACCEPT, immigrantId}), keybinding),
        gui.onMouseLeftButtonHeld(makeHighlight(Color(0, 255, 0, 100))));
  };
  auto getRejectButton = [=] (int immigrantId) {
    return gui.stack(
        gui.releaseRightButton(getButtonCallback({UserInputId::IMMIGRANT_REJECT, immigrantId})),
        gui.onMouseRightButtonHeld(makeHighlight(Color(255, 0, 0, 100))));
  };
  for (int i : All(info.immigration)) {
    auto& elem = info.immigration[i];
    SGuiElem button;
    if (elem.requirements.empty())
      button = gui.stack(makeVec(
          gui.sprite(GuiFactory::TexId::IMMIGRANT_BG, GuiFactory::Alignment::CENTER),
          cache->get(getAcceptButton, THIS_LINE, elem.id, elem.keybinding),
          cache->get(getRejectButton, THIS_LINE, elem.id)
      ));
    else
      button = gui.stack(makeVec(
          gui.sprite(GuiFactory::TexId::IMMIGRANT2_BG, GuiFactory::Alignment::CENTER),
          cache->get(getRejectButton, THIS_LINE, elem.id)
      ));
    if (tutorial && elem.tutorialHighlight && tutorial->highlights.contains(*elem.tutorialHighlight))
        button = gui.stack(std::move(button), gui.blink(makeHighlight(Color(255, 255, 0, 100))));
    auto initTime = elem.generatedTime;
    lines.addElem(gui.translate([=]() { return Vec2(0, initTime ? -getImmigrantAnimationOffset(*initTime) : 0);},
        gui.stack(
            std::move(button),
            gui.tooltip2(drawImmigrantInfo(elem), [](const Rectangle& r) { return r.topRight();}),
            gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(-3,
                gui.viewObject(ViewId::ROUND_SHADOW, 1, Color(255, 255, 255, 160)))))),
            gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(5,
                elem.count ? drawMinionAndLevel(elem.viewId, *elem.count, 1) : gui.viewObject(elem.viewId)))))
    )));
  }
  lines.addElem(gui.stack(makeVec(
      gui.sprite(GuiFactory::TexId::IMMIGRANT_BG, GuiFactory::Alignment::CENTER),
      gui.conditional(makeHighlight(Color(0, 255, 0, 100)), [this] { return immigrantHelpOpen; }),
      gui.button([this] { immigrantHelpOpen = !immigrantHelpOpen; }),
      gui.setWidth(elemWidth, gui.topMargin(-2, gui.centerHoriz(gui.label("?", 32, Color::GREEN))))
  )));
  return gui.setWidth(elemWidth, gui.stack(
        gui.stopMouseMovement(),
        lines.buildVerticalList()));
}

SGuiElem GuiBuilder::getImmigrationHelpText() {
  return gui.labelMultiLine("Welcome to the immigration system! The icons immediately to the left represent "
                            "creatures that would "
                            "like to join your dungeon. Left-click accepts, right-click rejects a candidate. "
                            "Some creatures have requirements that you need to fulfill before "
                            "they can join. Above this text you can examine all possible immigrants, along with their full "
                            "requirements. You can also click on the icons to set automatic acception or rejection.",
                            legendLineHeight);
}

SGuiElem GuiBuilder::drawImmigrationHelp(const CollectiveInfo& info) {
  const int elemWidth = 80;
  const int numPerLine = 8;
  const int iconScale = 2;
  auto lines = gui.getListBuilder(elemWidth);
  auto line = gui.getListBuilder(elemWidth);
  for (auto& elem : info.allImmigration) {
    auto icon = gui.viewObject(elem.viewId, iconScale);
    if (elem.autoState)
      switch (*elem.autoState) {
        case ImmigrantAutoState::AUTO_ACCEPT:
          icon = gui.stack(std::move(icon), gui.viewObject(ViewId::ACCEPT_IMMIGRANT, iconScale));
          break;
        case ImmigrantAutoState::AUTO_REJECT:
          icon = gui.stack(std::move(icon), gui.viewObject(ViewId::REJECT_IMMIGRANT, iconScale));
          break;
      }
    line.addElem(gui.stack(makeVec(
        gui.button(getButtonCallback({UserInputId::IMMIGRANT_AUTO_ACCEPT, elem.id})),
        gui.buttonRightClick(getButtonCallback({UserInputId::IMMIGRANT_AUTO_REJECT, elem.id})),
        gui.tooltip2(drawImmigrantInfo(elem), [](const Rectangle& r) { return r.bottomLeft();}),
        gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(-3,
            gui.viewObject(ViewId::ROUND_SHADOW, 1, Color(255, 255, 255, 160)))))),
        gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(5, std::move(icon))))))));
    if (line.getLength() >= numPerLine) {
      lines.addElem(line.buildHorizontalList());
      line.clear();
    }
  }
  if (numPerLine > info.allImmigration.size())
    line.addSpace(elemWidth * (numPerLine - info.allImmigration.size()));
  if (!line.isEmpty())
    lines.addElem(line.buildHorizontalList());

  lines.addElem(getImmigrationHelpText(), legendLineHeight * 6);
  return gui.miniWindow(gui.margins(gui.stack(
        gui.stopMouseMovement(),
        lines.buildVerticalList()), 15), [this] { immigrantHelpOpen = false; }, true);
}

SGuiElem GuiBuilder::getSunlightInfoGui(const GameSunlightInfo& sunlightInfo) {
  return gui.stack(
      gui.conditional(
          getHintCallback({"Time remaining till nightfall."}),
          getHintCallback({"Time remaining till day."}),
          [&] { return sunlightInfo.description == "day";}),
      gui.labelFun([&] { return sunlightInfo.description + " [" + toString(sunlightInfo.timeRemaining) + "]"; },
          [&] { return sunlightInfo.description == "day" ? Color::WHITE : Color::LIGHT_BLUE;}));
}

SGuiElem GuiBuilder::getTurnInfoGui(const GlobalTime& turn) {
  return gui.stack(getHintCallback({"Current turn."}),
      gui.labelFun([&turn] { return "T: " + toString(turn); }, Color::WHITE));
}

vector<SGuiElem> GuiBuilder::drawPlayerAttributes(const vector<AttributeInfo>& attr) {
  vector<SGuiElem> ret;
  for (auto& elem : attr) {
    auto attrText = toString(elem.value);
    if (elem.bonus != 0)
      attrText += (elem.bonus > 0 ? "+" : "") + toString(elem.bonus);
    ret.push_back(gui.stack(getTooltip({elem.name, elem.help}, THIS_LINE),
        gui.horizontalList(makeVec(
          gui.icon(elem.attr),
          gui.margins(gui.label(attrText), 0, 2, 0, 0)), 30)));
  }
  return ret;
}

vector<SGuiElem> GuiBuilder::drawPlayerAttributes(const ViewObject::CreatureAttributes& attributes) {
  vector<SGuiElem> ret;
  for (auto attr : ENUM_ALL(AttrType))
    ret.push_back(
        gui.horizontalList(makeVec(
          gui.icon(attr),
          gui.margins(gui.label(toString((int) attributes[attr])), 0, 2, 0, 0)), 30));
  return ret;
}

SGuiElem GuiBuilder::drawBottomPlayerInfo(const GameInfo& gameInfo) {
  return gui.getListBuilder(28)
      .addElem(gui.centerHoriz(gui.horizontalList(
              drawPlayerAttributes(gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>()->attributes), resourceSpace)))
      .addElem(gui.centerHoriz(gui.getListBuilder(140)
            .addElem(getTurnInfoGui(gameInfo.time))
            .addElem(getSunlightInfoGui(gameInfo.sunlightInfo))
            .buildHorizontalList()))
      .buildVerticalList();
}

static int viewObjectWidth = 27;

int GuiBuilder::getItemLineOwnerMargin() {
  return viewObjectWidth + 60;
}

static string getIntrinsicStateText(IntrinsicAttack::Active state) {
  switch (state) {
    case IntrinsicAttack::ALWAYS:
      return "always active.";
    case IntrinsicAttack::NO_WEAPON:
      return "active when no weapon is equipped.";
    case IntrinsicAttack::NEVER:
      return "disabled.";
  }
}

vector<string> GuiBuilder::getItemHint(const ItemInfo& item) {
  vector<string> out { capitalFirst(item.fullName)};
  if (!item.description.empty())
    out.push_back(item.description);
  if (item.equiped)
    out.push_back("Equipped.");
  if (item.pending)
    out.push_back("Not equipped yet.");
  if (item.locked)
    out.push_back("Locked: minion won't change to another item.");
  if (item.intrinsicState)
    out.push_back(capitalFirst(getIntrinsicStateText(*item.intrinsicState)));
  if (!item.unavailableReason.empty())
    out.push_back(item.unavailableReason);
  return out;
}

SGuiElem GuiBuilder::drawBestAttack(const BestAttack& info) {
  return gui.getListBuilder(30)
      .addElem(gui.icon(info.attr))
      .addElem(gui.topMargin(2, gui.label(toString((int) info.value))))
      .buildHorizontalList();
}

static string getWeightString(double weight) {
  return toString<int>((int)(weight * 10)) + "s";
}

static Color getIntrinsicStateColor(IntrinsicAttack::Active state) {
  switch (state) {
    case IntrinsicAttack::ALWAYS:
      return Color::GREEN;
    case IntrinsicAttack::NO_WEAPON:
      return Color::WHITE;
    case IntrinsicAttack::NEVER:
      return Color::GRAY;
  }
}

SGuiElem GuiBuilder::getItemLine(const ItemInfo& item, function<void(Rectangle)> onClick,
    function<void()> onMultiClick) {
  GuiFactory::ListBuilder line(gui);
  int leftMargin = -4;
  if (item.locked) {
    line.addElem(gui.viewObject(ViewId::KEY), viewObjectWidth);
    leftMargin -= viewObjectWidth - 3;
  }
  line.addElem(gui.viewObject(item.viewId), viewObjectWidth);
  Color color = item.equiped ? Color::GREEN : (item.pending || item.unavailable) ?
      Color::GRAY : Color::WHITE;
  if (item.intrinsicState)
    color = getIntrinsicStateColor(*item.intrinsicState);
  if (item.number > 1)
    line.addElemAuto(gui.rightMargin(0, gui.label(toString(item.number) + " ", color)));
  line.addMiddleElem(gui.label(item.name, color));
  auto mainLine = gui.stack(
      gui.buttonRect(onClick),
      line.buildHorizontalList(),
      getTooltip(getItemHint(item), (int) item.ids.getHash()));
  line.clear();
  line.addMiddleElem(std::move(mainLine));
  if (item.owner) {
    line.addBackElem(gui.viewObject(item.owner->viewId), viewObjectWidth);
    line.addBackElem(drawBestAttack(item.owner->bestAttack), getItemLineOwnerMargin() - viewObjectWidth);
  }
  if (item.price)
    line.addBackElemAuto(drawCost(*item.price));
  if (item.weight)
    line.addBackElemAuto(gui.label("[" + getWeightString(*item.weight * item.number) + "]"));
  if (onMultiClick && item.number > 1) {
    line.addBackElem(gui.stack(
        gui.label("[#]"),
        gui.button(onMultiClick),
        getTooltip({"Click to choose how many to pick up."}, THIS_LINE)), 25);
  }
  auto elem = line.buildHorizontalList();
  if (item.tutorialHighlight)
    elem = gui.stack(gui.tutorialHighlight(), std::move(elem));
  return gui.margins(std::move(elem), leftMargin, 0, 0, 0);
}

SGuiElem GuiBuilder::getTooltip(const vector<string>& text, int id) {
  return cache->get(
      [this](const vector<string>& text) {
        return gui.conditional(gui.tooltip(text), [this] { return !disableTooltip;}); },
      id, text);
}

const int listLineHeight = 30;
const int listBrokenLineHeight = 24;

int GuiBuilder::getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

vector<SDL_Keysym> GuiBuilder::getConfirmationKeys() {
  return {gui.getKey(SDL::SDLK_RETURN), gui.getKey(SDL::SDLK_KP_ENTER), gui.getKey(SDL::SDLK_KP_5)};
}

SGuiElem GuiBuilder::drawPlayerOverlay(const PlayerInfo& info) {
  if (info.lyingItems.empty()) {
    playerOverlayFocused = false;
    itemIndex = none;
    return gui.empty();
  }
  if (lastPlayerPositionHash && lastPlayerPositionHash != info.positionHash) {
    playerOverlayFocused = false;
    itemIndex = none;
  }
  lastPlayerPositionHash = info.positionHash;
  vector<SGuiElem> lines;
  const int maxElems = 6;
  const string title = "Click or press [Enter]:";
  int numElems = min<int>(maxElems, info.lyingItems.size());
  Vec2 size = Vec2(380, (1 + numElems) * legendLineHeight);
  if (!info.lyingItems.empty()) {
    for (int i : All(info.lyingItems)) {
      size.x = max(size.x, 40 + viewObjectWidth + renderer.getTextLength(info.lyingItems[i].name));
      lines.push_back(gui.leftMargin(14, gui.stack(
            gui.mouseHighlight(gui.highlight(listLineHeight), i, &itemIndex),
            gui.leftMargin(6, getItemLine(info.lyingItems[i],
          [this, i](Rectangle) { callbacks.input({UserInputId::PICK_UP_ITEM, i}); },
          [this, i]() { callbacks.input({UserInputId::PICK_UP_ITEM_MULTI, i}); })))));
    }
  }
  int totalElems = info.lyingItems.size();
  if (itemIndex >= totalElems)
    itemIndex = totalElems - 1;
  SGuiElem content;
  if (totalElems == 1 && !playerOverlayFocused)
    content = gui.stack(
        gui.margin(
          gui.leftMargin(3, gui.label(title, Color::YELLOW)),
          gui.scrollable(gui.verticalList(std::move(lines), legendLineHeight), &lyingItemsScroll),
          legendLineHeight, GuiFactory::TOP),
        gui.keyHandler([=] { callbacks.input({UserInputId::PICK_UP_ITEM, 0});}, getConfirmationKeys(), true));
  else {
    auto updateScrolling = [this, totalElems] (int dir) {
        if (itemIndex)
          itemIndex = (*itemIndex + dir + totalElems) % totalElems;
        else
          itemIndex = 0;
        lyingItemsScroll.set(*itemIndex * legendLineHeight + legendLineHeight / 2, clock->getRealMillis());
    };
    content = gui.stack(makeVec(
          gui.focusable(
              gui.stack(
                  gui.keyHandler([=] { if (itemIndex) { callbacks.input({UserInputId::PICK_UP_ITEM, *itemIndex});}},
                    getConfirmationKeys(), true),
                  gui.keyHandler([=] { updateScrolling(1); },
                    {gui.getKey(SDL::SDLK_DOWN), gui.getKey(SDL::SDLK_KP_2)}, true),
                  gui.keyHandler([=] { updateScrolling(-1); },
                    {gui.getKey(SDL::SDLK_UP), gui.getKey(SDL::SDLK_KP_8)}, true)),
              getConfirmationKeys(), {gui.getKey(SDL::SDLK_ESCAPE)}, playerOverlayFocused),
          gui.keyHandler([=] { if (!playerOverlayFocused) { itemIndex = 0; lyingItemsScroll.reset();} },
              getConfirmationKeys()),
          gui.keyHandler([=] { itemIndex = none; }, {gui.getKey(SDL::SDLK_ESCAPE)}),
          gui.margin(
            gui.leftMargin(3, gui.label(title, Color::YELLOW)),
            gui.scrollable(gui.verticalList(std::move(lines), legendLineHeight), &lyingItemsScroll),
            legendLineHeight, GuiFactory::TOP)));
  }
  int margin = 14;
  return gui.stack(
      gui.conditional(gui.stack(gui.fullScreen(gui.darken()), gui.miniWindow()), gui.translucentBackground(),
        [=] { return playerOverlayFocused;}),
      gui.margins(gui.preferredSize(size, std::move(content)), margin));
}

static string getActionText(ItemAction a) {
  switch (a) {
    case ItemAction::DROP: return "drop";
    case ItemAction::DROP_MULTI: return "drop some";
    case ItemAction::GIVE: return "give";
    case ItemAction::PAY: return "pay for";
    case ItemAction::EQUIP: return "equip";
    case ItemAction::THROW: return "throw";
    case ItemAction::UNEQUIP: return "remove";
    case ItemAction::APPLY: return "apply";
    case ItemAction::REPLACE: return "replace";
    case ItemAction::LOCK: return "lock";
    case ItemAction::UNLOCK: return "unlock";
    case ItemAction::REMOVE: return "remove";
    case ItemAction::CHANGE_NUMBER: return "change number";
    case ItemAction::NAME: return "name";
    case ItemAction::INTRINSIC_ALWAYS: return "set " + getIntrinsicStateText(IntrinsicAttack::ALWAYS);
    case ItemAction::INTRINSIC_NO_WEAPON: return "set " + getIntrinsicStateText(IntrinsicAttack::NO_WEAPON);
    case ItemAction::INTRINSIC_NEVER: return "set " + getIntrinsicStateText(IntrinsicAttack::NEVER);
  }
}

void GuiBuilder::drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width, bool darkBg) {
  if (elems.isEmpty())
    return;
  int contentHeight = elems.getSize();
  int margin = 15;
  SGuiElem menu = gui.miniWindow(gui.margins(elems.buildVerticalList(), 5 + margin, margin, margin, margin),
          [&exit] { exit = true; });
  Vec2 size(width + 2 * margin, contentHeight + 2 * margin);
  menuPos.y -= max(0, menuPos.y + size.y - renderer.getSize().y);
  menuPos.x -= max(0, menuPos.x + size.x - renderer.getSize().x);
  menu->setBounds(Rectangle(menuPos, menuPos + size));
  SGuiElem bg = gui.darken();
  bg->setBounds(Rectangle(renderer.getSize()));
  while (1) {
    callbacks.refreshScreen();
    if (darkBg)
      bg->render(renderer);
    menu->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {menu});
      if (exit)
        return;
    }
  }
}

optional<int> GuiBuilder::chooseAtMouse(const vector<string>& elems) {
  auto list = gui.getListBuilder(legendLineHeight);
  bool exit = false;
  optional<int> ret;
  for (int i : All(elems))
    list.addElem(gui.stack(
        gui.button([i, &ret, &exit] { ret = i; exit = true; }),
        gui.uiHighlightMouseOver(),
        gui.label(elems[i])
    ));
  drawMiniMenu(std::move(list), exit, renderer.getMousePos(), 200, false);
  return ret;
}

optional<ItemAction> GuiBuilder::getItemChoice(const ItemInfo& itemInfo, Vec2 menuPos, bool autoDefault) {
  if (itemInfo.actions.empty())
    return none;
  if (itemInfo.actions.size() == 1 && autoDefault)
    return itemInfo.actions[0];
  renderer.flushEvents(SDL::SDL_KEYDOWN);
  int choice = -1;
  optional<int> index = 0;
  disableTooltip = true;
  DestructorFunction dFun([this] { disableTooltip = false; });
  vector<string> options = itemInfo.actions.transform(bindFunction(getActionText));
  options.push_back("cancel");
  int count = options.size();
  SGuiElem stuff = gui.margins(
      drawListGui("", ListElem::convert(options), MenuType::NORMAL, &index, &choice, nullptr), 20, 15, 15, 10);
  stuff = gui.miniWindow(gui.margins(std::move(stuff), 0));
  Vec2 size(*stuff->getPreferredWidth() + 15, *stuff->getPreferredHeight());
  menuPos.x = min(menuPos.x, renderer.getSize().x - size.x);
  menuPos.y = min(menuPos.y, renderer.getSize().y - size.y);
  while (1) {
    callbacks.refreshScreen();
    stuff->setBounds(Rectangle(menuPos, menuPos + size));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {stuff});
      if (choice > -1 && index) {
        if (index < itemInfo.actions.size())
          return itemInfo.actions[*index];
        else
          return none;
      }
      if (choice == -100)
        return none;
      if (event.type == SDL::SDL_MOUSEBUTTONDOWN &&
          !Vec2(event.button.x, event.button.x).inRectangle(stuff->getBounds()))
        return none;
      auto scrollIndex = [&](int dir) {
        if (index)
          index = (*index + dir + count) % count;
        else
          index = 0;
      };
      if (event.type == SDL::SDL_KEYDOWN)
        switch (event.key.keysym.sym) {
          case SDL::SDLK_KP_8:
          case SDL::SDLK_UP:
            scrollIndex(-1);
            break;
          case SDL::SDLK_KP_2:
          case SDL::SDLK_DOWN:
            scrollIndex(1);
            break;
          case SDL::SDLK_KP_5:
          case SDL::SDLK_KP_ENTER:
          case SDL::SDLK_RETURN:
            if (index && index < itemInfo.actions.size())
              return itemInfo.actions[*index];
            break;
          case SDL::SDLK_ESCAPE: return none;
          default: break;
        }
    }
  }
}

vector<SGuiElem> GuiBuilder::drawSkillsList(const PlayerInfo& info) {
  vector<SGuiElem> lines;
  if (!info.skills.empty()) {
    lines.push_back(gui.label("Skills", Color::YELLOW));
    for (auto& elem : info.skills)
      lines.push_back(gui.stack(getTooltip({elem.help}, THIS_LINE),
            gui.label(capitalFirst(elem.name), Color::WHITE)));
    lines.push_back(gui.empty());
  }
  return lines;
}

SGuiElem GuiBuilder::getSpellIcon(const PlayerInfo::Spell& spell, bool active) {
  vector<SGuiElem> ret = makeVec(gui.spellIcon(spell.id));
  if (spell.timeout) {
    ret.push_back(gui.darken());
    ret.push_back(gui.centeredLabel(Renderer::HOR_VER, toString(*spell.timeout)));
  } else
  if (active)
    ret.push_back(gui.button(getButtonCallback({UserInputId::CAST_SPELL, spell.id})));
  ret.push_back(getTooltip({capitalFirst(spell.name), spell.help}, THIS_LINE));
  return gui.stack(std::move(ret));
}

const Vec2 spellIconSize = Vec2(47, 47);

SGuiElem GuiBuilder::drawSpellsList(const PlayerInfo& info, bool active) {
  constexpr int spellsPerRow = 5;
  if (!info.spells.empty()) {
    auto list = gui.getListBuilder(spellIconSize.y);
    list.addElem(gui.label("Spells", Color::YELLOW), legendLineHeight);
    auto line = gui.getListBuilder(spellIconSize.x);
    for (auto& elem : info.spells) {
      line.addElem(getSpellIcon(elem, active));
      if (line.getLength() >= spellsPerRow) {
        list.addElem(line.buildHorizontalList());
        line.clear();
      }
    }
    if (!line.isEmpty())
      list.addElem(line.buildHorizontalList());
    return list.buildVerticalList();
  } else
    return nullptr;
}

vector<SGuiElem> GuiBuilder::drawEffectsList(const PlayerInfo& info) {
  vector<SGuiElem> lines;
  for (int i : All(info.effects)) {
    auto& effect = info.effects[i];
    lines.push_back(gui.stack(
          getTooltip({effect.help}, THIS_LINE + i),
          gui.label(effect.name, effect.bad ? Color::RED : Color::WHITE)));
  }
  return lines;
}

static string getKeybindingDesc(char c) {
  if (c == ' ')
    return "[Space]";
  else
    return "["_s + (char)toupper(c) + "]";
}

static vector<string> help = {
    "Move around with number pad.",
    "Extended attack: ctrl + arrow.",
    "Fast travel: ctrl + arrow.",
    "Fire arrows: alt + arrow.",
};

SGuiElem GuiBuilder::getExpIncreaseLine(const PlayerInfo::LevelInfo& info, ExperienceType type) {
  if (info.limit[type] == 0)
    return nullptr;
  auto line = gui.getListBuilder();
  int i = 0;
  vector<string> attrNames;
  auto attrIcons = gui.getListBuilder();
  for (auto attr : getAttrIncreases()[type]) {
    attrIcons.addElem(gui.topMargin(-3, gui.icon(attr)), 22);
    attrNames.push_back(getName(attr));
  }
  line.addElem(attrIcons.buildHorizontalList(), 80);
  line.addElem(gui.label("+" + toString(0.01 * round(100 * info.level[type])),
      info.warning[type] ? Color::RED : Color::WHITE), 60);
  string limit = toString(info.limit[type]);
  line.addElemAuto(gui.label("  (limit " + limit + ")"));
  vector<string> tooltip {
      getName(type) + " training."_s,
      "Increases " + combine(attrNames) + ".",
      "The creature's limit for this type of training is " + limit + "."};
  if (info.warning[type])
    tooltip.push_back(*info.warning[type]);
  return gui.stack(
             getTooltip(tooltip, THIS_LINE),
             line.buildHorizontalList());
}

SGuiElem GuiBuilder::drawTrainingInfo(const PlayerInfo& info) {
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(gui.label("Training", Color::YELLOW));
  bool empty = true;
  for (auto expType : ENUM_ALL(ExperienceType)) {
    if (auto elem = getExpIncreaseLine(info.levelInfo, expType)) {
      lines.addElem(std::move(elem));
      empty = false;
    }
  }
  if (!empty)
    return lines.buildVerticalList();
  else
    return nullptr;
}

SGuiElem GuiBuilder::drawPlayerInventory(const PlayerInfo& info) {
  GuiFactory::ListBuilder list(gui, legendLineHeight);
  list.addSpace();
  list.addElem(gui.label(info.getTitle(), Color::WHITE));
  vector<SGuiElem> keyElems;
  bool isTutorial = false;
  for (int i : All(info.commands)) {
    if (info.commands[i].tutorialHighlight)
      isTutorial = true;
    if (info.commands[i].active)
      if (auto key = info.commands[i].keybinding)
        keyElems.push_back(gui.keyHandlerChar(getButtonCallback({UserInputId::PLAYER_COMMAND, i}), *key));
  }
  if (!info.commands.empty())
    list.addElem(gui.stack(
        gui.stack(std::move(keyElems)),
        gui.labelHighlight("[Commands]", Color::LIGHT_BLUE),
        gui.conditional(gui.tutorialHighlight(), [=]{ return isTutorial;}),
        gui.buttonRect([this, commands = info.commands] (Rectangle bounds) {
            auto lines = gui.getListBuilder(legendLineHeight);
            bool exit = false;
            for (auto& elem : help)
              lines.addElem(gui.label(elem, Color::LIGHT_BLUE));
            for (int i : All(commands)) {
              auto& command = commands[i];
              function<void()> buttonFun = [] {};
              if (command.active)
                buttonFun = [&exit, i, this] {
                    callbacks.input({UserInputId::PLAYER_COMMAND, i});
                    exit = true;
                };
              auto labelColor = command.active ? Color::WHITE : Color::GRAY;
              auto button = command.keybinding ? gui.buttonChar(buttonFun, *command.keybinding) : gui.button(buttonFun);
              if (command.tutorialHighlight)
                button = gui.stack(gui.tutorialHighlight(), std::move(button));
              lines.addElem(gui.stack(
                  button,
                  command.active ? gui.uiHighlightMouseOver(Color::GREEN) : gui.empty(),
                  gui.tooltip({command.description}),
                  gui.label((command.keybinding ? getKeybindingDesc(*command.keybinding) + " " : ""_s) +
                      command.name, labelColor)));
            }
            for (auto& button : getSettingsButtons())
              lines.addElem(std::move(button));
            drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 260, false);
       })));
  for (auto& elem : drawEffectsList(info))
    list.addElem(std::move(elem));
  list.addSpace();
  for (auto& elem : drawSkillsList(info))
    list.addElem(std::move(elem));
  if (auto spells = drawSpellsList(info, true)) {
    list.addElemAuto(std::move(spells));
    list.addSpace();
  }
  if (info.debt > 0) {
    list.addElem(gui.label("Debt", Color::YELLOW));
    list.addElem(gui.label("Click on debt or on individual items to pay.", Renderer::smallTextSize,
        Color::LIGHT_GRAY), legendLineHeight * 2 / 3);
    list.addElem(gui.stack(
        drawCost({ViewId::GOLD, info.debt}),
        gui.button(getButtonCallback(UserInputId::PAY_DEBT))));
    list.addSpace();
  }
  if (!info.inventory.empty()) {    
    list.addElem(gui.label("Inventory", Color::YELLOW));
    for (auto& item : info.inventory)
      list.addElem(getItemLine(item, [=](Rectangle butBounds) {
            if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), false))
              callbacks.input({UserInputId::INVENTORY_ITEM, InventoryItemInfo{item.ids, *choice}});}));
    double totWeight = 0;
    for (auto& item : info.inventory)
      totWeight += item.weight.value_or(0) * item.number;
    list.addElem(gui.label("Total weight: " + getWeightString(totWeight)));
    list.addElem(gui.label("Capacity: " +  (info.carryLimit ? getWeightString(*info.carryLimit) : "infinite"_s)));
    list.addSpace();
  }
  if (!info.intrinsicAttacks.empty()) {
    list.addElem(gui.label("Intrinsic attacks", Color::YELLOW));
    for (auto& item : info.intrinsicAttacks)
      list.addElem(getItemLine(item, [=](Rectangle butBounds) {
            if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), false))
              callbacks.input({UserInputId::INTRINSIC_ATTACK, InventoryItemInfo{item.ids, *choice}});}));
    list.addSpace();
  }
  if (auto elem = drawTrainingInfo(info))
    list.addElemAuto(std::move(elem));
  return
      gui.scrollable(list.buildVerticalList(), &inventoryScroll, &scrollbarsHeld);
}

static const char* getControlModeName(PlayerInfo::ControlMode m) {
  switch (m) {
    case PlayerInfo::FULL: return "full";
    case PlayerInfo::LEADER: return "leader";
  }
}

SGuiElem GuiBuilder::drawRightPlayerInfo(const PlayerInfo& info) {
  if (highlightedTeamMember && *highlightedTeamMember >= info.teamInfos.size())
    highlightedTeamMember = none;
  auto getIconHighlight = [&] (Color c) { return gui.topMargin(-1, gui.uiHighlight(c)); };
  auto vList = gui.getListBuilder(legendLineHeight);
  auto teamList = gui.getListBuilder();
  for (int i : All(info.teamInfos)) {
    auto& member = info.teamInfos[i];
    auto icon = gui.margins(gui.viewObject(member.viewId, 2), 1);
    if (member.creatureId != info.creatureId)
      icon = gui.stack(
          gui.mouseOverAction([this, i] { highlightedTeamMember = i;},
              [this, i] { if (highlightedTeamMember == i) highlightedTeamMember = none; }),
          gui.mouseHighlight2(getIconHighlight(Color::GREEN)),
          std::move(icon)
      );
    if (info.teamInfos.size() > 1) {
      if (member.creatureId == info.creatureId)
        icon = gui.stack(
            std::move(icon),
            gui.translate(gui.translucentBackgroundPassMouse(gui.translate(gui.labelUnicode(u8"⬆"), Vec2(2, -1))),
                Vec2(16, 50), Vec2(17, 21))
        );
      if (member.isPlayerControlled)
        icon = gui.stack(
            gui.margins(gui.rectangle(Color::GREEN.transparency(1094)), 2),
            std::move(icon)
        );
    }
    if (!member.teamMemberActions.empty()) {
      icon = gui.stack(std::move(icon),
        gui.buttonRect([memberId = member.creatureId, actions = member.teamMemberActions, this] (Rectangle bounds) {
              auto lines = gui.getListBuilder(legendLineHeight);
              bool exit = false;
              optional<TeamMemberAction> ret;
              for (auto action : actions) {
                auto buttonFun = [&exit, &ret, action] {
                  ret = action;
                  exit = true;
                };
                lines.addElem(gui.stack(
                      gui.uiHighlightMouseOver(),
                      gui.button(buttonFun),
                      gui.label(getText(action))));
              }
              drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 200, false);
              if (ret)
                callbacks.input({UserInputId::TEAM_MEMBER_ACTION, TeamMemberActionInfo{*ret, memberId}});
        }));
    }
    teamList.addElemAuto(std::move(icon));
    if (teamList.getLength() >= 6) {
      int bottomMargin = i == info.teamInfos.size() - 1 ? -5 : 15;
      vList.addElemAuto(gui.margins(teamList.buildHorizontalList(), 0, 25, 0, bottomMargin));
      teamList.clear();
    }
  }
  if (!teamList.isEmpty())
    vList.addElemAuto(gui.margins(teamList.buildHorizontalList(), 0, 25, 0, 20));
  vList.addSpace(10);
  if (info.teamInfos.size() > 1)
  vList.addElem(gui.stack(
      gui.labelHighlight("[Control mode: "_s + getControlModeName(info.controlMode) + "]", Color::LIGHT_BLUE),
      gui.button(getButtonCallback(UserInputId::TOGGLE_CONTROL_MODE), gui.getKey(SDL::SDLK_g))
  ));
  vList.addElem(gui.stack(
      gui.labelHighlight("[Exit control mode]", Color::LIGHT_BLUE),
      gui.button(getButtonCallback(UserInputId::EXIT_CONTROL_MODE), gui.getKey(SDL::SDLK_u))
  ));
  vList.addSpace(10);
  vList.addElem(gui.margins(gui.sprite(GuiFactory::TexId::HORI_LINE, GuiFactory::Alignment::TOP), -6, 0, -6, 0), 10);
  vector<SGuiElem> others;
  for (int i : All(info.teamInfos)) {
    auto& elem = info.teamInfos[i];
    if (elem.creatureId != info.creatureId)
      others.push_back(gui.conditional(drawPlayerInventory(elem), [this, i]{ return highlightedTeamMember == i;}));
    else
      others.push_back(gui.conditional(drawPlayerInventory(info),
          [this, i]{ return !highlightedTeamMember || highlightedTeamMember == i;}));
  }
  vList.addMiddleElem(gui.stack(std::move(others)));
  return gui.margins(vList.buildVerticalList(), 6, 0, 15, 5);
}

typedef CreatureInfo CreatureInfo;

struct CreatureMapElem {
  ViewId viewId;
  int count;
  CreatureInfo any;
};

SGuiElem GuiBuilder::drawMinionAndLevel(ViewId viewId, int level, int iconMult) {
  return gui.stack(makeVec(
        gui.viewObject(viewId, iconMult),
        gui.label(toString(level), 12 * iconMult)));
}

SGuiElem GuiBuilder::drawTeams(CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  const int elemWidth = 30;
  auto lines = gui.getListBuilder(legendLineHeight);
  for (int i : All(info.teams)) {
    auto& team = info.teams[i];
    const int numPerLine = 8;
    auto teamLine = gui.getListBuilder(legendLineHeight);
    vector<SGuiElem> currentLine;
    for (auto member : team.members) {
      auto& memberInfo = *info.getMinion(member);
      currentLine.push_back(drawMinionAndLevel(memberInfo.viewId, (int) memberInfo.bestAttack.value, 1));
      if (currentLine.size() >= numPerLine)
        teamLine.addElem(gui.horizontalList(std::move(currentLine), elemWidth));
    }
    if (!currentLine.empty())
      teamLine.addElem(gui.horizontalList(std::move(currentLine), elemWidth));
    ViewId leaderViewId = info.getMinion(team.members[0])->viewId;
    auto selectButton = [this](int teamId) {
      return gui.releaseLeftButton([=]() {
          onTutorialClicked(0, TutorialHighlight::CONTROL_TEAM);
          callbacks.input({UserInputId::SELECT_TEAM, teamId});
      });
    };
    const bool isTutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
    lines.addElemAuto(gui.stack(makeVec(
            gui.mouseOverAction([team, this] { mapGui->highlightTeam(team.members); },
              [team, this] { mapGui->unhighlightTeam(team.members); }),
            cache->get(selectButton, THIS_LINE, team.id),
            gui.conditional(gui.tutorialHighlight(),
                [=]{ return !wasTutorialClicked(0, TutorialHighlight::CONTROL_TEAM) && isTutorialHighlight; }),
            gui.uiHighlightConditional([team] () { return team.highlight; }),
            gui.uiHighlightMouseOver(),
            gui.dragListener([this, team](DragContent content) {
                UserInputId id;
                switch (content.getId()) {
                  case DragContentId::CREATURE:
                    id = UserInputId::ADD_TO_TEAM; break;
                  case DragContentId::CREATURE_GROUP:
                    id = UserInputId::ADD_GROUP_TO_TEAM; break;
                  default:
                    return;
                }
                callbacks.input({id, TeamCreatureInfo{team.id, content.get<UniqueEntity<Creature>::Id>()}});}),
            gui.dragSource({DragContentId::TEAM, team.id},
                           [=] { return gui.viewObject(leaderViewId);}),
            gui.getListBuilder(22)
              .addElem(gui.topMargin(8, gui.icon(GuiFactory::TEAM_BUTTON, GuiFactory::Alignment::TOP_CENTER)))
              .addElemAuto(teamLine.buildVerticalList()).buildHorizontalList())));
  }
  const char* hint = "Drag and drop minions onto the [new team] button to create a new team. "
    "You can drag them both from the map and the menus.";
  if (!tutorial || info.teams.empty()) {
    const bool isTutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::NEW_TEAM);
    lines.addElem(gui.stack(makeVec(
          gui.dragListener([this](DragContent content) {
              UserInputId id;
              switch (content.getId()) {
                case DragContentId::CREATURE:
                  id = UserInputId::CREATE_TEAM; break;
                case DragContentId::CREATURE_GROUP:
                  id = UserInputId::CREATE_TEAM_FROM_GROUP; break;
                default:
                  return;
              }
              callbacks.input({id, content.get<UniqueEntity<Creature>::Id>() });}),
          gui.conditional(gui.uiHighlightMouseOver(), [&]{return gui.getDragContainer().hasElement();} ),
          gui.conditional(gui.tutorialHighlight(), [yes = isTutorialHighlight && info.teams.empty()]{ return yes; }),
          getHintCallback({hint}),
          gui.button([this, hint] { callbacks.info(hint); }),
          gui.label("[new team]", Color::WHITE))));
  }
  return lines.buildVerticalList();
}

vector<SGuiElem> GuiBuilder::getSettingsButtons() {
  auto makeSetting = [&](bool& setting, const char* name, const char* hint) {
    return gui.stack(makeVec(
          getHintCallback({hint}),
          gui.label(name, [&setting]{ return setting ? Color::GREEN : Color::WHITE;}),
          gui.button([&setting] { setting = !setting; })));
  };

  return makeVec(
      makeSetting(mapGui->highlightMorale, "Highlight morale",
          "Morale affects minion's productivity and chances of fleeing from battle."),
      makeSetting(mapGui->highlightEnemies, "Highlight enemies", ""),
      makeSetting(mapGui->displayAllHealthBars, "Display health bars", ""),
      makeSetting(mapGui->hideFullHealthBars, "Hide full health bars", ""),
      makeSetting(mapGui->colorWoundedRed, "Color wounded creatures", "")
    );
}

SGuiElem GuiBuilder::drawMinions(CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  int newHash = info.getHash();
  if (newHash != minionsHash) {
    minionsHash = newHash;
    auto list = gui.getListBuilder(legendLineHeight);
    list.addElem(gui.label(info.monsterHeader, Color::WHITE));
    auto selectButton = [this](UniqueEntity<Creature>::Id creatureId) {
      return gui.releaseLeftButton(getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, creatureId}));
    };
    for (int i : All(info.minionGroups)) {
      auto& elem = info.minionGroups[i];
      auto line = gui.getListBuilder();
      line.addElem(gui.viewObject(elem.viewId), 40);
      SGuiElem tmp = gui.label(toString(elem.count) + "   " + elem.name, Color::WHITE);
      line.addElem(std::move(tmp), 200);
      list.addElem(gui.leftMargin(20, gui.stack(
          cache->get(selectButton, THIS_LINE, elem.creatureId),
          gui.dragSource({DragContentId::CREATURE_GROUP, elem.creatureId},
              [=]{ return gui.getListBuilder(10)
                  .addElemAuto(gui.label(toString(elem.count) + " "))
                  .addElem(gui.viewObject(elem.viewId)).buildHorizontalList();}),
          gui.uiHighlightConditional([highlight = elem.highlight]{return highlight;}),
          line.buildHorizontalList()
       )));
    }
    list.addElem(gui.label("Teams: ", Color::WHITE));
    list.addElemAuto(drawTeams(info, tutorial));
    list.addSpace();
    list.addElem(gui.stack(
              gui.label("Show tasks", [=]{ return showTasks ? Color::GREEN : Color::WHITE;}),
              gui.button([this] { bool was = showTasks; closeOverlayWindowsAndClearButton(); showTasks = !was; })));
    for (auto& button : getSettingsButtons())
      list.addElem(std::move(button));
    list.addElem(gui.stack(
              gui.label("Show message history"),
              gui.button(getButtonCallback(UserInputId::SHOW_HISTORY))));
    list.addSpace();
    if (!info.enemyGroups.empty()) {
      list.addElem(gui.label("Enemies:", Color::WHITE));
      for (auto& elem : info.enemyGroups){
        vector<SGuiElem> line;
        line.push_back(gui.viewObject(elem.viewId));
        line.push_back(gui.label(toString(elem.count) + "   " + elem.name, Color::WHITE));
        list.addElem(gui.stack(
            gui.button(getButtonCallback({UserInputId::GO_TO_ENEMY, elem.creatureId})),
            gui.uiHighlightMouseOver(),
            gui.horizontalList(std::move(line), 20)));
      }
    }
    minionsCache = gui.scrollable(list.buildVerticalList(), &minionsScroll, &scrollbarsHeld);
  }
  return gui.external(minionsCache.get());
}

const int taskMapWindowWidth = 400;

SGuiElem GuiBuilder::drawTasksOverlay(const CollectiveInfo& info) {
  vector<SGuiElem> lines;
  vector<SGuiElem> freeLines;
  for (auto& elem : info.taskMap) {
    if (elem.creature)
      if (auto minion = info.getMinion(*elem.creature)) {
        lines.push_back(gui.horizontalList(makeVec(
                gui.viewObject(minion->viewId),
                gui.label(elem.name, elem.priority ? Color::GREEN : Color::WHITE)), 35));
        continue;
      }
    freeLines.push_back(gui.horizontalList(makeVec(
            gui.empty(),
            gui.label(elem.name, elem.priority ? Color::GREEN : Color::WHITE)), 35));
  }
  if (info.taskMap.empty())
    lines.push_back(gui.label("No tasks present."));
  int lineHeight = 25;
  int margin = 20;
  append(lines, std::move(freeLines));
  int numLines = lines.size();
  return gui.preferredSize(taskMapWindowWidth, numLines * lineHeight + 2 * margin,
      gui.conditionalStopKeys(gui.stack(
          gui.keyHandler([this]{showTasks = false;}, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          gui.miniWindow(
        gui.margins(gui.scrollable(gui.verticalList(std::move(lines), lineHeight), &tasksScroll, &scrollbarsHeld),
          margin))), [this] { return showTasks; }));
}

SGuiElem GuiBuilder::drawRansomOverlay(const optional<CollectiveInfo::Ransom>& ransom) {
  if (!ransom)
    return gui.empty();
  GuiFactory::ListBuilder lines(gui, legendLineHeight);
  lines.addElem(gui.label(ransom->attacker + " demand " + toString(ransom->amount.second)
        + " gold for not attacking. Agree?"));
  if (ransom->canAfford)
    lines.addElem(gui.leftMargin(25, gui.stack(
          gui.mouseHighlight2(gui.highlight(legendLineHeight)),
          gui.button(getButtonCallback(UserInputId::PAY_RANSOM)),
          gui.label("Yes"))));
  else
    lines.addElem(gui.leftMargin(25, gui.label("Yes", Color::GRAY)));
  lines.addElem(gui.leftMargin(25, gui.stack(
        gui.mouseHighlight2(gui.highlight(legendLineHeight)),
        gui.button(getButtonCallback(UserInputId::IGNORE_RANSOM)),
        gui.label("No"))));
  return gui.setWidth(600, gui.miniWindow(gui.margins(lines.buildVerticalList(), 20)));
}

SGuiElem GuiBuilder::drawRebellionChanceText(CollectiveInfo::RebellionChance chance) {
  switch (chance) {
    case CollectiveInfo::RebellionChance::HIGH:
      return gui.label("high", Color::RED);
    case CollectiveInfo::RebellionChance::MEDIUM:
      return gui.label("medium", Color::ORANGE);
    case CollectiveInfo::RebellionChance::LOW:
      return gui.label("low", Color::YELLOW);
  }
}

SGuiElem GuiBuilder::drawWarningWindow(const optional<CollectiveInfo::RebellionChance>& rebellionChance,
    const optional<CollectiveInfo::NextWave>& wave) {
  SGuiElem window = gui.empty();
  if (rebellionChance) {
    GuiFactory::ListBuilder lines(gui, legendLineHeight);
    lines.addElem(gui.getListBuilder()
        .addElemAuto(gui.label("Chance of prisoner escape: "))
        .addElemAuto(drawRebellionChanceText(*rebellionChance))
        .buildHorizontalList());
    lines.addElem(gui.label("Remove prisoners or increase armed forces."));
    window = gui.setWidth(400, gui.translucentBackgroundWithBorder(gui.stack(
        gui.margins(lines.buildVerticalList(), 10),
        gui.alignment(GuiFactory::Alignment::TOP_RIGHT, gui.preferredSize(40, 40, gui.stack(
            gui.leftMargin(22, gui.label("x")),
            gui.button(getButtonCallback(UserInputId::DISMISS_WARNING_WINDOW)))))
      )));
  }
  return gui.getListBuilder().addElemAuto(std::move(window))
      .addSpace(10)
      .addElemAuto(drawNextWaveOverlay(wave)).buildVerticalList();
}

SGuiElem GuiBuilder::drawNextWaveOverlay(const optional<CollectiveInfo::NextWave>& wave) {
  if (!wave)
    return gui.empty();
  GuiFactory::ListBuilder lines(gui, legendLineHeight);
  lines.addElem(gui.label("Next enemy wave:"));
  lines.addElem(gui.getListBuilder()
        .addElem(gui.viewObject(wave->viewId), 30)
        .addElemAuto(gui.label(wave->attacker))// + " (" + toString(wave->count) + ")"))
        .buildHorizontalList());
  lines.addElem(gui.label("Attacking in " + toString(wave->numTurns) + " turns."));
  return gui.setWidth(300, gui.translucentBackgroundWithBorder(gui.stack(
        gui.margins(lines.buildVerticalList(), 10),
        gui.alignment(GuiFactory::Alignment::TOP_RIGHT, gui.preferredSize(40, 40, gui.stack(
            gui.leftMargin(22, gui.label("x")),
            gui.button(getButtonCallback(UserInputId::DISMISS_NEXT_WAVE)))))
      )));
}

SGuiElem GuiBuilder::drawWorkshopsOverlay(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  if (!info.chosenWorkshop)
    return gui.empty();
  int margin = 20;
  int rightElemMargin = 10;
  auto& options = info.chosenWorkshop->options;
  auto& queued = info.chosenWorkshop->queued;
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(gui.label("Available:", Color::YELLOW));
  for (int i : All(options)) {
    auto& elem = options[i];
    auto line = gui.getListBuilder();
    line.addElem(gui.viewObject(elem.viewId), 35);
    line.addElem(gui.label(elem.name, elem.unavailable ? Color::GRAY : Color::WHITE), 10);
    line.addBackElem(gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(*elem.price)), 80);
    SGuiElem guiElem = line.buildHorizontalList();
    if (elem.tutorialHighlight)
      guiElem = gui.stack(gui.tutorialHighlight(), std::move(guiElem));
    if (elem.unavailable) {
      CHECK(!elem.unavailableReason.empty());
      guiElem = gui.stack(getTooltip({elem.unavailableReason, elem.description}, THIS_LINE + i), std::move(guiElem));
    }
    else
      guiElem = gui.stack(
          getTooltip({elem.description}, THIS_LINE),
          gui.uiHighlightMouseOver(Color::GREEN),
          std::move(guiElem),
          gui.button(getButtonCallback({UserInputId::WORKSHOP_ADD, i})));
    lines.addElem(gui.rightMargin(rightElemMargin, std::move(guiElem)));
  }
  auto lines2 = gui.getListBuilder(legendLineHeight);
  lines2.addElem(gui.label("In production:", Color::YELLOW));
  for (int i : All(queued)) {
    auto& elem = queued[i];
    auto line = gui.getListBuilder();
    line.addMiddleElem(gui.stack(
        gui.buttonRect([=] (Rectangle bounds) {
              auto lines = gui.getListBuilder(legendLineHeight);
              bool exit = false;
              optional<ItemAction> ret;
              for (auto action : elem.actions) {
                function<void()> buttonFun = [] {};
                if (!elem.unavailable)
                  buttonFun = [&exit, &ret, action] {
                      ret = action;
                      exit = true;
                  };
                lines.addElem(gui.stack(
                      gui.button(buttonFun),
                      gui.label(getActionText(action))));
              }
              drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 200, false);
              if (ret)
                callbacks.input({UserInputId::WORKSHOP_ITEM_ACTION,
                    WorkshopQueuedActionInfo{i, *ret}});
        }),
        gui.getListBuilder()
            .addElem(gui.viewObject(elem.viewId), 35)
            .addElemAuto(gui.label(elem.name)).buildHorizontalList()));
    line.addBackElem(gui.stack(
        gui.uiHighlightMouseOver(Color::GREEN),
        gui.button(getButtonCallback({UserInputId::WORKSHOP_ITEM_ACTION,
            WorkshopQueuedActionInfo{i, ItemAction::CHANGE_NUMBER}}))), 35);
    line.addBackElem(gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(*elem.price)), 80);
    lines2.addElem(gui.stack(
        gui.bottomMargin(5,
            gui.progressBar(Color::DARK_GREEN.transparency(128), elem.productionState)),
        gui.rightMargin(rightElemMargin, gui.stack(
            //getTooltip({elem.description}, THIS_LINE), // this interferes with the mini menu
            gui.uiHighlightMouseOver(Color::GREEN),
            line.buildHorizontalList()
    ))));
  }
  return gui.preferredSize(860, 600,
    gui.miniWindow(gui.stack(
      gui.keyHandler(getButtonCallback({UserInputId::WORKSHOP, info.chosenWorkshop->index}),
        {gui.getKey(SDL::SDLK_ESCAPE)}, true),
      gui.getListBuilder(430)
            .addElem(gui.margins(gui.scrollable(
                  lines.buildVerticalList(), &workshopsScroll, &scrollbarsHeld), margin))
            .addElem(gui.margins(
                gui.scrollable(lines2.buildVerticalList(), &workshopsScroll2, &scrollbarsHeld2),
                margin)).buildHorizontalList())));
}

SGuiElem GuiBuilder::drawLibraryOverlay(const CollectiveInfo& collectiveInfo, const optional<TutorialInfo>& tutorial) {
  if (!collectiveInfo.libraryInfo)
    return gui.empty();
  auto& info = *collectiveInfo.libraryInfo;
  int margin = 20;
  int rightElemMargin = 10;
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(gui.rightMargin(rightElemMargin, gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(info.resource))));
  if (info.warning)
    lines.addElem(gui.label(*info.warning, Color::RED));
  lines.addElem(gui.label("Available technology:", Color::YELLOW));
  for (int i : All(info.available)) {
    auto& elem = info.available[i];
    auto line = gui.getListBuilder()
        .addElem(gui.label(elem.name, elem.active ? Color::WHITE : Color::GRAY), 10)
        .addBackElem(gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(elem.cost)), 80)
        .buildHorizontalList();
    line = gui.stack(std::move(line), getTooltip({elem.description}, THIS_LINE));
    if (elem.tutorialHighlight && tutorial && tutorial->highlights.contains(*elem.tutorialHighlight))
      line = gui.stack(gui.tutorialHighlight(), std::move(line));
    if (elem.active)
      line = gui.stack(
          gui.uiHighlightMouseOver(Color::GREEN),
          std::move(line),
          gui.button(getButtonCallback({UserInputId::LIBRARY_ADD, i})));
    lines.addElem(gui.rightMargin(rightElemMargin, std::move(line)));
  }
  lines.addSpace(legendLineHeight * 2 / 3);
  if (!info.researched.empty())
  lines.addElem(gui.label("Researched technology:", Color::YELLOW));
  for (int i : All(info.researched)) {
    auto& elem = info.researched[i];
    auto line = gui.getListBuilder()
        .addElem(gui.label(elem.name, Color::GRAY), 10)
        .addBackElem(gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(elem.cost)), 80)
        .buildHorizontalList();
    line = gui.stack(std::move(line), getTooltip({elem.description}, THIS_LINE));
    lines.addElem(gui.rightMargin(rightElemMargin, std::move(line)));
  }
  int height = lines.getSize();
  return gui.preferredSize(500, height + 2 * margin,
      gui.miniWindow(gui.stack(
          gui.keyHandler(getButtonCallback(UserInputId::LIBRARY_CLOSE), {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          gui.margins(gui.scrollable(lines.buildVerticalList(), &libraryScroll, &scrollbarsHeld), margin))));
}

SGuiElem GuiBuilder::drawMinionsOverlay(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  int margin = 20;
  int minionListWidth = 220;
  if (!info.chosenCreature)
    return gui.empty();
  setCollectiveTab(CollectiveTab::MINIONS);
  SGuiElem minionPage;
  auto& minions = info.chosenCreature->creatures;
  auto current = info.chosenCreature->chosenId;
  for (int i : All(minions))
    if (minions[i].creatureId == current)
      minionPage = gui.margins(drawMinionPage(minions[i], info, tutorial), 10, 15, 10, 10);
  if (!minionPage)
    return gui.empty();
  SGuiElem menu;
  SGuiElem leftSide = drawMinionButtons(minions, current, info.chosenCreature->teamId);
  if (info.chosenCreature->teamId) {
    auto list = gui.getListBuilder(legendLineHeight);
    list.addElem(gui.stack(
        gui.button(getButtonCallback({UserInputId::CANCEL_TEAM, *info.chosenCreature->teamId})),
        gui.labelHighlight("[Disband team]", Color::LIGHT_BLUE)));
    list.addElem(gui.label("Control a chosen minion to", Renderer::smallTextSize,
          Color::LIGHT_GRAY), Renderer::smallTextSize + 2);
    list.addElem(gui.label("command the team.", Renderer::smallTextSize,
          Color::LIGHT_GRAY));
    list.addElem(gui.empty(), legendLineHeight);
    leftSide = gui.marginAuto(list.buildVerticalList(), std::move(leftSide), GuiFactory::TOP);
  }
  menu = gui.stack(
      gui.horizontalList(makeVec(
          gui.margins(std::move(leftSide), 8, 15, 5, 0),
          gui.margins(gui.sprite(GuiFactory::TexId::VERT_BAR_MINI, GuiFactory::Alignment::LEFT),
            0, -15, 0, -15)), minionListWidth),
      gui.leftMargin(minionListWidth + 20, std::move(minionPage)));
  return gui.preferredSize(600 + minionListWidth, 600,
      gui.miniWindow(gui.stack(
      gui.keyHandler(getButtonCallback({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()}),
        {gui.getKey(SDL::SDLK_ESCAPE)}, true),
      gui.margins(std::move(menu), margin))));
}

SGuiElem GuiBuilder::drawBuildingsOverlay(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> elems;
  map<string, GuiFactory::ListBuilder> overlaysMap;
  int margin = 20;
  for (int i : All(info.buildings)) {
    auto& elem = info.buildings[i];
    if (!elem.groupName.empty()) {
      if (!overlaysMap.count(elem.groupName))
        overlaysMap.emplace(make_pair(elem.groupName, gui.getListBuilder(legendLineHeight)));
      overlaysMap.at(elem.groupName).addElem(getButtonLine(elem, i, CollectiveTab::BUILDINGS, tutorial));
      elems.push_back(gui.setWidth(300, gui.conditional(
            gui.miniWindow(gui.margins(getButtonLine(elem, i, CollectiveTab::BUILDINGS, tutorial), margin)),
            [i, this] { return getActiveButton(CollectiveTab::BUILDINGS) == i;})));
    }
  }
  for (auto& elem : overlaysMap) {
    auto& lines = elem.second;
    string groupName = elem.first;
    elems.push_back(gui.setWidth(300, gui.conditionalStopKeys(
          gui.miniWindow(gui.stack(
              gui.keyHandler([=] { clearActiveButton(); }, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
              gui.margins(lines.buildVerticalList(), margin))),
          [isRansom = !!info.ransom, this, groupName] {
              return !isRansom && collectiveTab == CollectiveTab::BUILDINGS && activeGroup == groupName;})));
  }
  return gui.stack(std::move(elems));
}

SGuiElem GuiBuilder::getClickActions(const ViewObject& object) {
  auto lines = gui.getListBuilder(legendLineHeight * 2 / 3);
  if (!object.getClickAction().empty()) {
    lines.addElem(gui.label(object.getClickAction()));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!object.getExtendedActions().empty()) {
    lines.addElem(gui.label("Right click:", Color::LIGHT_BLUE));
    for (auto action : object.getExtendedActions())
      lines.addElem(gui.label(action, Color::LIGHT_GRAY));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!lines.isEmpty())
    return lines.buildVerticalList();
  else
    return nullptr;
}

SGuiElem GuiBuilder::drawMapHintOverlay() {
  auto lines = gui.getListBuilder(legendLineHeight);
  vector<SGuiElem> allElems;
  if (!hint.empty()) {
    for (auto& line : hint)
      if (!line.empty())
        lines.addElem(gui.label(line));
  } else {
    auto& highlighted = mapGui->getLastHighlighted();
    if (auto& viewObject = highlighted.object) {
      lines.addElem(gui.getListBuilder()
            .addElem(gui.viewObject(viewObject->id()), 30)
            .addElemAuto(gui.label(viewObject->getDescription()))
            .buildHorizontalList());
      if (viewObject->hasModifier(ViewObject::Modifier::HOSTILE))
        lines.addElem(gui.label("Hostile", Color::ORANGE));
      for (auto status : viewObject->getCreatureStatus()) {
        lines.addElem(gui.label(getName(status), getColor(status)));
        lines.addElem(gui.label(getDescription(status), getColor(status)));
        break;
      }
      if (auto actions = getClickActions(*viewObject))
        if (highlighted.creaturePos)
          allElems.push_back(gui.absolutePosition(gui.translucentBackgroundWithBorderPassMouse(gui.margins(
              gui.setHeight(*actions->getPreferredHeight(), actions), 5, 1, 5, -2)),
              *highlighted.creaturePos + Vec2(60, 60)));
      if (!viewObject->getBadAdjectives().empty()) {
        lines.addElemAuto(gui.labelMultiLineWidth(viewObject->getBadAdjectives(), legendLineHeight * 2 / 3, 300,
            Renderer::textSize, Color::RED, ','));
        lines.addSpace(legendLineHeight / 3);
      }
      if (!viewObject->getGoodAdjectives().empty()) {
        lines.addElemAuto(gui.labelMultiLineWidth(viewObject->getGoodAdjectives(), legendLineHeight * 2 / 3, 300,
            Renderer::textSize, Color::GREEN, ','));
        lines.addSpace(legendLineHeight / 3);
      }
      if (auto& attributes = viewObject->getCreatureAttributes())
        lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(*attributes)));
      if (auto efficiency = viewObject->getAttribute(ViewObjectAttribute::EFFICIENCY))
        lines.addElem(gui.label("Efficiency: " + toString((int) (100.0f * *efficiency))));
      if (viewObject->hasModifier(ViewObjectModifier::PLANNED))
        lines.addElem(gui.label("Planned"));
      //if (indoors)
      //  ret.push_back(*indoors ? "Indoors" : "Outdoors");
      if (auto morale = viewObject->getAttribute(ViewObjectAttribute::MORALE))
        lines.addElem(gui.label("Morale: " + toString(*morale)));
    }
    if (highlighted.tilePos)
      lines.addElem(gui.label("Position: " + toString(*highlighted.tilePos)));
  }
  if (!lines.isEmpty())
    allElems.push_back(gui.margins(gui.translucentBackgroundWithBorderPassMouse(
        gui.margins(lines.buildVerticalList(), 10, 10, 10, 22)), 0, 0, -2, -2));
  return gui.stack(allElems);
}

void GuiBuilder::drawOverlays(vector<OverlayInfo>& ret, GameInfo& info) {
  if (info.tutorial)
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawTutorialOverlay, this), THIS_LINE,
         *info.tutorial), OverlayInfo::TUTORIAL});
  ret.push_back({drawMapHintOverlay(), OverlayInfo::MAP_HINT});
  switch (info.infoType) {
    case GameInfo::InfoType::BAND: {
      auto& collectiveInfo = *info.playerInfo.getReferenceMaybe<CollectiveInfo>();
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawImmigrationOverlay, this), THIS_LINE,
           collectiveInfo, info.tutorial), OverlayInfo::IMMIGRATION});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawRansomOverlay, this), THIS_LINE,
           collectiveInfo.ransom), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawWarningWindow, this), THIS_LINE,
           collectiveInfo.rebellionChance, collectiveInfo.nextWave), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawMinionsOverlay, this), THIS_LINE,
           collectiveInfo, info.tutorial), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawWorkshopsOverlay, this), THIS_LINE,
           collectiveInfo, info.tutorial), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawLibraryOverlay, this), THIS_LINE,
           collectiveInfo, info.tutorial), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawTasksOverlay, this), THIS_LINE,
           collectiveInfo), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawBuildingsOverlay, this), THIS_LINE,
           collectiveInfo, info.tutorial), OverlayInfo::TOP_LEFT});
      if (immigrantHelpOpen)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawImmigrationHelp, this), THIS_LINE,
            collectiveInfo), OverlayInfo::BOTTOM_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawGameSpeedDialog, this), THIS_LINE),
           OverlayInfo::GAME_SPEED});
      break;
    }
    case GameInfo::InfoType::PLAYER: {
      auto& playerInfo = *info.playerInfo.getReferenceMaybe<PlayerInfo>();
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawPlayerOverlay, this), THIS_LINE,
           playerInfo), OverlayInfo::TOP_LEFT});
      break;
    }
    default:
      break;
  }
}

int GuiBuilder::getNumMessageLines() const {
  return 3;
}

static Color makeBlack(const Color& col, double freshness) {
  double amount = 0.5 + freshness / 2;
  return Color(col.r * amount, col.g * amount, col.b * amount);
}

static Color getMessageColor(MessagePriority priority) {
  switch (priority) {
    case MessagePriority::NORMAL: return Color::WHITE;
    case MessagePriority::HIGH: return Color::ORANGE;
    case MessagePriority::CRITICAL: return Color::RED;
  }
}

static Color getMessageColor(const PlayerMessage& msg) {
  return makeBlack(getMessageColor(msg.getPriority()), msg.getFreshness());
}

const int messageArrowLength = 15;

static int getMessageLength(Renderer& renderer, const PlayerMessage& msg) {
  return renderer.getTextLength(msg.getText()) + (msg.isClickable() ? messageArrowLength : 0);
}

static int getNumFitting(Renderer& renderer, const vector<PlayerMessage>& messages, int maxLength, int numLines) {
  int currentWidth = 0;
  int currentLine = 0;
  for (int i = messages.size() - 1; i >= 0; --i) {
    int length = getMessageLength(renderer, messages[i]);
    length = min(length, maxLength);
    if (currentWidth > 0)
      ++length;
    if (currentWidth + length > maxLength) {
      if (currentLine == numLines - 1)
        return messages.size() - i - 1;
      currentWidth = 0;
      ++currentLine;
    }
    currentWidth += length;
  }
  return messages.size();
}

static vector<vector<PlayerMessage>> fitMessages(Renderer& renderer, const vector<PlayerMessage>& messages,
    int maxLength, int numLines) {
  int currentWidth = 0;
  vector<vector<PlayerMessage>> ret {{}};
  int numFitting = getNumFitting(renderer, messages, maxLength, numLines);
  for (int i : Range(messages.size() - numFitting, messages.size())) {
    int length = getMessageLength(renderer, messages[i]);
    length = min(length, maxLength);
    if (currentWidth > 0)
      ++length;
    if (currentWidth + length > maxLength) {
      if (ret.size() == numLines)
        break;
      currentWidth = 0;
      ret.emplace_back();
    }
    currentWidth += length;
    ret.back().push_back(messages[i]);
  }
  return ret;
}

static void cutToFit(Renderer& renderer, string& s, int maxLength) {
  while (!s.empty() && renderer.getTextLength(s) > maxLength)
    s.pop_back();
}

SGuiElem GuiBuilder::drawMessages(const vector<PlayerMessage>& messageBuffer, int maxMessageLength) {
  int hMargin = 10;
  int vMargin = 5;
  vector<vector<PlayerMessage>> messages = fitMessages(renderer, messageBuffer, maxMessageLength - 2 * hMargin,
      getNumMessageLines());
  int lineHeight = 20;
  vector<SGuiElem> lines;
  for (int i : All(messages)) {
    GuiFactory::ListBuilder line(gui);
    for (auto& message : messages[i]) {
      string text = (line.isEmpty() ? "" : " ") + message.getText();
      cutToFit(renderer, text, maxMessageLength - 2 * hMargin);
      if (message.isClickable()) {
        line.addElemAuto(gui.stack(
              gui.button(getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
              gui.labelHighlight(text, getMessageColor(message))));
        line.addElemAuto(gui.labelUnicode(u8"➚", getMessageColor(message)));
      } else
      line.addElemAuto(gui.stack(
            gui.button(getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
            gui.label(text, getMessageColor(message))));
    }
    if (!messages[i].empty())
      lines.push_back(line.buildHorizontalList());
  }
  if (!lines.empty())
    return gui.setWidth(maxMessageLength, gui.translucentBackground(
        gui.margins(gui.verticalList(std::move(lines), lineHeight), hMargin, vMargin, hMargin, vMargin)));
  else
    return gui.empty();
}

SGuiElem GuiBuilder::getVillageStateLabel(VillageInfo::Village::State state) {
  switch (state) {
    case VillageInfo::Village::FRIENDLY: return gui.label("Friendly", Color::GREEN);
    case VillageInfo::Village::HOSTILE: return gui.label("Hostile", Color::ORANGE);
    case VillageInfo::Village::CONQUERED: return gui.label("Conquered", Color::LIGHT_BLUE);
  }
}

static const char* getVillageActionText(VillageAction action) {
  switch (action) {
    case VillageAction::TRADE:
      return "Trade";
    case VillageAction::PILLAGE:
      return "Pillage";
  }
}

SGuiElem GuiBuilder::getVillageActionButton(UniqueEntity<Collective>::Id id, VillageInfo::Village::ActionInfo action) {
  if (action.disabledReason)
    return gui.stack(
        gui.label(getVillageActionText(action.action), Color::GRAY),
        getTooltip({*action.disabledReason}, THIS_LINE));
  else
    return gui.stack(
        gui.labelHighlight(getVillageActionText(action.action), Color::GREEN),
        gui.button(getButtonCallback({UserInputId::VILLAGE_ACTION,
            VillageActionInfo{id, action.action}})));
}

static Color getTriggerColor(double value) {
  return Color::f(1, max(0.0, 1 - value * 500), max(0.0, 1 - value * 500));
}

void GuiBuilder::showAttackTriggers(const vector<VillageInfo::Village::TriggerInfo>& triggers, Vec2 pos) {
  vector<SGuiElem> elems;
  for (auto& trigger : triggers)
#ifdef RELEASE
    if (trigger.value > 0)
#endif
    elems.push_back(gui.label(trigger.name
#ifndef RELEASE
          + " " + toString(trigger.value)
#endif
          ,getTriggerColor(trigger.value)));
  bool exit = false;
  if (!elems.empty()) {
    auto list = gui.getListBuilder(legendLineHeight);
    list.addElem(gui.label("Potential attack triggers:"), titleLineHeight);
    for (auto& elem : elems)
      list.addElem(std::move(elem));
    drawMiniMenu(std::move(list), exit, pos, 300, true);
  }
}

static const char* getHeader(VillainType type) {
  switch (type) {
    case VillainType::MAIN: return "Main villains:";
    case VillainType::LESSER: return "Lesser villains:";
    case VillainType::ALLY: return "Allies:";
    case VillainType::NONE: return "Other:";
    case VillainType::PLAYER: return "Player:";
  }
}

SGuiElem GuiBuilder::drawVillages(VillageInfo& info) {
  int currentHash = combineHash(info);
  if (currentHash != villagesHash) {
    villagesHash = currentHash;
    auto lines = gui.getListBuilder(legendLineHeight);
    int titleMargin = -11;
    int numConquered = 0;
    for (auto& villain : info.villages[VillainType::MAIN])
      if (villain.state == villain.CONQUERED)
        ++numConquered;
    lines.addElem(gui.leftMargin(titleMargin, gui.label(toString(numConquered) + "/" +
            toString(info.villages[VillainType::MAIN].size()) + " main villains conquered.")), titleLineHeight);
    for (auto villainType : {VillainType::MAIN, VillainType::LESSER, VillainType::ALLY, VillainType::NONE}) {
      if (info.villages[villainType].size() > 0) {
        lines.addSpace();
        lines.addElem(gui.leftMargin(titleMargin, gui.label(getHeader(villainType))), titleLineHeight);
      }
      for (auto& elem : info.villages[villainType]) {
        string title = capitalFirst(elem.name) + (elem.tribeName.empty() ?
              string() : " (" + elem.tribeName + ")");
        SGuiElem header;
        if (elem.access == VillageInfo::Village::LOCATION)
          header = gui.stack(gui.button(getButtonCallback({UserInputId::GO_TO_VILLAGE, elem.id})),
            gui.getListBuilder()
                .addElemAuto(gui.labelHighlight(title))
                .addSpace(7)
                .addElemAuto(gui.labelUnicode(u8"➚")).buildHorizontalList());
        else
          header = gui.label(title);
        lines.addElem(std::move(header));
        if (elem.access == VillageInfo::Village::NO_LOCATION)
          lines.addElem(gui.leftMargin(40, gui.label("Location unknown", Color::LIGHT_BLUE)));
        else if (elem.access == VillageInfo::Village::INACTIVE)
          lines.addElem(gui.leftMargin(40, gui.label("Outside influence zone", Color::GRAY)));
        GuiFactory::ListBuilder line(gui);
        line.addElemAuto(gui.margins(getVillageStateLabel(elem.state), 40, 0, 40, 0));
        vector<VillageInfo::Village::TriggerInfo> triggers = elem.triggers;
        sort(triggers.begin(), triggers.end(),
            [] (const VillageInfo::Village::TriggerInfo& t1, const VillageInfo::Village::TriggerInfo& t2) {
                return t1.value > t2.value;});
    #ifdef RELEASE
        triggers = triggers.filter([](const VillageInfo::Village::TriggerInfo& t) { return t.value > 0;});
    #endif
        if (!triggers.empty())
          line.addElemAuto(gui.stack(
              gui.labelHighlight("Triggers", Color::RED),
              gui.buttonRect([this, triggers](Rectangle bounds) {
                  showAttackTriggers(triggers, bounds.topRight() + Vec2(20, 0));})));
        lines.addElem(line.buildHorizontalList());
        for (auto action : elem.actions)
          lines.addElem(gui.margins(getVillageActionButton(elem.id, action), 40, 0, 0, 0));

      }
    }
    if (lines.isEmpty())
      return gui.label("No foreign tribes discovered.");
    int numVillains = info.numTotalVillains;
    if (numSeenVillains == -1)
      numSeenVillains = numVillains;
    villagesCache = gui.stack(
      gui.onRenderedAction([this, numVillains] { numSeenVillains = numVillains;}),
      gui.scrollable(lines.buildVerticalList(), &villagesScroll, &scrollbarsHeld));
  }
  return gui.external(villagesCache.get());
}

const double menuLabelVPadding = 0.15;

Rectangle GuiBuilder::getMenuPosition(MenuType type, int numElems) {
  int windowWidth = 800;
  int windowHeight = 400;
  int ySpacing;
  int yOffset = 0;
  switch (type) {
    case MenuType::YES_NO:
      ySpacing = (renderer.getSize().y - 250) / 2;
      break;
    case MenuType::MAIN_NO_TILES:
      ySpacing = (renderer.getSize().y - windowHeight) / 2;
      break;
    case MenuType::MAIN:
      windowWidth = 0.41 * renderer.getSize().y;
      ySpacing = (1.0 + (5 - numElems) * 0.1) * renderer.getSize().y / 3;
      break;
    case MenuType::GAME_CHOICE:
      windowWidth = 0.41 * renderer.getSize().y;
      ySpacing = renderer.getSize().y * 0.25;
      yOffset = renderer.getSize().y * 0.05;
      break;
    default: ySpacing = 100; break;
  }
  int xSpacing = (renderer.getSize().x - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing + yOffset, xSpacing + windowWidth, renderer.getSize().y - ySpacing + yOffset);
}

vector<SGuiElem> GuiBuilder::getMultiLine(const string& text, Color color, MenuType menuType, int maxWidth) {
  vector<SGuiElem> ret;
  for (const string& s : gui.breakText(text, maxWidth)) {
    if (menuType != MenuType::MAIN)
      ret.push_back(gui.label(s, color));
    else
      ret.push_back(gui.mainMenuLabelBg(s, menuLabelVPadding));

  }
  return ret;
}

SGuiElem GuiBuilder::menuElemMargins(SGuiElem elem) {
  return elem;
}

SGuiElem GuiBuilder::getHighlight(SGuiElem line, MenuType type, const string& label, int numActive, optional<int>* highlight) {
  switch (type) {
    case MenuType::MAIN:
      return gui.stack(std::move(line),
          gui.mouseHighlight(menuElemMargins(gui.mainMenuLabel(label, menuLabelVPadding)), numActive, highlight));
    default:
      return gui.stack(gui.mouseHighlight(
          gui.leftMargin(0, gui.translate(gui.uiHighlightLine(), Vec2(0, 0))),
          numActive, highlight), std::move(line));
  }
}

SGuiElem GuiBuilder::drawListGui(const string& title, const vector<ListElem>& options,
    MenuType menuType, optional<int>* highlight, int* choice, vector<int>* positions) {
  auto lines = gui.getListBuilder(listLineHeight);
  if (!title.empty()) {
    lines.addElem(gui.label(capitalFirst(title), Color::WHITE));
    lines.addSpace();
  }
  int numActive = 0;
  int secColumnWidth = 0;
  int columnWidth = 300;
  for (auto& elem : options) {
    columnWidth = max(columnWidth, renderer.getTextLength(elem.getText()) + 50);
    if (!elem.getSecondColumn().empty())
      secColumnWidth = max(secColumnWidth, 80 + renderer.getTextLength(elem.getSecondColumn()));
  }
  columnWidth = min(columnWidth, getMenuPosition(menuType, options.size()).width() - secColumnWidth - 140);
  if (menuType == MenuType::MAIN)
    columnWidth = 1000000;
  for (int i : All(options)) {
    Color color;
    switch (options[i].getMod()) {
      case ListElem::TITLE: color = gui.titleText; break;
      case ListElem::INACTIVE: color = gui.inactiveText; break;
      case ListElem::TEXT:
      case ListElem::NORMAL: color = gui.text; break;
    }
    if (auto p = options[i].getMessagePriority())
      color = getMessageColor(*p);
    vector<SGuiElem> label1 = getMultiLine(options[i].getText(), color, menuType, columnWidth);
    if (options.size() == 1 && label1.size() > 1) { // hacky way of checking that we display a wall of text
      for (auto& line : label1)
        lines.addElem(std::move(line));
      break;
    }
    SGuiElem line;
    if (menuType != MenuType::MAIN)
      line = gui.verticalList(std::move(label1), legendLineHeight);
    else
      line = std::move(label1.getOnlyElement());
    if (!options[i].getTip().empty())
      line = gui.stack(std::move(line),
          gui.tooltip({options[i].getTip()}));
    if (!options[i].getSecondColumn().empty())
      line = gui.horizontalList(makeVec(std::move(line),
            gui.label(options[i].getSecondColumn(), color)), columnWidth + 80);
    line = menuElemMargins(std::move(line));
    if (highlight && options[i].getMod() == ListElem::NORMAL) {
      line = gui.stack(
          gui.button([=]() { *choice = numActive; }),
          getHighlight(std::move(line), menuType, options[i].getText(), numActive, highlight));
      ++numActive;
    }
    if (positions && menuType != MenuType::MAIN)
      positions->push_back(lines.getSize() + *line->getPreferredHeight() / 2);
    lines.addElemAuto(std::move(line));
  }
  if (menuType != MenuType::MAIN) {
    return lines.buildVerticalList();
  } else
    return lines.buildVerticalListFit();
}

static optional<GuiFactory::IconId> getMoraleIcon(double morale) {
  if (morale >= 0.7)
    return GuiFactory::MORALE_4;
  if (morale >= 0.2)
    return GuiFactory::MORALE_3;
  if (morale < -0.7)
    return GuiFactory::MORALE_1;
  if (morale < -0.2)
    return GuiFactory::MORALE_2;
  else
    return none;
}

static map<ViewId, vector<PlayerInfo>> groupByViewId(const vector<PlayerInfo>& minions) {
  map<ViewId, vector<PlayerInfo>> ret;
  for (auto& elem : minions)
    ret[elem.viewId].push_back(elem);
  return ret;
}

SGuiElem GuiBuilder::drawMinionButtons(const vector<PlayerInfo>& minions, UniqueEntity<Creature>::Id current,
    optional<TeamId> teamId) {
  CHECK(!minions.empty());
  map<ViewId, vector<PlayerInfo>> minionMap = groupByViewId(minions);
  auto selectButton = [this](UniqueEntity<Creature>::Id creatureId) {
    return gui.releaseLeftButton(getButtonCallback({UserInputId::CREATURE_BUTTON, creatureId}));
  };
  auto list = gui.getListBuilder(legendLineHeight);
  for (auto& elem : minionMap) {
    list.addElem(gui.topMargin(5, gui.viewObject(elem.first)), legendLineHeight + 5);
    for (auto& minion : elem.second) {
      auto minionId = minion.creatureId;
      GuiFactory::ListBuilder line(gui);
      if (teamId)
        line.addElem(gui.leftMargin(-16, gui.stack(
            gui.button(getButtonCallback({UserInputId::REMOVE_FROM_TEAM, TeamCreatureInfo{*teamId, minionId}})),
            gui.labelUnicode(u8"✘", Color::RED))), 1);
      line.addElemAuto(gui.rightMargin(5, gui.label(minion.getFirstName())));
      if (auto icon = getMoraleIcon(minion.morale))
        line.addElem(gui.topMargin(-2, gui.icon(*icon)), 20);
      line.addBackElem(drawBestAttack(minion.bestAttack), 52);
      list.addElem(gui.stack(makeVec(
            cache->get(selectButton, THIS_LINE, minionId),
            gui.leftMargin(teamId ? -10 : 0, gui.stack(
                 gui.uiHighlightConditional([=] { return !teamId && mapGui->isCreatureHighlighted(minionId);}, Color::YELLOW),
                 gui.uiHighlightConditional([=] { return current == minionId;}))),
            gui.dragSource({DragContentId::CREATURE, minionId},
              [=]{ return gui.viewObject(minion.viewId);}),
            line.buildHorizontalList())));
    }
  }
  return gui.scrollable(list.buildVerticalList(), &minionButtonsScroll, &scrollbarsHeld);
}

static string getTaskText(MinionActivity option) {
  switch (option) {
    case MinionActivity::IDLE: return "Idle";
    case MinionActivity::SLEEP: return "Sleeping";
    case MinionActivity::CONSTRUCTION: return "Construction";
    case MinionActivity::DIGGING: return "Digging";
    case MinionActivity::HAULING: return "Hauling";
    case MinionActivity::WORKING: return "Labour";
    case MinionActivity::EAT: return "Eating";
    case MinionActivity::EXPLORE_NOCTURNAL: return "Exploring (night)";
    case MinionActivity::EXPLORE_CAVES: return "Exploring caves";
    case MinionActivity::EXPLORE: return "Exploring";
    case MinionActivity::RITUAL: return "Rituals";
    case MinionActivity::CROPS: return "Crops";
    case MinionActivity::TRAIN: return "Training";
    case MinionActivity::ARCHERY: return "Archery range";
    case MinionActivity::CRAFT: return "Crafting";
    case MinionActivity::STUDY: return "Studying";
    case MinionActivity::COPULATE: return "Copulating";
    case MinionActivity::SPIDER: return "Spinning webs";
    case MinionActivity::THRONE: return "Throne";
    case MinionActivity::BE_WHIPPED: return "Being whipped";
    case MinionActivity::BE_TORTURED: return "Being tortured";
    case MinionActivity::BE_EXECUTED: return "Being executed";
  }
}

static Color getTaskColor(PlayerInfo::MinionActivityInfo info) {
  if (info.inactive)
    return Color::GRAY;
  else if (info.current)
    return Color::GREEN;
  else
    return Color::WHITE;
}

vector<SGuiElem> GuiBuilder::drawItemMenu(const vector<ItemInfo>& items, ItemMenuCallback callback,
    bool doneBut) {
  vector<SGuiElem> lines;
  for (int i : All(items))
    lines.push_back(getItemLine(items[i], [=] (Rectangle bounds) { callback(bounds, i);} ));
  if (doneBut)
    lines.push_back(gui.stack(
          gui.button([=] { callback(Rectangle(), none); }, gui.getKey(SDL::SDLK_ESCAPE)),
          gui.centeredLabel(Renderer::HOR, "[done]", Color::LIGHT_BLUE)));
  return lines;
}

SGuiElem GuiBuilder::drawQuartersButton(const PlayerInfo& minion, const CollectiveInfo& collective) {
  auto current = minion.quarters ? gui.viewObject(*minion.quarters) : gui.label("none");
  return gui.stack(
      gui.uiHighlightMouseOver(),
      gui.getListBuilder()
            .addElemAuto(gui.label("Assigned Quarters: ", Color::YELLOW))
            .addElemAuto(std::move(current))
            .buildHorizontalList(),
      gui.buttonRect([this, minionId = minion.creatureId, allQuarters = collective.allQuarters] (Rectangle bounds) {
          auto tasks = gui.getListBuilder(legendLineHeight);
          bool exit = false;
          auto retAction = [&] (optional<int> index) {
            callbacks.input({UserInputId::ASSIGN_QUARTERS, AssignQuartersInfo{index, minionId}});
          };
          tasks.addElem(gui.stack(
              gui.button([&retAction, &exit] { retAction(none); exit = true; }),
              gui.uiHighlightMouseOver(),
              gui.label("none")));
          for (int i : All(allQuarters)) {
            tasks.addElem(gui.stack(
                gui.button([i, &retAction, &exit] { retAction(i); exit = true; }),
                gui.uiHighlightMouseOver(),
                gui.getListBuilder(32)
                    .addElem(gui.viewObject(allQuarters[i]))
                    .addElem(gui.label(toString(i)))
                    .buildHorizontalList()));
          }
          drawMiniMenu(std::move(tasks), exit, bounds.bottomLeft(), 50, true);
        }));
}

SGuiElem GuiBuilder::drawActivityButton(const PlayerInfo& minion) {
  string curTask = "(none)";
  for (auto task : minion.minionTasks)
    if (task.current)
      curTask = getTaskText(task.task);
  return gui.stack(
      gui.uiHighlightMouseOver(),
      gui.getListBuilder()
          .addElemAuto(gui.label("Activity: ", Color::YELLOW))
          .addElemAuto(gui.label(curTask))
          .buildHorizontalList(),
      gui.buttonRect([=] (Rectangle bounds) {
          auto tasks = gui.getListBuilder(legendLineHeight);
          bool exit = false;
          TaskActionInfo retAction;
          retAction.creature = minion.creatureId;
          for (auto task : minion.minionTasks) {
            function<void()> buttonFun = [] {};
            if (!task.inactive)
              buttonFun = [&exit, &retAction, task] {
                  retAction.switchTo = task.task;
                  exit = true;
                };
            auto lockButton = task.locked
                  ? gui.rightMargin(20, gui.labelUnicode(u8"✓", [&retAction, task] {
                      return (retAction.lock.contains(task.task) ^ *task.locked) ?
                          Color::LIGHT_GRAY : Color::GREEN;}))
                  : gui.empty();
            tasks.addElem(GuiFactory::ListBuilder(gui)
                .addMiddleElem(gui.stack(
                    gui.button(buttonFun),
                    gui.label(getTaskText(task.task), getTaskColor(task))))
                .addBackElemAuto(gui.stack(
                    getTooltip({"Click to turn this task on/off."}, THIS_LINE),
                    gui.button([&retAction, task] {
                      retAction.lock.toggle(task.task);
                    }),
                    lockButton)).buildHorizontalList());
          }
          drawMiniMenu(std::move(tasks), exit, bounds.bottomLeft(), 200, true);
          callbacks.input({UserInputId::CREATURE_TASK_ACTION, retAction});
        }));
}

SGuiElem GuiBuilder::drawAttributesOnPage(vector<SGuiElem>&& attrs) {
  if (attrs.empty())
    return gui.empty();
  vector<SGuiElem> lines[2];
  for (int i : All(attrs))
    lines[i % 2].push_back(std::move(attrs[i]));
  int elemWidth = 100;
  return gui.verticalList(makeVec(
      gui.horizontalList(std::move(lines[0]), elemWidth),
      gui.horizontalList(std::move(lines[1]), elemWidth)), legendLineHeight);
}

SGuiElem GuiBuilder::drawEquipmentAndConsumables(const PlayerInfo& minion) {
  const vector<ItemInfo>& items = minion.inventory;
  if (items.empty())
    return gui.empty();
  auto lines = gui.getListBuilder(legendLineHeight);
  vector<SGuiElem> itemElems = drawItemMenu(items,
      [=](Rectangle butBounds, optional<int> index) {
        const ItemInfo& item = items[*index];
        if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), true))
          callbacks.input({UserInputId::CREATURE_EQUIPMENT_ACTION,
              EquipmentActionInfo{minion.creatureId, item.ids, item.slot, *choice}});
      });
  lines.addElem(gui.label("Equipment", Color::YELLOW));
  for (int i : All(itemElems))
    if (items[i].type == items[i].EQUIPMENT)
      lines.addElem(gui.leftMargin(3, std::move(itemElems[i])));
  lines.addElem(gui.label("Consumables", Color::YELLOW));
  for (int i : All(itemElems))
    if (items[i].type == items[i].CONSUMABLE)
      lines.addElem(gui.leftMargin(3, std::move(itemElems[i])));
  lines.addElem(gui.stack(
      gui.labelHighlight("[Add consumable]", Color::LIGHT_BLUE),
      gui.button(getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
              EquipmentActionInfo{minion.creatureId, {}, none, ItemAction::REPLACE}}))));
  for (int i : All(itemElems))
    if (items[i].type == items[i].OTHER) {
      lines.addElem(gui.label("Other", Color::YELLOW));
      break;
    }
  for (int i : All(itemElems))
    if (items[i].type == items[i].OTHER)
      lines.addElem(gui.leftMargin(3, std::move(itemElems[i])));
  return lines.buildVerticalList();
}

vector<SGuiElem> GuiBuilder::drawMinionActions(const PlayerInfo& minion, const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> line;
  const bool tutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
  for (auto action : minion.actions)
    switch (action) {
      case PlayerInfo::CONTROL:
        line.push_back(gui.stack(
            tutorialHighlight
                ? gui.labelHighlightBlink("[Control]", Color::LIGHT_BLUE, Color::WHITE)
                : gui.labelHighlight("[Control]", Color::LIGHT_BLUE),
            gui.button(getButtonCallback({UserInputId::CREATURE_CONTROL, minion.creatureId}))));
        break;
      case PlayerInfo::RENAME:
        line.push_back(gui.stack(
            gui.labelHighlight("[Rename]", Color::LIGHT_BLUE),
            gui.button([=] {
                if (auto name = getTextInput("Rename minion", minion.firstName, 10, "Press escape to cancel."))
                callbacks.input({UserInputId::CREATURE_RENAME,
                    RenameActionInfo{minion.creatureId, *name}}); })));
        break;
      case PlayerInfo::BANISH:
        line.push_back(gui.stack(
            gui.labelHighlight("[Banish]", Color::LIGHT_BLUE),
            gui.button(getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId}))));
        break;
      case PlayerInfo::CONSUME:
        line.push_back(gui.stack(
            gui.labelHighlight("[Absorb]", Color::LIGHT_BLUE),
            gui.button(getButtonCallback({UserInputId::CREATURE_CONSUME, minion.creatureId}))));
        break;
    }
  return line;
}

SGuiElem GuiBuilder::drawMinionPage(const PlayerInfo& minion, const CollectiveInfo& collective,
    const optional<TutorialInfo>& tutorial) {
  auto list = gui.getListBuilder(legendLineHeight);
  list.addElem(gui.label(minion.getTitle()));
  if (!minion.description.empty())
    list.addElem(gui.label(minion.description, Renderer::smallTextSize, Color::LIGHT_GRAY));
  list.addElem(gui.horizontalList(drawMinionActions(minion, tutorial), 140));
  auto leftLines = gui.getListBuilder(legendLineHeight);
  leftLines.addElem(gui.label("Attributes", Color::YELLOW));
  leftLines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(minion.attributes)));
  for (auto& elem : drawEffectsList(minion))
    leftLines.addElem(std::move(elem));
  leftLines.addSpace();
  if (auto elem = drawTrainingInfo(minion)) {
    leftLines.addElemAuto(std::move(elem));
    leftLines.addSpace();
  }
  leftLines.addElem(drawActivityButton(minion));
  if (minion.canAssignQuarters)
    leftLines.addElem(drawQuartersButton(minion, collective));
  leftLines.addSpace();
  for (auto& elem : drawSkillsList(minion))
    leftLines.addElem(std::move(elem));
  if (auto spells = drawSpellsList(minion, false))
    leftLines.addElemAuto(std::move(spells));
  int topMargin = list.getSize() + 20;
  return gui.margin(list.buildVerticalList(),
      gui.scrollable(gui.horizontalListFit(makeVec(
          gui.rightMargin(15, leftLines.buildVerticalList()),
          drawEquipmentAndConsumables(minion))), &minionPageScroll, &scrollbarsHeld),
      topMargin, GuiFactory::TOP);
}

SGuiElem GuiBuilder::drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>& queue, const string& title,
    pair<ViewId, int> budget, const vector<ItemInfo>& items, ScrollPosition* scrollPos) {
  int titleExtraSpace = 10;
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  lines.addElem(GuiFactory::ListBuilder(gui)
      .addElemAuto(gui.label(title))
      .addBackElemAuto(drawCost(budget)).buildHorizontalList(),
     getStandardLineHeight() + titleExtraSpace);
  for (SGuiElem& elem : drawItemMenu(items,
        [&queue, items] (Rectangle, optional<int> index) { if (index) queue.push(*items[*index].ids.begin());}))
    lines.addElem(std::move(elem));
  return gui.setWidth(380,
      gui.miniWindow(gui.margins(gui.scrollable(lines.buildVerticalList(), scrollPos), 15),
          [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawPillageItemMenu(SyncQueue<optional<int>>& queue, const string& title,
    const vector<ItemInfo>& items, ScrollPosition* scrollPos) {
  int titleExtraSpace = 10;
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  lines.addElem(gui.label(title), getStandardLineHeight() + titleExtraSpace);
  for (SGuiElem& elem : drawItemMenu(items,
        [&queue, &items] (Rectangle, optional<int> index) {
            if (index && !items[*index].unavailable) queue.push(*index);}))
    lines.addElem(std::move(elem));
  return gui.setWidth(380,
      gui.miniWindow(gui.margins(gui.scrollable(lines.buildVerticalList(), scrollPos), 15),
                     [&queue] { queue.push(none); }));
}

static Color getHighlightColor(VillainType type) {
  switch (type) {
    case VillainType::MAIN:
      return Color::RED;
    case VillainType::LESSER:
      return Color::YELLOW;
    case VillainType::ALLY:
      return Color::GREEN;
    case VillainType::PLAYER:
      return Color::WHITE;
    case VillainType::NONE:
      FATAL << "Tried to render villain of type NONE";
      return Color::WHITE;
  }
}

SGuiElem GuiBuilder::drawCampaignGrid(const Campaign& c, optional<Vec2>* marked, function<bool(Vec2)> activeFun,
    function<void(Vec2)> clickFun){
  int iconScale = 2;
  int iconSize = 24 * iconScale;
  auto rows = gui.getListBuilder(iconSize);
  auto& sites = c.getSites();
  for (int y : sites.getBounds().getYRange()) {
    auto columns = gui.getListBuilder(iconSize);
    for (int x : sites.getBounds().getXRange()) {
      vector<SGuiElem> v;
      for (int i : All(sites[x][y].viewId)) {
        v.push_back(gui.asciiBackground(sites[x][y].viewId[i]));
        if (i == 0)
          v.push_back(gui.viewObject(sites[x][y].viewId[i], iconScale));
        else {
          if (sites[x][y].viewId[i] == ViewId::CANIF_TREE || sites[x][y].viewId[i] == ViewId::DECID_TREE)
            v.push_back(gui.topMargin(1 * iconScale,
                  gui.viewObject(ViewId::ROUND_SHADOW, iconScale, Color(255, 255, 255, 160))));
          v.push_back(gui.topMargin(-2 * iconScale, gui.viewObject(sites[x][y].viewId[i], iconScale)));
        }
      }
      columns.addElem(gui.stack(std::move(v)));
    }
    auto columns2 = gui.getListBuilder(iconSize);
    for (int x : sites.getBounds().getXRange()) {
      Vec2 pos(x, y);
      vector<SGuiElem> elem;
      if (auto id = sites[x][y].getDwellerViewId()) {
        elem.push_back(gui.asciiBackground(*id));
        if (c.getPlayerPos() && c.isInInfluence(pos))
          elem.push_back(gui.viewObject(ViewId::SQUARE_HIGHLIGHT, iconScale,
              getHighlightColor(*sites[pos].getVillainType())));
        if (c.getPlayerPos() == pos && (!marked || !*marked)) // hacky way of checking this is adventurer embark position
          elem.push_back(gui.viewObject(ViewId::SQUARE_HIGHLIGHT, iconScale));
        if (activeFun(pos))
          elem.push_back(gui.stack(
                gui.button([pos, clickFun] { clickFun(pos); }),
                gui.mouseHighlight2(gui.viewObject(ViewId::SQUARE_HIGHLIGHT, iconScale))));
        elem.push_back(gui.topMargin(1 * iconScale,
              gui.viewObject(ViewId::ROUND_SHADOW, iconScale, Color(255, 255, 255, 160))));
        if (marked)
          elem.push_back(gui.conditional(gui.viewObject(ViewId::SQUARE_HIGHLIGHT, iconScale),
                [marked, pos] { return *marked == pos;}));
        elem.push_back(gui.topMargin(-2 * iconScale, gui.viewObject(*id, iconScale)));
        if (c.isDefeated(pos))
          elem.push_back(gui.viewObject(ViewId::CAMPAIGN_DEFEATED, iconScale));
      } else {
        if (activeFun(pos))
          elem.push_back(gui.stack(
                gui.button([pos, clickFun] { clickFun(pos); }),
                gui.mouseHighlight2(gui.viewObject(ViewId::SQUARE_HIGHLIGHT, iconScale))));
        if (marked)
          elem.push_back(gui.conditional(gui.viewObject(ViewId::SQUARE_HIGHLIGHT, iconScale),
                [marked, pos] { return *marked == pos;}));
      }
      if (auto desc = sites[x][y].getDwellerDescription())
        elem.push_back(gui.tooltip({*desc}, milliseconds{0}));
      columns2.addElem(gui.stack(std::move(elem)));
    }
    rows.addElem(gui.stack(columns.buildHorizontalList(), columns2.buildHorizontalList()));
  }
  return gui.stack(
    gui.miniBorder2(),
    gui.margins(rows.buildVerticalList(), 8));
}

SGuiElem GuiBuilder::drawWorldmap(Semaphore& sem, const Campaign& campaign) {
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  lines.addElem(gui.centerHoriz(gui.label("Map of " + campaign.getWorldName())));
  lines.addElem(gui.centerHoriz(gui.label("Use the travel command while controlling a minion or team "
          "to travel to another site.", Renderer::smallTextSize, Color::LIGHT_GRAY)));
  lines.addElemAuto(gui.centerHoriz(drawCampaignGrid(campaign, nullptr,
      [](Vec2) { return false; },
      [](Vec2) { })));
  lines.addBackElem(gui.centerHoriz(
        gui.stack(
          gui.button([&] { sem.v(); }),
          gui.labelHighlight("[Close]", Color::LIGHT_BLUE))
        ));
  return gui.preferredSize(1000, 630,
      gui.window(gui.margins(lines.buildVerticalList(), 15), [&sem] { sem.v(); }));
}

SGuiElem GuiBuilder::drawChooseSiteMenu(SyncQueue<optional<Vec2>>& queue, const string& message,
    const Campaign& campaign, optional<Vec2>& sitePos) {
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  lines.addElem(gui.centerHoriz(gui.label(message)));
  lines.addElemAuto(gui.centerHoriz(drawCampaignGrid(campaign, &sitePos,
      [&campaign](Vec2 pos) { return campaign.canTravelTo(pos); },
      [&sitePos](Vec2 pos) { sitePos = pos; })));
  lines.addBackElem(gui.centerHoriz(gui.getListBuilder()
        .addElemAuto(gui.conditional(
            gui.stack(
                gui.button([&] { queue.push(*sitePos); }),
                gui.labelHighlight("[Confirm]", Color::LIGHT_BLUE)),
            gui.label("[Confirm]", Color::LIGHT_GRAY),
            [&] { return !!sitePos; }))
        .addSpace(10)
        .addElemAuto(
            gui.stack(
                gui.button([&queue] { queue.push(none); }, gui.getKey(SDL::SDLK_ESCAPE), true),
                gui.labelHighlight("[Cancel]", Color::LIGHT_BLUE))).buildHorizontalList()));
  return gui.preferredSize(1000, 600,
      gui.window(gui.margins(lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease) {
  return gui.getListBuilder()
      .addElemAuto(canIncrease
          ? gui.stack(
                gui.labelHighlight("[+]", Color::LIGHT_BLUE),
                gui.button([callback] { callback(1); }))
          : gui.label("[+]", Color::GRAY))
      .addSpace(10)
      .addElemAuto(canDecrease
          ? gui.stack(
                gui.labelHighlight("[-]", Color::LIGHT_BLUE),
                gui.button([callback] { callback(-1); }))
          : gui.label("[-]", Color::GRAY))
      .buildHorizontalList();
}

SGuiElem GuiBuilder::drawOptionElem(Options* options, OptionId id, function<void()> onChanged, optional<string> defaultString) {
  auto line = gui.getListBuilder();
  string valueString = options->getValueString(id);
  if (defaultString && valueString.empty())
    valueString = *defaultString;
  string name = options->getName(id);
  switch (options->getType(id)) {
    case Options::PLAYER_TYPE: {
      auto viewId = CreatureFactory::getViewId(options->getCreatureId(id));
      line.addElemAuto(gui.label(name + ": "));
      line.addElem(gui.stack(
            gui.tooltip2(gui.miniWindow(gui.margins(gui.viewObject(viewId, 2), 15)), [](const Rectangle& r) { return r.topRight();}),
            gui.viewObject(viewId, 1),
            gui.button([=] { options->setNextCreatureId(id); onChanged(); })), 30);
      break;
    }
    case Options::STRING:
      line.addElemAuto(gui.label(name + ": "));
      line.addElemAuto(gui.stack(
          gui.button([=] {
              if (auto val = getTextInput("Enter " + name, valueString, 10, "Leave blank to use a random name. Hit enter to confirm.")) {
                options->setValue(id, *val);
                onChanged();
              }}),
          gui.labelHighlight(valueString, Color::WHITE)));
      break;
    case Options::INT: {
      auto limits = options->getLimits(id);
      int value = options->getIntValue(id);
      line.addElemAuto(gui.label(name + ": " + valueString));
      line.addBackElemAuto(drawPlusMinus([=] (int v) {
            options->setValue(id, value + v); onChanged();}, value < limits->second, value > limits->first));
      }
      break;
    case Options::BOOL: {
      bool value = options->getBoolValue(id);
      line.addElemAuto(
            gui.getListBuilder()
                .addElemAuto(gui.label(name + ": "))
                .addElemAuto(gui.stack(
                    gui.button([=]{options->setValue(id, int(!value)); onChanged();}),
                    gui.labelHighlight("[" + valueString + "]", Color::LIGHT_BLUE)))
                .buildHorizontalList());
      }
      break;
  }
  return gui.stack(
      gui.tooltip({options->getHint(id).value_or("")}),
      line.buildHorizontalList()
  );
}

GuiFactory::ListBuilder GuiBuilder::drawRetiredGames(RetiredGames& retired, function<void()> reloadCampaign,
    optional<int> maxActive) {
  auto lines = gui.getListBuilder(legendLineHeight);
  const vector<RetiredGames::RetiredGame>& allGames = retired.getAllGames();
  bool displayActive = !maxActive;
  for (int i : All(allGames)) {
    if (i == retired.getNumLocal() && !displayActive)
      lines.addElem(gui.label("Online dungeons:", Color::YELLOW));
    if (retired.isActive(i) == displayActive) {
      auto header = gui.getListBuilder();
      bool maxedOut = !displayActive && retired.getNumActive() >= *maxActive;
      if (retired.isActive(i))
        header.addElem(gui.stack(
              gui.labelUnicode(u8"✘", Color::RED),
              gui.button([i, reloadCampaign, &retired] { retired.setActive(i, false); reloadCampaign();})), 15);
      header.addElem(gui.label(allGames[i].gameInfo.getName(),
          maxedOut ? Color::LIGHT_GRAY : Color::WHITE), 170);
      for (auto& minion : allGames[i].gameInfo.getMinions())
        header.addElem(drawMinionAndLevel(minion.viewId, minion.level, 1), 25);
      header.addSpace(20);
      if (allGames[i].numTotal > 0)
        header.addElemAuto(gui.stack(
          gui.tooltip({"Number of times this dungeon has been conquered over how many times it has been loaded."}),
          gui.label("Conquer rate: " + toString(allGames[i].numWon) + "/" + toString(allGames[i].numTotal))));
      SGuiElem line = header.buildHorizontalList();
      if (allGames[i].numTotal > 0 && displayActive)
        line = gui.stack(std::move(line), gui.tooltip({
              "Conquer rate: " + toString(allGames[i].numWon) + "/" + toString(allGames[i].numTotal)}));
      if (!retired.isActive(i) && !maxedOut) {
        line = gui.stack(
            gui.uiHighlightMouseOver(Color::GREEN),
            std::move(line),
            gui.button([i, reloadCampaign, &retired] { retired.setActive(i, true); reloadCampaign();}));
      }
      lines.addElem(std::move(line));
    }
  }
  return lines;
}

static const char* getGameTypeName(CampaignType type) {
  switch (type) {
    case CampaignType::CAMPAIGN: return "Campaign";
    case CampaignType::ENDLESS: return "Endless";
    case CampaignType::FREE_PLAY: return "Free play";
    case CampaignType::SINGLE_KEEPER: return "Single map";
    case CampaignType::QUICK_MAP: return "Quick map";
  }
}

SGuiElem GuiBuilder::drawMenuWarning(View::CampaignOptions::WarningType type) {
  switch (type) {
    case View::CampaignOptions::NO_RETIRE:
      return gui.labelMultiLine("Warning: you won't be able to retire your dungeon in this mode.",
              legendLineHeight, Renderer::textSize, Color::RED);
    case View::CampaignOptions::OTHER_MODES:
      return gui.getListBuilder()
          .addElemAuto(gui.labelUnicode(u8"⬅", Color::WHITE))
          .addElemAuto(gui.label(" Other game modes are here.", Color::WHITE))
          .buildHorizontalList();
  }
}

SGuiElem GuiBuilder::drawCampaignMenu(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    Options* options, View::CampaignMenuState& menuState) {
  const auto& campaign = campaignOptions.campaign;
  auto& retiredGames = campaignOptions.retired;
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  GuiFactory::ListBuilder centerLines(gui, getStandardLineHeight());
  GuiFactory::ListBuilder rightLines(gui, getStandardLineHeight());
  int optionMargin = 50;
  centerLines.addElem(gui.centerHoriz(gui.stack(
       gui.labelHighlight("Game mode: "_s + getGameTypeName(campaign.getType())),
       gui.buttonRect([&queue, this, campaignOptions] (Rectangle bounds) {
           auto lines = gui.getListBuilder(legendLineHeight);
           bool exit = false;
           for (auto& info : campaignOptions.availableTypes) {
             lines.addElem(gui.stack(
                 gui.labelHighlight(getGameTypeName(info.type)),
                 gui.button([&, info] { queue.push({CampaignActionId::CHANGE_TYPE, info.type}); exit = true; })));
             for (auto& desc : info.description)
               lines.addElem(gui.leftMargin(0, gui.label("- " + desc, Color::LIGHT_GRAY, Renderer::smallTextSize)),
                   legendLineHeight * 2 / 3);
             lines.addSpace(legendLineHeight / 3);
           }
           drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 350, true);
       }))));
  centerLines.addElem(gui.centerHoriz(gui.stack(
            gui.labelHighlight("[Help]", Color::LIGHT_BLUE),
                gui.button([&] { menuState.helpText = !menuState.helpText; }))));
  lines.addElem(gui.leftMargin(optionMargin, gui.label("World name: " + campaign.getWorldName())));
  auto getDefaultString = [&](OptionId id) -> optional<string> {
    switch(id) {
      case OptionId::ADVENTURER_NAME:
      case OptionId::KEEPER_NAME:
        return campaignOptions.player->getName().first();
      default:
        return none;
    }
  };
  for (OptionId id : campaignOptions.primaryOptions)
    lines.addElem(gui.leftMargin(optionMargin, drawOptionElem(options, id,
            [&queue, id] { queue.push({CampaignActionId::UPDATE_OPTION, id});}, getDefaultString(id))));
  lines.addSpace(10);
  if (auto& title = campaignOptions.mapTitle)
    lines.addBackElem(gui.centerHoriz(gui.label(*title)));
  lines.addBackElemAuto(gui.centerHoriz(drawCampaignGrid(campaign, nullptr,
        [&campaign](Vec2 pos) { return campaign.canEmbark(pos); },
        [&queue](Vec2 pos) { queue.push({CampaignActionId::CHOOSE_SITE, pos}); })));
  lines.addBackElem(gui.topMargin(10, gui.centerHoriz(gui.getListBuilder()
        .addElemAuto(gui.conditional(
            gui.stack(
                gui.button([&] { queue.push(CampaignActionId::CONFIRM); }),
                gui.labelHighlight("[Confirm]", Color::LIGHT_BLUE)),
            gui.label("[Confirm]", Color::LIGHT_GRAY),
            [&campaign] { return !!campaign.getPlayerPos(); }))
        .addSpace(10)
        .addElemAuto(
          gui.stack(
            gui.button([&queue] { queue.push(CampaignActionId::REROLL_MAP);}),
            gui.labelHighlight("[Re-roll map]", Color::LIGHT_BLUE)))
        .addSpace(10)
        .addElemAuto(
            gui.stack(
                gui.button([&queue] { queue.push(CampaignActionId::CANCEL); }, gui.getKey(SDL::SDLK_ESCAPE)),
                gui.labelHighlight("[Cancel]", Color::LIGHT_BLUE))).buildHorizontalList())));
  GuiFactory::ListBuilder secondaryOptionLines(gui, getStandardLineHeight());
  if (!campaignOptions.secondaryOptions.empty()) {
    for (OptionId id : campaignOptions.secondaryOptions)
      rightLines.addElem(
          drawOptionElem(options, id, [&queue, id] { queue.push({CampaignActionId::UPDATE_OPTION, id});},
              getDefaultString(id)));
  }
  if (retiredGames) {
    auto addedDungeons = drawRetiredGames(
        *retiredGames, [&queue] { queue.push(CampaignActionId::UPDATE_MAP);}, none);
    int addedHeight = addedDungeons.getSize();
    if (!addedDungeons.isEmpty()) {
      addedHeight += legendLineHeight;
      secondaryOptionLines.addElem(gui.label("Retired villains added:", Color::YELLOW));
      secondaryOptionLines.addElem(addedDungeons.buildVerticalList(), addedHeight);
    }
    GuiFactory::ListBuilder retiredList = drawRetiredGames(*retiredGames,
        [&queue] { queue.push(CampaignActionId::UPDATE_MAP);}, options->getIntValue(OptionId::MAIN_VILLAINS));
    if (retiredList.isEmpty())
      retiredList.addElem(gui.label("No more retired dungeons found :("));
    else
      secondaryOptionLines.addElem(gui.label("Available villains:", Color::YELLOW));
    int listHeight = min(360 - addedHeight, retiredList.getSize() + 30);
    secondaryOptionLines.addElem(gui.scrollable(retiredList.buildVerticalList()), listHeight);
    rightLines.addElem(gui.stack(
        gui.labelHighlight("[Add retired dungeons]", Color::LIGHT_BLUE),
        gui.button([&menuState] { menuState.settings = !menuState.settings;})));
  } else if (campaignOptions.warning)
    rightLines.addElem(gui.leftMargin(-20, drawMenuWarning(*campaignOptions.warning)));
  int retiredPosX = 640;
  vector<SGuiElem> interior;
  interior.push_back(lines.buildVerticalList());
  interior.push_back(centerLines.buildVerticalList());
  interior.push_back(gui.margins(rightLines.buildVerticalList(), retiredPosX, 0, 50, 0));
  auto closeHelp = [&menuState] { menuState.helpText = false;};
  interior.push_back(
        gui.conditionalStopKeys(gui.margins(gui.miniWindow2(gui.stack(
                gui.button(closeHelp),  // to make the window closable on a click anywhere
                gui.margins(gui.labelMultiLine(campaignOptions.introText, legendLineHeight), 10)),
            closeHelp), 100, 50, 100, 280),
            [&menuState] { return menuState.helpText;}));

  int optionsSize = secondaryOptionLines.getSize();
  if (!secondaryOptionLines.isEmpty())
    interior.push_back(
          gui.conditionalStopKeys(gui.translate(gui.miniWindow2(gui.margins(secondaryOptionLines.buildVerticalList(), 10),
              [&] { menuState.settings = false;}), Vec2(30, 70),
                  Vec2(650, 50 + optionsSize)),
          [&menuState] { return menuState.settings;}));
  return
      gui.preferredSize(1000, 705,
      gui.window(gui.margins(gui.stack(std::move(interior)), 5), [&queue] { queue.push(CampaignActionId::CANCEL); }));
}

SGuiElem GuiBuilder::drawCreatureInfo(SyncQueue<bool>& queue, const string& title, bool prompt,
    const vector<CreatureInfo>& creatures) {
  auto lines = gui.getListBuilder(getStandardLineHeight() + 10);
  lines.addElem(gui.centerHoriz(gui.label(title)));
  const int windowWidth = 450;
  auto line = gui.getListBuilder(60);
  for (auto& elem : creatures) {
    line.addElem(gui.stack(
          gui.viewObject(elem.viewId, 2),
          gui.label(toString((int) elem.bestAttack.value), 20)));
    if (line.getSize() >= windowWidth - 50) {
      lines.addElem(gui.centerHoriz(line.buildHorizontalList()), 70);
      line.clear();
    }
  }
  if (!line.isEmpty())
      lines.addElem(gui.centerHoriz(line.buildHorizontalList()), 70);
  auto bottomLine = gui.getListBuilder()
      .addElemAuto(gui.stack(
          gui.labelHighlight("[Ok]", Color::LIGHT_BLUE),
          gui.button([&queue] { queue.push(true);})));
  if (prompt)
    bottomLine.addElemAuto(gui.stack(
        gui.labelHighlight("[Cancel]", Color::LIGHT_BLUE),
        gui.button([&queue] { queue.push(false);})));
  lines.addElem(gui.centerHoriz(bottomLine.buildHorizontalList()));
  int margin = 25;
  return gui.setWidth(2 * margin + windowWidth,
      gui.window(gui.margins(lines.buildVerticalList(), margin), [&queue] { queue.push(false); }));

}

SGuiElem GuiBuilder::drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>& queue, const string& title,
      const vector<CreatureInfo>& team, const string& cancelText) {
  auto lines = gui.getListBuilder(getStandardLineHeight() + 10);
  lines.addElem(gui.centerHoriz(gui.label(title)));
  const int windowWidth = 450;
  auto line = gui.getListBuilder(60);
  for (auto& elem : team) {
    line.addElemAuto(gui.stack(
          gui.mouseHighlight2(gui.uiHighlight()),
          gui.button([&queue, elem] { queue.push(elem.uniqueId); }),
          gui.margins(gui.stack(gui.viewObject(elem.viewId, 2),
              gui.label(toString((int) elem.bestAttack.value), 20)), 5, 5, 5, 5)));
    if (line.getLength() > 6) {
      lines.addElemAuto(gui.centerHoriz(line.buildHorizontalList()));
      lines.addSpace(20);
      line.clear();
    }
  }
  if (!line.isEmpty()) {
    lines.addElemAuto(gui.centerHoriz(line.buildHorizontalList()));
    lines.addSpace(20);
  }
  if (!cancelText.empty())
    lines.addElem(gui.centerHoriz(gui.stack(
          gui.labelHighlight("[" + cancelText + "]", Color::LIGHT_BLUE),
          gui.button([&queue] { queue.push(none);}))));
  int margin = 25;
  return gui.setWidth(2 * margin + windowWidth,
      gui.window(gui.margins(lines.buildVerticalList(), margin), [&queue] { queue.push(none); }));
}

/*SGuiElem GuiBuilder::drawTeamLeaderMenu(SyncQueue<vector<UniqueEntity<Creature>::Id>>& queue, const string& title,
      const vector<CreatureInfo>& team) {
  auto lines = gui.getListBuilder(getStandardLineHeight() + 10);
  lines.addElem(gui.centerHoriz(gui.label(title)));
  const int windowWidth = 450;
  auto line = gui.getListBuilder(60);
  auto result = make_shared<vector<UniqueEntity<Creature>::Id>>();
  auto toggle = [result] (UniqueEntity<Creature>::Id id) {
    if (result->contains(id))
      result->removeElement(id);
    else
      result->push_back(id);
  };
  for (auto& elem : team) {
    line.addElem(gui.stack(
          gui.conditional(gui.bottomMargin(22, gui.rightMargin(6,
                gui.icon(gui.HIGHLIGHT, GuiFactory::Alignment::CENTER_STRETCHED, Color::GREEN))),
                [id = elem.uniqueId, result] { return result->contains(id); } ),
          gui.button([id = elem.uniqueId, toggle] { toggle(id); }),
          gui.viewObject(elem.viewId, 2),
          gui.label(toString((int) elem.bestAttack.value), 20)));
    if (line.getSize() >= windowWidth - 50) {
      lines.addElem(gui.centerHoriz(line.buildHorizontalList()), 70);
      line.clear();
    }
  }
  if (!line.isEmpty())
      lines.addElem(gui.centerHoriz(line.buildHorizontalList()), 70);
  lines.addElem(gui.centerHoriz(gui.getListBuilder()
        .addElemAuto(gui.stack(
            gui.labelHighlight("[Confirm]", Color::LIGHT_BLUE),
            gui.button([&queue, result] { queue.push(*result);})))
        .addElemAuto(gui.stack(
            gui.labelHighlight("[Cancel]", Color::LIGHT_BLUE),
            gui.button([&queue] { queue.push({});})))
        .buildHorizontalList()));
  int margin = 25;
  return gui.setWidth(2 * margin + windowWidth,
      gui.window(gui.margins(lines.buildVerticalList(), margin), [&queue] { queue.push({}); }));
}*/

SGuiElem GuiBuilder::drawHighscorePage(const HighscoreList& page, ScrollPosition *scrollPos) {
  GuiFactory::ListBuilder lines(gui, legendLineHeight);
  for (auto& elem : page.scores) {
    GuiFactory::ListBuilder line(gui);
    Color color = elem.highlight ? Color::GREEN : Color::WHITE;
    line.addElemAuto(gui.label(elem.text, color));
    line.addBackElem(gui.label(elem.score, color), 130);
    lines.addElem(gui.leftMargin(30, line.buildHorizontalList()));
  }
  return gui.scrollable(lines.buildVerticalList(), scrollPos);
}

SGuiElem GuiBuilder::drawHighscores(const vector<HighscoreList>& list, Semaphore& sem, int& tabNum,
    vector<ScrollPosition>& scrollPos, bool& online) {
  vector<SGuiElem> pages;
  int numTabs = list.size() / 2;
  GuiFactory::ListBuilder topLine(gui, 200);
  for (int i : All(list)) {
    for (int j : All(list[i].scores))
      if (list[i].scores[j].highlight) {
        if (i < numTabs)
          tabNum = i;
        scrollPos[i] = j;
      }
    pages.push_back(gui.conditional(drawHighscorePage(list[i], &scrollPos[i]),
          [&tabNum, i, &online, numTabs] { return (online && tabNum == i - numTabs) ||
              (!online && tabNum == i) ; }));
    if (i < numTabs)
    topLine.addElem(gui.stack(
        gui.margins(gui.uiHighlightConditional([&tabNum, i] { return tabNum == i;}), 42, 0, 32, 0),
        gui.centeredLabel(Renderer::HOR, list[i].name),
        gui.button([&tabNum, i] { tabNum = i;})));
  }
  SGuiElem onlineBut = gui.stack(
      gui.label("Online", [&online] { return online ? Color::GREEN : Color::WHITE;}),
      gui.button([&online] { online = !online; }));
  Vec2 size = getMenuPosition(MenuType::NORMAL, 0).getSize();
  return gui.preferredSize(size, gui.stack(makeVec(
      gui.keyHandler([&tabNum, numTabs] { tabNum = (tabNum + 1) % numTabs; }, {gui.getKey(SDL::SDLK_RIGHT)}),
      gui.keyHandler([&tabNum, numTabs] { tabNum = (tabNum + numTabs - 1) % numTabs; }, {gui.getKey(SDL::SDLK_LEFT)}),
      gui.window(
        gui.margin(gui.leftMargin(25, std::move(onlineBut)),
        gui.topMargin(30, gui.margin(gui.leftMargin(5, topLine.buildHorizontalListFit()),
            gui.margins(gui.stack(std::move(pages)), 25, 60, 0, 30), legendLineHeight, GuiFactory::TOP)), legendLineHeight, GuiFactory::TOP),
        [&] { sem.v(); }))));

}

SGuiElem GuiBuilder::drawMinimapIcons() {
  return gui.minimapBar(
      gui.preferredSize(48, 48, gui.stack(
          getHintCallback({"Open world map. You can also press 't'."}),
          gui.mouseHighlight2(gui.icon(GuiFactory::IconId::MINIMAP_WORLD2), gui.icon(GuiFactory::IconId::MINIMAP_WORLD1)),
          gui.button(getButtonCallback(UserInputId::DRAW_WORLD_MAP), gui.getKey(SDL::SDLK_t))
      )),
      gui.preferredSize(48, 48, gui.stack(
          getHintCallback({"Scroll to your character. You can also press 'k'."}),
          gui.mouseHighlight2(gui.icon(GuiFactory::IconId::MINIMAP_CENTER2), gui.icon(GuiFactory::IconId::MINIMAP_CENTER1)),
          gui.button(getButtonCallback(UserInputId::SCROLL_TO_HOME), gui.getKey(SDL::SDLK_k))
          ))
  );
}

Rectangle GuiBuilder::getTextInputPosition() {
  Vec2 center = renderer.getSize() / 2;
  return Rectangle(center - Vec2(300, 129), center + Vec2(300, 129));
}

SGuiElem GuiBuilder::getTextContent(const string& title, const string& value, const string& hint) {
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(
      gui.variableLabel([&] { return title + ":  " + value + "_"; }, legendLineHeight), 3 * legendLineHeight);
  if (!hint.empty())
    lines.addElem(gui.label(hint, gui.inactiveText));
  return lines.buildVerticalList();
}

optional<string> GuiBuilder::getTextInput(const string& title, const string& value, int maxLength,
    const string& hint) {
  bool dismiss = false;
  string text = value;
  SGuiElem dismissBut = gui.margins(gui.stack(makeVec(
        gui.button([&](){ dismiss = true; }),
        gui.mouseHighlight2(gui.mainMenuHighlight()),
        gui.centerHoriz(
            gui.label("Dismiss", Color::WHITE), renderer.getTextLength("Dismiss")))), 0, 5, 0, 0);
  SGuiElem stuff = gui.margins(getTextContent(title, text, hint), 30, 50, 0, 0);
  stuff = gui.margin(gui.centerHoriz(std::move(dismissBut), renderer.getTextLength("Dismiss") + 100),
    std::move(stuff), 30, gui.BOTTOM);
  stuff = gui.window(std::move(stuff), [&dismiss] { dismiss = true; });
  SGuiElem bg = gui.darken();
  bg->setBounds(Rectangle(renderer.getSize()));
  SDL::SDL_StartTextInput();
  OnExit tmp([]{ SDL::SDL_StopTextInput();});
  while (1) {
    callbacks.refreshScreen();
    bg->render(renderer);
    stuff->setBounds(getTextInputPosition());
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {stuff});
      if (dismiss)
        return none;
      if (event.type == SDL::SDL_TEXTINPUT)
        if (text.size() < maxLength)
          text += event.text.text;
/*        if ((isalnum(event.text.unicode) || event.text.unicode == ' ') && text.size() < maxLength)
          text += event.text.unicode;*/
      if (event.type == SDL::SDL_KEYDOWN)
        switch (event.key.keysym.sym) {
          case SDL::SDLK_BACKSPACE:
              if (!text.empty())
                text.pop_back();
              break;
          case SDL::SDLK_KP_ENTER:
          case SDL::SDLK_RETURN: return text;
          case SDL::SDLK_ESCAPE: return none;
          default: break;
        }
    }
  }
}

SGuiElem GuiBuilder::drawLevelMap(Semaphore& sem, const CreatureView* view) {
  auto miniMap = make_shared<MinimapGui>([]{});
  auto levelBounds = view->getLevel()->getBounds();
  miniMap->update(levelBounds, view);
  return gui.preferredSize(630, 630,
      gui.miniWindow(gui.stack(
          gui.buttonPos([=, &sem](Rectangle bounds, Vec2 pos) {
              mapGui->setCenter(levelBounds.width() * pos.x / bounds.width(),
                  levelBounds.height() * pos.y / bounds.height());
              sem.v(); }),
          gui.drawCustom([=](Renderer& r, Rectangle bounds) {
              miniMap->renderMap(r, bounds);
          })
      ), [&sem] { sem.v(); }));
}
