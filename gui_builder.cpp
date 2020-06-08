﻿/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

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
#include "team_order.h"
#include "lasting_effect.h"
#include "skill.h"
#include "player_role.h"
#include "tribe_alignment.h"
#include "avatar_menu_option.h"
#include "view_object_action.h"
#include "mod_info.h"
#include "special_trait.h"
#include "encyclopedia.h"
#include "item_action.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

#define THIS_LINE __LINE__

#define WL(FUN, ...) withLine(__LINE__, gui.FUN(__VA_ARGS__))

GuiBuilder::GuiBuilder(Renderer& r, GuiFactory& g, Clock* c, Options* o, Callbacks call)
    : renderer(r), gui(g), clock(c), options(o), callbacks(call), gameSpeed(GameSpeed::NORMAL),
      fpsCounter(60), upsCounter(60), cache(1000) {
}

void GuiBuilder::reset() {
  gameSpeed = GameSpeed::NORMAL;
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
  return WL(mouseOverAction, [this, s]() { hint = s; });
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
    bottomWindow = none;
  }
}

CollectiveTab GuiBuilder::getCollectiveTab() const {
  return collectiveTab;
}

void GuiBuilder::closeOverlayWindows() {
  // send a random id which wont be found
  callbacks.input({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()});
  callbacks.input({UserInputId::WORKSHOP, -1});
  bottomWindow = none;
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
  return WL(getListBuilder)
      .addElemAuto(WL(rightMargin, 5, WL(label, costText, color)))
      .addElem(WL(viewObject, cost.first), 25)
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
  auto getValue = [this] (CollectiveInfo::Button button, int num, CollectiveTab tab,
      const optional<TutorialInfo>& tutorial) {
    auto line = WL(getListBuilder);
    line.addElem(WL(viewObject, button.viewId), 35);
    auto tutorialHighlight = button.tutorialHighlight;
    if (button.state != CollectiveInfo::Button::ACTIVE)
      line.addElemAuto(WL(label, button.name + " " + button.count, Color::GRAY, button.hotkey));
    else {
      line.addElemAuto(WL(label, button.name + " " + button.count, Color::WHITE, button.hotkey));
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
    auto tutorialElem = WL(empty);
    if (tutorial && tutorialHighlight && tutorial->highlights.contains(*tutorialHighlight))
      tutorialElem = WL(conditional, WL(tutorialHighlight),
          [=]{ return !wasTutorialClicked(num, *tutorialHighlight); });
    return WL(setHeight, legendLineHeight, WL(stack, makeVec(
        getHintCallback({capitalFirst(button.help)}),
        WL(buttonChar, std::move(buttonFun), !button.hotkeyOpensGroup ? button.hotkey : 0, true, true),
        WL(uiHighlightConditional, [=] { return getActiveButton(tab) == num; }),
        tutorialElem,
        line.buildHorizontalList())));
  };
  return cache->get(getValue, THIS_LINE, button, num, tab, tutorial);
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

SGuiElem GuiBuilder::drawBuildings(const vector<CollectiveInfo::Button>& buttons, const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> keypressOnly;
  auto tab = CollectiveTab::BUILDINGS;
  auto elems = WL(getListBuilder, legendLineHeight);
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
      auto line = WL(getListBuilder);
      line.addElem(WL(viewObject, buttons[i].viewId), 35);
      char hotkey = buttons[i].hotkeyOpensGroup ? buttons[i].hotkey : 0;
      SGuiElem tutorialElem = WL(empty);
      if (tutorialHighlight)
        tutorialElem = WL(conditional, WL(tutorialHighlight),
             [=]{ return !wasTutorialClicked(combineHash(lastGroup), *tutorialHighlight); });
      line.addElemAuto(WL(label, lastGroup, Color::WHITE, hotkey));
      elems.addElem(WL(stack, makeVec(
          WL(keyHandlerChar, buttonFunHotkey, hotkey, true, true),
          WL(button, labelFun),
          tutorialElem,
          WL(uiHighlightConditional, [=] { return activeGroup == lastGroup;}),
          line.buildHorizontalList())));
    }
    if (buttons[i].groupName.empty())
      elems.addElem(getButtonLine(buttons[i], i, tab, tutorial));
    else
      keypressOnly.push_back(WL(invisible, getButtonLine(buttons[i], i, tab, tutorial)));
  }
  keypressOnly.push_back(elems.buildVerticalList());
  return WL(scrollable, WL(stack, std::move(keypressOnly)), &buildingsScroll, &scrollbarsHeld);
}

SGuiElem GuiBuilder::drawTechnology(CollectiveInfo& info) {
  int hash = combineHash(info.workshopButtons);
  if (hash != technologyHash) {
    technologyHash = hash;
    auto lines = WL(getListBuilder, legendLineHeight);
    lines.addSpace(legendLineHeight / 2);
    lines.addSpace(legendLineHeight / 2);
    lines.addSpace(legendLineHeight / 2);
    for (int i : All(info.workshopButtons)) {
      auto& button = info.workshopButtons[i];
      auto line = WL(getListBuilder);
      line.addElem(WL(viewObject, button.viewId), 35);
      line.addElemAuto(WL(label, button.name, button.unavailable ? Color::GRAY : Color::WHITE));
      SGuiElem elem = line.buildHorizontalList();
      if (button.active)
        elem = WL(stack,
            WL(uiHighlightLine, Color::GREEN),
            std::move(elem));
      if (!button.unavailable)
        elem = WL(stack,
          WL(button, [this, i] {
            workshopsScroll2.reset();
            workshopsScroll.reset();
            getButtonCallback(UserInput(UserInputId::WORKSHOP, i))(); }),
          std::move(elem));
      lines.addElem(std::move(elem));
    }
    technologyCache = lines.buildVerticalList();
  }
  return WL(external, technologyCache.get());
}

SGuiElem GuiBuilder::drawKeeperHelp(const GameInfo&) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(standardButton,
      WL(getListBuilder)
          .addElemAuto(WL(topMargin, -2, WL(viewObject, ViewId("special_bmbw"))))
          .addElemAuto(WL(label, "Bestiary"))
          .buildHorizontalList(),
      WL(button, [this]() { toggleBottomWindow(BESTIARY); })
  ));
  lines.addSpace(5);
  lines.addElem(WL(standardButton,
      WL(getListBuilder)
          .addElemAuto(WL(topMargin, -2, WL(viewObject, ViewId("book"))))
          .addElemAuto(WL(label, "Spell schools"))
          .buildHorizontalList(),
      WL(button, [this]() { toggleBottomWindow(SPELL_SCHOOLS); })
  ));
  lines.addSpace(5);
  lines.addElem(WL(standardButton,
      WL(getListBuilder)
          .addElemAuto(WL(topMargin, -2, WL(viewObject, ViewId("scroll"))))
          .addElemAuto(WL(label, "Items"))
          .buildHorizontalList(),
      WL(button, [this]() { toggleBottomWindow(ITEMS_HELP); })
  ));
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
  for (string line : helpText)
    lines.addElem(WL(label, line, Color::LIGHT_BLUE));
  return lines.buildVerticalList();
}

void GuiBuilder::addFpsCounterTick() {
  fpsCounter.addTick();
}

void GuiBuilder::addUpsCounterTick() {
  upsCounter.addTick();
}

const int resourceSpace = 110;

SGuiElem GuiBuilder::drawResources(const vector<CollectiveInfo::Resource>& numResource,
    const optional<TutorialInfo>& tutorial, int width) {
  auto line = WL(getListBuilder, resourceSpace);
  auto moreResources = WL(getListBuilder, legendLineHeight);
  int numResourcesShown = min(numResource.size(), (width - 250) / resourceSpace);
  if (numResourcesShown < 1)
    return gui.empty();
  if (numResourcesShown < numResource.size())
    --numResourcesShown;
  int maxMoreName = 0;
  for (int i : Range(numResourcesShown, numResource.size()))
    maxMoreName = max(maxMoreName, renderer.getTextLength(numResource[i].name));
  for (int i : All(numResource)) {
    auto res = WL(getListBuilder);
    if ( i >= numResourcesShown)
      res.addElem(WL(label, numResource[i].name), maxMoreName + 20);
    res.addElem(WL(viewObject, numResource[i].viewId), 30);
    res.addElem(WL(labelFun, [&numResource, i] { return toString<int>(numResource[i].count); },
          [&numResource, i] { return numResource[i].count >= 0 ? Color::WHITE : Color::RED; }), 50);
    auto tutorialHighlight = numResource[i].tutorialHighlight;
    auto tutorialElem = WL(conditional, WL(tutorialHighlight), [tutorialHighlight, tutorial] {
        return tutorial && tutorialHighlight && tutorial->highlights.contains(*tutorialHighlight);
    });
    auto elem = WL(stack,
          tutorialElem,
          getHintCallback({numResource[i].name}),
          res.buildHorizontalList());
    if (i < numResourcesShown)
      line.addElem(std::move(elem));
    else
      moreResources.addElem(std::move(elem));
  }
  if (!moreResources.isEmpty()) {
    int height = moreResources.getSize();
    line.addElem(WL(stack,
          WL(label, "[more]"),
          WL(tooltip2, WL(miniWindow, WL(margins, moreResources.buildVerticalList(), 15)),
              [=](const Rectangle& r) { return r.topLeft() - Vec2(0, height + 35);})
    ));
  }
  return line.buildHorizontalList();
}

SGuiElem GuiBuilder::drawBottomBandInfo(GameInfo& gameInfo, int width) {
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<CollectiveInfo>();
  GameSunlightInfo& sunlightInfo = gameInfo.sunlightInfo;
  auto bottomLine = WL(getListBuilder);
  const int space = 55;
  bottomLine.addSpace(space);
  bottomLine.addElem(WL(labelFun, [&info] {
      return capitalFirst(info.populationString) + ": " + toString(info.minionCount) + " / " +
        toString(info.minionLimit); }), 150);
  bottomLine.addSpace(space);
  bottomLine.addElem(getTurnInfoGui(gameInfo.time), 50);
  bottomLine.addSpace(space);
  bottomLine.addElem(getSunlightInfoGui(sunlightInfo), 80);
  return WL(getListBuilder, legendLineHeight)
        .addElem(WL(centerHoriz, drawResources(info.numResource, gameInfo.tutorial, width)))
        .addElem(WL(centerHoriz, bottomLine.buildHorizontalList()))
        .buildVerticalList();
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
  auto getIconHighlight = [&] (Color c) { return WL(topMargin, -1, WL(uiHighlight, c)); };
  auto& collectiveInfo = *info.playerInfo.getReferenceMaybe<CollectiveInfo>();
  int hash = combineHash(collectiveInfo, info.villageInfo, info.modifiedSquares, info.totalSquares, info.tutorial);
  if (hash != rightBandInfoHash) {
    rightBandInfoHash = hash;
    vector<SGuiElem> buttons = makeVec(
        WL(icon, gui.BUILDING),
        WL(icon, gui.MINION),
        WL(icon, gui.LIBRARY),
        WL(icon, gui.HELP)
    );
    for (int i : All(buttons)) {
      buttons[i] = WL(stack,
          WL(conditional, getIconHighlight(Color::GREEN), [this, i] { return int(collectiveTab) == i;}),
          std::move(buttons[i]),
          WL(button, [this, i]() { setCollectiveTab(CollectiveTab(i)); }));
    }
    if (info.tutorial) {
      if (info.tutorial->highlights.contains(TutorialHighlight::RESEARCH))
        buttons[2] = WL(stack, WL(conditional,
            WL(blink, getIconHighlight(Color::YELLOW)),
            [this] { return collectiveTab != CollectiveTab::TECHNOLOGY;}),
            buttons[2]);
      for (auto& building : collectiveInfo.buildings)
        if (auto& highlight = building.tutorialHighlight)
          if (info.tutorial->highlights.contains(*highlight)) {
            buttons[0] = WL(stack,
                WL(conditional,
                    WL(blink, getIconHighlight(Color::YELLOW)),
                    [this] { return collectiveTab != CollectiveTab::BUILDINGS;}),
                buttons[0]);
            break;
          }
      EnumSet<TutorialHighlight> minionHighlights = { TutorialHighlight::NEW_TEAM, TutorialHighlight::CONTROL_TEAM};
      if (!info.tutorial->highlights.intersection(minionHighlights).isEmpty())
        buttons[1] = WL(stack,
            WL(conditional,
                WL(blink, getIconHighlight(Color::YELLOW)),
                [this] { return collectiveTab != CollectiveTab::MINIONS;}),
            buttons[1]);
    }
    if ((!info.tutorial || !info.tutorial->highlights.contains(TutorialHighlight::RESEARCH)) &&
        collectiveInfo.avatarLevelInfo.numAvailable > 0)
      buttons[2] = WL(stack, WL(conditional,
          getIconHighlight(Color::YELLOW),
          [this] { return collectiveTab != CollectiveTab::TECHNOLOGY;}),
          buttons[2]);
    vector<pair<CollectiveTab, SGuiElem>> elems = makeVec(
        make_pair(CollectiveTab::MINIONS, drawMinions(collectiveInfo, info.tutorial)),
        make_pair(CollectiveTab::BUILDINGS, cache->get(bindMethod(
            &GuiBuilder::drawBuildings, this), THIS_LINE, collectiveInfo.buildings, info.tutorial)),
        make_pair(CollectiveTab::KEY_MAPPING, drawKeeperHelp(info)),
        make_pair(CollectiveTab::TECHNOLOGY, drawLibraryContent(collectiveInfo, info.tutorial))
    );
    vector<SGuiElem> tabs;
    for (auto& elem : elems) {
      auto tab = elem.first;
      tabs.push_back(WL(conditional, std::move(elem.second), [tab, this] { return tab == collectiveTab;}));
    }
    SGuiElem main = WL(stack, std::move(tabs));
    main = WL(margins, std::move(main), 15, 15, 15, 5);
    int numButtons = buttons.size();
    auto buttonList = WL(getListBuilder, 50);
    for (auto& elem : buttons)
      buttonList.addElem(std::move(elem));
    SGuiElem butGui = WL(margins,
        WL(centerHoriz, buttonList.buildHorizontalList()), 0, 5, 9, 5);
    auto bottomLine = WL(getListBuilder);
    bottomLine.addElemAuto(WL(stack,
        WL(getListBuilder)
            .addElemAuto(WL(label, "speed: "))
            .addElem(WL(labelFun, [this] { return getCurrentGameSpeedName();},
              [this] { return clock->isPaused() ? Color::RED : Color::WHITE; }), 70).buildHorizontalList(),
        WL(button, [&] { gameSpeedDialogOpen = !gameSpeedDialogOpen; })));
    int modifiedSquares = info.modifiedSquares;
    int totalSquares = info.totalSquares;
    bottomLine.addBackElem(WL(stack,
        WL(labelFun, [=]()->string {
          switch (counterMode) {
            case CounterMode::FPS:
              return "FPS " + toString(fpsCounter.getFps()) + " / " + toString(upsCounter.getFps());
            case CounterMode::LAT:
              return "LAT " + toString(fpsCounter.getMaxLatency()) + "ms / " + toString(upsCounter.getMaxLatency()) + "ms";
            case CounterMode::SMOD:
              return "SMOD " + toString(modifiedSquares) + "/" + toString(totalSquares);
          }
        }, Color::WHITE),
        WL(button, [=]() { counterMode = (CounterMode) ( ((int) counterMode + 1) % 3); })), 120);
    main = WL(margin, WL(leftMargin, 10, bottomLine.buildHorizontalList()),
        std::move(main), 18, gui.BOTTOM);
    rightBandInfoCache = WL(margin, std::move(butGui), std::move(main), 55, gui.TOP);
  }
  return WL(external, rightBandInfoCache.get());
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
  lines.push_back(WL(stack,
        WL(uiHighlightMouseOver),
        WL(getListBuilder, keyMargin)
            .addElem(WL(label, "pause"))
            .addElem(WL(label, "[space]")).buildHorizontalList(),
        WL(button, pauseFun)));
  hotkeys.push_back(WL(keyHandler, pauseFun, {gui.getKey(SDL::SDLK_SPACE)}));
  for (GameSpeed speed : ENUM_ALL(GameSpeed)) {
    auto speedFun = [=] { gameSpeed = speed; gameSpeedDialogOpen = false; clock->cont();};
    auto colorFun = [this, speed] { return speed == gameSpeed ? Color::GREEN : Color::WHITE; };
    lines.push_back(WL(stack,
          WL(uiHighlightMouseOver),
          WL(getListBuilder, keyMargin)
              .addElem(WL(label, getGameSpeedName(speed), colorFun))
              .addElem(WL(label, "'" + string(1, getHotkeyChar(speed)) + "' ", colorFun)).buildHorizontalList(),
          WL(button, speedFun)));
    hotkeys.push_back(WL(keyHandler, speedFun, {gui.getKey(getHotkey(speed))}));
  }
  reverse(lines.begin(), lines.end());
  int margin = 20;
  Vec2 size(150 + 2 * margin, legendLineHeight * lines.size() - 10 + 2 * margin);
  SGuiElem dialog = WL(miniWindow,
      WL(margins, WL(verticalList, std::move(lines), legendLineHeight), margin));
  return WL(stack, WL(stack, std::move(hotkeys)),
      WL(preferredSize, size, WL(conditional, std::move(dialog), [this] { return gameSpeedDialogOpen; })));
}

SGuiElem GuiBuilder::drawSpecialTrait(const ImmigrantDataInfo::SpecialTraitInfo& trait) {
  return WL(label, trait.label, trait.bad ? Color::RED : Color::GREEN);
}

void GuiBuilder::toggleBottomWindow(GuiBuilder::BottomWindowId id) {
  if (bottomWindow == id)
    bottomWindow = none;
  else
    bottomWindow = id;
}

SGuiElem GuiBuilder::drawImmigrantInfo(const ImmigrantDataInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  if (info.autoState)
    switch (*info.autoState) {
      case ImmigrantAutoState::AUTO_ACCEPT:
        lines.addElem(WL(label, "(Immigrant will be accepted automatically)", Color::GREEN));
        break;
      case ImmigrantAutoState::AUTO_REJECT:
        lines.addElem(WL(label, "(Immigrant will be rejected automatically)", Color::RED));
        break;
    }
  lines.addElem(
      WL(getListBuilder)
          .addElemAuto(WL(label, capitalFirst(info.name)))
          .addSpace(100)
          .addBackElemAuto(info.cost ? drawCost(*info.cost) : WL(empty))
          .buildHorizontalList());
  lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(info.attributes)));
  if (info.timeLeft)
    lines.addElem(WL(label, "Turns left: " + toString(info.timeLeft)));
  for (auto& req : info.specialTraits)
    lines.addElem(drawSpecialTrait(req));
  for (auto& req : info.info)
    lines.addElem(WL(label, req, Color::WHITE));
  for (auto& req : info.requirements)
    lines.addElem(WL(label, req, Color::ORANGE));
  return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
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
  auto continueButton = WL(setHeight, legendLineHeight, WL(buttonLabelBlink, "Continue", [this] {
          callbacks.input(UserInputId::TUTORIAL_CONTINUE);
          tutorialClicks.clear();
          closeOverlayWindowsAndClearButton();
      }));
  auto backButton = WL(setHeight, legendLineHeight, WL(buttonLabel, "Go back", getButtonCallback(UserInputId::TUTORIAL_GO_BACK)));
  SGuiElem warning;
  if (info.warning)
    warning = WL(label, *info.warning, Color::RED);
  else
    warning = WL(label, "Press [Space] to unpause.",
        [this]{ return clock->isPaused() ? Color::RED : Color::TRANSPARENT;});
  return WL(preferredSize, 520, 290, WL(stack, WL(darken), WL(rectangleBorder, Color::GRAY),
      WL(margins, WL(stack,
        WL(labelMultiLine, info.message, legendLineHeight),
        WL(leftMargin, 50, WL(alignment, GuiFactory::Alignment::BOTTOM_CENTER, WL(setHeight, legendLineHeight, warning))),
        WL(alignment, GuiFactory::Alignment::BOTTOM_RIGHT, info.canContinue ? continueButton : WL(empty)),
        WL(leftMargin, 50, WL(alignment, GuiFactory::Alignment::BOTTOM_LEFT, info.canGoBack ? backButton : WL(empty)))
      ), 20, 20, 20, 10)));
}

static Color getTriggerColor(double value) {
  return Color::f(1, max(0.0, 1 - value * 500), max(0.0, 1 - value * 500));
}

SGuiElem GuiBuilder::drawVillainType(VillainType type) {
  auto getColor = [] (VillainType type) {
    switch (type) {
      case VillainType::MAIN: return Color::RED;
      case VillainType::LESSER: return Color::ORANGE;
      case VillainType::ALLY: return Color::GREEN;
      case VillainType::NONE: return Color::GRAY;
      case VillainType::PLAYER: return Color::GREEN;
    }
  };
  return WL(label, getName(type), getColor(type));
}

SGuiElem GuiBuilder::drawVillainInfoOverlay(const VillageInfo::Village& info, bool showDismissHint) {
  auto lines = WL(getListBuilder, legendLineHeight);
  string name = info.tribeName;
  if (info.name)
    name  = *info.name + ", " + name;
  lines.addElem(
      WL(getListBuilder)
          .addElemAuto(WL(label, capitalFirst(name)))
          .addSpace(100)
          .addElemAuto(drawVillainType(info.type))
          .buildHorizontalList());
  if (info.attacking)
    lines.addElem(WL(label, "Attacking!", Color::RED));
  if (info.access == VillageInfo::Village::NO_LOCATION)
    lines.addElem(WL(label, "Location unknown", Color::LIGHT_BLUE));
  else if (info.access == VillageInfo::Village::INACTIVE)
    lines.addElem(WL(label, "Outside of influence zone", Color::GRAY));
  if (info.isConquered)
    lines.addElem(WL(label, "Conquered", Color::LIGHT_BLUE));
  auto triggers = info.triggers;
  sort(triggers.begin(), triggers.end(),
      [] (const VillageInfo::Village::TriggerInfo& t1, const VillageInfo::Village::TriggerInfo& t2) {
          return t1.value > t2.value;});
  if (!triggers.empty()) {
    auto line = WL(getListBuilder);
    line.addElemAuto(WL(label, "Triggered by: "));
    bool addComma = false;
    for (auto& t : triggers) {
      auto name = t.name;
  /*#ifndef RELEASE
      name += " " + toString(t.value);
  #endif*/
      if (addComma)
        line.addElemAuto(WL(label, ", "));
      line.addElemAuto(WL(label, name, getTriggerColor(t.value)));
      addComma = true;
    }
    lines.addElem(line.buildHorizontalList());
  }
  for (auto action : info.actions)
    if (action.disabledReason)
      lines.addElem(WL(label, *action.disabledReason, Color::ORANGE));
  if (showDismissHint)
    lines.addElem(WL(label, "Right click to dismiss", Renderer::smallTextSize, Color::WHITE), legendLineHeight * 2 / 3);
  return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
}

static const char* getVillageActionText(VillageAction action) {
  switch (action) {
    case VillageAction::TRADE:
      return "trade";
    case VillageAction::PILLAGE:
      return "pillage";
  }
}

SGuiElem GuiBuilder::drawVillainsOverlay(const VillageInfo& info) {
  const int labelOffsetY = 3;
  auto lines = WL(getListBuilder);
  lines.addSpace(90);
  if (!info.villages.empty())
    lines.addElemAuto(WL(stack,
          WL(getListBuilder)
              .addElem(WL(icon, GuiFactory::EXPAND_UP), 12)
              .addElemAuto(WL(translate, WL(label, "All villains"), Vec2(0, labelOffsetY)))
              .addSpace(10)
              .buildHorizontalList(),
          WL(conditional, WL(margins, WL(rectangle, Color(0, 255, 0, 100)), -7, 2, 2, 2),
              [this] { return bottomWindow == ALL_VILLAINS; }),
          WL(button, [this] { toggleBottomWindow(ALL_VILLAINS); })
    ));
  for (int i : All(info.villages)) {
    SGuiElem label;
    string labelText;
    SGuiElem button = WL(empty);
    auto& elem = info.villages[i];
    if (elem.attacking) {
      labelText = "attacking";
      label = WL(label, labelText, Color::RED);
    }
    else if (!elem.triggers.empty()) {
      labelText = "triggered";
      label = WL(label, labelText, Color::ORANGE);
    } else
      for (auto& action : elem.actions)
        if (!action.disabledReason) {
          labelText = getVillageActionText(action.action);
          label = WL(label, labelText, Color::GREEN);
          button = WL(button, getButtonCallback({UserInputId::VILLAGE_ACTION,
              VillageActionInfo{elem.id, action.action}}));
          break;
        }
    if (!label || info.dismissedInfos.count({elem.id, labelText}))
      continue;
    label = WL(translate, std::move(label), Vec2(0, labelOffsetY));
    auto infoOverlay = drawVillainInfoOverlay(elem, true);
    int infoOverlayHeight = *infoOverlay->getPreferredHeight();
    lines.addElemAuto(WL(stack, makeVec(
        std::move(button),
        WL(releaseRightButton, getButtonCallback({UserInputId::DISMISS_VILLAGE_INFO,
            DismissVillageInfo{elem.id, labelText}})),
        WL(onMouseRightButtonHeld, WL(margins, WL(rectangle, Color(255, 0, 0, 100)), 0, 2, 2, 2)),
        WL(getListBuilder)
            .addElemAuto(WL(stack,
                 WL(setWidth, 34, WL(centerVert, WL(centerHoriz, WL(bottomMargin, -3,
                     WL(viewObject, ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
                 WL(setWidth, 34, WL(centerVert, WL(centerHoriz, WL(bottomMargin, 5,
                     WL(viewObject, elem.viewId)))))))
            .addElemAuto(WL(rightMargin, 5, WL(translate, std::move(label), Vec2(-2, 0))))
            .buildHorizontalList(),
        WL(conditional, getTooltip2(std::move(infoOverlay), [=](const Rectangle& r) { return r.topLeft() - Vec2(0, 0 + infoOverlayHeight);}),
            [this]{return !bottomWindow;})
    )));
  }
  return WL(setHeight, 29, WL(stack,
        WL(stopMouseMovement),
        WL(translucentBackgroundWithBorder, WL(topMargin, 0, lines.buildHorizontalList()))));
}

SGuiElem GuiBuilder::drawAllVillainsOverlay(const VillageInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  if (info.numMainVillains > 0) {
    lines.addElem(WL(label, toString(info.numConqueredMainVillains) + "/" + toString(info.numMainVillains) +
        " main villains conquered"));
    lines.addSpace(legendLineHeight / 2);
  }
  for (int i : All(info.villages)) {
    auto& elem = info.villages[i];
    auto infoOverlay = drawVillainInfoOverlay(elem, false);
    auto labelColor = Color::WHITE;
    if (elem.access == elem.INACTIVE)
      labelColor = Color::GRAY;
    else if (elem.attacking)
      labelColor = Color::RED;
    else if (!elem.triggers.empty())
      labelColor = Color::ORANGE;
    string name = elem.tribeName;
    if (elem.name)
      name  = *elem.name + ", " + name;
    auto label = WL(label, capitalFirst(name), labelColor);
    if (elem.isConquered)
      label = WL(stack, std::move(label), WL(crossOutText, Color::RED));
    for (auto& action : elem.actions)
      if (!action.disabledReason) {
        label = WL(stack,
            WL(getListBuilder)
                .addElemAuto(std::move(label))
                .addBackElemAuto(WL(label, " ["_s + getVillageActionText(action.action) + "]", Color::GREEN))
                .buildHorizontalList(),
            WL(button, getButtonCallback({UserInputId::VILLAGE_ACTION,
                  VillageActionInfo{elem.id, action.action}}))
        );
        break;
      }
    lines.addElem(WL(stack,
        WL(getListBuilder)
            .addElemAuto(WL(setWidth, 34, WL(centerVert, WL(centerHoriz, WL(stack,
                 WL(bottomMargin, -3, WL(viewObject, ViewId("round_shadow"), 1, Color(255, 255, 255, 160))),
                 WL(bottomMargin, 5, WL(viewObject, elem.viewId))
            )))))
            .addElemAuto(WL(rightMargin, 5, WL(translate, WL(renderInBounds, std::move(label)), Vec2(0, 0))))
            .buildHorizontalList(),
        WL(tooltip2, std::move(infoOverlay), [=](const Rectangle& r) { return r.topRight();})));
  }
  return WL(setWidth, 350, WL(miniWindow, WL(margins, WL(stack,
        WL(stopMouseMovement),
        WL(scrollable, lines.buildVerticalList(), &tasksScroll, &scrollbarsHeld)), 15), [this] { bottomWindow = none; }, true));
}

SGuiElem GuiBuilder::drawImmigrationOverlay(const vector<ImmigrantDataInfo>& immigrants,
    const optional<TutorialInfo>& tutorial, bool drawHelp) {
  const int elemWidth = getImmigrationBarWidth();
  auto makeHighlight = [=] (Color c) { return WL(margins, WL(rectangle, c), 4); };
  auto lines = WL(getListBuilder, elemWidth);
  auto getAcceptButton = [=] (int immigrantId, optional<Keybinding> keybinding) {
    return WL(stack,
        WL(releaseLeftButton, getButtonCallback({UserInputId::IMMIGRANT_ACCEPT, immigrantId}), keybinding),
        WL(onMouseLeftButtonHeld, makeHighlight(Color(0, 255, 0, 100))));
  };
  auto getRejectButton = [=] (int immigrantId) {
    return WL(stack,
        WL(releaseRightButton, getButtonCallback({UserInputId::IMMIGRANT_REJECT, immigrantId})),
        WL(onMouseRightButtonHeld, makeHighlight(Color(255, 0, 0, 100))));
  };
  for (int i : All(immigrants)) {
    auto& elem = immigrants[i];
    SGuiElem button;
    if (elem.requirements.empty())
      button = WL(stack, makeVec(
          WL(sprite, GuiFactory::TexId::IMMIGRANT_BG, GuiFactory::Alignment::CENTER),
          cache->get(getAcceptButton, THIS_LINE, elem.id, elem.keybinding),
          cache->get(getRejectButton, THIS_LINE, elem.id)
      ));
    else
      button = WL(stack, makeVec(
          WL(sprite, GuiFactory::TexId::IMMIGRANT2_BG, GuiFactory::Alignment::CENTER),
          cache->get(getRejectButton, THIS_LINE, elem.id)
      ));
    if (!elem.specialTraits.empty())
      button = WL(stack,
          std::move(button),
          WL(icon, GuiFactory::IconId::SPECIAL_IMMIGRANT)
      );
    if (tutorial && elem.tutorialHighlight && tutorial->highlights.contains(*elem.tutorialHighlight))
        button = WL(stack, std::move(button), WL(blink, makeHighlight(Color(255, 255, 0, 100))));
    auto initTime = elem.generatedTime;
    lines.addElem(WL(translate, [=]() { return Vec2(0, initTime ? -getImmigrantAnimationOffset(*initTime) : 0);},
        WL(stack,
            std::move(button),
            WL(tooltip2, drawImmigrantInfo(elem), [](const Rectangle& r) { return r.topRight();}),
            WL(setWidth, elemWidth, WL(centerVert, WL(centerHoriz, WL(bottomMargin, -3,
                WL(viewObject, ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
            WL(setWidth, elemWidth, WL(centerVert, WL(centerHoriz, WL(bottomMargin, 5,
                elem.count ? drawMinionAndLevel(elem.viewId, *elem.count, 1) : WL(viewObject, elem.viewId)))))
    )));
  }
  if (drawHelp)
    lines.addElem(WL(stack, makeVec(
        WL(sprite, GuiFactory::TexId::IMMIGRANT_BG, GuiFactory::Alignment::CENTER),
        WL(conditional, makeHighlight(Color(0, 255, 0, 100)), [this] { return bottomWindow == IMMIGRATION_HELP; }),
        WL(button, [this] { toggleBottomWindow(IMMIGRATION_HELP); }),
        WL(setWidth, elemWidth, WL(topMargin, -2, WL(centerHoriz, WL(label, "?", 32, Color::GREEN))))
    )));
  return WL(setWidth, elemWidth, WL(stack,
        WL(stopMouseMovement),
        lines.buildVerticalList()));
}

SGuiElem GuiBuilder::getImmigrationHelpText() {
  return WL(labelMultiLine, "Welcome to the immigration system! The icons immediately to the left represent "
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
  auto lines = WL(getListBuilder, elemWidth);
  auto line = WL(getListBuilder, elemWidth);
  for (auto& elem : info.allImmigration) {
    auto icon = WL(viewObject, elem.viewId, iconScale);
    if (elem.autoState)
      switch (*elem.autoState) {
        case ImmigrantAutoState::AUTO_ACCEPT:
          icon = WL(stack, std::move(icon), WL(viewObject, ViewId("accept_immigrant"), iconScale));
          break;
        case ImmigrantAutoState::AUTO_REJECT:
          icon = WL(stack, std::move(icon), WL(viewObject, ViewId("reject_immigrant"), iconScale));
          break;
      }
    line.addElem(WL(stack, makeVec(
        WL(button, getButtonCallback({UserInputId::IMMIGRANT_AUTO_ACCEPT, elem.id})),
        WL(buttonRightClick, getButtonCallback({UserInputId::IMMIGRANT_AUTO_REJECT, elem.id})),
        WL(tooltip2, drawImmigrantInfo(elem), [](const Rectangle& r) { return r.bottomLeft();}),
        WL(setWidth, elemWidth, WL(centerVert, WL(centerHoriz, WL(bottomMargin, -3,
            WL(viewObject, ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
        WL(setWidth, elemWidth, WL(centerVert, WL(centerHoriz, WL(bottomMargin, 5, std::move(icon))))))));
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
  return WL(miniWindow, WL(margins, WL(stack,
        WL(stopMouseMovement),
        lines.buildVerticalList()), 15), [this] { bottomWindow = none; }, true);
}

SGuiElem GuiBuilder::getSunlightInfoGui(const GameSunlightInfo& sunlightInfo) {
  return WL(stack,
      WL(conditional,
          getHintCallback({"Time remaining till nightfall."}),
          getHintCallback({"Time remaining till day."}),
          [&] { return sunlightInfo.description == "day";}),
      WL(labelFun, [&] { return sunlightInfo.description + " [" + toString(sunlightInfo.timeRemaining) + "]"; },
          [&] { return sunlightInfo.description == "day" ? Color::WHITE : Color::LIGHT_BLUE;}));
}

SGuiElem GuiBuilder::getTurnInfoGui(const GlobalTime& turn) {
  return WL(stack, getHintCallback({"Current turn."}),
      WL(labelFun, [&turn] { return "T: " + toString(turn); }, Color::WHITE));
}

vector<SGuiElem> GuiBuilder::drawPlayerAttributes(const vector<AttributeInfo>& attr) {
  vector<SGuiElem> ret;
  for (auto& elem : attr) {
    auto getValue = [this](const AttributeInfo& elem) {
      auto attrText = toString(elem.value);
      if (elem.bonus != 0)
        attrText += toStringWithSign(elem.bonus);
      return WL(stack, getTooltip({elem.name, elem.help}, THIS_LINE),
          WL(horizontalList, makeVec(
              WL(icon, elem.attr),
              WL(margins, WL(label, attrText), 0, 2, 0, 0)), 30));
    };
    ret.push_back(cache->get(getValue, THIS_LINE, elem));
  }
  return ret;
}

vector<SGuiElem> GuiBuilder::drawPlayerAttributes(const ViewObject::CreatureAttributes& attributes) {
  vector<SGuiElem> ret;
  for (auto attr : ENUM_ALL(AttrType))
    ret.push_back(
        WL(horizontalList, makeVec(
          WL(icon, attr),
          WL(margins, WL(label, toString((int) attributes[attr])), 0, 2, 0, 0)), 30));
  return ret;
}

SGuiElem GuiBuilder::drawBottomPlayerInfo(const GameInfo& gameInfo) {
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
  return WL(getListBuilder, legendLineHeight)
      .addElem(WL(centerHoriz, WL(horizontalList,
           drawPlayerAttributes(gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>()->attributes), resourceSpace)))
      .addElem(WL(centerHoriz, WL(getListBuilder)
           .addElemAuto(info.avatarLevelInfo ? WL(standardButton, WL(stack,
               WL(margins, WL(progressBar, Color::DARK_GREEN.transparency(128), info.avatarLevelInfo->progress), -3, -1, 0, 7),
               info.avatarLevelInfo->numAvailable > 0 ? WL(margins,
                   WL(uiHighlightLine), 3, -2, -3, 2) : WL(empty),
               WL(getListBuilder)
//                     .addSpace(10)
                   .addElemAuto(WL(topMargin, -2, WL(viewObject, info.avatarLevelInfo->viewId)))
                   .addElemAuto(WL(label, "Level: " + toString(info.avatarLevelInfo->level)))
                   .buildHorizontalList()),
               WL(button, [this]() { closeOverlayWindowsAndClearButton(); callbacks.input(UserInputId::LEVEL_UP);})
           ) : WL(empty))
          .addSpace(20)
          .addElem(getTurnInfoGui(gameInfo.time), 90)
          .addElem(getSunlightInfoGui(gameInfo.sunlightInfo), 140)
          .buildHorizontalList()))
      .buildVerticalList();
}

static int viewObjectWidth = 27;

int GuiBuilder::getItemLineOwnerMargin() {
  return viewObjectWidth + 60;
}

static const char* getIntrinsicStateText(const ItemInfo& item) {
  if (auto state = item.intrinsicAttackState) {
    if (state == item.INACTIVE)
      return "Inactive";
    return item.intrinsicExtraAttack ? "Extra attack in addition to wielded weapon" : "Used when no weapon equiped";
  }
  return nullptr;
}

static optional<Color> getIntrinsicStateColor(const ItemInfo& item) {
  if (auto state = item.intrinsicAttackState) {
    if (state == item.INACTIVE)
      return Color::LIGHT_GRAY;
    return item.intrinsicExtraAttack ? Color::GREEN : Color::WHITE;
  }
  return none;
}

vector<string> GuiBuilder::getItemHint(const ItemInfo& item) {
  vector<string> out { capitalFirst(item.fullName)};
  out.append(item.description);
  if (item.equiped)
    out.push_back("Equipped.");
  if (item.pending)
    out.push_back("Not equipped yet.");
  if (auto text = getIntrinsicStateText(item))
    out.push_back(text);
  if (!item.unavailableReason.empty())
    out.push_back(item.unavailableReason);
  return out;
}

SGuiElem GuiBuilder::drawBestAttack(const BestAttack& info) {
  return WL(getListBuilder, 30)
      .addElem(WL(icon, info.attr))
      .addElem(WL(topMargin, 2, WL(label, toString((int) info.value))))
      .buildHorizontalList();
}

static string getWeightString(double weight) {
  return toString<int>((int)(weight * 10)) + "s";
}

SGuiElem GuiBuilder::getItemLine(const ItemInfo& item, function<void(Rectangle)> onClick,
    function<void()> onMultiClick) {
  auto line = WL(getListBuilder);
  int leftMargin = 0;
  auto viewId = WL(viewObject, item.viewId);
  if (item.viewIdModifiers.contains(ViewObjectModifier::AURA))
    viewId = WL(stack, std::move(viewId), WL(viewObject, ViewId("item_aura")));
  line.addElem(std::move(viewId), viewObjectWidth);
  Color color = item.equiped ? Color::GREEN : (item.pending || item.unavailable) ?
      Color::GRAY : Color::WHITE;
  if (auto col = getIntrinsicStateColor(item))
    color = *col;
  auto name = item.name;
  if (item.number > 1)
    line.addElemAuto(WL(rightMargin, 0, WL(label, toString(item.number) + " ", color)));
  else
    name = capitalFirst(name);
  line.addMiddleElem(WL(label, name, color));
  auto mainLine = WL(stack,
      WL(buttonRect, onClick),
      WL(renderInBounds, line.buildHorizontalList()),
      getTooltip(getItemHint(item), (int) item.ids.getHash()));
  line.clear();
  line.addMiddleElem(std::move(mainLine));
  line.addBackSpace(5);
  if (item.owner) {
    line.addBackElem(WL(viewObject, item.owner->viewId), viewObjectWidth);
    line.addBackElem(drawBestAttack(item.owner->bestAttack), getItemLineOwnerMargin() - viewObjectWidth);
  }
  if (item.price)
    line.addBackElemAuto(drawCost(*item.price));
  if (item.weight)
    line.addBackElemAuto(WL(label, "[" + getWeightString(*item.weight * item.number) + "]"));
  if (onMultiClick && item.number > 1) {
    line.addBackElem(WL(stack,
        WL(label, "[#]"),
        WL(button, onMultiClick),
        getTooltip({"Click to choose how many to pick up."}, int(item.ids.begin()->getGenericId()))), 25);
  }
  auto elem = line.buildHorizontalList();
  if (item.tutorialHighlight)
    elem = WL(stack, WL(tutorialHighlight), std::move(elem));
  return WL(margins, std::move(elem), leftMargin, 0, 0, 0);
}

SGuiElem GuiBuilder::getTooltip(const vector<string>& text, int id) {
  return cache->get(
      [this](const vector<string>& text) {
        return WL(conditional, WL(tooltip, text), [this] { return !disableTooltip;}); },
      id, text);
}

SGuiElem GuiBuilder::getTooltip2(SGuiElem elem, GuiFactory::PositionFun fun) {
  return WL(conditional, WL(tooltip2, std::move(elem), std::move(fun)), [this] { return !disableTooltip;});
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
    return WL(empty);
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
      lines.push_back(WL(leftMargin, 14, WL(stack,
            WL(mouseHighlight, WL(highlight, listLineHeight), i, &itemIndex),
            WL(leftMargin, 6, getItemLine(info.lyingItems[i],
          [this, i](Rectangle) { callbacks.input({UserInputId::PICK_UP_ITEM, i}); },
          [this, i]() { callbacks.input({UserInputId::PICK_UP_ITEM_MULTI, i}); })))));
    }
  }
  int totalElems = info.lyingItems.size();
  if (itemIndex.value_or(-1) >= totalElems)
    itemIndex = totalElems - 1;
  SGuiElem content;
  if (totalElems == 1 && !playerOverlayFocused)
    content = WL(stack,
        WL(margin,
          WL(leftMargin, 3, WL(label, title, Color::YELLOW)),
          WL(scrollable, WL(verticalList, std::move(lines), legendLineHeight), &lyingItemsScroll),
          legendLineHeight, GuiFactory::TOP),
        WL(keyHandler, [=] { callbacks.input({UserInputId::PICK_UP_ITEM, 0});}, getConfirmationKeys(), true));
  else {
    auto updateScrolling = [this, totalElems] (int dir) {
        if (itemIndex)
          itemIndex = (*itemIndex + dir + totalElems) % totalElems;
        else
          itemIndex = 0;
        lyingItemsScroll.set(*itemIndex * legendLineHeight + legendLineHeight / 2, clock->getRealMillis());
    };
    content = WL(stack, makeVec(
          WL(focusable,
              WL(stack,
                  WL(keyHandler, [=] { if (itemIndex) { callbacks.input({UserInputId::PICK_UP_ITEM, *itemIndex});}},
                    getConfirmationKeys(), true),
                  WL(keyHandler, [=] { updateScrolling(1); },
                    {gui.getKey(SDL::SDLK_DOWN), gui.getKey(SDL::SDLK_KP_2)}, true),
                  WL(keyHandler, [=] { updateScrolling(-1); },
                    {gui.getKey(SDL::SDLK_UP), gui.getKey(SDL::SDLK_KP_8)}, true)),
              getConfirmationKeys(), {gui.getKey(SDL::SDLK_ESCAPE)}, playerOverlayFocused),
          WL(keyHandler, [=] { if (!playerOverlayFocused) { itemIndex = 0; lyingItemsScroll.reset();} },
              getConfirmationKeys()),
          WL(keyHandler, [=] { itemIndex = none; }, {gui.getKey(SDL::SDLK_ESCAPE)}),
          WL(margin,
            WL(leftMargin, 3, WL(label, title, Color::YELLOW)),
            WL(scrollable, WL(verticalList, std::move(lines), legendLineHeight), &lyingItemsScroll),
            legendLineHeight, GuiFactory::TOP)));
  }
  int margin = 14;
  return WL(stack,
      WL(conditional, WL(stack, WL(fullScreen, WL(darken)), WL(miniWindow)), WL(translucentBackground),
        [=] { return playerOverlayFocused;}),
      WL(margins, WL(preferredSize, size, std::move(content)), margin));
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
    case ItemAction::REMOVE: return "remove item";
    case ItemAction::CHANGE_NUMBER: return "change number";
    case ItemAction::NAME: return "name";
    case ItemAction::INTRINSIC_ACTIVATE: return "activate attack";
    case ItemAction::INTRINSIC_DEACTIVATE: return "deactivate attack";
  }
}

void GuiBuilder::drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width, bool darkBg) {
  if (elems.isEmpty())
    return;
  int contentHeight = elems.getSize();
  int margin = 15;
  SGuiElem menu = WL(miniWindow, WL(margins, elems.buildVerticalList(), 5 + margin, margin, margin, margin),
          [&exit] { exit = true; });
  Vec2 size(width + 2 * margin, contentHeight + 2 * margin);
  menuPos.y -= max(0, menuPos.y + size.y - renderer.getSize().y);
  menuPos.x -= max(0, menuPos.x + size.x - renderer.getSize().x);
  menu->setBounds(Rectangle(menuPos, menuPos + size));
  SGuiElem bg = WL(darken);
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
  auto list = WL(getListBuilder, legendLineHeight);
  bool exit = false;
  optional<int> ret;
  for (int i : All(elems))
    list.addElem(WL(stack,
        WL(button, [i, &ret, &exit] { ret = i; exit = true; }),
        WL(uiHighlightMouseOver),
        WL(label, elems[i])
    ));
  drawMiniMenu(std::move(list), exit, renderer.getMousePos(), 200, false);
  return ret;
}

SGuiElem GuiBuilder::withLine(int n, SGuiElem e) {
  e->setLineNumber(n);
  return e;
}

GuiFactory::ListBuilder GuiBuilder::withLine(int n, GuiFactory::ListBuilder e) {
  e.lineNumber = n;
  return e;
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
  SGuiElem stuff = WL(margins,
      drawListGui("", ListElem::convert(options), MenuType::NORMAL, &index, &choice, nullptr), 20, 15, 15, 10);
  stuff = WL(miniWindow, WL(margins, std::move(stuff), 0));
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
        if (*index < itemInfo.actions.size())
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
            if (index && *index < itemInfo.actions.size())
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
    lines.push_back(WL(label, "Skills", Color::YELLOW));
    for (auto& elem : info.skills)
      lines.push_back(WL(stack, getTooltip({elem.help}, THIS_LINE),
            WL(label, capitalFirst(elem.name), Color::WHITE)));
    lines.push_back(WL(empty));
  }
  return lines;
}

const Vec2 spellIconSize = Vec2(47, 47);

SGuiElem GuiBuilder::getSpellIcon(const SpellInfo& spell, int index, bool active, GenericId creatureId) {
  vector<SGuiElem> ret;
  if (!spell.timeout) {
    ret.push_back(WL(mouseHighlight2, WL(standardButtonHighlight), WL(standardButton)));
    ret.push_back(WL(centerHoriz, WL(centerVert, WL(labelUnicode, spell.symbol, Color::WHITE))));
    if (active)
      ret.push_back(WL(button, getButtonCallback({UserInputId::CAST_SPELL, index})));
  } else {
    ret.push_back(WL(standardButton));
    ret.push_back(WL(centerHoriz, WL(centerVert, WL(labelUnicode, spell.symbol, Color::GRAY))));
    ret.push_back(WL(darken));
    ret.push_back(WL(centeredLabel, Renderer::HOR_VER, toString(*spell.timeout)));
  }
  ret.push_back(getTooltip(concat({capitalFirst(spell.name)}, spell.help), THIS_LINE + index + creatureId));
  return WL(preferredSize, spellIconSize, WL(stack, std::move(ret)));
}

SGuiElem GuiBuilder::drawSpellsList(const vector<SpellInfo>& spells, GenericId creatureId, bool active) {
  constexpr int spellsPerRow = 5;
  if (!spells.empty()) {
    auto list = WL(getListBuilder, spellIconSize.y);
    list.addElem(WL(label, "Abilities", Color::YELLOW), legendLineHeight);
    auto line = WL(getListBuilder);
    for (int index : All(spells)) {
      auto& elem = spells[index];
      line.addElemAuto(getSpellIcon(elem, index, active, creatureId));
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
    lines.push_back(WL(stack,
          getTooltip({effect.help}, THIS_LINE + i),
          WL(renderInBounds, WL(label, effect.name, effect.bad ? Color::RED : Color::WHITE))));
  }
  return lines;
}

static string getKeybindingDesc(char c) {
  if (c == ' ')
    return "[Space]";
  else
    return "["_s + (char)toupper(c) + "]";
}

static string toStringRounded(double value, double precision) {
  return toString(precision * round(value / precision));
}

SGuiElem GuiBuilder::getExpIncreaseLine(const CreatureExperienceInfo& info, ExperienceType type,
    function<void()> increaseCallback, bool infoOnly) {
  if (info.limit[type] == 0)
    return nullptr;
  auto line = WL(getListBuilder);
  int i = 0;
  vector<string> attrNames;
  auto attrIcons = WL(getListBuilder);
  for (auto attr : getAttrIncreases()[type]) {
    attrIcons.addElem(WL(topMargin, -3, WL(icon, attr)), 22);
    attrNames.push_back(getName(attr));
  }
  line.addElem(attrIcons.buildHorizontalList(), 80);
  if (!infoOnly)
    line.addElem(WL(label, "+" + toStringRounded(info.level[type], 0.01),
        info.warning[type] ? Color::RED : Color::WHITE), 50);
  if (increaseCallback && info.numAvailableUpgrades > 0) {
    line.addElemAuto(info.level[type] < info.limit[type]
        ? WL(buttonLabel, "+", increaseCallback)
        : WL(buttonLabelInactive, "+"));
    line.addSpace(15);
  }
  string limit = toString(info.limit[type]);
  line.addElemAuto(WL(label, "  (limit " + limit + ")"));
  vector<string> tooltip {
      getName(type) + " training."_s,
      "Increases " + combine(attrNames) + ".",
      "The creature's limit for this type of training is " + limit + "."};
  if (info.warning[type])
    tooltip.push_back(*info.warning[type]);
  return WL(stack,
             getTooltip(tooltip, THIS_LINE),
             line.buildHorizontalList());
}

SGuiElem GuiBuilder::drawTrainingInfo(const CreatureExperienceInfo& info,
    function<void(optional<ExperienceType>)> increaseCallback, bool infoOnly) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, "Training", Color::YELLOW));
  if (increaseCallback) {
    lines.addElem(WL(label, getPlural("point", info.numAvailableUpgrades) + " available"));
  }
  bool empty = true;
  for (auto expType : ENUM_ALL(ExperienceType)) {
    if (auto elem = getExpIncreaseLine(info, expType,
        [increaseCallback, expType] { increaseCallback(expType); }, infoOnly)) {
      lines.addElem(std::move(elem));
      empty = false;
    }
  }
  if (!infoOnly)
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, "Combat experience: ", Color::YELLOW))
      .addElemAuto(WL(label, toStringRounded(info.combatExperience, 0.01)))
      .buildHorizontalList());
  if (increaseCallback) {
    lines.addSpace(15);
    lines.addElem(WL(centerHoriz,
        WL(buttonLabel, "Dismiss", [increaseCallback] { increaseCallback(none); })));
  }
  if (!empty)
    return lines.buildVerticalList();
  else
    return nullptr;
}

SGuiElem GuiBuilder::drawPlayerInventory(const PlayerInfo& info) {
  auto list = WL(getListBuilder, legendLineHeight);
  list.addSpace();
  auto titleLine = WL(getListBuilder);
  list.addElem(WL(renderInBounds, drawTitleButton(info)));
  if (auto killsLabel = drawKillsLabel(info))
    list.addElem(std::move(killsLabel));
  vector<SGuiElem> keyElems;
  bool isTutorial = false;
  for (int i : All(info.commands)) {
    if (info.commands[i].tutorialHighlight)
      isTutorial = true;
    if (info.commands[i].active)
      if (auto key = info.commands[i].keybinding)
        keyElems.push_back(WL(keyHandlerChar, getButtonCallback({UserInputId::PLAYER_COMMAND, i}), *key));
  }
  if (!info.commands.empty())
    list.addElem(WL(stack,
        WL(stack, std::move(keyElems)),
        WL(conditional, WL(tutorialHighlight), [=]{ return isTutorial;}),
        WL(buttonLabel, "Commands",
          WL(buttonRect, [this, commands = info.commands] (Rectangle bounds) {
              auto lines = WL(getListBuilder, legendLineHeight);
              bool exit = false;
              for (int i : All(commands)) {
                auto& command = commands[i];
                function<void()> buttonFun = [] {};
                if (command.active)
                  buttonFun = [&exit, i, this] {
                      callbacks.input({UserInputId::PLAYER_COMMAND, i});
                      exit = true;
                  };
                auto labelColor = command.active ? Color::WHITE : Color::GRAY;
                auto button = command.keybinding ? WL(buttonChar, buttonFun, *command.keybinding) : WL(button, buttonFun);
                if (command.tutorialHighlight)
                  button = WL(stack, WL(tutorialHighlight), std::move(button));
                lines.addElem(WL(stack,
                    button,
                    command.active ? WL(uiHighlightMouseOver, Color::GREEN) : WL(empty),
                    WL(tooltip, {command.description}),
                    WL(label, (command.keybinding ? getKeybindingDesc(*command.keybinding) + " " : ""_s) +
                        command.name, labelColor)));
              }
              drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 260, false);
         }))));
  for (auto& elem : drawEffectsList(info))
    list.addElem(std::move(elem));
  list.addSpace();
  for (auto& elem : drawSkillsList(info))
    list.addElem(std::move(elem));
  if (auto spells = drawSpellsList(info.spells, info.creatureId.getGenericId(), true)) {
    list.addElemAuto(std::move(spells));
    list.addSpace();
  }
  if (info.debt > 0) {
    list.addElem(WL(label, "Debt", Color::YELLOW));
    list.addElem(WL(label, "Click on debt or on individual items to pay.", Renderer::smallTextSize,
        Color::LIGHT_GRAY), legendLineHeight * 2 / 3);
    list.addElem(WL(stack,
        drawCost({ViewId("gold"), info.debt}),
        WL(button, getButtonCallback(UserInputId::PAY_DEBT))));
    list.addSpace();
  }
  if (!info.inventory.empty()) {
    list.addElem(WL(label, "Inventory", Color::YELLOW));
    for (auto& item : info.inventory)
      list.addElem(getItemLine(item, [=](Rectangle butBounds) {
            if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), false))
              callbacks.input({UserInputId::INVENTORY_ITEM, InventoryItemInfo{item.ids, *choice}});}));
    double totWeight = 0;
    for (auto& item : info.inventory)
      totWeight += item.weight.value_or(0) * item.number;
    list.addElem(WL(label, "Total weight: " + getWeightString(totWeight)));
    list.addElem(WL(label, "Capacity: " +  (info.carryLimit ? getWeightString(*info.carryLimit) : "infinite"_s)));
    list.addSpace();
  }
  if (!info.intrinsicAttacks.empty()) {
    list.addElem(WL(label, "Intrinsic attacks", Color::YELLOW));
    for (auto& item : info.intrinsicAttacks)
      list.addElem(getItemLine(item, [=](Rectangle butBounds) {
            if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), false))
              callbacks.input({UserInputId::INTRINSIC_ATTACK, InventoryItemInfo{item.ids, *choice}});}));
    list.addSpace();
  }
  if (!info.avatarLevelInfo)
    if (auto elem = drawTrainingInfo(info.experienceInfo))
      list.addElemAuto(std::move(elem));
  return list.buildVerticalList();
}

static const char* getControlModeName(PlayerInfo::ControlMode m) {
  switch (m) {
    case PlayerInfo::FULL: return "full";
    case PlayerInfo::LEADER: return "leader";
  }
}


SGuiElem GuiBuilder::drawRightPlayerInfo(const PlayerInfo& info) {
  if (!info.controlMode)
    return WL(margins, WL(scrollable, drawPlayerInventory(info), &inventoryScroll, &scrollbarsHeld), 6, 0, 15, 5);
  if (highlightedTeamMember && *highlightedTeamMember >= info.teamInfos.size())
    highlightedTeamMember = none;
  auto getIconHighlight = [&] (Color c) { return WL(topMargin, -1, WL(uiHighlight, c)); };
  auto vList = WL(getListBuilder, legendLineHeight);
  auto teamList = WL(getListBuilder);
  for (int i : All(info.teamInfos)) {
    auto& member = info.teamInfos[i];
    auto icon = WL(margins, WL(viewObject, member.viewId, 2), 1);
    if (member.creatureId != info.creatureId)
      icon = WL(stack,
          WL(mouseHighlight2, getIconHighlight(Color::GREEN)),
          WL(mouseOverAction, [this, i] { highlightedTeamMember = i;},
              [this, i] { if (highlightedTeamMember == i) highlightedTeamMember = none; }),
          std::move(icon)
      );
    if (info.teamInfos.size() > 1) {
      if (member.creatureId == info.creatureId)
        icon = WL(stack,
            std::move(icon),
            WL(translate, WL(translucentBackgroundPassMouse, WL(translate, WL(labelUnicode, u8"⬆"), Vec2(2, -1))),
                Vec2(16, 50), Vec2(17, 21))
        );
      if (member.isPlayerControlled)
        icon = WL(stack,
            WL(margins, WL(rectangle, Color::GREEN.transparency(1094)), 2),
            std::move(icon)
        );
    }
    if (!member.teamMemberActions.empty()) {
      icon = WL(stack, std::move(icon),
        WL(buttonRect, [memberId = member.creatureId, actions = member.teamMemberActions, this] (Rectangle bounds) {
              auto lines = WL(getListBuilder, legendLineHeight);
              bool exit = false;
              optional<TeamMemberAction> ret;
              for (auto action : actions) {
                auto buttonFun = [&exit, &ret, action] {
                  ret = action;
                  exit = true;
                };
                lines.addElem(WL(stack,
                      WL(uiHighlightMouseOver),
                      WL(button, buttonFun),
                      WL(label, getText(getText(action)))));
              }
              drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 200, false);
              if (ret)
                callbacks.input({UserInputId::TEAM_MEMBER_ACTION, TeamMemberActionInfo{*ret, memberId}});
        }));
    }
    teamList.addElemAuto(std::move(icon));
    if (teamList.getLength() >= 5) {
      vList.addElemAuto(WL(margins, teamList.buildHorizontalList(), 0, 25, 0, 0));
      teamList.clear();
    }
  }
  if (!teamList.isEmpty())
    vList.addElemAuto(WL(margins, teamList.buildHorizontalList(), 0, 25, 0, 3));
  vList.addSpace(22);
  if (info.teamInfos.size() > 1) {
    if (info.teamOrders) {
      auto orderList = WL(getListBuilder);
      for (auto order : ENUM_ALL(TeamOrder)) {
        auto switchFun = getButtonCallback({UserInputId::TOGGLE_TEAM_ORDER, order});
        orderList.addElemAuto(WL(stack,
            info.teamOrders->contains(order)
                ? WL(buttonLabelSelected, getName(order), switchFun)
                : WL(buttonLabel, getName(order), switchFun),
            getTooltip({getDescription(order)}, THIS_LINE)
        ));
        orderList.addSpace(15);
      }
      vList.addElem(orderList.buildHorizontalList());
      vList.addSpace(legendLineHeight / 2);
    }
    vList.addElem(WL(buttonLabel, "Control mode: "_s + getControlModeName(*info.controlMode) + "",
        WL(button, getButtonCallback(UserInputId::TOGGLE_CONTROL_MODE), gui.getKey(SDL::SDLK_g))));
    vList.addSpace(legendLineHeight / 2);
  }
  vList.addElem(WL(buttonLabel, "Exit control mode",
        WL(button, getButtonCallback(UserInputId::EXIT_CONTROL_MODE), gui.getKey(SDL::SDLK_u))));
  vList.addSpace(10);
  vList.addElem(WL(margins, WL(sprite, GuiFactory::TexId::HORI_LINE, GuiFactory::Alignment::TOP), -6, 0, -6, 0), 10);
  vector<SGuiElem> others;
  for (int i : All(info.teamInfos)) {
    auto& elem = info.teamInfos[i];
    if (elem.creatureId != info.creatureId)
      others.push_back(WL(conditional, drawPlayerInventory(elem), [this, i]{ return highlightedTeamMember == i;}));
    else
      others.push_back(WL(conditional, drawPlayerInventory(info),
          [this, i]{ return !highlightedTeamMember || highlightedTeamMember == i;}));
  }
  vList.addElemAuto(WL(stack, std::move(others)));
  return WL(margins, WL(scrollable, vList.buildVerticalList(), &inventoryScroll, &scrollbarsHeld), 6, 0, 15, 5);
}

typedef CreatureInfo CreatureInfo;

struct CreatureMapElem {
  ViewId viewId;
  int count;
  CreatureInfo any;
};

SGuiElem GuiBuilder::drawMinionAndLevel(ViewIdList viewId, int level, int iconMult) {
  return WL(stack, makeVec(
        WL(viewObject, viewId, iconMult),
        WL(label, toString(level), 12 * iconMult)));
}

SGuiElem GuiBuilder::drawTeams(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  const int elemWidth = 30;
  auto lines = WL(getListBuilder, legendLineHeight);
  const char* hint = "Drag and drop minions onto the [new team] button to create a new team. "
    "You can drag them both from the map and the menus.";
  if (!tutorial || info.teams.empty()) {
    const bool isTutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::NEW_TEAM);
    lines.addElem(WL(stack, makeVec(
          WL(dragListener, [this](DragContent content) {
              content.visit<void>(
                  [&](UniqueEntity<Creature>::Id id) {
                    callbacks.input({UserInputId::CREATE_TEAM, id});
                  },
                  [&](const string& group) {
                    callbacks.input({UserInputId::CREATE_TEAM_FROM_GROUP, group});
                  },
                  [&](TeamId) { }
              );
          ;}),
          WL(conditional, WL(uiHighlightMouseOver), [&]{return gui.getDragContainer().hasElement();} ),
          WL(conditional, WL(tutorialHighlight), [yes = isTutorialHighlight && info.teams.empty()]{ return yes; }),
          getHintCallback({hint}),
          WL(button, [this, hint] { callbacks.info(hint); }),
          WL(label, "[new team]", Color::WHITE))));
  }
  for (int i : All(info.teams)) {
    auto& team = info.teams[i];
    const int numPerLine = 7;
    auto teamLine = WL(getListBuilder, legendLineHeight);
    vector<SGuiElem> currentLine;
    for (auto member : team.members) {
      auto& memberInfo = *info.getMinion(member);
      currentLine.push_back(drawMinionAndLevel(memberInfo.viewId, (int) memberInfo.bestAttack.value, 1));
      if (currentLine.size() >= numPerLine)
        teamLine.addElem(WL(horizontalList, std::move(currentLine), elemWidth));
    }
    if (!currentLine.empty())
      teamLine.addElem(WL(horizontalList, std::move(currentLine), elemWidth));
    auto leaderViewId = info.getMinion(team.members[0])->viewId;
    auto selectButton = [this](int teamId) {
      return WL(releaseLeftButton, [=]() {
          onTutorialClicked(0, TutorialHighlight::CONTROL_TEAM);
          callbacks.input({UserInputId::SELECT_TEAM, teamId});
      });
    };
    const bool isTutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
    lines.addElemAuto(WL(stack, makeVec(
            WL(conditional, WL(tutorialHighlight),
                [=]{ return !wasTutorialClicked(0, TutorialHighlight::CONTROL_TEAM) && isTutorialHighlight; }),
            WL(uiHighlightConditional, [team] () { return team.highlight; }),
            WL(uiHighlightMouseOver),
            WL(mouseOverAction, [team, this] { mapGui->highlightTeam(team.members); },
                [team, this] { mapGui->unhighlightTeam(team.members); }),
            cache->get(selectButton, THIS_LINE, team.id),
            WL(dragListener, [this, team](DragContent content) {
                content.visit<void>(
                    [&](UniqueEntity<Creature>::Id id) {
                      callbacks.input({UserInputId::ADD_TO_TEAM, TeamCreatureInfo{team.id, id}});
                    },
                    [&](const string& group) {
                      callbacks.input({UserInputId::ADD_GROUP_TO_TEAM, TeamGroupInfo{team.id, group}});
                    },
                    [&](TeamId) { }
                );
            }),
            WL(dragSource, team.id, [=] { return WL(viewObject, leaderViewId);}),
            WL(getListBuilder, 22)
              .addElem(WL(topMargin, 8, WL(icon, GuiFactory::TEAM_BUTTON, GuiFactory::Alignment::TOP_CENTER)))
              .addElemAuto(teamLine.buildVerticalList()).buildHorizontalList())));
  }
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawMinions(CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  int newHash = info.getHash();
  if (newHash != minionsHash) {
    minionsHash = newHash;
    auto list = WL(getListBuilder, legendLineHeight);
    list.addElem(WL(label, info.monsterHeader, Color::WHITE));
    auto selectButton = [this](const string& group) {
      return WL(releaseLeftButton, getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, group}));
    };
    auto addGroup = [&] (const CollectiveInfo::CreatureGroup& elem) {
      auto line = WL(getListBuilder);
      line.addElem(WL(viewObject, elem.viewId), 40);
      SGuiElem tmp = WL(label, toString(elem.count) + "   " + elem.name, Color::WHITE);
      line.addElem(WL(renderInBounds, std::move(tmp)), 200);
      list.addElem(WL(stack, makeVec(
          cache->get(selectButton, THIS_LINE, elem.name),
          WL(dragSource, elem.name,
              [=]{ return WL(getListBuilder, 10)
                  .addElemAuto(WL(label, toString(elem.count) + " "))
                  .addElem(WL(viewObject, elem.viewId)).buildHorizontalList();}),
          WL(button, [this, group = elem.name, viewId = elem.viewId, id = elem.creatureId] {
              callbacks.input(UserInput(UserInputId::CREATURE_DRAG, id));
              gui.getDragContainer().put(group, WL(viewObject, viewId), Vec2(-100, -100));
          }),
          WL(uiHighlightConditional, [highlight = elem.highlight]{return highlight;}),
          line.buildHorizontalList()
       )));
    };
    for (auto& group : info.minionGroups)
      addGroup(group);
    if (!info.automatonGroups.empty()) {
      list.addElem(WL(label, "Automatons by ability: ", Color::WHITE));
      for (auto& group : info.automatonGroups)
        addGroup(group);
    }
    list.addElem(WL(label, "Teams: ", Color::WHITE));
    list.addElemAuto(drawTeams(info, tutorial));
    list.addSpace();
    list.addElem(WL(stack,
              WL(label, "Show tasks", [=]{ return bottomWindow == TASKS ? Color::GREEN : Color::WHITE;}),
              WL(button, [this] { toggleBottomWindow(TASKS); })));
    list.addElem(WL(stack,
              WL(label, "Show message history"),
              WL(button, getButtonCallback(UserInputId::SHOW_HISTORY))));
    list.addSpace();
    if (!info.onGoingAttacks.empty()) {
      list.addElem(WL(label, "Ongoing attacks:", Color::WHITE));
      for (auto& elem : info.onGoingAttacks)
        list.addElem(WL(stack,
            WL(button, getButtonCallback({UserInputId::GO_TO_ENEMY, elem.creatureId})),
            WL(uiHighlightMouseOver),
            WL(getListBuilder)
                .addElem(WL(viewObject, elem.viewId), 30)
                .addElemAuto(WL(label, elem.attacker))
                .buildHorizontalList()));
    }
    minionsCache = WL(scrollable, list.buildVerticalList(), &minionsScroll, &scrollbarsHeld);
  }
  return WL(external, minionsCache.get());
}

const int taskMapWindowWidth = 400;

SGuiElem GuiBuilder::drawTasksOverlay(const CollectiveInfo& info) {
  vector<SGuiElem> lines;
  vector<SGuiElem> freeLines;
  for (auto& elem : info.taskMap) {
    if (elem.creature)
      if (auto minion = info.getMinion(*elem.creature)) {
        lines.push_back(WL(horizontalList, makeVec(
                WL(viewObject, minion->viewId),
                WL(label, elem.name, elem.priority ? Color::GREEN : Color::WHITE)), 35));
        continue;
      }
    freeLines.push_back(WL(horizontalList, makeVec(
            WL(empty),
            WL(label, elem.name, elem.priority ? Color::GREEN : Color::WHITE)), 35));
  }
  if (info.taskMap.empty())
    lines.push_back(WL(label, "No tasks present."));
  int lineHeight = 25;
  int margin = 20;
  append(lines, std::move(freeLines));
  int numLines = lines.size();
  return WL(preferredSize, taskMapWindowWidth, numLines * lineHeight + 2 * margin + 10,
      WL(stack,
          WL(keyHandler, [this]{bottomWindow = none;}, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          WL(miniWindow,
        WL(margins, WL(scrollable, WL(verticalList, std::move(lines), lineHeight), &tasksScroll, &scrollbarsHeld),
          margin))));
}

SGuiElem GuiBuilder::drawRansomOverlay(const CollectiveInfo::Ransom& ransom) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, ransom.attacker + " demand " + toString(ransom.amount.second)
        + " gold for not attacking. Agree?"));
  if (ransom.canAfford)
    lines.addElem(WL(leftMargin, 25, WL(stack,
          WL(mouseHighlight2, WL(highlight, legendLineHeight)),
          WL(button, getButtonCallback(UserInputId::PAY_RANSOM)),
          WL(label, "Yes"))));
  else
    lines.addElem(WL(leftMargin, 25, WL(label, "Yes", Color::GRAY)));
  lines.addElem(WL(leftMargin, 25, WL(stack,
        WL(mouseHighlight2, WL(highlight, legendLineHeight)),
        WL(button, getButtonCallback(UserInputId::IGNORE_RANSOM)),
        WL(label, "No"))));
  return WL(setWidth, 600, WL(miniWindow, WL(margins, lines.buildVerticalList(), 20)));
}

SGuiElem GuiBuilder::drawKeeperDangerOverlay(const string& message) {
  auto lines = WL(getListBuilder, legendLineHeight);
  int width = 600;
  lines.addElemAuto(gui.labelMultiLineWidth(message, legendLineHeight, width));
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(buttonLabel, "Take control", getButtonCallback(UserInputId::CONTROL_KEEPER)))
      .addSpace(20)
      .addElemAuto(WL(buttonLabel, "Dismiss for 200 turns", getButtonCallback(UserInputId::DISMISS_KEEPER_DANGER)))
      .buildHorizontalList()
  );
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(getListBuilder)
        .addElemAuto(WL(label, "When the Keeper is in danger: "))
        .addElemAuto(drawBoolOptionElem(OptionId::KEEPER_WARNING, "show this pop-up, "))
        .addElemAuto(drawBoolOptionElem(OptionId::KEEPER_WARNING_PAUSE, "pause the game"))
        .buildHorizontalList());
  int margins = 20;
  return WL(setWidth, width + margins, WL(miniWindow, WL(margins, lines.buildVerticalList(), margins)));
}

SGuiElem GuiBuilder::drawRebellionChanceText(CollectiveInfo::RebellionChance chance) {
  switch (chance) {
    case CollectiveInfo::RebellionChance::HIGH:
      return WL(label, "high", Color::RED);
    case CollectiveInfo::RebellionChance::MEDIUM:
      return WL(label, "medium", Color::ORANGE);
    case CollectiveInfo::RebellionChance::LOW:
      return WL(label, "low", Color::YELLOW);
  }
}

SGuiElem GuiBuilder::drawWarningWindow(const CollectiveInfo::RebellionChance& rebellionChance) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, "Chance of prisoner escape: "))
      .addElemAuto(drawRebellionChanceText(rebellionChance))
      .buildHorizontalList());
  lines.addElem(WL(label, "Remove prisoners or increase armed forces."));
  return WL(setWidth, 400, WL(translucentBackgroundWithBorder, WL(stack,
      WL(margins, lines.buildVerticalList(), 10),
      WL(alignment, GuiFactory::Alignment::TOP_RIGHT, WL(preferredSize, 40, 40, WL(stack,
          WL(leftMargin, 22, WL(label, "x")),
          WL(button, getButtonCallback(UserInputId::DISMISS_WARNING_WINDOW)))))
    )));
}

SGuiElem GuiBuilder::drawNextWaveOverlay(const CollectiveInfo::NextWave& wave) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, "Next enemy wave:"));
  lines.addElem(WL(getListBuilder)
        .addElem(WL(viewObject, wave.viewId), 30)
        .addElemAuto(WL(label, wave.attacker))// + " (" + toString(wave->count) + ")"))
        .buildHorizontalList());
  lines.addElem(WL(label, "Attacking in " + toString(wave.numTurns) + " turns."));
  return WL(setWidth, 300, WL(translucentBackgroundWithBorder, WL(stack,
        WL(margins, lines.buildVerticalList(), 10),
        WL(alignment, GuiFactory::Alignment::TOP_RIGHT, WL(preferredSize, 40, 40, WL(stack,
            WL(leftMargin, 22, WL(label, "x")),
            WL(button, getButtonCallback(UserInputId::DISMISS_NEXT_WAVE)))))
      )));
}

SGuiElem GuiBuilder::drawItemUpgradeButton(const CollectiveInfo::QueuedItemInfo& elem) {
  auto buttonHandler = WL(buttonRect, [=] (Rectangle bounds) {
      auto lines = WL(getListBuilder, legendLineHeight);
      disableTooltip = true;
      DestructorFunction dFun([this] { disableTooltip = false; });
      bool exit = false;
      optional<WorkshopUpgradeInfo> ret;
      for (int i : All(elem.added)) {
        auto buttonFun = [&exit, &ret, i, itemIndex = elem.itemIndex] {
            ret = WorkshopUpgradeInfo{ itemIndex,  i, true};
            exit = true;
        };
        auto& upgrade = elem.added[i];
        auto idLine = WL(getListBuilder);
        idLine.addElemAuto(WL(label, "Remove "));
        idLine.addElemAuto(WL(viewObject, upgrade.viewId));
        idLine.addElemAuto(WL(label, upgrade.name));
        lines.addElem(WL(stack,
              WL(button, buttonFun),
              WL(uiHighlightMouseOver),
              idLine.buildHorizontalList(),
              WL(tooltip, {upgrade.description})
        ));
      }
      if (elem.added.size() < elem.maxUpgrades)
        for (int i : All(elem.available)) {
          auto buttonFun = [&exit, &ret, i, itemIndex = elem.itemIndex] {
              ret = WorkshopUpgradeInfo{ itemIndex,  i, false};
              exit = true;
          };
          auto& upgrade = elem.available[i];
          auto idLine = WL(getListBuilder);
          idLine.addElemAuto(WL(label, "Add "));
          idLine.addElemAuto(WL(viewObject, upgrade.viewId));
          idLine.addElemAuto(WL(label, upgrade.name + " (" + toString(upgrade.count) + " available)"));
          lines.addElem(WL(stack,
                WL(button, buttonFun),
                WL(uiHighlightMouseOver),
                idLine.buildHorizontalList(),
                WL(tooltip, {upgrade.description})
          ));
        }
      lines.addElem(WL(label, "Available slots: " + toString(elem.maxUpgrades - elem.added.size())));
      lines.addElem(WL(label, "Upgraded items can only be crafted by a craftsman of legendary skills.",
          Renderer::smallTextSize, Color::LIGHT_GRAY));
      drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 450, false);
      if (ret)
        callbacks.input({UserInputId::WORKSHOP_UPGRADE, *ret});
  });
  auto line = WL(getListBuilder);
  for (int upgradeIndex : All(elem.added)) {
    auto& upgrade = elem.added[upgradeIndex];
    line.addElemAuto(WL(viewObject, upgrade.viewId));
  }
  if (!elem.added.empty())
    return WL(standardButton, line.buildHorizontalList(), std::move(buttonHandler));
  else
    return WL(buttonLabel, "upgrade", std::move(buttonHandler));
}

SGuiElem GuiBuilder::drawWorkshopsOverlay(const CollectiveInfo::ChosenWorkshopInfo& info,
    const optional<TutorialInfo>& tutorial) {
  int margin = 20;
  int rightElemMargin = 10;
  auto& options = info.options;
  auto& queued = info.queued;
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, "Available:", Color::YELLOW));
  for (int itemIndex : All(options)) {
    auto& elem = options[itemIndex];
    if (elem.hidden)
      continue;
    auto line = WL(getListBuilder);
    line.addElem(WL(viewObject, elem.viewId), 35);
    auto label = WL(label, elem.name, elem.unavailable ? Color::GRAY : Color::WHITE);
    if (elem.ingredient)
      label = WL(getListBuilder)
          .addElemAuto(std::move(label))
          .addElemAuto(WL(label, " from "))
          .addElemAuto(WL(viewObject, elem.ingredient->viewId))
          .buildHorizontalList();
    line.addMiddleElem(WL(renderInBounds, std::move(label)));
    if (elem.price)
      line.addBackElem(WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(*elem.price)), 80);
    SGuiElem guiElem = line.buildHorizontalList();
    if (elem.tutorialHighlight)
      guiElem = WL(stack, WL(tutorialHighlight), std::move(guiElem));
    if (elem.unavailable) {
      CHECK(!elem.unavailableReason.empty());
      guiElem = WL(stack, getTooltip(concat({elem.unavailableReason}, elem.description), THIS_LINE + itemIndex), std::move(guiElem));
    }
    else
      guiElem = WL(stack,
          WL(uiHighlightMouseOver),
          std::move(guiElem),
          WL(button, getButtonCallback({UserInputId::WORKSHOP_ADD, itemIndex})),
          getTooltip({elem.description}, THIS_LINE + itemIndex)
      );
    lines.addElem(WL(rightMargin, rightElemMargin, std::move(guiElem)));
  }
  auto lines2 = WL(getListBuilder, legendLineHeight);
  lines2.addElem(WL(label, "In production:", Color::YELLOW));
  for (int i : All(queued)) {
    auto& elem = queued[i];
    auto line = WL(getListBuilder);
    auto color = elem.paid ? Color::WHITE : Color::RED;
    auto label = WL(label, elem.itemInfo.name, color);
    if (elem.itemInfo.ingredient)
      label = WL(getListBuilder)
          .addElemAuto(std::move(label))
          .addElemAuto(WL(label, " from ", color))
          .addElemAuto(WL(viewObject, elem.itemInfo.ingredient->viewId))
          .buildHorizontalList();
    line.addMiddleElem(WL(stack,
        WL(button, getButtonCallback({UserInputId::REMOVE_WORKSHOP_ITEM, elem.itemIndex})),
        WL(uiHighlightMouseOver),
        WL(getListBuilder)
            .addElem(WL(viewObject, elem.itemInfo.viewId), 35)
            .addMiddleElem(WL(renderInBounds, std::move(label)))
            .buildHorizontalList()
    ));
    if ((!elem.available.empty() || !elem.added.empty()) && elem.maxUpgrades > 0) {
      line.addBackElemAuto(WL(leftMargin, 7, drawItemUpgradeButton(elem)));
    }
    if (elem.itemInfo.price)
      line.addBackElem(WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(*elem.itemInfo.price)), 80);
    lines2.addElem(WL(stack,
        WL(bottomMargin, 5,
            WL(progressBar, Color::DARK_GREEN.transparency(128), elem.productionState)),
        WL(rightMargin, rightElemMargin, line.buildHorizontalList()),
        getTooltip(elem.itemInfo.description, THIS_LINE + i)
    ));
  }
  return WL(preferredSize, 940, 600,
    WL(miniWindow, WL(stack,
      WL(keyHandler, getButtonCallback({UserInputId::WORKSHOP, info.index}),
        {gui.getKey(SDL::SDLK_ESCAPE)}, true),
      WL(getListBuilder, 470)
            .addElem(WL(margins, WL(scrollable,
                  lines.buildVerticalList(), &workshopsScroll, &scrollbarsHeld), margin))
            .addElem(WL(margins,
                WL(scrollable, lines2.buildVerticalList(), &workshopsScroll2, &scrollbarsHeld2),
                margin)).buildHorizontalList())));
}

SGuiElem GuiBuilder::drawCreatureUpgradeMenu(SyncQueue<optional<ExperienceType>>& queue,
    const CreatureExperienceInfo& info) {
  const int margin = 20;
  auto lines = WL(getListBuilder, legendLineHeight);
  if (auto elem = drawTrainingInfo(info, [&queue](optional<ExperienceType> exp) { queue.push(exp); }))
    lines.addElemAuto(std::move(elem));
  else
    return nullptr;
  return WL(preferredSize, 500, 320,
      WL(window, WL(margins, lines.buildVerticalList(), margin), [&queue] { queue.push(none); }));
}

static string getName(TechId id) {
  return id.data();
}

SGuiElem GuiBuilder::drawTechUnlocks(const CollectiveInfo::LibraryInfo::TechInfo& tech) {
  auto lines = WL(getListBuilder, legendLineHeight);
  const int width = 450;
  const int margin = 15;
  lines.addElemAuto(WL(labelMultiLineWidth, tech.description, legendLineHeight * 2 / 3, width - 2 * margin));
  lines.addSpace(legendLineHeight);
  for (int i : All(tech.unlocks)) {
    auto& info = tech.unlocks[i];
    if (i == 0 || info.type != tech.unlocks[i - 1].type) {
      if (i > 0)
        lines.addSpace(legendLineHeight / 2);
      lines.addElem(WL(label, "Unlocks " + info.type + ":"));
    }
    lines.addElem(WL(getListBuilder)
        .addElemAuto(WL(viewObject, info.viewId))
        .addSpace(5)
        .addElemAuto(WL(label, capitalFirst(info.name)))
        .buildHorizontalList());
  }
  return WL(setWidth, width, WL(miniWindow, WL(margins, lines.buildVerticalList(), margin)));
}

SGuiElem GuiBuilder::drawLibraryContent(const CollectiveInfo& collectiveInfo, const optional<TutorialInfo>& tutorial) {
  auto& info = collectiveInfo.libraryInfo;
  const int rightElemMargin = 10;
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addSpace(5);
  lines.addElem(WL(renderInBounds, WL(getListBuilder)
      .addElemAuto(WL(topMargin, -2, WL(viewObject, collectiveInfo.avatarLevelInfo.viewId)))
      .addSpace(4)
      .addElemAuto(WL(label, toString(collectiveInfo.avatarLevelInfo.title)))
      .buildHorizontalList()));
  lines.addElem(WL(label, "Level " + toString(collectiveInfo.avatarLevelInfo.level)));
  lines.addElem(WL(stack,
      WL(margins, WL(progressBar, Color::DARK_GREEN.transparency(128), collectiveInfo.avatarLevelInfo.progress), -4, 0, -1, 6),
      WL(label, "Next level progress: " + toString(info.currentProgress) + "/" + toString(info.totalProgress))
  ));
  //lines.addElem(WL(rightMargin, rightElemMargin, WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(info.resource))));
  if (info.warning)
    lines.addElem(WL(label, *info.warning, Renderer::smallTextSize, Color::RED));
  auto emptyElem = WL(empty);
  auto getUnlocksTooltip = [&] (auto& elem) {
    return WL(tooltip2, drawTechUnlocks(elem),
        [=](const Rectangle&) { return emptyElem->getBounds().topRight() + Vec2(35, 0); });
  };
  if (!info.available.empty()) {
    lines.addElem(WL(label, "Research:", Color::YELLOW));
    lines.addElem(WL(label, "(" + getPlural("item", collectiveInfo.avatarLevelInfo.numAvailable) + " available)", Color::YELLOW));
    for (int i : All(info.available)) {
      auto& elem = info.available[i];
      auto line = WL(renderInBounds, WL(label, capitalFirst(getName(elem.id)), elem.active ? Color::WHITE : Color::GRAY));
      line = WL(stack, std::move(line), getUnlocksTooltip(elem));
      if (elem.tutorialHighlight && tutorial && tutorial->highlights.contains(*elem.tutorialHighlight))
        line = WL(stack, WL(tutorialHighlight), std::move(line));
      if (i == 0)
        line = WL(stack, std::move(line), emptyElem);
      if (elem.active)
        line = WL(stack,
            WL(uiHighlightMouseOver, Color::GREEN),
            std::move(line),
            WL(button, getButtonCallback({UserInputId::LIBRARY_ADD, elem.id})));
      lines.addElem(WL(rightMargin, rightElemMargin, std::move(line)));
    }
    lines.addSpace(legendLineHeight * 2 / 3);
  }
  if (!info.researched.empty())
  lines.addElem(WL(label, "Already researched:", Color::YELLOW));
  for (int i : All(info.researched)) {
    auto& elem = info.researched[i];
    auto line = WL(renderInBounds, WL(label, capitalFirst(getName(elem.id)), Color::GRAY));
    line = WL(stack, std::move(line), getUnlocksTooltip(elem));
    lines.addElem(WL(rightMargin, rightElemMargin, std::move(line)));
  }
  const int margin = 0;
  return WL(margins, WL(scrollable, lines.buildVerticalList(), &libraryScroll, &scrollbarsHeld), margin);
}

SGuiElem GuiBuilder::drawMinionsOverlay(const CollectiveInfo::ChosenCreatureInfo& chosenCreature,
    const vector<ViewId>& allQuarters, const optional<TutorialInfo>& tutorial) {
  int margin = 20;
  int minionListWidth = 220;
  setCollectiveTab(CollectiveTab::MINIONS);
  SGuiElem minionPage;
  auto& minions = chosenCreature.creatures;
  auto current = chosenCreature.chosenId;
  for (int i : All(minions))
    if (minions[i].creatureId == current)
      minionPage = WL(margins, drawMinionPage(minions[i], allQuarters, tutorial), 10, 15, 10, 10);
  if (!minionPage)
    return WL(empty);
  SGuiElem menu;
  SGuiElem leftSide = drawMinionButtons(minions, current, chosenCreature.teamId);
  if (chosenCreature.teamId) {
    auto list = WL(getListBuilder, legendLineHeight);
    list.addElem(
        WL(buttonLabel, "Disband team", getButtonCallback({UserInputId::CANCEL_TEAM, *chosenCreature.teamId})));
    list.addElem(WL(label, "Control a chosen minion to", Renderer::smallTextSize,
          Color::LIGHT_GRAY), Renderer::smallTextSize + 2);
    list.addElem(WL(label, "command the team.", Renderer::smallTextSize,
          Color::LIGHT_GRAY));
    list.addElem(WL(empty), legendLineHeight);
    leftSide = WL(marginAuto, list.buildVerticalList(), std::move(leftSide), GuiFactory::TOP);
  }
  menu = WL(stack,
      WL(horizontalList, makeVec(
          WL(margins, std::move(leftSide), 8, 15, 5, 0),
          WL(margins, WL(sprite, GuiFactory::TexId::VERT_BAR_MINI, GuiFactory::Alignment::LEFT),
            0, -15, 0, -15)), minionListWidth),
      WL(leftMargin, minionListWidth + 20, std::move(minionPage)));
  return WL(preferredSize, 640 + minionListWidth, 600,
      WL(miniWindow, WL(stack,
      WL(keyHandler, getButtonCallback({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()}),
        {gui.getKey(SDL::SDLK_ESCAPE)}, true),
      WL(margins, std::move(menu), margin))));
}

SGuiElem GuiBuilder::drawBestiaryPage(const PlayerInfo& minion) {
  auto list = WL(getListBuilder, legendLineHeight);
  list.addElem(WL(getListBuilder)
      .addElemAuto(WL(viewObject, minion.viewId))
      .addSpace(5)
      .addElemAuto(WL(label, minion.name))
      .buildHorizontalList());
  if (!minion.description.empty())
    list.addElem(WL(label, minion.description, Renderer::smallTextSize, Color::LIGHT_GRAY));
  auto leftLines = WL(getListBuilder, legendLineHeight);
  leftLines.addElem(WL(label, "Attributes", Color::YELLOW));
  leftLines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(minion.attributes)));
  for (auto& elem : drawEffectsList(minion))
    leftLines.addElem(std::move(elem));
  leftLines.addSpace();
  if (auto elem = drawTrainingInfo(minion.experienceInfo, nullptr, true))
    leftLines.addElemAuto(std::move(elem));
  if (!minion.spellSchools.empty()) {
    auto line = WL(getListBuilder)
        .addElemAuto(WL(label, "Spell schools: ", Color::YELLOW));
    for (auto& school : minion.spellSchools)
      line.addElemAuto(drawSpellSchoolLabel(school));
    leftLines.addElem(line.buildHorizontalList());
  }
  leftLines.addSpace();
  for (auto& elem : drawSkillsList(minion))
    leftLines.addElem(std::move(elem));
  if (auto spells = drawSpellsList(minion.spells, minion.creatureId.getGenericId(), false))
    leftLines.addElemAuto(std::move(spells));
  int topMargin = list.getSize() + 20;
  return WL(margin, list.buildVerticalList(),
      WL(scrollable, WL(horizontalListFit, makeVec(
          WL(rightMargin, 15, leftLines.buildVerticalList()),
          drawEquipmentAndConsumables(minion, true))), &minionPageScroll, &scrollbarsHeld),
      topMargin, GuiFactory::TOP);
}

SGuiElem GuiBuilder::drawBestiaryButtons(const vector<PlayerInfo>& minions, int index) {
  CHECK(!minions.empty());
  auto list = WL(getListBuilder, legendLineHeight);
  for (int i : All(minions)) {
    auto& minion = minions[i];
    auto viewId = minion.viewId;
    auto line = WL(getListBuilder);
    line.addElemAuto(WL(viewObject, viewId));
    line.addSpace(5);
    line.addMiddleElem(WL(rightMargin, 5, WL(renderInBounds, WL(label, minion.name))));
    line.addBackElem(drawBestAttack(minion.bestAttack), 60);
    list.addElem(WL(stack,
          WL(button, [this, i] { bestiaryIndex = i; }),
          (i == index ? WL(uiHighlightLine) : WL(empty)),
          line.buildHorizontalList()));
  }
  return WL(scrollable, gui.margins(list.buildVerticalList(), 0, 5, 10, 0), &minionButtonsScroll, &scrollbarsHeld2);
}

SGuiElem GuiBuilder::drawBestiaryOverlay(const vector<PlayerInfo>& creatures, int index) {
  int margin = 20;
  int minionListWidth = 330;
  SGuiElem menu;
  SGuiElem leftSide = drawBestiaryButtons(creatures, index);
  menu = WL(stack,
      WL(horizontalList, makeVec(
          WL(margins, std::move(leftSide), 8, 15, 5, 0),
          WL(margins, WL(sprite, GuiFactory::TexId::VERT_BAR_MINI, GuiFactory::Alignment::LEFT),
            0, -15, 0, -15)), minionListWidth),
      WL(leftMargin, minionListWidth + 20, WL(margins, drawBestiaryPage(creatures[index]), 10, 15, 10, 10)));
  return WL(preferredSize, 640 + minionListWidth, 600,
      WL(miniWindow, WL(stack,
          WL(keyHandler, [=] { toggleBottomWindow(BottomWindowId::BESTIARY); }, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          WL(margins, std::move(menu), margin))));
}

SGuiElem GuiBuilder::drawSpellSchoolPage(const SpellSchoolInfo& school) {
  auto list = WL(getListBuilder, legendLineHeight);
  list.addElem(WL(label, capitalFirst(school.name)));
  list.addSpace(legendLineHeight / 2);
  for (auto& spell : school.spells)
    list.addElem(drawSpellLabel(spell));
  return WL(scrollable, gui.margins(list.buildVerticalList(), 0, 5, 10, 0), &minionButtonsScroll, &scrollbarsHeld2);
}

SGuiElem GuiBuilder::drawSpellSchoolButtons(const vector<SpellSchoolInfo>& schools, int index) {
  CHECK(!schools.empty());
  auto list = WL(getListBuilder, legendLineHeight);
  for (int i : All(schools)) {
    auto& school = schools[i];
    auto line = WL(getListBuilder);
    line.addMiddleElem(WL(rightMargin, 5, WL(renderInBounds, WL(label, capitalFirst(school.name)))));
    list.addElem(WL(stack,
          WL(button, [this, i] { spellSchoolIndex = i; }),
          (i == index ? WL(uiHighlightLine) : WL(empty)),
          line.buildHorizontalList()));
  }
  return WL(scrollable, gui.margins(list.buildVerticalList(), 0, 5, 10, 0), &minionButtonsScroll, &scrollbarsHeld);
}

SGuiElem GuiBuilder::drawSpellSchoolsOverlay(const vector<SpellSchoolInfo>& schools, int index) {
  int margin = 20;
  int minionListWidth = 230;
  SGuiElem menu;
  SGuiElem leftSide = drawSpellSchoolButtons(schools, index);
  menu = WL(stack,
      WL(horizontalList, makeVec(
          WL(margins, std::move(leftSide), 8, 15, 5, 0),
          WL(margins, WL(sprite, GuiFactory::TexId::VERT_BAR_MINI, GuiFactory::Alignment::LEFT),
            0, -15, 0, -15)), minionListWidth),
      WL(leftMargin, minionListWidth + 20, WL(margins, drawSpellSchoolPage(schools[index]), 10, 15, 10, 10)));
  return WL(preferredSize, 440 + minionListWidth, 600,
      WL(miniWindow, WL(stack,
          WL(keyHandler, [=] { toggleBottomWindow(BottomWindowId::SPELL_SCHOOLS); }, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          WL(margins, std::move(menu), margin))));
}

SGuiElem GuiBuilder::drawItemsHelpButtons(const vector<ItemInfo>& items) {
  CHECK(!items.empty());
  auto list = WL(getListBuilder, legendLineHeight);
  for (int i : All(items)) {
    auto& item = items[i];
    auto line = WL(getListBuilder);
    line.addMiddleElem(WL(rightMargin, 5, WL(renderInBounds, getItemLine(item, [](Rectangle){}))));
    list.addElem(line.buildHorizontalList());
  }
  return WL(scrollable, gui.margins(list.buildVerticalList(), 0, 5, 10, 0), &minionButtonsScroll, &scrollbarsHeld);
}

SGuiElem GuiBuilder::drawItemsHelpOverlay(const vector<ItemInfo>& items) {
  int margin = 20;
  int itemsListWidth = 430;
  SGuiElem menu;
  SGuiElem leftSide = drawItemsHelpButtons(items);
  return WL(preferredSize, itemsListWidth, 600,
      WL(miniWindow, WL(stack,
          WL(keyHandler, [=] { toggleBottomWindow(BottomWindowId::ITEMS_HELP); }, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          WL(margins, std::move(leftSide), margin))));
}

SGuiElem GuiBuilder::drawBuildingsOverlay(const vector<CollectiveInfo::Button>& buildings,
    bool ransom, const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> elems;
  map<string, GuiFactory::ListBuilder> overlaysMap;
  int margin = 20;
  for (int i : All(buildings)) {
    auto& elem = buildings[i];
    if (!elem.groupName.empty()) {
      if (!overlaysMap.count(elem.groupName))
        overlaysMap.emplace(make_pair(elem.groupName, WL(getListBuilder, legendLineHeight)));
      overlaysMap.at(elem.groupName).addElem(getButtonLine(elem, i, CollectiveTab::BUILDINGS, tutorial));
      elems.push_back(WL(setWidth, 350, WL(conditional,
            WL(miniWindow, WL(margins, getButtonLine(elem, i, CollectiveTab::BUILDINGS, tutorial), margin)),
            [i, this] { return getActiveButton(CollectiveTab::BUILDINGS) == i;})));
    }
  }
  for (auto& elem : overlaysMap) {
    auto& lines = elem.second;
    string groupName = elem.first;
    elems.push_back(WL(setWidth, 350, WL(conditionalStopKeys,
          WL(miniWindow, WL(stack,
              WL(keyHandler, [=] { clearActiveButton(); }, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
              WL(margins, lines.buildVerticalList(), margin))),
          [ransom, this, groupName] {
              return !ransom && collectiveTab == CollectiveTab::BUILDINGS && activeGroup == groupName;})));
  }
  return WL(stack, std::move(elems));
}

SGuiElem GuiBuilder::getClickActions(const ViewObject& object) {
  auto lines = WL(getListBuilder, legendLineHeight * 2 / 3);
  if (auto action = object.getClickAction()) {
    lines.addElem(WL(label, getText(*action)));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!object.getExtendedActions().isEmpty()) {
    lines.addElem(WL(label, "Right click:", Color::LIGHT_BLUE));
    for (auto action : object.getExtendedActions())
      lines.addElem(WL(label, getText(action), Color::LIGHT_GRAY));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!lines.isEmpty())
    return lines.buildVerticalList();
  else
    return nullptr;
}

SGuiElem GuiBuilder::drawLyingItemsList(const string& title, const ItemCounts& itemCounts, int maxWidth) {
  auto lines = WL(getListBuilder, legendLineHeight);
  auto line = WL(getListBuilder);
  int currentWidth = 0;
  for (auto& elemPair : itemCounts) {
    auto cnt = elemPair.second;
    auto id = elemPair.first;
    if (line.isEmpty() && lines.isEmpty() && !title.empty()) {
      line.addElemAuto(WL(label, title));
      currentWidth = line.getSize();
    }
    auto elem = cnt > 1
        ? drawMinionAndLevel({id}, cnt, 1)
        : WL(viewObject, id);
    if (currentWidth + *elem->getPreferredWidth() > maxWidth) {
      lines.addElem(line.buildHorizontalList());
      currentWidth = 0;
      line.clear();
    }
    currentWidth += *elem->getPreferredWidth();
    line.addElemAuto(std::move(elem));
  }
  if (line.getLength() > 0)
    lines.addElem(line.buildHorizontalList());
  return lines.buildVerticalList();
}

static string getMoraleNumber(double morale) {
#ifndef RELEASE
  return toString(morale);
#else
  return toString(round(10.0 * morale) / 10);
#endif
}

static const char* getHealthName(bool spirit) {
  return spirit ? "Materialization:" : "Health:";
}

SGuiElem GuiBuilder::drawMapHintOverlay() {
  auto lines = WL(getListBuilder, legendLineHeight);
  vector<SGuiElem> allElems;
  if (!hint.empty()) {
    for (auto& line : hint)
      if (!line.empty())
        lines.addElem(WL(label, line));
  } else {
    auto& highlighted = mapGui->getLastHighlighted();
    auto& index = highlighted.viewIndex;
    auto tryHighlight = [&] (HighlightType type, const char* text, Color color) {
      if (index.isHighlight(type)) {
        lines.addElem(WL(getListBuilder)
              .addElem(WL(viewObject, ViewId("magic_field", color)), 30)
              .addElemAuto(WL(label, text))
              .buildHorizontalList());
        lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
      }
    };
    tryHighlight(HighlightType::ALLIED_TOTEM, "Allied magical field", Color::GREEN);
    tryHighlight(HighlightType::HOSTILE_TOTEM, "Hostile magical field", Color::RED);
    for (auto layer : ENUM_ALL_REVERSE(ViewLayer))
      if (index.hasObject(layer)) {
        auto& viewObject = index.getObject(layer);
        lines.addElemAuto(WL(getListBuilder)
              .addElem(WL(viewObject, viewObject.getViewIdList()), 30)
              .addElemAuto(WL(labelMultiLineWidth, viewObject.getDescription(), legendLineHeight * 2 / 3, 300))
              .buildHorizontalList());
        lines.addSpace(legendLineHeight / 3);
        if (layer == ViewLayer::CREATURE)
          lines.addElemAuto(drawLyingItemsList("Inventory: ", highlighted.viewIndex.getEquipmentCounts(), 250));
        if (viewObject.hasModifier(ViewObject::Modifier::HOSTILE))
          lines.addElem(WL(label, "Hostile", Color::ORANGE));
        for (auto status : viewObject.getCreatureStatus()) {
          lines.addElem(WL(label, getName(status), getColor(status)));
          if (auto desc = getDescription(status))
            lines.addElem(WL(label, *desc, getColor(status)));
          break;
        }
        if (!disableClickActions)
          if (auto actions = getClickActions(viewObject))
            if (highlighted.tileScreenPos)
              allElems.push_back(WL(absolutePosition, WL(translucentBackgroundWithBorderPassMouse, WL(margins,
                  WL(setHeight, *actions->getPreferredHeight(), actions), 5, 1, 5, -2)),
                  highlighted.creaturePos.value_or(*highlighted.tileScreenPos) + Vec2(60, 60)));
        if (!viewObject.getBadAdjectives().empty()) {
          lines.addElemAuto(WL(labelMultiLineWidth, viewObject.getBadAdjectives(), legendLineHeight * 2 / 3, 300,
              Renderer::textSize, Color::RED, ','));
          lines.addSpace(legendLineHeight / 3);
        }
        if (!viewObject.getGoodAdjectives().empty()) {
          lines.addElemAuto(WL(labelMultiLineWidth, viewObject.getGoodAdjectives(), legendLineHeight * 2 / 3, 300,
              Renderer::textSize, Color::GREEN, ','));
          lines.addSpace(legendLineHeight / 3);
        }
        if (viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
          lines.addElem(WL(label, "Can only be healed using rituals."));
        if (auto& attributes = viewObject.getCreatureAttributes())
          lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(*attributes)));
        if (auto health = viewObject.getAttribute(ViewObjectAttribute::HEALTH))
          lines.addElem(WL(stack,
                WL(margins, WL(progressBar, MapGui::getHealthBarColor(*health,
                    viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE)).transparency(70), *health), -2, 0, 0, 3),
                WL(label, getHealthName(viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
                    + toString((int) (100.0f * *health)) + "%")));
        if (auto morale = viewObject.getAttribute(ViewObjectAttribute::MORALE))
          lines.addElem(WL(stack,
                WL(margins, WL(progressBar, (*morale >= 0 ? Color::GREEN : Color::RED).transparency(70), fabs(*morale)), -2, 0, 0, 3),
                WL(label, "Morale: " + getMoraleNumber(*morale))));
        if (auto luxury = viewObject.getAttribute(ViewObjectAttribute::LUXURY))
          lines.addElem(WL(stack,
                WL(margins, WL(progressBar, Color::GREEN.transparency(70), fabs(*luxury)), -2, 0, 0, 3),
                WL(label, "Luxury: " + getMoraleNumber(*luxury))));
        if (viewObject.hasModifier(ViewObjectModifier::UNPAID))
          lines.addElem(WL(label, "Cannot afford item", Color::RED));
        if (viewObject.hasModifier(ViewObjectModifier::PLANNED))
          lines.addElem(WL(label, "Planned"));
        lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
      }
    if (highlighted.viewIndex.isHighlight(HighlightType::INSUFFICIENT_LIGHT))
      lines.addElem(WL(label, "Insufficient light", Color::RED));
    if (highlighted.viewIndex.isHighlight(HighlightType::INDOORS))
      lines.addElem(WL(label, "Indoors"));
    else
      lines.addElem(WL(label, "Outdoors"));
    if (highlighted.tilePos)
      lines.addElem(WL(label, "Position: " + toString(*highlighted.tilePos)));
    lines.addElemAuto(drawLyingItemsList("Lying here: ", highlighted.viewIndex.getItemCounts(), 250));
  }
  if (!lines.isEmpty())
    allElems.push_back(WL(margins, WL(translucentBackgroundWithBorderPassMouse,
        WL(margins, lines.buildVerticalList(), 10, 10, 10, 22)), 0, 0, -2, -2));
  return WL(stack, allElems);
}

SGuiElem GuiBuilder::drawScreenshotOverlay() {
  const int width = 600;
  const int height = 360;
  const int margin = 20;
  return WL(preferredSize, width, height, WL(stack,
      WL(keyHandler, getButtonCallback(UserInputId::CANCEL_SCREENSHOT), {gui.getKey(SDL::SDLK_ESCAPE)}, true),
      WL(rectangle, Color::TRANSPARENT, Color::LIGHT_GRAY),
      WL(translate, WL(centerHoriz, WL(miniWindow, WL(margins, WL(getListBuilder, legendLineHeight)
              .addElem(WL(labelMultiLineWidth, "Your dungeon will be shared in Steam Workshop with an attached screenshot. "
                  "Steer the rectangle below to a particularly pretty or representative area of your dungeon and confirm.",
                  legendLineHeight, width - 2 * margin), legendLineHeight * 4)
              .addElem(WL(centerHoriz, WL(getListBuilder)
                  .addElemAuto(WL(buttonLabel, "Confirm", getButtonCallback({UserInputId::TAKE_SCREENSHOT, Vec2(width, height)})))
                  .addSpace(15)
                  .addElemAuto(WL(buttonLabel, "Cancel", getButtonCallback(UserInputId::CANCEL_SCREENSHOT)))
                  .buildHorizontalList()))
              .buildVerticalList(), margin))),
          Vec2(0, -legendLineHeight * 6), Vec2(width, legendLineHeight * 6))
  ));
}

void GuiBuilder::drawOverlays(vector<OverlayInfo>& ret, GameInfo& info) {
  if (info.takingScreenshot) {
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawScreenshotOverlay, this), THIS_LINE),
        OverlayInfo::CENTER});
    return;
  }
  if (info.tutorial)
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawTutorialOverlay, this), THIS_LINE,
         *info.tutorial), OverlayInfo::TUTORIAL});
  switch (info.infoType) {
    case GameInfo::InfoType::BAND: {
      auto& collectiveInfo = *info.playerInfo.getReferenceMaybe<CollectiveInfo>();
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawVillainsOverlay, this), THIS_LINE,
          info.villageInfo), OverlayInfo::VILLAINS});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawImmigrationOverlay, this), THIS_LINE,
          collectiveInfo.immigration, info.tutorial, !collectiveInfo.allImmigration.empty()),
          OverlayInfo::IMMIGRATION});
      if (info.keeperInDanger)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawKeeperDangerOverlay, this), THIS_LINE,
             *info.keeperInDanger), OverlayInfo::TOP_LEFT});
      else if (collectiveInfo.ransom)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawRansomOverlay, this), THIS_LINE,
            *collectiveInfo.ransom), OverlayInfo::TOP_LEFT});
      else if (collectiveInfo.chosenCreature)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawMinionsOverlay, this), THIS_LINE,
            *collectiveInfo.chosenCreature, collectiveInfo.allQuarters, info.tutorial), OverlayInfo::TOP_LEFT});
      else if (collectiveInfo.chosenWorkshop)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawWorkshopsOverlay, this), THIS_LINE,
            *collectiveInfo.chosenWorkshop, info.tutorial), OverlayInfo::TOP_LEFT});
      else if (bottomWindow == TASKS)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawTasksOverlay, this), THIS_LINE,
            collectiveInfo), OverlayInfo::TOP_LEFT});
      else if (collectiveInfo.nextWave)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawNextWaveOverlay, this), THIS_LINE,
            *collectiveInfo.nextWave), OverlayInfo::TOP_LEFT});
      else if (collectiveInfo.rebellionChance)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawWarningWindow, this), THIS_LINE,
            *collectiveInfo.rebellionChance), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawBuildingsOverlay, this), THIS_LINE,
          collectiveInfo.buildings, !!collectiveInfo.ransom, info.tutorial), OverlayInfo::TOP_LEFT});
      if (bottomWindow == IMMIGRATION_HELP)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawImmigrationHelp, this), THIS_LINE,
            collectiveInfo), OverlayInfo::BOTTOM_LEFT});
      if (bottomWindow == ALL_VILLAINS)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawAllVillainsOverlay, this), THIS_LINE,
            info.villageInfo), OverlayInfo::BOTTOM_LEFT});
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
  if (bottomWindow == BESTIARY)
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawBestiaryOverlay, this), THIS_LINE,
         info.encyclopedia->bestiary, bestiaryIndex), OverlayInfo::TOP_LEFT});
  if (bottomWindow == SPELL_SCHOOLS)
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawSpellSchoolsOverlay, this), THIS_LINE,
         info.encyclopedia->spellSchools, spellSchoolIndex), OverlayInfo::TOP_LEFT});
  if (bottomWindow == ITEMS_HELP)
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawItemsHelpOverlay, this), THIS_LINE,
         info.encyclopedia->items), OverlayInfo::TOP_LEFT});
  ret.push_back({drawMapHintOverlay(), OverlayInfo::MAP_HINT});
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
    auto line = WL(getListBuilder);
    for (auto& message : messages[i]) {
      string text = (line.isEmpty() ? "" : " ") + message.getText();
      cutToFit(renderer, text, maxMessageLength - 2 * hMargin);
      if (message.isClickable()) {
        line.addElemAuto(WL(stack,
              WL(button, getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
              WL(labelHighlight, text, getMessageColor(message))));
        line.addElemAuto(WL(labelUnicodeHighlight, u8"➚", getMessageColor(message)));
      } else
      line.addElemAuto(WL(stack,
            WL(button, getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
            WL(label, text, getMessageColor(message))));
    }
    if (!messages[i].empty())
      lines.push_back(line.buildHorizontalList());
  }
  if (!lines.empty())
    return WL(setWidth, maxMessageLength, WL(translucentBackground,
        WL(margins, WL(verticalList, std::move(lines), lineHeight), hMargin, vMargin, hMargin, vMargin)));
  else
    return WL(empty);
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
    case MenuType::YES_NO_BELOW:
      ySpacing = (renderer.getSize().y - 250) / 2;
      yOffset = (renderer.getSize().y - 500) / 2;
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
    case MenuType::NORMAL:
      ySpacing = 100;
      break;
    case MenuType::NORMAL_BELOW:
      yOffset = (renderer.getSize().y - 500) / 2;
      ySpacing = (renderer.getSize().y - 400) / 2;
      break;
  }
  int xSpacing = (renderer.getSize().x - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing + yOffset, xSpacing + windowWidth, renderer.getSize().y - ySpacing + yOffset);
}

vector<SGuiElem> GuiBuilder::getMultiLine(const string& text, Color color, MenuType menuType, int maxWidth, int fontSize) {
  vector<SGuiElem> ret;
  for (const string& s : gui.breakText(text, maxWidth, fontSize)) {
    if (menuType != MenuType::MAIN)
      ret.push_back(WL(label, s, fontSize, color));
    else
      ret.push_back(WL(mainMenuLabelBg, s, menuLabelVPadding));

  }
  return ret;
}

static int getFontSize(ListElem::ElemMod mod) {
  switch (mod) {
    case ListElem::HELP_TEXT:
      return Renderer::smallTextSize;
    default:
      return Renderer::textSize;
  }
}

Color GuiBuilder::getElemColor(ListElem::ElemMod mod) {
  switch (mod) {
    case ListElem::TITLE:
      return gui.titleText;
    case ListElem::HELP_TEXT:
    case ListElem::INACTIVE:
      return gui.inactiveText;
    case ListElem::TEXT:
    case ListElem::NORMAL:
      return gui.text;
  }
}

SGuiElem GuiBuilder::getHighlight(SGuiElem line, MenuType type, const string& label, int numActive, optional<int>* highlight) {
  switch (type) {
    case MenuType::MAIN:
      return WL(stack, std::move(line),
          WL(mouseHighlight, WL(mainMenuLabel, label, menuLabelVPadding), numActive, highlight));
    default:
      return WL(stack, WL(mouseHighlight,
          WL(leftMargin, 0, WL(translate, WL(uiHighlightLine), Vec2(0, 0))),
          numActive, highlight), std::move(line));
  }
}

static int getLineHeight(ListElem::ElemMod mod) {
  switch (mod) {
    case ListElem::HELP_TEXT:
      return legendLineHeight * 2 / 3;
    default:
      return legendLineHeight;
  }
}

SGuiElem GuiBuilder::getMainMenuLinks(const string& personalMessage, SGuiElem elem) {
  auto getButton = [&](const char* viewId, const char* label, const char* url) {
    return WL(stack,
        WL(button, [url] { openUrl(url); }),
        WL(getListBuilder)
          .addElem(WL(viewObject, ViewId(viewId)), 30)
          .addElemAuto(WL(labelHighlight, label))
          .buildHorizontalList()
    );
  };
  auto makeWindow = [&](SGuiElem elem) {
    return WL(centerHoriz, WL(translucentBackgroundWithBorder, WL(margins, std::move(elem), 15, 4, 15, 5)));
  };
  auto buttonsLine = makeWindow(WL(getListBuilder)
      .addElemAuto(getButton("keeper4", "News", "https://keeperrl.com/category/News/"))
      .addSpace(100)
      .addElemAuto(getButton("elementalist", "Wiki", "http://keeperrl.com/wiki/index.php?title=Main_Page"))
      .addSpace(100)
      .addElemAuto(getButton("jester", "Discord", "https://discordapp.com/invite/XZfCCs5"))
      .buildHorizontalList());
  if (!personalMessage.empty())
    buttonsLine = WL(getListBuilder, legendLineHeight)
        .addElemAuto(makeWindow(WL(label, personalMessage)))
        .addSpace(10)
        .addElem(std::move(buttonsLine))
        .buildVerticalList();
  return WL(stack, std::move(elem),
      WL(fullScreen, WL(alignment, GuiFactory::Alignment::BOTTOM, WL(bottomMargin, 10, std::move(buttonsLine)))));

}

SGuiElem GuiBuilder::drawListGui(const string& title, const vector<ListElem>& options,
    MenuType menuType, optional<int>* highlight, int* choice, vector<int>* positions) {
  auto lines = WL(getListBuilder, listLineHeight);
  if (!title.empty()) {
    lines.addElem(WL(label, capitalFirst(title), Color::WHITE));
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
    Color color = getElemColor(options[i].getMod());
    if (auto p = options[i].getMessagePriority())
      color = getMessageColor(*p);
    vector<SGuiElem> label1 = getMultiLine(options[i].getText(), color, menuType, columnWidth, getFontSize(options[i].getMod()));
    if (options.size() == 1 && label1.size() > 1) { // hacky way of checking that we display a wall of text
      for (auto& line : label1)
        lines.addElem(std::move(line));
      break;
    }
    SGuiElem line;
    if (menuType != MenuType::MAIN)
      line = WL(verticalList, std::move(label1), getLineHeight(options[i].getMod()));
    else
      line = std::move(label1.getOnlyElement());
    if (!options[i].getTip().empty())
      line = WL(stack, std::move(line),
          WL(tooltip, {options[i].getTip()}));
    if (!options[i].getSecondColumn().empty())
      line = WL(horizontalList, makeVec(std::move(line),
            WL(label, options[i].getSecondColumn(), color)), columnWidth + 80);
    if (highlight && options[i].getMod() == ListElem::NORMAL) {
      line = WL(stack,
          WL(button, [=]() { *choice = numActive; }),
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

using MinionGroupMap = unordered_map<ViewIdList, vector<PlayerInfo>, CustomHash<ViewIdList>>;
static MinionGroupMap groupByViewId(const vector<PlayerInfo>& minions) {
  MinionGroupMap ret;
  for (auto& elem : minions)
    ret[elem.viewId].push_back(elem);
  return ret;
}

SGuiElem GuiBuilder::drawMinionButtons(const vector<PlayerInfo>& minions, UniqueEntity<Creature>::Id current,
    optional<TeamId> teamId) {
  CHECK(!minions.empty());
  auto minionMap = groupByViewId(minions);
  auto selectButton = [this](UniqueEntity<Creature>::Id creatureId) {
    return WL(releaseLeftButton, getButtonCallback({UserInputId::CREATURE_BUTTON, creatureId}));
  };
  auto list = WL(getListBuilder, legendLineHeight);
  for (auto& elem : minionMap) {
    list.addElem(WL(topMargin, 5, WL(viewObject, elem.first)), legendLineHeight + 5);
    for (auto& minion : elem.second) {
      auto minionId = minion.creatureId;
      auto line = WL(getListBuilder);
      if (teamId)
        line.addElem(WL(leftMargin, -16, WL(stack,
            WL(button, getButtonCallback({UserInputId::REMOVE_FROM_TEAM, TeamCreatureInfo{*teamId, minionId}})),
            WL(labelUnicodeHighlight, u8"✘", Color::RED))), 1);
      line.addMiddleElem(WL(rightMargin, 5, WL(renderInBounds, WL(label, minion.getFirstName()))));
      if (minion.morale)
        if (auto icon = getMoraleIcon(*minion.morale))
          line.addBackElem(WL(topMargin, -2, WL(icon, *icon)), 20);
      line.addBackElem(drawBestAttack(minion.bestAttack), 52);
      list.addElem(WL(stack, makeVec(
            cache->get(selectButton, THIS_LINE, minionId),
            WL(leftMargin, teamId ? -10 : 0, WL(stack,
                 WL(uiHighlightConditional, [=] { return !teamId && mapGui->isCreatureHighlighted(minionId);}, Color::YELLOW),
                 WL(uiHighlightConditional, [=] { return current == minionId;}))),
            WL(dragSource, minionId, [=]{ return WL(viewObject, minion.viewId);}),
            // not sure if this did anything...
            /*WL(button, [this, minionId, viewId = minion.viewId] {
                callbacks.input(UserInput(UserInputId::CREATURE_DRAG, minionId));
                gui.getDragContainer().put()
                mapGui->setDraggedCreature(minionId, viewId, Vec2(-100, -100), DragContentId::CREATURE_GROUP); }, false),*/
            line.buildHorizontalList())));
    }
  }
  return WL(scrollable, list.buildVerticalList(), &minionButtonsScroll, &scrollbarsHeld);
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
    case MinionActivity::POETRY: return "Fine arts";
    case MinionActivity::MINION_ABUSE: return "Abusing minions";
    case MinionActivity::COPULATE: return "Copulating";
    case MinionActivity::SPIDER: return "Spinning webs";
    case MinionActivity::GUARDING: return "Guarding";
    case MinionActivity::DISTILLATION: return "Distilling";
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
    lines.push_back(WL(stack,
          WL(button, [=] { callback(Rectangle(), none); }, gui.getKey(SDL::SDLK_ESCAPE)),
          WL(centeredLabel, Renderer::HOR, "[done]", Color::LIGHT_BLUE)));
  return lines;
}

SGuiElem GuiBuilder::drawQuartersButton(const PlayerInfo& minion, const vector<ViewId>& allQuarters) {
  auto current = minion.quarters ? WL(viewObject, *minion.quarters) : WL(label, "none");
  return WL(stack,
      WL(uiHighlightMouseOver),
      WL(getListBuilder)
            .addElemAuto(WL(label, "Assigned Quarters: ", Color::YELLOW))
            .addElemAuto(std::move(current))
            .buildHorizontalList(),
      WL(buttonRect, [this, minionId = minion.creatureId, allQuarters] (Rectangle bounds) {
          auto tasks = WL(getListBuilder, legendLineHeight);
          bool exit = false;
          auto retAction = [&] (optional<int> index) {
            callbacks.input({UserInputId::ASSIGN_QUARTERS, AssignQuartersInfo{index, minionId}});
          };
          tasks.addElem(WL(stack,
              WL(button, [&retAction, &exit] { retAction(none); exit = true; }),
              WL(uiHighlightMouseOver),
              WL(label, "none")));
          for (int i : All(allQuarters)) {
            tasks.addElem(WL(stack,
                WL(button, [i, &retAction, &exit] { retAction(i); exit = true; }),
                WL(uiHighlightMouseOver),
                WL(getListBuilder, 32)
                    .addElem(WL(viewObject, allQuarters[i]))
                    .addElem(WL(label, toString(i + 1)))
                    .buildHorizontalList()));
          }
          drawMiniMenu(std::move(tasks), exit, bounds.bottomLeft(), 50, true);
        }));
}

function<void(Rectangle)> GuiBuilder::getActivityButtonFun(const PlayerInfo& minion) {
  return [=] (Rectangle bounds) {
    auto tasks = WL(getListBuilder, legendLineHeight);
    tasks.addElem(WL(getListBuilder)
        .addBackElemAuto(WL(label, "Enable", Renderer::smallTextSize))
        .addBackSpace(40)
        .addBackElem(WL(renderInBounds, WL(label, "Disable for all " + makePlural(minion.groupName), Renderer::smallTextSize)), 134)
        .buildHorizontalList());
    bool exit = false;
    TaskActionInfo retAction;
    retAction.creature = minion.creatureId;
    retAction.groupName = minion.groupName;
    for (int i : All(minion.minionTasks)) {
      auto& task = minion.minionTasks[i];
      function<void()> buttonFun = [] {};
      if (!task.inactive)
        buttonFun = [&exit, &retAction, task] {
            retAction.switchTo = task.task;
            exit = true;
          };
      auto lockButton = WL(rightMargin, 20, WL(conditional,
            [&retAction, task] {
              if (!!task.locked && !(retAction.lockGroup.contains(task.task) ^ task.lockedForGroup)) {
                if (retAction.lock.contains(task.task) ^ *task.locked)
                  return 1;
                else
                  return 2;
              } else
                return 0;
            }, {WL(empty), WL(labelUnicodeHighlight, u8"✘", Color::RED),
                 WL(labelUnicodeHighlight, u8"✓", Color::GREEN)}));
      auto lockButton2 = !!task.locked
            ? WL(rightMargin, 20, WL(conditional, WL(labelUnicodeHighlight, u8"✘", Color::RED),
                 WL(labelUnicodeHighlight, u8"✘", Color::LIGHT_GRAY), [&retAction, task] {
                      return retAction.lockGroup.contains(task.task) ^ task.lockedForGroup;}))
            : WL(empty);
      tasks.addElem(WL(getListBuilder)
          .addMiddleElem(WL(stack,
              WL(button, buttonFun),
              WL(label, getTaskText(task.task), getTaskColor(task))))
          .addBackElem(WL(stack,
              getTooltip({"Click to turn this activity on/off for this minion."}, THIS_LINE + i),
              WL(button, [&retAction, task] {
                retAction.lock.toggle(task.task);
              }),
              lockButton), 37)
          .addBackSpace(43)
          .addBackElemAuto(WL(stack,
              getTooltip({"Click to turn this activity off for all " + makePlural(minion.groupName)}, THIS_LINE + i + 54321),
              WL(button, [&retAction, task] {
                retAction.lockGroup.toggle(task.task);
              }),
              lockButton2))
          .addBackSpace(99)
          .buildHorizontalList());
    }
    drawMiniMenu(std::move(tasks), exit, bounds.bottomLeft(), 362, true);
    callbacks.input({UserInputId::CREATURE_TASK_ACTION, retAction});
  };
}

SGuiElem GuiBuilder::drawActivityButton(const PlayerInfo& minion) {
  string curTask = "(none)";
  for (auto task : minion.minionTasks)
    if (task.current)
      curTask = getTaskText(task.task);
  return WL(stack,
      WL(uiHighlightMouseOver),
      WL(getListBuilder)
          .addElemAuto(WL(label, "Activity: ", Color::YELLOW))
          .addElemAuto(WL(label, curTask))
          .buildHorizontalList(),
      WL(buttonRect, getActivityButtonFun(minion)));
}

SGuiElem GuiBuilder::drawAttributesOnPage(vector<SGuiElem> attrs) {
  if (attrs.empty())
    return WL(empty);
  vector<SGuiElem> lines[2];
  for (int i : All(attrs))
    lines[i % 2].push_back(std::move(attrs[i]));
  int elemWidth = 100;
  return WL(verticalList, makeVec(
      WL(horizontalList, std::move(lines[0]), elemWidth),
      WL(horizontalList, std::move(lines[1]), elemWidth)), legendLineHeight);
}

SGuiElem GuiBuilder::drawEquipmentAndConsumables(const PlayerInfo& minion, bool infoOnly) {
  const vector<ItemInfo>& items = minion.inventory;
  auto lines = WL(getListBuilder, legendLineHeight);
  if (minion.bodyPartLimit > 0) {
    lines.addElem(WL(label, "Body parts (" + toString(minion.bodyParts.size()) + "/"
        + toString(minion.bodyPartLimit) + ")", Color::YELLOW));
    for (auto& part : minion.bodyParts)
      lines.addElem(getItemLine(part, [](Rectangle) {}));
    if (minion.bodyParts.size() < minion.bodyPartLimit)
      lines.addElem(WL(buttonLabel, "Install body part",
          getButtonCallback({UserInputId::CREATURE_ADD_BODY_PART, minion.creatureId})));
  }
  if (!items.empty()) {
    lines.addElem(WL(label, "Equipment", Color::YELLOW));
    vector<SGuiElem> itemElems;
    if (!infoOnly)
      for (int i : All(items)) {
        SGuiElem keyElem = WL(empty);
        auto labelElem = getItemLine(items[i], [this, creatureId = minion.creatureId, item = items[i]] (Rectangle bounds) {
            if (auto choice = getItemChoice(item, bounds.bottomLeft() + Vec2(50, 0), true))
              callbacks.input({UserInputId::CREATURE_EQUIPMENT_ACTION,
                  EquipmentActionInfo{creatureId, item.ids, item.slot, *choice}});
        });
        if (!items[i].ids.empty()) {
          int offset = *labelElem->getPreferredWidth();
          labelElem = WL(stack, std::move(labelElem),
                      WL(mouseHighlight2, WL(leftMargin, offset,
                          items[i].ids.empty() ?  WL(label, "+", Color::YELLOW) : WL(labelUnicode, u8"✘", Color::RED)), nullptr, false));
          keyElem = WL(stack,
              WL(button,
              getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
                  EquipmentActionInfo{minion.creatureId, items[i].ids, items[i].slot, ItemAction::LOCK}})),
              items[i].locked ? 
                WL(viewObject, ViewId("key")) :
                WL(viewObject, ViewId("key_semi_higlight")) : //if not locked and hovered show this ViewId instead. Else, perform code below.
                WL(mouseHighlight2, WL(viewObject, ViewId("key_highlight"))),
              getTooltip({"Locked items won't be automatically swapped by minion."}, THIS_LINE + i)
          );
        }
        itemElems.push_back(
            WL(getListBuilder)
                .addElem(std::move(keyElem), 20)
                .addMiddleElem(std::move(labelElem))
                .buildHorizontalList());
      }
    else
      for (int i : All(items))
        itemElems.push_back(getItemLine(items[i], [](Rectangle) {} ));
    for (int i : All(itemElems))
      if (items[i].type == items[i].EQUIPMENT)
        lines.addElem(WL(leftMargin, 3, std::move(itemElems[i])));
    for (int i : All(itemElems))
      if (!infoOnly || items[i].type == items[i].CONSUMABLE) {
        lines.addElem(WL(label, "Consumables", Color::YELLOW));
        break;
      }
    for (int i : All(itemElems))
      if (items[i].type == items[i].CONSUMABLE)
        lines.addElem(WL(leftMargin, 3, std::move(itemElems[i])));
    if (!infoOnly)
      lines.addElem(WL(buttonLabel, "Add consumable",
          getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
              EquipmentActionInfo{minion.creatureId, {}, none, ItemAction::REPLACE}})));
    for (int i : All(itemElems))
      if (items[i].type == items[i].OTHER) {
        lines.addElem(WL(label, "Other", Color::YELLOW));
        break;
      }
    for (int i : All(itemElems))
      if (items[i].type == items[i].OTHER)
        lines.addElem(WL(leftMargin, 3, std::move(itemElems[i])));
  }
  if (!minion.intrinsicAttacks.empty()) {
    lines.addElem(WL(label, "Intrinsic attacks", Color::YELLOW));
    for (auto& item : minion.intrinsicAttacks)
      lines.addElem(getItemLine(item, [=](Rectangle) {}));
  }
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawMinionActions(const PlayerInfo& minion, const optional<TutorialInfo>& tutorial) {
  auto line = WL(getListBuilder);
  const bool tutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
  for (auto action : minion.actions) {
    switch (action) {
      case PlayerInfo::CONTROL: {
        auto callback = getButtonCallback({UserInputId::CREATURE_CONTROL, minion.creatureId});
        line.addElemAuto(tutorialHighlight
            ? WL(buttonLabelBlink, "Control", callback)
            : WL(buttonLabel, "Control", callback));
        break;
      }
      case PlayerInfo::RENAME:
        line.addElemAuto(WL(buttonLabel, "Rename", [=] {
            if (auto name = getTextInput("Rename minion", minion.firstName, maxFirstNameLength, "Press escape to cancel."))
              callbacks.input({UserInputId::CREATURE_RENAME, RenameActionInfo{minion.creatureId, *name}}); }));
        break;
      case PlayerInfo::BANISH:
        line.addElemAuto(WL(buttonLabel, "Banish",
            getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId})));
        break;
      case PlayerInfo::DISASSEMBLE:
        line.addElemAuto(WL(buttonLabel, "Disassemble",
            getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId})));
        break;
      case PlayerInfo::CONSUME:
        line.addElemAuto(WL(buttonLabel, "Absorb",
            getButtonCallback({UserInputId::CREATURE_CONSUME, minion.creatureId})));
        break;
      case PlayerInfo::LOCATE:
        line.addElemAuto(WL(buttonLabel, "Locate",
            getButtonCallback({UserInputId::CREATURE_LOCATE, minion.creatureId})));
        break;
    }
    line.addSpace(50);
  }
  return line.buildHorizontalList();
}

SGuiElem GuiBuilder::drawKillsLabel(const PlayerInfo& minion) {
  if (!minion.kills.empty()) {
    auto lines = WL(getListBuilder, legendLineHeight);
    auto line = WL(getListBuilder);
    const int rowSize = 8;
    for (auto& kill : minion.kills) {
      line.addElemAuto(WL(viewObject, kill, 1));
      if (line.getLength() >= rowSize) {
        lines.addElem(line.buildHorizontalList());
        line.clear();
      }
    }
    if (!line.isEmpty())
      lines.addElem(line.buildHorizontalList());
    return WL(stack,
        WL(label, toString(minion.kills.size()) + " kills"),
        WL(tooltip2, WL(miniWindow, WL(margins, lines.buildVerticalList(), 15)), [](Rectangle rect){ return rect.bottomLeft(); })
    );
  } else
    return nullptr;
}

SGuiElem GuiBuilder::drawTitleButton(const PlayerInfo& minion) {
  if (!minion.killTitles.empty()) {
    auto titleLine = WL(getListBuilder);
    titleLine.addElemAuto(WL(label, minion.title));
    auto lines = WL(getListBuilder, legendLineHeight);
    for (auto& title : minion.killTitles)
      lines.addElem(WL(label, title));
    auto addLegend = [&] (const char* text) {
      lines.addElem(WL(label, text, Renderer::smallTextSize, Color::LIGHT_GRAY), legendLineHeight * 2 / 3);
    };
    addLegend("Titles are awarded for killing tribe leaders, and increase");
    addLegend("each attribute up to a maximum of the attribute's base value.");
    titleLine.addElemAuto(WL(label, toString("+"), Color::YELLOW));
    return WL(stack,
        titleLine.buildHorizontalList(),
        WL(tooltip2, WL(miniWindow, WL(margins, lines.buildVerticalList(), 15)), [](Rectangle rect){ return rect.bottomLeft(); })
    );
  } else
    return WL(label, minion.title);
}

SGuiElem GuiBuilder::drawSpellLabel(const SpellInfo& spell) {
  auto color = spell.available ? Color::WHITE : Color::LIGHT_GRAY;
  return WL(getListBuilder)
      .addElemAuto(getSpellIcon(spell, 0, false, 0))
      .addElemAuto(WL(label, spell.name, color))
      .addBackElemAuto(WL(label, "Level " + toString(*spell.level), color))
      .buildHorizontalList();
}

SGuiElem GuiBuilder::drawSpellSchoolLabel(const SpellSchoolInfo& school) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, "Experience type: "_s + getName(school.experienceType)));
  for (auto& spell : school.spells) {
    lines.addElem(drawSpellLabel(spell));
  }
  return WL(stack,
      WL(label, school.name),
      WL(tooltip2, WL(setWidth, 350, WL(miniWindow, WL(margins, lines.buildVerticalList(), 15))),
          [](const Rectangle& r) { return r.bottomRight(); }));
}

SGuiElem GuiBuilder::drawMinionPage(const PlayerInfo& minion, const vector<ViewId>& allQuarters,
    const optional<TutorialInfo>& tutorial) {
  auto list = WL(getListBuilder, legendLineHeight);
  auto titleLine = WL(getListBuilder);
  titleLine.addElemAuto(drawTitleButton(minion));
  if (auto killsLabel = drawKillsLabel(minion))
    titleLine.addBackElemAuto(std::move(killsLabel));
  list.addElem(titleLine.buildHorizontalList());
  if (!minion.description.empty())
    list.addElem(WL(label, minion.description, Renderer::smallTextSize, Color::LIGHT_GRAY));
  list.addElem(drawMinionActions(minion, tutorial));
  auto leftLines = WL(getListBuilder, legendLineHeight);
  leftLines.addElem(WL(label, "Attributes", Color::YELLOW));
  leftLines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(minion.attributes)));
  for (auto& elem : drawEffectsList(minion))
    leftLines.addElem(std::move(elem));
  leftLines.addSpace();
  if (auto elem = drawTrainingInfo(minion.experienceInfo))
    leftLines.addElemAuto(std::move(elem));
  if (!minion.spellSchools.empty()) {
    auto line = WL(getListBuilder)
        .addElemAuto(WL(label, "Spell schools: ", Color::YELLOW));
    for (int i : All(minion.spellSchools)) {
      if (i > 0)
        line.addElemAuto(WL(label, ", "));
      line.addElemAuto(drawSpellSchoolLabel(minion.spellSchools[i]));
    }
    leftLines.addElem(line.buildHorizontalList());
  }
  leftLines.addSpace();
  leftLines.addElem(drawActivityButton(minion));
  if (minion.canAssignQuarters)
    leftLines.addElem(drawQuartersButton(minion, allQuarters));
  leftLines.addSpace();
  for (auto& elem : drawSkillsList(minion))
    leftLines.addElem(std::move(elem));
  if (auto spells = drawSpellsList(minion.spells, minion.creatureId.getGenericId(), false))
    leftLines.addElemAuto(std::move(spells));
  int topMargin = list.getSize() + 20;
  return WL(margin, list.buildVerticalList(),
      WL(scrollable, WL(horizontalListFit, makeVec(
          WL(rightMargin, 15, leftLines.buildVerticalList()),
          drawEquipmentAndConsumables(minion))), &minionPageScroll, &scrollbarsHeld2),
      topMargin, GuiFactory::TOP);
}

SGuiElem GuiBuilder::drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>& queue, const string& title,
    pair<ViewId, int> budget, const vector<ItemInfo>& items, ScrollPosition* scrollPos) {
  int titleExtraSpace = 10;
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, title))
      .addBackElemAuto(drawCost(budget)).buildHorizontalList(),
     getStandardLineHeight() + titleExtraSpace);
  for (SGuiElem& elem : drawItemMenu(items,
        [&queue, items] (Rectangle, optional<int> index) { if (index) queue.push(*items[*index].ids.begin());}))
    lines.addElem(std::move(elem));
  return WL(setWidth, 380,
      WL(miniWindow, WL(margins, WL(scrollable, lines.buildVerticalList(), scrollPos), 15),
          [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawPillageItemMenu(SyncQueue<optional<int>>& queue, const string& title,
    const vector<ItemInfo>& items, ScrollPosition* scrollPos) {
  int titleExtraSpace = 10;
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(label, title), getStandardLineHeight() + titleExtraSpace);
  for (SGuiElem& elem : drawItemMenu(items,
        [&queue, &items] (Rectangle, optional<int> index) {
            if (index && !items[*index].unavailable) queue.push(*index);}))
    lines.addElem(std::move(elem));
  return WL(setWidth, 380,
      WL(miniWindow, WL(margins, WL(scrollable, lines.buildVerticalList(), scrollPos), 15),
                     [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawChooseNumberMenu(SyncQueue<optional<int>>& queue, const string& title,
    Range range, int initial, int increments) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(centerHoriz, WL(label, title)));
  lines.addSpace(legendLineHeight / 2);
  auto currentChoice = make_shared<int>((initial - range.getStart()) / increments);
  auto getCurrent = [range, currentChoice, increments] {
    return range.getStart() + *currentChoice * increments;
  };
  const int sideMargin = 50;
  lines.addElem(WL(leftMargin, sideMargin, WL(rightMargin, sideMargin, WL(stack,
      WL(margins, WL(rectangle, Color::ALMOST_DARK_GRAY), 0, legendLineHeight / 3, 0, legendLineHeight / 3),
      WL(slider, WL(preferredSize, 10, 25, WL(rectangle, Color::WHITE)), currentChoice,
          range.getLength() / increments))
  )));
  const int width = 380;
  lines.addElem(WL(stack,
      WL(centerHoriz, WL(labelFun, [getCurrent]{ return toString(getCurrent()); }), 30),
      WL(translate, WL(centeredLabel, Renderer::HOR, toString(range.getStart())), Vec2(sideMargin, 0), Vec2(1, 0)),
      WL(translate, WL(centeredLabel, Renderer::HOR, toString(range.getEnd())), Vec2(-sideMargin, 0), Vec2(1, 0),
          GuiFactory::TranslateCorner::TOP_RIGHT)
  ));
  auto confirmFun = [&queue, getCurrent] { queue.push(getCurrent());};
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(stack,
          WL(keyHandler, confirmFun, {gui.getKey(SDL::SDLK_RETURN)}, true),
          WL(buttonLabel, "Confirm", confirmFun)))
      .addSpace(15)
      .addElemAuto(WL(buttonLabel, "Cancel", [&queue] { queue.push(none);}))
      .buildHorizontalList()
  ));
  return WL(setWidth, width,
      WL(miniWindow, WL(margins, lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawTickBox(shared_ptr<bool> value, const string& title) {
  return WL(stack,
      WL(button, [value]{ *value = !*value; }),
      WL(getListBuilder)
          .addElemAuto(
              WL(conditional, WL(labelUnicodeHighlight, u8"✓", Color::GREEN), [value] { return *value; }))
          .addElemAuto(WL(label, title))
          .buildHorizontalList());
}

SGuiElem GuiBuilder::drawBugreportMenu(bool saveFile, function<void(optional<BugReportInfo>)> callback) {
  auto lines = WL(getListBuilder, legendLineHeight);
  const int width = 300;
  const int windowMargin = 15;
  lines.addElem(WL(centerHoriz, WL(label, "Submit bug report")));
  auto text = make_shared<string>();
  auto withScreenshot = make_shared<bool>(true);
  auto withSavefile = make_shared<bool>(saveFile);
  lines.addElem(WL(label, "Enter bug desciption:"));
  lines.addElemAuto(WL(stack,
        WL(rectangle, Color::GRAY),
        WL(margins, WL(textInput, width - 10, 5, text), 5)));
  lines.addSpace();
  lines.addElem(drawTickBox(withScreenshot, "Include screenshot"));
  if (saveFile)
    lines.addElem(drawTickBox(withSavefile, "Include save file"));
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(buttonLabel, "Upload",
          [=] { callback(BugReportInfo{*text, *withSavefile, *withScreenshot});}))
      .addSpace(10)
      .addElemAuto(WL(buttonLabel, "Cancel", [callback] { callback(none);}))
      .buildHorizontalList()
  ));
  return WL(setWidth, width + 2 * windowMargin,
      WL(miniWindow, WL(margins, lines.buildVerticalList(), windowMargin), [callback]{ callback(none); }));
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
  int iconScale = c.getMapZoom();
  int iconSize = 24 * iconScale;
  auto rows = WL(getListBuilder, iconSize);
  auto& sites = c.getSites();
  for (int y : sites.getBounds().getYRange()) {
    auto columns = WL(getListBuilder, iconSize);
    for (int x : sites.getBounds().getXRange()) {
      vector<SGuiElem> v;
      for (int i : All(sites[x][y].viewId)) {
        v.push_back(WL(asciiBackground, sites[x][y].viewId[i]));
        if (i == 0)
          v.push_back(WL(viewObject, sites[x][y].viewId[i], iconScale));
        else {
          if (sites[x][y].viewId[i] == ViewId("canif_tree") || sites[x][y].viewId[i] == ViewId("decid_tree"))
            v.push_back(WL(topMargin, 1 * iconScale,
                  WL(viewObject, ViewId("round_shadow"), iconScale, Color(255, 255, 255, 160))));
          v.push_back(WL(topMargin, -2 * iconScale, WL(viewObject, sites[x][y].viewId[i], iconScale)));
        }
      }
      columns.addElem(WL(stack, std::move(v)));
    }
    auto columns2 = WL(getListBuilder, iconSize);
    for (int x : sites.getBounds().getXRange()) {
      Vec2 pos(x, y);
      vector<SGuiElem> elem;
      if (auto id = sites[x][y].getDwellerViewId()) {
        elem.push_back(WL(asciiBackground, id->front()));
        if (c.getPlayerPos() && c.isInInfluence(pos))
          elem.push_back(WL(viewObject, ViewId("square_highlight"), iconScale,
              getHighlightColor(*sites[pos].getVillainType())));
        if (c.getPlayerPos() == pos && (!marked || !*marked)) // hacky way of checking this is adventurer embark position
          elem.push_back(WL(viewObject, ViewId("square_highlight"), iconScale));
        if (activeFun(pos))
          elem.push_back(WL(stack,
                WL(button, [pos, clickFun] { clickFun(pos); }),
                WL(mouseHighlight2, WL(viewObject, ViewId("square_highlight"), iconScale))));
        elem.push_back(WL(topMargin, 1 * iconScale,
              WL(viewObject, ViewId("round_shadow"), iconScale, Color(255, 255, 255, 160))));
        if (marked)
          elem.push_back(WL(conditional, WL(viewObject, ViewId("square_highlight"), iconScale),
                [marked, pos] { return *marked == pos;}));
        elem.push_back(WL(topMargin, -2 * iconScale, WL(viewObject, *id, iconScale)));
        if (c.isDefeated(pos))
          elem.push_back(WL(viewObject, ViewId("campaign_defeated"), iconScale));
      } else {
        if (activeFun(pos))
          elem.push_back(WL(stack,
                WL(button, [pos, clickFun] { clickFun(pos); }),
                WL(mouseHighlight2, WL(viewObject, ViewId("square_highlight"), iconScale))));
        if (marked)
          elem.push_back(WL(conditional, WL(viewObject, ViewId("square_highlight"), iconScale),
                [marked, pos] { return *marked == pos;}));
      }
      if (auto desc = sites[x][y].getDwellerDescription())
        elem.push_back(WL(tooltip, {*desc}, milliseconds{0}));
      columns2.addElem(WL(stack, std::move(elem)));
    }
    rows.addElem(WL(stack, columns.buildHorizontalList(), columns2.buildHorizontalList()));
  }
  Vec2 maxSize(min(sites.getBounds().width() * iconSize, 17 * 48), min(sites.getBounds().height() * iconSize, 9 * 48));
  auto mapContent = rows.buildVerticalList();
  if (*mapContent->getPreferredWidth() > maxSize.x || *mapContent->getPreferredHeight() > maxSize.y)
    mapContent = WL(scrollArea, std::move(mapContent));
  int margin = 8;
  return WL(preferredSize, maxSize + Vec2(margin, margin) * 2, WL(stack,
    WL(miniBorder2),
    WL(margins, std::move(mapContent), margin)));
}

SGuiElem GuiBuilder::drawWorldmap(Semaphore& sem, const Campaign& campaign) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, "Map of " + campaign.getWorldName())));
  lines.addElem(WL(centerHoriz, WL(label, "Use the travel command while controlling a minion or team "
          "to travel to another site.", Renderer::smallTextSize, Color::LIGHT_GRAY)));
  lines.addElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, nullptr,
      [](Vec2) { return false; },
      [](Vec2) { })));
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(centerHoriz, WL(buttonLabel, "Close", [&] { sem.v(); })));
  return WL(preferredSize, 1000, 630,
      WL(window, WL(margins, lines.buildVerticalList(), 15), [&sem] { sem.v(); }));
}

SGuiElem GuiBuilder::drawChooseSiteMenu(SyncQueue<optional<Vec2>>& queue, const string& message,
    const Campaign& campaign, optional<Vec2>& sitePos) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, message)));
  lines.addElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, &sitePos,
      [&campaign](Vec2 pos) { return campaign.canTravelTo(pos); },
      [&sitePos](Vec2 pos) { sitePos = pos; })));
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(conditional,
          WL(buttonLabel, "Confirm", [&] { queue.push(*sitePos); }),
          WL(buttonLabelInactive, "Confirm"),
          [&] { return !!sitePos; }))
      .addSpace(15)
      .addElemAuto(WL(buttonLabel, "Cancel",
          WL(button, [&queue] { queue.push(none); }, gui.getKey(SDL::SDLK_ESCAPE), true)))
      .buildHorizontalList()));
  return WL(preferredSize, 1000, 600,
      WL(window, WL(margins, lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
}

static const char* getText(AvatarMenuOption option) {
  switch (option) {
    case AvatarMenuOption::TUTORIAL:
      return "Tutorial";
    case AvatarMenuOption::LOAD_GAME:
      return "Load game";
    case AvatarMenuOption::CHANGE_MOD:
      return "Mods";
    case AvatarMenuOption::GO_BACK:
      return "Go back";
  }
}

SGuiElem GuiBuilder::drawGenderButtons(const vector<View::AvatarData>& avatars,
    shared_ptr<int> gender, shared_ptr<int> chosenAvatar) {
  vector<SGuiElem> genderOptions;
  for (int avatarIndex : All(avatars)) {
    auto& avatar = avatars[avatarIndex];
    auto genderList = WL(getListBuilder);
    if (avatar.viewId.size() > 1 || !avatar.settlementNames)
      for (int i : All(avatar.viewId)) {
        auto selectFun = [i, gender] { *gender = i; };
        genderList.addElemAuto(WL(conditional,
            WL(conditional, WL(buttonLabelSelected, capitalFirst(avatar.genderNames[i]), selectFun, false, true),
                WL(buttonLabel, capitalFirst(capitalFirst(avatar.genderNames[i])), selectFun, false, true),
               [gender, i] { return *gender == i; }),
            [=] { return avatarIndex == *chosenAvatar; }));
      }
    genderOptions.push_back(genderList.buildHorizontalListFit(0.2));
  }
  return WL(stack, std::move(genderOptions));
}

static int getChosenGender(shared_ptr<int> gender, shared_ptr<int> chosenAvatar,
    const vector<View::AvatarData>& avatars) {
  return min(*gender, avatars[*chosenAvatar].viewId.size() - 1);
}

SGuiElem GuiBuilder::drawFirstNameButtons(const vector<View::AvatarData>& avatars,
    shared_ptr<int> gender, shared_ptr<int> chosenAvatar, shared_ptr<int> chosenName) {
  vector<SGuiElem> firstNameOptions = {};
  for (int avatarIndex : All(avatars)) {
    auto& avatar = avatars[avatarIndex];
    for (int genderIndex : All(avatar.viewId)) {
      auto elem = WL(getListBuilder)
          .addElemAuto(WL(label, avatar.settlementNames ? "Settlement: " : "First name: "))
          .addMiddleElem(WL(textField, maxFirstNameLength,
              [=] {
                auto entered = options->getValueString(avatar.nameOption);
                return entered.empty() ?
                    avatar.firstNames[genderIndex][*chosenName % avatar.firstNames[genderIndex].size()] :
                    entered;
              },
              [=] (string s) {
                options->setValue(avatar.nameOption, s);
              }))
          .addSpace(10)
          .addBackElemAuto(WL(buttonLabel, "🔄", [=] { options->setValue(avatar.nameOption, ""_s); ++*chosenName; }, true, false, true))
          .buildHorizontalList();
      firstNameOptions.push_back(WL(conditional, std::move(elem),
          [=]{ return getChosenGender(gender, chosenAvatar, avatars) == genderIndex && avatarIndex == *chosenAvatar; }));
    }
  }
  return WL(stack, std::move(firstNameOptions));
}

SGuiElem GuiBuilder::drawRoleButtons(shared_ptr<PlayerRole> chosenRole, shared_ptr<int> chosenAvatar, shared_ptr<int> avatarPage,
    const vector<View::AvatarData>& avatars) {
  auto roleList = WL(getListBuilder);
  for (auto role : ENUM_ALL(PlayerRole)) {
    auto chooseFun = [chosenRole, chosenAvatar, &avatars, role, avatarPage] {
      *chosenRole = role;
      *avatarPage = 0;
      for (int i : All(avatars))
        if (avatars[i].role == *chosenRole) {
          *chosenAvatar = i;
          break;
        }
    };
    roleList.addElemAuto(
        WL(conditional, WL(buttonLabelSelected, capitalFirst(getName(role)), chooseFun, false, true),
            WL(buttonLabel, capitalFirst(getName(role)), chooseFun, false, true),
            [chosenRole, role] { return *chosenRole == role; })
    );
  }
  return roleList.buildHorizontalListFit(0.2);
}

constexpr int avatarsPerPage = 10;

SGuiElem GuiBuilder::drawChosenCreatureButtons(PlayerRole role, shared_ptr<int> chosenAvatar, shared_ptr<int> gender, int page,
    const vector<View::AvatarData>& avatars) {
  auto allLines = WL(getListBuilder, legendLineHeight);
  auto line = WL(getListBuilder);
  const int start = page * avatarsPerPage;
  const int end = (page + 1) * avatarsPerPage;
  int processed = 0;
  for (int i : All(avatars)) {
    auto& elem = avatars[i];
    auto viewIdFun = [gender, id = elem.viewId] { return id[min(*gender, id.size() - 1)].back(); };
    if (elem.role == role) {
      if (processed >= start && processed < end)
        line.addElemAuto(WL(stack,
            WL(button, [i, chosenAvatar]{ *chosenAvatar = i; }),
            WL(mouseHighlight2,
                WL(rightMargin, 10, WL(topMargin, -5, WL(viewObject, viewIdFun, 2))),
                WL(rightMargin, 10, WL(conditional2,
                    WL(topMargin, -5, WL(viewObject, viewIdFun, 2)),
                    WL(viewObject, viewIdFun, 2), [=](GuiElem*){ return *chosenAvatar == i;})))
        ));
      ++processed;
      if (line.getLength() >= 5) {
        allLines.addElemAuto(line.buildHorizontalList());
        line.clear();
      }
    }
  }
  if (!line.isEmpty())
    allLines.addElemAuto(line.buildHorizontalList());
  return allLines.buildVerticalList();
};

SGuiElem GuiBuilder::drawAvatarsForRole(const vector<View::AvatarData>& avatars, shared_ptr<int> avatarPage,
    shared_ptr<int> chosenAvatar, shared_ptr<int> gender, shared_ptr<PlayerRole> chosenRole) {
  vector<SGuiElem> avatarsForRole;
  int maxAvatarPagesHeight = 0;
  for (auto role : ENUM_ALL(PlayerRole)) {
    int numAvatars = 0;
    for (auto& a : avatars)
      if (a.role == role)
        ++numAvatars;
    vector<SGuiElem> avatarPages;
    const int numPages = 1 + (numAvatars - 1) / avatarsPerPage;
    for (int page = 0; page < numPages; ++page)
      avatarPages.push_back(WL(conditional,
          drawChosenCreatureButtons(role, chosenAvatar, gender, page, avatars),
          [avatarPage, page] { return *avatarPage == page; }));
    maxAvatarPagesHeight = max(maxAvatarPagesHeight, *avatarPages[0]->getPreferredHeight());
    if (numPages > 1) {
      avatarPages.push_back(WL(alignment, GuiFactory::Alignment::BOTTOM_RIGHT,
          WL(translate,
              WL(getListBuilder, 24)
                .addElem(WL(conditional2,
                    WL(buttonLabel, "<", [avatarPage]{ *avatarPage = max(0, *avatarPage - 1); }),
                    WL(buttonLabelInactive, "<"),
                    [=] (GuiElem*) { return *avatarPage > 0; }))
                .addElem(WL(conditional2,
                    WL(buttonLabel, ">", [=]{ *avatarPage = min(numPages - 1, *avatarPage + 1); }),
                    WL(buttonLabelInactive, ">"),
                    [=] (GuiElem*) { return *avatarPage < numPages - 1; }))
                .buildHorizontalList(),
             Vec2(12, 32), Vec2(48, 30)
          )));
    }
    avatarsForRole.push_back(WL(conditional, WL(stack, std::move(avatarPages)),
        [chosenRole, role] { return *chosenRole == role; }));
  }
  return WL(setHeight, maxAvatarPagesHeight, WL(stack, std::move(avatarsForRole)));
}

SGuiElem GuiBuilder::drawAvatarMenu(SyncQueue<variant<View::AvatarChoice, AvatarMenuOption>>& queue,
    const vector<View::AvatarData>& avatars) {
  auto gender = make_shared<int>(0);
  auto chosenAvatar = make_shared<int>(0);
  auto chosenName = make_shared<int>(0);
  auto avatarPage = make_shared<int>(0);
  auto chosenRole = make_shared<PlayerRole>(PlayerRole::KEEPER);
  auto leftLines = WL(getListBuilder, legendLineHeight);
  auto rightLines = WL(getListBuilder, legendLineHeight);
  leftLines.addElem(drawRoleButtons(chosenRole, chosenAvatar, avatarPage, avatars));
  leftLines.addSpace(15);
  leftLines.addElem(drawGenderButtons(avatars, gender, chosenAvatar));
  leftLines.addSpace(15);
  leftLines.addElem(drawFirstNameButtons(avatars, gender, chosenAvatar, chosenName));
  leftLines.addSpace(15);
  rightLines.addElemAuto(drawAvatarsForRole(avatars, avatarPage, chosenAvatar, gender, chosenRole));
  rightLines.addSpace(12);
  rightLines.addElem(WL(labelFun, [&avatars, chosenAvatar] {
      return capitalFirst(avatars[*chosenAvatar].name) + ", " + getName(avatars[*chosenAvatar].alignment);}));
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElemAuto(WL(getListBuilder)
      .addElemAuto(WL(rightMargin, 30, leftLines.buildVerticalList()))
      .addElemAuto(rightLines.buildVerticalList())
      .buildHorizontalListFit()
  );
  vector<SGuiElem> descriptions;
  for (int avatarIndex : All(avatars)) {
    auto& avatar = avatars[avatarIndex];
    descriptions.push_back(WL(conditional,
        WL(labelMultiLineWidth, avatar.description, legendLineHeight, 530, Renderer::textSize, Color::LIGHT_GRAY),
        [avatarIndex, chosenAvatar] { return avatarIndex == *chosenAvatar; }));
  }
  lines.addBackElem(WL(stack, descriptions), 2.5 * legendLineHeight);
  lines.addBackElem(WL(centerHoriz, WL(buttonLabel, "Start new game",
      [&queue, chosenAvatar, chosenName, gender, &avatars, this] {
        auto chosenGender = getChosenGender(gender, chosenAvatar, avatars);
        auto enteredName = options->getValueString(avatars[*chosenAvatar].nameOption);
        auto& firstNames = avatars[*chosenAvatar].firstNames[chosenGender];
        queue.push(View::AvatarChoice{*chosenAvatar, chosenGender, enteredName.empty() ?
            firstNames[*chosenName % firstNames.size()] :
            enteredName});
      })));
  auto menuLines = WL(getListBuilder, legendLineHeight)
      .addElemAuto(
          WL(preferredSize, 600, 350, WL(window, WL(margins,
              lines.buildVerticalList(), 15), [&queue]{ queue.push(AvatarMenuOption::GO_BACK); })));
  auto othersLine = WL(getListBuilder);
  for (auto option : ENUM_ALL(AvatarMenuOption)) {
    othersLine.addElemAuto(
        WL(stack,
              WL(button, [&queue, option]{ queue.push(option); }),
              WL(buttonLabelWithMargin, getText(option), false)));
  }
  menuLines.addElem(WL(margins, othersLine.buildHorizontalListFit(), 40, 0, 40, 0), 30);
  return menuLines.buildVerticalList();
}

SGuiElem GuiBuilder::drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease, bool leftRight) {
  string plus = leftRight ? "<"  : "+";
  string minus = leftRight ? ">"  : "-";
  return WL(margins, WL(getListBuilder)
      .addElem(canIncrease
          ? WL(buttonLabel, plus, [callback] { callback(1); }, false)
          : WL(buttonLabelInactive, plus, false), 10)
      .addSpace(12)
      .addElem(canDecrease
          ? WL(buttonLabel, minus, [callback] { callback(-1); }, false)
          : WL(buttonLabelInactive, minus, false), 10)
      .buildHorizontalList(), 0, 2, 0, 2);
}

SGuiElem GuiBuilder::drawBoolOptionElem(OptionId id, string name) {
  auto line = WL(getListBuilder);
  line.addElemAuto(WL(conditional,
      WL(labelUnicode, u8"✓", Color::GREEN),
      WL(labelUnicode, u8"✘", Color::RED),
      [this, id]{ return options->getBoolValue(id); })
  );
  line.addElemAuto(WL(label, name));
  return WL(stack,
      WL(button, [this, id] { options->setValue(id, int(!options->getBoolValue(id))); }),
      line.buildHorizontalList()
  );
}

SGuiElem GuiBuilder::drawOptionElem(OptionId id, function<void()> onChanged, optional<string> defaultString) {
  auto getValue = [id, options = this->options, defaultString] {
    string valueString = options->getValueString(id);
    if (!valueString.empty() || !defaultString)
      return valueString;
    else
      return *defaultString;
  };
  string name = options->getName(id);
  SGuiElem ret;
  auto limits = options->getLimits(id);
  int value = options->getIntValue(id);
  ret = WL(getListBuilder)
      .addElem(WL(labelFun, [=]{ return name + ": " + getValue(); }), renderer.getTextLength(name) + 20)
      .addBackElemAuto(drawPlusMinus([=] (int v) { options->setValue(id, value + v); onChanged();},
          value < limits->getEnd() - 1, value > limits->getStart(), options->hasChoices(id)))
      .buildHorizontalList();
  return WL(stack,
      WL(tooltip, {options->getHint(id).value_or("")}),
      std::move(ret)
  );
}

GuiFactory::ListBuilder GuiBuilder::drawRetiredGames(RetiredGames& retired, function<void()> reloadCampaign,
    optional<int> maxActive, string searchString) {
  auto lines = WL(getListBuilder, legendLineHeight);
  const vector<RetiredGames::RetiredGame>& allGames = retired.getAllGames();
  bool displayActive = !maxActive;
  for (int i : All(allGames)) {
    if (i == retired.getNumLocal() && !displayActive)
      lines.addElem(WL(label, allGames[i].subscribed ? "Subscribed:" : "Online:", Color::YELLOW));
    else
      if (i > 0 && !allGames[i].subscribed && allGames[i - 1].subscribed && !displayActive)
        lines.addElem(WL(label, "Online:", Color::YELLOW));
    if (searchString != "" && !contains(toLower(allGames[i].gameInfo.name), toLower(searchString)))
      continue;
    if (retired.isActive(i) == displayActive) {
      auto header = WL(getListBuilder);
      bool maxedOut = !displayActive && retired.getNumActive() >= *maxActive;
      header.addElem(WL(label, allGames[i].gameInfo.name,
          maxedOut ? Color::LIGHT_GRAY : Color::WHITE), 170);
      for (auto& minion : allGames[i].gameInfo.minions)
        header.addElem(drawMinionAndLevel(minion.viewId, minion.level, 1), 25);
      header.addSpace(20);
      if (retired.isActive(i))
        header.addElemAuto(WL(buttonLabel, "Remove", [i, reloadCampaign, &retired] { retired.setActive(i, false); reloadCampaign();}));
      else if (!maxedOut)
        header.addElemAuto(WL(buttonLabel, "Add", [i, reloadCampaign, &retired] { retired.setActive(i, true); reloadCampaign();}));
      lines.addElem(header.buildHorizontalList());
      if (allGames[i].numTotal > 0)
        lines.addElem(WL(stack,
          WL(tooltip, {"Number of times this dungeon has been conquered over how many times it has been loaded."}),
          WL(label, "Conquer rate: " + toString(allGames[i].numWon) + "/" + toString(allGames[i].numTotal),
              Renderer::smallTextSize, gui.inactiveText)), legendLineHeight * 2 / 3);
      if (!allGames[i].gameInfo.spriteMods.empty()) {
        auto modsList = combine(allGames[i].gameInfo.spriteMods, true);
        lines.addElem(WL(stack,
            WL(tooltip, {"These mods may be required to successfully load this dungeon:", modsList}),
            WL(renderInBounds, WL(label, "Requires mods:" + modsList, Renderer::smallTextSize, gui.inactiveText))),
            legendLineHeight * 2 / 3);
      }
      lines.addSpace(legendLineHeight / 3);
    }
  }
  return lines;
}

static const char* getGameTypeName(CampaignType type) {
  switch (type) {
    case CampaignType::FREE_PLAY: return "Campaign";
    case CampaignType::SINGLE_KEEPER: return "Single map";
    case CampaignType::QUICK_MAP: return "Quick map";
  }
}

SGuiElem GuiBuilder::drawMenuWarning(View::CampaignOptions::WarningType type) {
  switch (type) {
    case View::CampaignOptions::NO_RETIRE:
      return WL(labelMultiLine, "Warning: you won't be able to retire your dungeon in this mode.",
              legendLineHeight, Renderer::textSize, Color::RED);
  }
}

SGuiElem GuiBuilder::drawBiomeMenu(SyncQueue<CampaignAction>& queue,
    const vector<View::CampaignOptions::BiomeInfo>& biomes, int chosen) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, "base biome: " + biomes[chosen].name));
  auto choices = WL(getListBuilder);
  for (int i : All(biomes)) {
    choices.addElemAuto(WL(stack,
        i == chosen ? WL(standardButtonHighlight) : WL(standardButton),
        WL(margins, WL(viewObject, biomes[i].viewId), 5, 5, 5, 5),
        WL(button, [&queue, i] { queue.push({CampaignActionId::BIOME, i}); })
    ));
  }
  lines.addElemAuto(choices.buildHorizontalList());
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawCampaignMenu(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    View::CampaignMenuState& menuState) {
  const auto& campaign = campaignOptions.campaign;
  auto& retiredGames = campaignOptions.retired;
  auto lines = WL(getListBuilder, getStandardLineHeight());
  auto centerLines = WL(getListBuilder, getStandardLineHeight());
  auto rightLines = WL(getListBuilder, getStandardLineHeight());
  int optionMargin = 50;
  centerLines.addElem(WL(centerHoriz,
       WL(label, "Game mode: "_s + getGameTypeName(campaign.getType()))));
  rightLines.addElem(WL(leftMargin, -55,
      WL(buttonLabel, "Change",
      WL(buttonRect, [&queue, this, campaignOptions] (Rectangle bounds) {
          auto lines = WL(getListBuilder, legendLineHeight);
          bool exit = false;
          for (auto& info : campaignOptions.availableTypes) {
            lines.addElem(WL(buttonLabel, getGameTypeName(info.type),
                [&, info] { queue.push({CampaignActionId::CHANGE_TYPE, info.type}); exit = true; }));
            for (auto& desc : info.description)
              lines.addElem(WL(leftMargin, 0, WL(label, "- " + desc, Color::LIGHT_GRAY, Renderer::smallTextSize)),
                  legendLineHeight * 2 / 3);
            lines.addSpace(legendLineHeight / 3);
          }
          drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 350, true);
      }))));
  centerLines.addSpace(10);
  centerLines.addElem(WL(centerHoriz,
            WL(buttonLabel, "Help", [&] { menuState.helpText = !menuState.helpText; })));
  lines.addElem(WL(leftMargin, optionMargin, WL(label, "World name: " + campaign.getWorldName())));
  lines.addSpace(10);
  auto retiredMenuLines = WL(getListBuilder, getStandardLineHeight());
  if (retiredGames) {
    retiredMenuLines.addElem(WL(getListBuilder)
        .addElemAuto(WL(label, "Search: "))
        .addElem(WL(textField, 10, [ret = campaignOptions.searchString] { return ret; },
            [&queue](string s){ queue.push(CampaignAction(CampaignActionId::SEARCH_RETIRED, std::move(s)));}), 200)
        .addSpace(10)
        .addElemAuto(WL(buttonLabel, "X",
            [&queue]{ queue.push(CampaignAction(CampaignActionId::SEARCH_RETIRED, string()));}))
        .buildHorizontalList()
    );
    auto addedDungeons = drawRetiredGames(
        *retiredGames, [&queue] { queue.push(CampaignActionId::UPDATE_MAP);}, none, "");
    int addedHeight = addedDungeons.getSize();
    if (!addedDungeons.isEmpty()) {
      retiredMenuLines.addElem(WL(label, "Added:", Color::YELLOW));
      retiredMenuLines.addElem(addedDungeons.buildVerticalList(), addedHeight);
    }
    GuiFactory::ListBuilder retiredList = drawRetiredGames(*retiredGames,
        [&queue] { queue.push(CampaignActionId::UPDATE_MAP);}, options->getIntValue(OptionId::MAIN_VILLAINS), campaignOptions.searchString);
    if (retiredList.isEmpty())
      retiredList.addElem(WL(label, "No retired dungeons found :("));
    else
      retiredMenuLines.addElem(WL(label, "Local:", Color::YELLOW));
    retiredMenuLines.addElemAuto(retiredList.buildVerticalList());
    lines.addElem(WL(leftMargin, optionMargin,
        WL(buttonLabel, "Add retired dungeons", [&menuState] { menuState.retiredWindow = !menuState.retiredWindow;})));
  }
  auto optionsLines = WL(getListBuilder, getStandardLineHeight());
  if (!campaignOptions.options.empty()) {
    lines.addSpace(10);
    lines.addElem(WL(leftMargin, optionMargin,
        WL(buttonLabel, "Difficulty", [&menuState] { menuState.options = !menuState.options;})));
    for (OptionId id : campaignOptions.options)
      optionsLines.addElem(
          drawOptionElem(id, [&queue, id] { queue.push({CampaignActionId::UPDATE_OPTION, id});}, none));
  }
  lines.addBackElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, nullptr,
        [&campaign](Vec2 pos) { return campaign.canEmbark(pos); }, [](Vec2) {})));
  lines.addSpace(10);
  lines.addBackElem(WL(centerHoriz, WL(getListBuilder)
        .addElemAuto(WL(conditional,
            WL(buttonLabel, "Confirm", [&] { queue.push(CampaignActionId::CONFIRM); }),
            WL(buttonLabelInactive, "Confirm"),
            [&campaign] { return !!campaign.getPlayerPos(); }))
        .addSpace(20)
        .addElemAuto(WL(buttonLabel, "Re-roll map", [&queue] { queue.push(CampaignActionId::REROLL_MAP);}))
        .addSpace(20)
        .addElemAuto(WL(buttonLabel, "Go back",
            WL(button, [&queue] { queue.push(CampaignActionId::CANCEL); }, gui.getKey(SDL::SDLK_ESCAPE))))
        .buildHorizontalList()));
  if (campaignOptions.warning)
    rightLines.addElem(WL(leftMargin, -20, drawMenuWarning(*campaignOptions.warning)));
  if (!campaignOptions.biomes.empty())
    rightLines.addElemAuto(drawBiomeMenu(queue, campaignOptions.biomes, campaignOptions.chosenBiome));
  int retiredPosX = 640;
  vector<SGuiElem> interior;
  interior.push_back(lines.buildVerticalList());
  interior.push_back(centerLines.buildVerticalList());
  interior.push_back(WL(margins, rightLines.buildVerticalList(), retiredPosX, 0, 50, 0));
  auto closeHelp = [&menuState] { menuState.helpText = false;};
  interior.push_back(
        WL(conditionalStopKeys, WL(margins, WL(miniWindow2, WL(stack,
                WL(button, closeHelp),  // to make the window closable on a click anywhere
                WL(margins, WL(labelMultiLine, campaignOptions.introText, legendLineHeight), 10)),
            closeHelp), 100, 50, 100, 280),
            [&menuState] { return menuState.helpText;}));
  auto addOverlay = [&] (GuiFactory::ListBuilder builder, bool& visible) {
    int size = 500;
    if (!builder.isEmpty())
      interior.push_back(
            WL(conditionalStopKeys, WL(translate, WL(miniWindow2,
                WL(margins, WL(scrollable, WL(topMargin, 3, builder.buildVerticalList())), 10),
                [&] { visible = false;}), Vec2(30, 70),
                    Vec2(444, 50 + size)),
            [&visible] { return visible;}));
  };
  addOverlay(std::move(retiredMenuLines), menuState.retiredWindow);
  addOverlay(std::move(optionsLines), menuState.options);
  return
      WL(preferredSize, 1000, 705,
      WL(window, WL(margins, WL(stack, std::move(interior)), 5), [&queue] { queue.push(CampaignActionId::CANCEL); }));
}

SGuiElem GuiBuilder::drawCreatureList(const vector<CreatureInfo>& creatures, function<void(UniqueEntity<Creature>::Id)> button) {
  auto minionLines = WL(getListBuilder, getStandardLineHeight() + 10);
  auto line = WL(getListBuilder, 60);
  for (auto& elem : creatures) {
    auto icon = WL(margins, WL(stack, WL(viewObject, elem.viewId, 2),
        WL(label, toString((int) elem.bestAttack.value), 20)), 5, 5, 5, 5);
    if (button)
      line.addElemAuto(WL(stack,
            WL(mouseHighlight2, WL(uiHighlight)),
            WL(button, [id = elem.uniqueId, button] { button(id); }),
            std::move(icon)));
    else
      line.addElemAuto(std::move(icon));
    if (line.getLength() > 6) {
      minionLines.addElemAuto(WL(centerHoriz, line.buildHorizontalList()));
      minionLines.addSpace(20);
      line.clear();
    }
  }
  if (!line.isEmpty()) {
    minionLines.addElemAuto(WL(centerHoriz, line.buildHorizontalList()));
    minionLines.addSpace(20);
  }
  return minionLines.buildVerticalList();
}

SGuiElem GuiBuilder::drawCreatureInfo(SyncQueue<bool>& queue, const string& title, bool prompt,
    const vector<CreatureInfo>& creatures) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, title)));
  const int windowWidth = 540;
  lines.addMiddleElem(WL(scrollable, WL(margins, drawCreatureList(creatures, nullptr), 10)));
  lines.addSpace(15);
  auto bottomLine = WL(getListBuilder)
      .addElemAuto(WL(buttonLabel, "Confirm", [&queue] { queue.push(true);}));
  if (prompt) {
    bottomLine.addSpace(20);
    bottomLine.addElemAuto(WL(buttonLabel, "Cancel", [&queue] { queue.push(false);}));
  }
  lines.addBackElem(WL(centerHoriz, bottomLine.buildHorizontalList()));
  int margin = 15;
  return WL(setWidth, 2 * margin + windowWidth,
      WL(window, WL(margins, lines.buildVerticalList(), margin), [&queue] { queue.push(false); }));
}

SGuiElem GuiBuilder::drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>& queue, const string& title,
      const vector<CreatureInfo>& team, const string& cancelText) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, title)));
  const int windowWidth = 480;
  lines.addMiddleElem(WL(scrollable, WL(margins, drawCreatureList(team, [&queue] (auto id) { queue.push(id); }), 10)));
  lines.addSpace(15);
  if (!cancelText.empty())
    lines.addBackElem(WL(centerHoriz, WL(buttonLabel, cancelText, WL(button, [&queue] { queue.push(none);}))), legendLineHeight);
  int margin = 15;
  return WL(setWidth, 2 * margin + windowWidth,
      WL(window, WL(margins, lines.buildVerticalList(), margin), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawModMenu(SyncQueue<optional<ModAction>>& queue, int highlighted, const vector<ModInfo>& mods) {
  auto localItems = WL(getListBuilder, legendLineHeight);
  auto subscribedItems = WL(getListBuilder, legendLineHeight);
  auto onlineItems = WL(getListBuilder, legendLineHeight);
  auto activeItems = WL(getListBuilder, legendLineHeight);;
  shared_ptr<int> chosenMod = make_shared<int>(highlighted);
  vector<SGuiElem> modPages;
  const int margin = 15;
  const int pageWidth = 480;
  const int listWidth = 200;
  for (int i : All(mods)) {
    auto name = mods[i].name;
    auto itemLabel =
    WL(stack,
        WL(renderInBounds, WL(label, name)),
        WL(uiHighlightConditional, [chosenMod, i] { return *chosenMod == i; }),
        WL(button, [chosenMod, i] { *chosenMod = i; })
    );
    if (mods[i].isActive)
      activeItems.addElem(std::move(itemLabel));
    else if (mods[i].isLocal)
      localItems.addElem(std::move(itemLabel));
    else if (mods[i].isSubscribed)
      subscribedItems.addElem(std::move(itemLabel));
    else
      onlineItems.addElem(std::move(itemLabel));
    auto lines = WL(getListBuilder, legendLineHeight);
    auto stars = WL(getListBuilder);
    if (mods[i].upvotes + mods[i].downvotes > 0) {
      const int maxStars = 5;
      double rating = double(mods[i].upvotes) / (mods[i].downvotes + mods[i].upvotes);
      for (int j = 0; j < 5; ++j)
        stars.addElemAuto(WL(labelUnicode, j < rating * maxStars ? "★" : "☆", Color::YELLOW));
    }
    lines.addElem(WL(getListBuilder)
        .addElemAuto(WL(label, mods[i].name))
        .addBackElemAuto(stars.buildHorizontalList())
        .buildHorizontalList());
    lines.addElem(WL(label, !mods[i].details.author.empty() ? ("by " + mods[i].details.author) : "",
        Renderer::smallTextSize, Color::LIGHT_GRAY));
    lines.addMiddleElem(WL(scrollable, WL(labelMultiLineWidth, mods[i].details.description, legendLineHeight,
        pageWidth - 2 * margin - 60)));
    lines.addSpace(10);
    auto buttons = WL(getListBuilder);
    for (int j : All(mods[i].actions)) {
      buttons.addElemAuto(
          WL(buttonLabel, mods[i].actions[j], [&queue, i, j] { queue.push(ModAction{i, j}); })
      );
      buttons.addSpace(15);
    }
    if (mods[i].versionInfo.steamId != 0) {
      buttons.addElemAuto(
          WL(buttonLabel, "Show in Steam Workshop", [id = mods[i].versionInfo.steamId] {
            openUrl("https://steamcommunity.com/sharedfiles/filedetails/?id=" + toString(id));
          })
      );
      buttons.addSpace(15);
    }
    lines.addBackElem(buttons.buildHorizontalList());
    modPages.push_back(WL(conditional,
        lines.buildVerticalList(),
        [chosenMod, i] { return *chosenMod == i; }
    ));
  }
  auto allItems = WL(getListBuilder, legendLineHeight);
  auto addList = [&] (GuiFactory::ListBuilder& list, const char* title) {
    if (!list.isEmpty()) {
      allItems.addSpace(legendLineHeight / 2);
      allItems.addElem(WL(label, title, Color::YELLOW));
      allItems.addElemAuto(list.buildVerticalList());
    }
  };
  addList(activeItems, "Active:");
  addList(localItems, "Installed:");
  addList(subscribedItems, "Subscribed:");
  addList(onlineItems, "Online:");
  allItems.addSpace(15);
  allItems.addElem(WL(buttonLabel, "Create new", [&queue] { queue.push(ModAction{-1, 0}); }));
  const int windowWidth = 2 * margin + pageWidth + listWidth;
  return WL(preferredSize, windowWidth, 400,
      WL(window, WL(margins, WL(getListBuilder)
          .addElem(WL(scrollable, WL(rightMargin, 10, allItems.buildVerticalList())), listWidth)
          .addSpace(25)
          .addMiddleElem(WL(stack, std::move(modPages)))
          .buildHorizontalList(), margin), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawHighscorePage(const HighscoreList& page, ScrollPosition *scrollPos) {
  auto lines = WL(getListBuilder, legendLineHeight);
  for (auto& elem : page.scores) {
    auto line = WL(getListBuilder);
    Color color = elem.highlight ? Color::GREEN : Color::WHITE;
    line.addElemAuto(WL(label, elem.text, color));
    line.addBackElem(WL(label, elem.score, color), 130);
    lines.addElem(WL(leftMargin, 30, line.buildHorizontalList()));
  }
  return WL(scrollable, lines.buildVerticalList(), scrollPos);
}

SGuiElem GuiBuilder::drawHighscores(const vector<HighscoreList>& list, Semaphore& sem, int& tabNum,
    vector<ScrollPosition>& scrollPos, bool& online) {
  vector<SGuiElem> pages;
  int numTabs = list.size() / 2;
  auto topLine = WL(getListBuilder, 200);
  for (int i : All(list)) {
    for (int j : All(list[i].scores))
      if (list[i].scores[j].highlight) {
        if (i < numTabs)
          tabNum = i;
        scrollPos[i] = j;
      }
    pages.push_back(WL(conditional, drawHighscorePage(list[i], &scrollPos[i]),
          [&tabNum, i, &online, numTabs] { return (online && tabNum == i - numTabs) ||
              (!online && tabNum == i) ; }));
    if (i < numTabs)
    topLine.addElem(WL(stack,
        WL(margins, WL(uiHighlightConditional, [&tabNum, i] { return tabNum == i;}), 42, 0, 32, 0),
        WL(centeredLabel, Renderer::HOR, list[i].name),
        WL(button, [&tabNum, i] { tabNum = i;})));
  }
  SGuiElem onlineBut = WL(stack,
      WL(label, "Online", [&online] { return online ? Color::GREEN : Color::WHITE;}),
      WL(button, [&online] { online = !online; }));
  Vec2 size = getMenuPosition(MenuType::NORMAL, 0).getSize();
  return WL(preferredSize, size, WL(stack, makeVec(
      WL(keyHandler, [&tabNum, numTabs] { tabNum = (tabNum + 1) % numTabs; }, {gui.getKey(SDL::SDLK_RIGHT)}),
      WL(keyHandler, [&tabNum, numTabs] { tabNum = (tabNum + numTabs - 1) % numTabs; }, {gui.getKey(SDL::SDLK_LEFT)}),
      WL(window,
        WL(margin, WL(leftMargin, 25, std::move(onlineBut)),
        WL(topMargin, 30, WL(margin, WL(leftMargin, 5, topLine.buildHorizontalListFit()),
            WL(margins, WL(stack, std::move(pages)), 25, 60, 0, 30), legendLineHeight, GuiFactory::TOP)), legendLineHeight, GuiFactory::TOP),
        [&] { sem.v(); }))));

}

SGuiElem GuiBuilder::drawZLevelButton(const CurrentLevelInfo& info, Color textColor) {
  auto name = info.name.value_or("Z-Level " + toString(info.levelDepth));
  return WL(stack,
      WL(centerHoriz, WL(labelHighlight, name, textColor)),
      info.zLevels.empty() ? WL(empty) : WL(buttonRect, [this, info] (Rectangle bounds) {
          auto tasks = WL(getListBuilder, legendLineHeight);
          bool exit = false;
          auto retAction = [&] (optional<int> index) {
            if (index)
              callbacks.input(UserInput{UserInputId::SCROLL_STAIRS, *index - info.levelDepth});
          };
          int maxWidth = 0;
          for (int i : All(info.zLevels)) {
            auto elem = WL(stack,
                WL(button, [i, &retAction, &exit] { retAction(i); exit = true; }),
                WL(labelHighlight, info.zLevels[i].value_or("Z-Level " + toString(i))));
            maxWidth = max(maxWidth, *elem->getPreferredWidth());
            tasks.addElem(WL(centerHoriz, std::move(elem)));
          }
          drawMiniMenu(std::move(tasks), exit, Vec2(bounds.middle().x - maxWidth / 2 - 15, bounds.bottom()),
              maxWidth, false);
        }));
}

SGuiElem GuiBuilder::drawMinimapIcons(const GameInfo& gameInfo) {
  auto tutorialPredicate = [&gameInfo] {
    return gameInfo.tutorial && gameInfo.tutorial->highlights.contains(TutorialHighlight::MINIMAP_BUTTONS);
  };
  Color textColor(209, 181, 130);
  auto lines = WL(getListBuilder, legendLineHeight);
  if (auto& info = gameInfo.currentLevel) {
    auto getButton = [&](bool enabled, string label, UserInput inputId) {
      auto ret = WL(preferredSize, legendLineHeight, legendLineHeight, WL(stack,
          WL(margins, WL(rectangle, Color(56, 36, 0), Color(57, 41, 0)), 2),
          WL(centerHoriz, WL(topMargin, -2, WL(mouseHighlight2,
                                        WL(label, label, 24, enabled ? Color::YELLOW : Color::DARK_GRAY),
                                        WL(label, label, 24, enabled ? textColor : Color::DARK_GRAY))))
      ));
      if (enabled)
        ret = WL(stack,
            std::move(ret),
            WL(button, getButtonCallback(inputId)));
      return ret;
    };
    auto line = WL(getListBuilder);
    if (!info->zLevels.empty())
      line.addElemAuto(getButton(info->levelDepth > 0, "<", UserInput{UserInputId::SCROLL_STAIRS, -1 }));
    line.addMiddleElem(WL(topMargin, 3, drawZLevelButton(*info, textColor)));
    if (!info->zLevels.empty())
      line.addBackElemAuto(getButton(info->levelDepth < info->zLevels.size() - 1, ">",
          UserInput{UserInputId::SCROLL_STAIRS, 1}));
    lines.addElem(WL(stack,
        WL(stopMouseMovement),
        WL(rectangle, Color(47, 31, 0), Color::BLACK),
        line.buildHorizontalList()
    ));
  }
  auto travelButton = [&] {
    if (gameInfo.tutorial || !gameInfo.isSingleMap)
      return WL(stack,
          getHintCallback({"Open world map. You can also press 't'."}),
          WL(mouseHighlight2, WL(icon, GuiFactory::IconId::MINIMAP_WORLD2),
             WL(icon, GuiFactory::IconId::MINIMAP_WORLD1)),
          WL(conditional, WL(blink, WL(icon, GuiFactory::IconId::MINIMAP_WORLD2)), tutorialPredicate),
          WL(button, getButtonCallback(UserInputId::DRAW_WORLD_MAP), gui.getKey(SDL::SDLK_t)));
    else
      return WL(icon, GuiFactory::IconId::MINIMAP_WORLD1);
  }();
  return lines.addElemAuto(
      WL(centerHoriz, WL(minimapBar,
        WL(preferredSize, 48, 48, std::move(travelButton)),
        WL(preferredSize, 48, 48, WL(stack,
            getHintCallback({"Scroll to your character. You can also press 'k'."}),
            WL(mouseHighlight2, WL(icon, GuiFactory::IconId::MINIMAP_CENTER2),
               WL(icon, GuiFactory::IconId::MINIMAP_CENTER1)),
            WL(conditional, WL(blink, WL(icon, GuiFactory::IconId::MINIMAP_CENTER2)), tutorialPredicate),
            WL(button, getButtonCallback(UserInputId::SCROLL_TO_HOME), gui.getKey(SDL::SDLK_k))
            ))
  ))).buildVerticalList();
}

Rectangle GuiBuilder::getTextInputPosition() {
  Vec2 center = renderer.getSize() / 2;
  return Rectangle(center - Vec2(300, 129), center + Vec2(300, 129));
}

SGuiElem GuiBuilder::getTextContent(const string& title, const string& value, const string& hint) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, capitalFirst(title)));
  lines.addElem(
      WL(variableLabel, [&] { return value + "_"; }, legendLineHeight), 2 * legendLineHeight);
  if (!hint.empty())
    lines.addElem(WL(label, hint, gui.inactiveText));
  return lines.buildVerticalList();
}

optional<string> GuiBuilder::getTextInput(const string& title, const string& value, int maxLength,
    const string& hint) {
  bool dismiss = false;
  string text = value;
  SGuiElem dismissBut = WL(margins, WL(stack, makeVec(
        WL(button, [&](){ dismiss = true; }),
        WL(mouseHighlight2, WL(mainMenuHighlight)),
        WL(centerHoriz,
            WL(label, "Dismiss", Color::WHITE), renderer.getTextLength("Dismiss")))), 0, 5, 0, 0);
  SGuiElem stuff = WL(margins, getTextContent(title, text, hint), 30, 50, 0, 0);
  stuff = WL(margin, WL(centerHoriz, std::move(dismissBut), renderer.getTextLength("Dismiss") + 100),
    std::move(stuff), 30, gui.BOTTOM);
  stuff = WL(window, std::move(stuff), [&dismiss] { dismiss = true; });
  SGuiElem bg = WL(darken);
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
  auto levelBounds = view->getCreatureViewLevel()->getBounds();
  miniMap->update(levelBounds, view, renderer);
  return WL(preferredSize, 630, 630,
      WL(miniWindow, WL(stack,
          WL(buttonPos, [=, &sem](Rectangle bounds, Vec2 pos) {
              mapGui->setCenter(levelBounds.width() * pos.x / bounds.width(),
                  levelBounds.height() * pos.y / bounds.height());
              sem.v(); }),
          WL(drawCustom, [=](Renderer& r, Rectangle bounds) {
              miniMap->renderMap(r, bounds);
          })
      ), [&sem] { sem.v(); }));
}
