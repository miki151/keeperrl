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
#include "user_input.h"
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
#include "team_order.h"
#include "lasting_effect.h"
#include "player_role.h"
#include "tribe_alignment.h"
#include "avatar_menu_option.h"
#include "view_object_action.h"
#include "mod_info.h"
#include "special_trait.h"
#include "encyclopedia.h"
#include "item_action.h"
#include "ai_type.h"
#include "container_range.h"
#include "keybinding_map.h"
#include "steam_input.h"
#include "tutorial_state.h"
#include "campaign_menu_index.h"
#include "tileset.h"
#include "zones.h"

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
  villainsIndex = none;
  techIndex = none;
  helpIndex = none;
  minionsIndex = none;
  inventoryIndex = none;
  abilityIndex = none;
}

bool GuiBuilder::isEnlargedMinimap() const {
  return bottomWindow == BottomWindowId::ENLARGED_MINIMAP;
}

void GuiBuilder::toggleEnlargedMinimap() {
  if (isEnlargedMinimap())
    bottomWindow = none;
  else
    bottomWindow = BottomWindowId::ENLARGED_MINIMAP;
}

void GuiBuilder::closeOverlayWindowsAndClearButton() {
  closeOverlayWindows();
  clearActiveButton();
}

optional<int> GuiBuilder::getActiveButton() const {
  return activeButton;
}

void GuiBuilder::setActiveGroup(const string& group, optional<TutorialHighlight> tutorial) {
  closeOverlayWindowsAndClearButton();
  activeGroup = group;
  if (tutorial)
    onTutorialClicked(combineHash(group), *tutorial);
}

void GuiBuilder::setActiveButton(int num, ViewId viewId, optional<string> group,
    optional<TutorialHighlight> tutorial, bool buildingSelected) {
  closeOverlayWindowsAndClearButton();
  activeButton = num;
  mapGui->setActiveButton(viewId, num, buildingSelected);
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
    mapGui->clearActiveButton();
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

SGuiElem GuiBuilder::getButtonLine(CollectiveInfo::Button button, int num, const optional<TutorialInfo>& tutorial) {
  auto getValue = [this] (CollectiveInfo::Button button, int num, const optional<TutorialInfo>& tutorial) {
    auto line = WL(getListBuilder);
    line.addElem(WL(viewObject, button.viewId), 35);
    auto tutorialHighlight = button.tutorialHighlight;
    if (button.state != CollectiveInfo::Button::ACTIVE)
      line.addElemAuto(WL(label, button.name + " " + button.count, Color::GRAY));
    else {
      line.addElemAuto(WL(label, button.name + " " + button.count, Color::WHITE));
    }
    if (button.cost)
      line.addBackElemAuto(drawCost(*button.cost));
    function<void()> buttonFun;
    ViewId viewId = button.viewId;
    if (button.state != CollectiveInfo::Button::INACTIVE)
      buttonFun = [=, building = button.isBuilding] {
        if (getActiveButton() == num)
          clearActiveButton();
        else {
          setCollectiveTab(CollectiveTab::BUILDINGS);
          setActiveButton(num, viewId, none, tutorialHighlight, building);
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
        WL(button, buttonFun),
        !button.hotkeyOpensGroup && !!button.key
            ? WL(keyHandler, buttonFun, *button.key, true)
            : WL(empty),
        WL(uiHighlightFrameFilled, [=] { return getActiveButton() == num; }),
        tutorialElem,
        line.buildHorizontalList())));
  };
  return cache->get(getValue, THIS_LINE, button, num, tutorial);
}

static optional<int> getFirstActive(const vector<CollectiveInfo::Button>& buttons, int begin) {
  for (int i = begin; i < buttons.size() && buttons[i].groupName == buttons[begin].groupName; ++i)
    if (buttons[i].state == CollectiveInfo::Button::ACTIVE)
      return i;
  return none;
}

static optional<int> getNextActive(const vector<CollectiveInfo::Button>& buttons, int begin,
    optional<int> current, int direction) {
  if (!current)
    return getFirstActive(buttons, begin);
  CHECK(*current >= begin);
  int i = *current;
  do {
    i += direction;
    if (i < 0 || i >= buttons.size() || buttons[i].groupName != buttons[begin].groupName) {
      if (direction == 1)
        i = begin;
      else {
        int ind = begin;
        while (ind + 1 < buttons.size() && buttons[ind + 1].groupName == buttons[begin].groupName)
          ++ind;
        i = ind;
      }
    } if (current == i)
      return none;
  } while (buttons[i].state != CollectiveInfo::Button::ACTIVE);
  return i;
}

SGuiElem GuiBuilder::drawBuildings(const vector<CollectiveInfo::Button>& buttons, const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> keypressOnly;
  auto elems = WL(getListBuilder, legendLineHeight);
  elems.addSpace(5);
  string lastGroup;
  keypressOnly.push_back(WL(conditionalStopKeys, WL(stack,
          WL(keyHandler, [this, buttons] {
            clearActiveButton();
          }, {gui.getKey(C_BUILDINGS_CANCEL)}, true),
          WL(keyHandler, [this, buttons] {
            if (!!activeButton && !!activeGroup)
              setActiveGroup(buttons[*activeButton].groupName, none);
            else if (!!activeButton)
              activeGroup = buttons[*activeButton].groupName;
            else {
              clearActiveButton();
            }
          }, {gui.getKey(C_BUILDINGS_LEFT)}, true)
      ),
      [this] {
        return collectiveTab == CollectiveTab::BUILDINGS && (!!activeGroup || !!activeButton);
      }
  ));
  keypressOnly.push_back(
      WL(keyHandler, [this, newGroup = buttons[0].groupName] {
        closeOverlayWindowsAndClearButton();
        setCollectiveTab(CollectiveTab::BUILDINGS);
        setActiveGroup(newGroup, none);
      }, {gui.getKey(C_BUILDINGS_MENU)}, true));
  keypressOnly.push_back(
      WL(keyHandlerBool, [this, newGroup = buttons[0].groupName] {
        if (collectiveTab == CollectiveTab::BUILDINGS && !activeButton && !activeGroup) {
          closeOverlayWindows();
          setActiveGroup(newGroup, none);
          return true;
        }
        return false;
      }, {gui.getKey(C_BUILDINGS_DOWN), gui.getKey(C_BUILDINGS_UP)}));
  for (int i : All(buttons)) {
    if (!buttons[i].groupName.empty() && buttons[i].groupName != lastGroup) {
      optional<TutorialHighlight> tutorialHighlight;
      if (tutorial)
        for (int j = i; j < buttons.size() && buttons[i].groupName == buttons[j].groupName; ++j)
          if (auto highlight = buttons[j].tutorialHighlight)
            if (tutorial->highlights.contains(*highlight))
              tutorialHighlight = *highlight;
      keypressOnly.push_back(WL(conditionalStopKeys,
          WL(keyHandler, [this, tutorialHighlight, newGroup = buttons[i].groupName, i, buttons, lastGroup] {
            optional<string> group;
            int button;
            if (!activeButton) {
              group = newGroup;
              if (auto firstBut = getNextActive(buttons, i, none, 1))
                button = *firstBut;
              else return;
            } else
              button = *activeButton;
            setActiveButton(button, buttons[button].viewId, group, tutorialHighlight, buttons[button].isBuilding);
          }, {gui.getKey(C_BUILDINGS_CONFIRM), gui.getKey(C_BUILDINGS_RIGHT)}, true),
          [this, newGroup = buttons[i].groupName] {
            return collectiveTab == CollectiveTab::BUILDINGS && activeGroup == newGroup;
          }
      ));
      keypressOnly.push_back(WL(conditionalStopKeys,
          WL(keyHandler, [this, tutorialHighlight, newGroup = buttons[i].groupName, i, buttons] {
            if (auto newBut = getNextActive(buttons, i, getActiveButton(), 1))
              setActiveButton(*newBut, buttons[*newBut].viewId, newGroup, tutorialHighlight, buttons[*newBut].isBuilding);
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          [this, newGroup = buttons[i].groupName] {
            return collectiveTab == CollectiveTab::BUILDINGS && !!activeButton && activeGroup == newGroup;
          }
      ));
      keypressOnly.push_back(WL(conditionalStopKeys,
          WL(keyHandler, [this, tutorialHighlight, newGroup = buttons[i].groupName, i, buttons] {
            if (auto newBut = getNextActive(buttons, i, getActiveButton(), -1))
              setActiveButton(*newBut, buttons[*newBut].viewId, newGroup, tutorialHighlight, buttons[*newBut].isBuilding);
          }, {gui.getKey(C_BUILDINGS_UP)}, true),
          [this, newGroup = buttons[i].groupName] {
            return collectiveTab == CollectiveTab::BUILDINGS && activeGroup == newGroup;
          }
      ));
      keypressOnly.push_back(WL(conditionalStopKeys,
          WL(keyHandler, [this, tutorialHighlight, newGroup = buttons[i].groupName] {
            setActiveGroup(newGroup, tutorialHighlight);
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          [this, lastGroup, backGroup = buttons.back().groupName] {
            return collectiveTab == CollectiveTab::BUILDINGS && !activeButton && (
                activeGroup == lastGroup || (activeGroup == backGroup && lastGroup.empty()));
          }
      ));
      keypressOnly.push_back(WL(conditionalStopKeys,
          WL(keyHandler, [this, tutorialHighlight, nextGroup = lastGroup.empty() ? buttons.back().groupName : lastGroup] {
            setActiveGroup(nextGroup, tutorialHighlight);
          }, {gui.getKey(C_BUILDINGS_UP)}, true),
          [this, newGroup = buttons[i].groupName] {
            return collectiveTab == CollectiveTab::BUILDINGS && activeGroup == newGroup && !activeButton;
          }
      ));
      lastGroup = buttons[i].groupName;
      function<void()> buttonFunHotkey = [=] {
        if (activeGroup != lastGroup) {
          setCollectiveTab(CollectiveTab::BUILDINGS);
          if (auto firstBut = getNextActive(buttons, i, none, 1))
            setActiveButton(*firstBut, buttons[*firstBut].viewId, lastGroup, tutorialHighlight,
                buttons[*firstBut].isBuilding);
          else
            setActiveGroup(lastGroup, tutorialHighlight);
        } else
        if (auto newBut = getNextActive(buttons, i, getActiveButton(), 1))
          setActiveButton(*newBut, buttons[*newBut].viewId, lastGroup, tutorialHighlight, buttons[*newBut].isBuilding);
        else
          clearActiveButton();
      };
      function<void()> labelFun = [=] {
        if (activeGroup != lastGroup) {
          setCollectiveTab(CollectiveTab::BUILDINGS);
          setActiveGroup(lastGroup, tutorialHighlight);
        } else {
          clearActiveButton();
        }
      };
      auto line = WL(getListBuilder);
      line.addElem(WL(viewObject, buttons[i].viewId), 35);
      optional<Keybinding> key;
      if (buttons[i].hotkeyOpensGroup)
        key = buttons[i].key;
      SGuiElem tutorialElem = WL(empty);
      if (tutorialHighlight)
        tutorialElem = WL(conditional, WL(tutorialHighlight),
             [=]{ return !wasTutorialClicked(combineHash(lastGroup), *tutorialHighlight); });
      line.addElemAuto(WL(label, lastGroup, Color::WHITE));
      elems.addElem(WL(stack, makeVec(
          buttons[i].key ? WL(keyHandler, buttonFunHotkey, *buttons[i].key, true) : WL(empty),
          WL(button, labelFun),
          tutorialElem,
          WL(uiHighlightLineConditional, [=] { return activeGroup == lastGroup;}),
          WL(uiHighlightFrame, [=] { return activeGroup == lastGroup && !activeButton;}),
          line.buildHorizontalList())));
    }
    CHECK(!buttons[i].groupName.empty());
    keypressOnly.push_back(WL(invisible, getButtonLine(buttons[i], i, tutorial)));
  }
  keypressOnly.push_back(elems.buildVerticalList());
  return WL(stopScrollEvent,
      WL(scrollable, WL(stack, std::move(keypressOnly)), &buildingsScroll, &scrollbarsHeld),
      [this] { return collectiveTab != CollectiveTab::BUILDINGS || (!activeGroup && !activeButton); });
}

SGuiElem GuiBuilder::drawKeeperHelp(const GameInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  int buttonCnt = 0;
  auto addScriptedButton = [this, &lines, &buttonCnt] (const ScriptedHelpInfo& info) {
    lines.addElem(WL(buttonLabelFocusable,
        WL(getListBuilder)
            .addElemAuto(WL(topMargin, -2, WL(viewObject, *info.viewId)))
            .addSpace(5)
            .addElemAuto(WL(label, *info.title))
            .buildHorizontalList(),
        [this, scriptedId = info.scriptedId]() {
          scriptedUIState.scrollPos[0].reset();
          if (bottomWindow == SCRIPTED_HELP && scriptedHelpId == scriptedId)
            bottomWindow = none;
          else
            openScriptedHelp(scriptedId);
        },
        [this, buttonCnt] { return helpIndex == buttonCnt; }
    ));
    ++buttonCnt;
    lines.addSpace(5);
  };
  constexpr int numBuiltinPages = 6;
  for (auto elem : Iter(info.scriptedHelp))
    if (elem.index() < numBuiltinPages && !!elem->viewId && !!elem->title)
      addScriptedButton(*elem);
  lines.addSpace(15);
  auto addBuiltinButton = [this, &lines, &buttonCnt] (ViewId viewId, string name, BottomWindowId windowId) {
    lines.addElem(WL(buttonLabelFocusable,
        WL(getListBuilder)
            .addElemAuto(WL(topMargin, -2, WL(viewObject, viewId)))
            .addElemAuto(WL(label, name))
            .buildHorizontalList(),
        [this, windowId]() { toggleBottomWindow(windowId); },
        [this, buttonCnt] { return helpIndex == buttonCnt; }
    ));
    ++buttonCnt;
    lines.addSpace(5);
  };
  addBuiltinButton(ViewId("special_bmbw"), "Bestiary", BESTIARY);
  addBuiltinButton(ViewId("scroll"), "Items", ITEMS_HELP);
  addBuiltinButton(ViewId("book"), "Spell schools", SPELL_SCHOOLS);
  lines.addSpace(10);
  for (auto elem : Iter(info.scriptedHelp))
    if (elem.index() >= numBuiltinPages && !!elem->viewId && !!elem->title)
      addScriptedButton(*elem);
  return WL(stack, makeVec(
      WL(keyHandler, [this] {
        closeOverlayWindowsAndClearButton();
        helpIndex = 0;
        setCollectiveTab(CollectiveTab::KEY_MAPPING);
      }, {gui.getKey(C_HELP_MENU)}, true),
      WL(conditionalStopKeys, WL(stack,
          lines.buildVerticalList(),
          WL(keyHandler, [this] {
            helpIndex = none;
          }, {gui.getKey(C_BUILDINGS_CANCEL)}, true),
          WL(keyHandler, [this, buttonCnt] {
            if (!helpIndex) {
              closeOverlayWindowsAndClearButton();
              helpIndex = 0;
            } else
              helpIndex = (*helpIndex + 1) % buttonCnt;
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          WL(keyHandler, [this, buttonCnt] {
            if (!helpIndex) {
              closeOverlayWindowsAndClearButton();
              helpIndex = 0;
            } else
              helpIndex = (*helpIndex - 1 + buttonCnt) % buttonCnt;
          }, {gui.getKey(C_BUILDINGS_UP)}, true)
      ), [this] { return collectiveTab == CollectiveTab::KEY_MAPPING; })
  ));
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
  int numResourcesShown = min(numResource.size(), width / resourceSpace);
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
    if (i < numResourcesShown - 1)
      line.addElem(std::move(elem));
    else if (i == numResourcesShown - 1)
      line.addElemAuto(std::move(elem));
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
  auto getIconHighlight = [&] (Color c) { return WL(topMargin, -1, WL(uiHighlight, c.transparency(120))); };
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
      buttons[i] = WL(stack, makeVec(
          WL(conditional, getIconHighlight(GuiFactory::highlightColor()), [this, i] {
            return int(collectiveTab) == i;
          }),
          std::move(buttons[i]),
          WL(button, [this, i]() { setCollectiveTab(CollectiveTab(i)); })
      ));
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
        (collectiveInfo.avatarLevelInfo.numAvailable > 0 || collectiveInfo.availablePromotions > 0))
      buttons[2] = WL(stack, WL(conditional,
          getIconHighlight(Color::YELLOW),
          [this] { return collectiveTab != CollectiveTab::TECHNOLOGY;}),
          buttons[2]);
    if (!!info.tutorial && info.tutorial->highlights.contains(TutorialHighlight::HELP_TAB))
      buttons[3] = WL(stack,
            WL(conditional,
                WL(blink, getIconHighlight(Color::YELLOW)),
                [this] { return collectiveTab != CollectiveTab::KEY_MAPPING;}),
            buttons[3]);
    vector<pair<CollectiveTab, SGuiElem>> elems = makeVec(
        make_pair(CollectiveTab::MINIONS, drawMinions(collectiveInfo, minionsIndex, info.tutorial)),
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
    auto speedMenu = [&] (Rectangle rect) {
      vector<SGuiElem> lines;
      vector<function<void()>> callbacks;
      int selected = EnumInfo<GameSpeed>::size;
      for (GameSpeed speed : ENUM_ALL_REVERSE(GameSpeed)) {
        if (gameSpeed == speed)
          selected = callbacks.size();
        callbacks.push_back([this, speed] { gameSpeed = speed; clock->cont();});
        lines.push_back(WL(label, getGameSpeedName(speed)));
      }
      if (clock->isPaused())
        selected = callbacks.size();
      lines.push_back(WL(label, "pause"));
      callbacks.push_back([this] {
        if (clock->isPaused())
          clock->cont();
        else
          clock->pause();
      });
      drawMiniMenu(std::move(lines), std::move(callbacks), {}, rect.topLeft() - Vec2(0, 180), 190, true,
          true, &selected);
    };
    bottomLine.addElemAuto(WL(stack,
        WL(getListBuilder)
            .addElemAuto(WL(label, "speed: "))
            .addElem(WL(labelFun, [this] { return getCurrentGameSpeedName();},
              [this] { return clock->isPaused() ? Color::RED : Color::WHITE; }), 70).buildHorizontalList(),
        WL(buttonRect, speedMenu),
        getGameSpeedHotkeys()
    ));
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
  return rightBandInfoCache;
}

GuiBuilder::GameSpeed GuiBuilder::getGameSpeed() const {
  return gameSpeed;
}

void GuiBuilder::setGameSpeed(GameSpeed speed) {
  gameSpeed = speed;
}

static Keybinding getHotkey(GuiBuilder::GameSpeed speed) {
  switch (speed) {
    case GuiBuilder::GameSpeed::SLOW: return Keybinding("SPEED_SLOW");
    case GuiBuilder::GameSpeed::NORMAL: return Keybinding("SPEED_NORMAL");
    case GuiBuilder::GameSpeed::FAST: return Keybinding("SPEED_FAST");
    case GuiBuilder::GameSpeed::VERY_FAST: return Keybinding("SPEED_VERY_FAST");
  }
}

SGuiElem GuiBuilder::getGameSpeedHotkeys() {
  auto pauseFun = [this] {
    if (clock->isPaused())
      clock->cont();
    else
      clock->pause();
  };
  vector<SGuiElem> hotkeys;
  hotkeys.push_back(WL(keyHandler, pauseFun, Keybinding("PAUSE")));
  hotkeys.push_back(WL(keyHandler, [this] {
    if (int(gameSpeed) < EnumInfo<GameSpeed>::size - 1)
      gameSpeed = GameSpeed(int(gameSpeed) + 1);
  }, Keybinding("SPEED_UP")));
  hotkeys.push_back(WL(keyHandler, [this] {
    if (int(gameSpeed) > 0)
      gameSpeed = GameSpeed(int(gameSpeed) - 1);
  }, Keybinding("SPEED_DOWN")));
  for (GameSpeed speed : ENUM_ALL(GameSpeed)) {
    auto speedFun = [=] { gameSpeed = speed; clock->cont();};
    hotkeys.push_back(WL(keyHandler, speedFun, getHotkey(speed)));
  }
  return WL(stack, hotkeys);
}

SGuiElem GuiBuilder::drawSpecialTrait(const ImmigrantDataInfo::SpecialTraitInfo& trait) {
  return WL(label, trait.label, trait.bad ? Color::RED : Color::GREEN);
}

void GuiBuilder::openScriptedHelp(string id) {
  bottomWindow = SCRIPTED_HELP;
  scriptedUIState = ScriptedUIState{};
  scriptedHelpId = id;
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
          .addElemAuto(WL(label, capitalFirst(info.creature.name)))
          .addSpace(100)
          .addBackElemAuto(info.cost ? drawCost(*info.cost) : WL(empty))
          .buildHorizontalList());
  lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(info.creature.attributes)));
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

bool GuiBuilder::hasController() const {
  return !gui.getSteamInput()->controllers.empty();
}

SGuiElem GuiBuilder::drawTutorialOverlay(const TutorialInfo& info) {
  auto continueCallback = [this] {
    callbacks.input(UserInputId::TUTORIAL_CONTINUE);
    tutorialClicks.clear();
    closeOverlayWindowsAndClearButton();
  };
  auto continueButton = WL(setHeight, legendLineHeight, WL(buttonLabelBlink, "Continue", continueCallback));
  auto backButton = WL(setHeight, legendLineHeight, WL(buttonLabel, "Go back",
      getButtonCallback(UserInputId::TUTORIAL_GO_BACK)));
  SGuiElem warning;
  if (info.warning)
    warning = WL(label, *info.warning, Color::RED);
  else
    warning = WL(label, "Press "_s + (hasController() ? "[Y]" : "[Space]") + " to unpause.",
        [this]{ return clock->isPaused() ? Color::RED : Color::TRANSPARENT;});
  return WL(preferredSize, 520, 290, WL(stack, WL(darken), WL(rectangleBorder, Color::GRAY),
      WL(margins, WL(stack,
        WL(labelMultiLine, getMessage(info.state, hasController()), legendLineHeight),
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
      case VillainType::PLAYER: return Color::GREEN;
      default: return Color::GRAY;
    }
  };
  return WL(label, getName(type), getColor(type));
}

static const char* getVillageActionText(VillageAction action) {
  switch (action) {
    case VillageAction::TRADE:
      return "trade";
    case VillageAction::PILLAGE:
      return "pillage";
  }
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
  if (showDismissHint)
    lines.addElemAuto(getVillainDismissHint(info.action));
  return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
}

SGuiElem GuiBuilder::getVillainDismissHint(optional<VillageAction> action) {
  auto lines = WL(getListBuilder, legendLineHeight);
  if (!hasController())
    lines.addElem(WL(label, "Right click to clear", Renderer::smallTextSize()), legendLineHeight * 2 / 3);
  else if (!villainsIndex)
    lines.addElem(WL(label, "Left trigger to clear", Renderer::smallTextSize()), legendLineHeight * 2 / 3);
  else {
    auto line = WL(getListBuilder)
        .addElemAuto(gui.steamInputGlyph(C_ZOOM, 16))
        .addElemAuto(WL(label, " to clear", Renderer::smallTextSize()));
    if (!!action)
      line.addSpace(30)
          .addElemAuto(gui.steamInputGlyph(C_BUILDINGS_CONFIRM, 16))
          .addElemAuto(WL(label, " to "_s + getVillageActionText(*action), Renderer::smallTextSize()));
    lines.addElem(std::move(line).buildHorizontalList(), legendLineHeight * 2 / 3);
  }
  return lines.buildVerticalList();
}

static Color getNextWaveColor(const CollectiveInfo::NextWave& info) {
  if (info.numTurns.getVisibleInt() < 100)
    return Color::RED;
  if (info.numTurns.getVisibleInt() < 500)
    return Color::ORANGE;
  return Color::YELLOW;
}

SGuiElem GuiBuilder::drawNextWaveOverlay(const CollectiveInfo::NextWave& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, capitalFirst(info.attacker)));
  lines.addElem(WL(label, "Attacking in " + toString(info.numTurns.getVisibleInt()) + " turns!",
      getNextWaveColor(info)));
  lines.addElemAuto(getVillainDismissHint(none));
  return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
}

static Color getRebellionChanceColor(CollectiveInfo::RebellionChance chance) {
  switch (chance) {
    case CollectiveInfo::RebellionChance::HIGH:
      return Color::RED;
    case CollectiveInfo::RebellionChance::MEDIUM:
      return Color::ORANGE;
    case CollectiveInfo::RebellionChance::LOW:
      return Color::YELLOW;
  }
}

static const char* getRebellionChanceText(CollectiveInfo::RebellionChance chance) {
  switch (chance) {
    case CollectiveInfo::RebellionChance::HIGH:
      return "high";
    case CollectiveInfo::RebellionChance::MEDIUM:
      return "medium";
    case CollectiveInfo::RebellionChance::LOW:
      return "low";
  }
}

SGuiElem GuiBuilder::drawRebellionOverlay(const CollectiveInfo::RebellionChance& rebellionChance) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, "Chance of prisoner escape: "))
      .addElemAuto(WL(label, getRebellionChanceText(rebellionChance), getRebellionChanceColor(rebellionChance)))
      .buildHorizontalList());
  lines.addElem(WL(label, "Remove prisoners or increase armed forces."));
  lines.addElemAuto(getVillainDismissHint(none));
  return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
}

SGuiElem GuiBuilder::drawVillainsOverlay(const VillageInfo& info, const optional<CollectiveInfo::NextWave>& nextWave,
    const optional<CollectiveInfo::RebellionChance>& rebellionChance, optional<int> villainsIndexDummy) {
  const int labelOffsetY = 3;
  auto lines = WL(getListBuilder);
  lines.addSpace(50);
  int buttonsCnt = 0;
  auto getHighlight = [&] {
    return WL(margins, WL(translate,
        WL(uiHighlightConditional,
            [this, buttonsCnt] { return villainsIndex == buttonsCnt; },
            GuiFactory::highlightColor(200)),
        Vec2(0, labelOffsetY)), 0, -1, 0, 5);
  };
  if (!info.villages.empty()) {
    auto callback = [this, info] (Rectangle rect) { drawAllVillainsMenu(rect.topLeft(), info); };
    lines.addElemAuto(WL(stack,
        WL(leftMargin, -10, getHighlight()),
        WL(getListBuilder)
            .addElem(WL(icon, GuiFactory::EXPAND_UP), 12)
            .addElemAuto(WL(translate, WL(label, "All villains"), Vec2(0, labelOffsetY)))
            .addSpace(10)
            .buildHorizontalList(),
        WL(buttonRect, callback),
        WL(conditionalStopKeys, WL(keyHandlerRect, callback, {gui.getKey(C_BUILDINGS_CONFIRM)}, true),
            [this, buttonsCnt] { return villainsIndex == buttonsCnt; })
    ));
    ++buttonsCnt;
  } else if (!nextWave)
    return gui.empty();
  auto addVillainButton = [&](function<void()> dismissCallback, function<void()> actionCallback, SGuiElem label,
      const ViewIdList& viewId, SGuiElem infoOverlay) {
    auto infoOverlaySize = *infoOverlay->getPreferredSize();
    lines.addElemAuto(WL(stack, makeVec(
        getHighlight(),
        WL(button, actionCallback),
        WL(releaseRightButton, dismissCallback),
        WL(conditionalStopKeys, WL(stack,
                WL(keyHandler, dismissCallback, {gui.getKey(C_ZOOM)}, true),
                WL(keyHandler, actionCallback, {gui.getKey(C_BUILDINGS_CONFIRM)}, true)
            ), [this, buttonsCnt] { return villainsIndex == buttonsCnt; }),
        WL(onMouseRightButtonHeld, WL(margins, WL(rectangle, Color(255, 0, 0, 100)), 0, 2, 2, 2)),
        WL(getListBuilder)
            .addElemAuto(WL(stack,
                 WL(setWidth, 34, WL(centerVert, WL(centerHoriz, WL(bottomMargin, -3,
                     WL(viewObject, ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
                 WL(setWidth, 34, WL(centerVert, WL(centerHoriz, WL(bottomMargin, 5,
                     WL(viewObject, viewId)))))))
            .addElemAuto(WL(rightMargin, 5, WL(translate, std::move(label), Vec2(-2, labelOffsetY))))
            .buildHorizontalList(),
        WL(conditional, WL(preferredSize, 5, 5, WL(translate, infoOverlay, Vec2(0, -infoOverlaySize.y), infoOverlaySize)),
            [this, buttonsCnt] { return villainsIndex == buttonsCnt; }),
        WL(conditional, getTooltip2(infoOverlay, [=](const Rectangle& r) { return r.topLeft() - Vec2(0, 0 + infoOverlaySize.y);}),
            [this]{return !bottomWindow && !villainsIndex;})
    )));
    ++buttonsCnt;
  };
  if (rebellionChance)
    addVillainButton(
        getButtonCallback({UserInputId::DISMISS_WARNING_WINDOW}),
        []{},
        WL(label, "rebellion risk", getRebellionChanceColor(*rebellionChance)),
        {ViewId("prisoner")},
        drawRebellionOverlay(*rebellionChance)
    );
  if (nextWave)
    addVillainButton(
        getButtonCallback({UserInputId::DISMISS_NEXT_WAVE}),
        []{},
        WL(label, "next enemy wave", getNextWaveColor(*nextWave)),
        nextWave->viewId,
        drawNextWaveOverlay(*nextWave)
    );
  for (int i : All(info.villages)) {
    SGuiElem label;
    string labelText;
    function<void()> actionCallback = [](){};
    auto& elem = info.villages[i];
    if (elem.attacking) {
      labelText = "attacking";
      label = WL(label, labelText, Color::RED);
    } else if (!elem.triggers.empty()) {
      labelText = "triggered";
      label = WL(label, labelText, Color::ORANGE);
    } else if (elem.action) {
      labelText = getVillageActionText(*elem.action);
      label = WL(label, labelText, Color::GREEN);
      actionCallback = getButtonCallback({UserInputId::VILLAGE_ACTION,
          VillageActionInfo{elem.id, *elem.action}});
    }
    if (!label || info.dismissedInfos.count({elem.id, labelText}))
      continue;
    addVillainButton(
        getButtonCallback({UserInputId::DISMISS_VILLAGE_INFO, DismissVillageInfo{elem.id, labelText}}),
        std::move(actionCallback),
        std::move(label),
        elem.viewId,
        drawVillainInfoOverlay(elem, true)
    );
  }
  if (villainsIndex)
    villainsIndex = min(buttonsCnt - 1, *villainsIndex);
  return WL(setHeight, 29, WL(stack,
        WL(stopMouseMovement),
        WL(keyHandler, [this] { closeOverlayWindowsAndClearButton(); villainsIndex = 0; },
            {gui.getKey(C_VILLAINS_MENU)}, true),
        WL(conditionalStopKeys, WL(stack,
            WL(keyHandler, [this] { villainsIndex = none; }, {gui.getKey(C_BUILDINGS_CANCEL)}, true),
            WL(keyHandler, [this, buttonsCnt] { villainsIndex = (*villainsIndex + 1) % buttonsCnt; },
                {gui.getKey(C_BUILDINGS_RIGHT)}, true),
            WL(keyHandler, [this, buttonsCnt] { villainsIndex = (*villainsIndex - 1 + buttonsCnt) % buttonsCnt; },
                {gui.getKey(C_BUILDINGS_LEFT)}, true)
        ), [this] { return !!villainsIndex; }),
        WL(translucentBackgroundWithBorder, WL(topMargin, 0, lines.buildHorizontalList()))));
}

void GuiBuilder::drawAllVillainsMenu(Vec2 pos, const VillageInfo& info) {
  vector<SGuiElem> lines;
  vector<function<void()>> callbacks;
  vector<SGuiElem> tooltips;
  if (info.numMainVillains > 0) {
    lines.push_back(WL(label, toString(info.numConqueredMainVillains) + "/" + toString(info.numMainVillains) +
        " main villains conquered"));
    callbacks.push_back(nullptr);
    tooltips.push_back(nullptr);
  }
  for (int i : All(info.villages)) {
    auto& elem = info.villages[i];
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
    if (!!elem.action) {
      label = WL(getListBuilder)
          .addElemAuto(std::move(label))
          .addBackElemAuto(WL(label, " ["_s + getVillageActionText(*elem.action) + "]", Color::GREEN))
          .buildHorizontalList();
      callbacks.push_back(getButtonCallback({UserInputId::VILLAGE_ACTION,
                VillageActionInfo{elem.id, *elem.action}}));
    } else
      callbacks.push_back(nullptr);
    lines.push_back(WL(getListBuilder)
        .addElemAuto(WL(setWidth, 34, WL(centerVert, WL(centerHoriz, WL(stack,
             WL(bottomMargin, -3, WL(viewObject, ViewId("round_shadow"), 1, Color(255, 255, 255, 160))),
             WL(bottomMargin, 5, WL(viewObject, elem.viewId))
        )))))
        .addElemAuto(WL(rightMargin, 5, WL(translate, WL(renderInBounds, std::move(label)), Vec2(0, 0))))
        .buildHorizontalList());
    tooltips.push_back(drawVillainInfoOverlay(elem, false));
  }
  int height = lines.size() * legendLineHeight;
  drawMiniMenu(std::move(lines), std::move(callbacks), std::move(tooltips), pos - Vec2(20, height + 30),
      350, true, true);
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
                elem.count ? drawMinionAndLevel(elem.creature.viewId, *elem.count, 1) : WL(viewObject, elem.creature.viewId)))))
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

string GuiBuilder::leftClickText() {
  return hasController() ? "right trigger" : "left click";
}

string GuiBuilder::rightClickText() {
  return hasController() ? "left trigger": "right click";
}

SGuiElem GuiBuilder::getImmigrationHelpText() {
  return WL(labelMultiLine,
      "Welcome to the immigration system! The icons immediately to the left represent "
      "creatures that would "
      "like to join your dungeon. " + capitalFirst(leftClickText()) + " accepts, " + rightClickText() + " rejects a candidate. "
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
    auto icon = WL(viewObject, elem.creature.viewId, iconScale);
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
              WL(viewObject, elem.viewId),
              WL(margins, WL(label, attrText), 0, 2, 0, 0)), 30));
    };
    if (elem.value != 0 || elem.bonus != 0)
      ret.push_back(cache->get(getValue, THIS_LINE, elem));
  }
  return ret;
}

vector<SGuiElem> GuiBuilder::drawPlayerAttributes(const ViewObject::CreatureAttributes& attributes) {
  vector<SGuiElem> ret;
  for (auto& attr : attributes)
    if (attr.second > 0)
      ret.push_back(
          WL(horizontalList, makeVec(
            WL(viewObject, attr.first),
            WL(margins, WL(label, toString((int) attr.second)), 0, 2, 0, 0)), 30));
  return ret;
}

SGuiElem GuiBuilder::drawBottomPlayerInfo(const GameInfo& gameInfo) {
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
  return WL(getListBuilder, legendLineHeight)
      .addElem(WL(centerHoriz, WL(horizontalList,
           drawPlayerAttributes(gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>()->attributes), resourceSpace)))
      .addElem(WL(centerHoriz, WL(getListBuilder)
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
    out.push_back(item.representsSteed() ? "Currently riding." : "Equipped.");
  if (item.pending)
    out.push_back(item.representsSteed() ? "Currently not riding." : "Not equipped yet.");
  if (auto text = getIntrinsicStateText(item))
    out.push_back(text);
  if (!item.unavailableReason.empty())
    out.push_back(item.unavailableReason);
  return out;
}

SGuiElem GuiBuilder::drawBestAttack(const BestAttack& info) {
  return WL(getListBuilder, 30)
      .addElem(WL(viewObject, info.viewId))
      .addElem(WL(topMargin, 2, WL(label, toString((int) info.value))))
      .buildHorizontalList();
}

static string getWeightString(double weight) {
  return toString<int>((int)(weight * 10)) + "s";
}

SGuiElem GuiBuilder::getItemLine(const ItemInfo& item, function<void(Rectangle)> onClick,
    function<void()> onMultiClick, bool forceEnableTooltip, bool renderInBounds) {
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
  auto lineElem = line.buildHorizontalList();
  if (renderInBounds)
    lineElem = WL(renderInBounds, std::move(lineElem));
  auto mainLine = WL(stack,
      WL(buttonRect, onClick),
      std::move(lineElem),
      getTooltip(getItemHint(item), item.ids.empty() ? 0 : (int) item.ids.front().getHash(), milliseconds{700}, forceEnableTooltip));
  line.clear();
  line.addMiddleElem(std::move(mainLine));
  line.addBackSpace(5);
  if (item.owner) {
    if (item.owner->second > 1)
      line.addBackElemAuto(WL(label, toString(item.owner->second)));
    line.addBackElem(WL(viewObject, item.owner->first.viewId), viewObjectWidth);
    line.addBackElem(drawBestAttack(item.owner->first.bestAttack), getItemLineOwnerMargin() - viewObjectWidth);
  }
  if (item.price)
    line.addBackElemAuto(drawCost(*item.price));
  if (item.weight)
    line.addBackElemAuto(WL(label, "[" + getWeightString(*item.weight * item.number) + "]"));
  if (onMultiClick && item.number > 1) {
    line.addBackElem(WL(stack,
        WL(label, "[#]"),
        WL(button, onMultiClick),
        getTooltip({"Click to choose how many to pick up."}, int(item.ids.front().getGenericId()))), 25);
  }
  auto elem = line.buildHorizontalList();
  if (item.tutorialHighlight)
    elem = WL(stack, WL(tutorialHighlight), std::move(elem));
  return WL(margins, std::move(elem), leftMargin, 0, 0, 0);
}

SGuiElem GuiBuilder::getTooltip(const vector<string>& text, int id, milliseconds delay, bool forceEnableTooltip) {
  return cache->get(
      [this, delay, forceEnableTooltip](const vector<string>& text) {
        return forceEnableTooltip
            ? WL(tooltip, text, delay)
            : WL(conditional, WL(tooltip, text, delay), [this] { return !disableTooltip;}); },
      id, text);
}

SGuiElem GuiBuilder::drawImmigrantCreature(const ImmigrantCreatureInfo& creature) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(getListBuilder)
      .addElem(WL(viewObject, creature.viewId), 35)
      .addElemAuto(WL(label, capitalFirst(creature.name)))
      .buildHorizontalList());
  lines.addSpace(legendLineHeight / 2);
  lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(creature.attributes)));
  bool trainingHeader = false;
  for (auto& info : creature.trainingLimits) {
    if (!trainingHeader)
      lines.addElem(WL(label, "Training potential", Color::YELLOW));
    trainingHeader = true;
    auto line = WL(getListBuilder);
    for (auto& viewId : info.attributes) {
      line.addElem(WL(topMargin, -3, WL(viewObject, viewId)), 22);
    }
    line.addSpace(15);
    line.addElemAuto(WL(label, "+" + toString(info.limit)));
    lines.addElem(line.buildHorizontalList());
  }
  if (!creature.spellSchools.empty())
    lines.addElem(WL(getListBuilder)
        .addElemAuto(WL(label, "Spell schools: ", Color::YELLOW))
        .addElemAuto(WL(label, combine(creature.spellSchools, true)))
        .buildHorizontalList());
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::getTooltip2(SGuiElem elem, GuiFactory::PositionFun fun) {
  return WL(conditional, WL(tooltip2, std::move(elem), std::move(fun)), [this] { return !disableTooltip;});
}

const int listLineHeight = 30;
const int listBrokenLineHeight = 24;

int GuiBuilder::getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

SGuiElem GuiBuilder::drawPlayerOverlay(const PlayerInfo& info, bool dummy) {
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
  auto title = gui.getKeybindingMap()->getGlyph(WL(label, "Lying here: ", Color::YELLOW), &gui, C_BUILDINGS_CONFIRM,
      "[Enter]"_s);
  title = WL(leftMargin, 3, std::move(title));
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
          title,
          WL(scrollable, WL(verticalList, std::move(lines), legendLineHeight), &lyingItemsScroll),
          legendLineHeight, GuiFactory::TOP),
        WL(conditionalStopKeys,
            WL(keyHandler, [=] { callbacks.input({UserInputId::PICK_UP_ITEM, 0});}, Keybinding("MENU_SELECT"), true),
            [this] { return playerOverlayFocused; }),
        WL(keyHandlerBool, [=] {
          if (!playerInventoryFocused() && renderer.getDiscreteJoyPos(ControllerJoy::WALKING) == Vec2(0, 0)) {
            callbacks.input({UserInputId::PICK_UP_ITEM, 0});
            return true;
          }
          return false;
        }, Keybinding("MENU_SELECT")));
  else {
    auto updateScrolling = [this, totalElems] (int dir) {
        if (itemIndex)
          itemIndex = (*itemIndex + dir + totalElems) % totalElems;
        else
          itemIndex = 0;
        lyingItemsScroll.set(*itemIndex * legendLineHeight + legendLineHeight / 2.0, clock->getRealMillis());
    };
    content = WL(stack,
        WL(conditionalStopKeys, WL(stack,
            WL(focusable,
                WL(stack,
                    WL(keyHandler, [=] { if (itemIndex) { callbacks.input({UserInputId::PICK_UP_ITEM, *itemIndex});}},
                      Keybinding("MENU_SELECT"), true),
                    WL(keyHandler, [=] { updateScrolling(1); }, Keybinding("MENU_DOWN"), true),
                    WL(keyHandler, [=] { updateScrolling(-1); }, Keybinding("MENU_UP"), true)),
                Keybinding("MENU_SELECT"),
                Keybinding("EXIT_MENU"),
                playerOverlayFocused),
            WL(keyHandler, [this] { if (!itemIndex ) itemIndex = 0; }, Keybinding("MENU_SELECT")),
            WL(keyHandler, [=] { itemIndex = none; }, Keybinding("EXIT_MENU"))
            ), [this] { return !playerInventoryFocused(); }),
        WL(margin,
          title,
          WL(scrollable, WL(verticalList, std::move(lines), legendLineHeight), &lyingItemsScroll),
          legendLineHeight, GuiFactory::TOP)
    );
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
    case ItemAction::DROP_STEED: return "unassign steed";
    case ItemAction::GIVE: return "give";
    case ItemAction::PAY: return "pay for";
    case ItemAction::EQUIP: return "equip";
    case ItemAction::THROW: return "throw";
    case ItemAction::UNEQUIP: return "remove";
    case ItemAction::APPLY: return "apply";
    case ItemAction::REPLACE: return "replace";
    case ItemAction::REPLACE_STEED: return "assign steed";
    case ItemAction::LOCK: return "lock";
    case ItemAction::REMOVE: return "remove item";
    case ItemAction::CHANGE_NUMBER: return "change number";
    case ItemAction::NAME: return "name";
    case ItemAction::INTRINSIC_ACTIVATE: return "activate attack";
    case ItemAction::INTRINSIC_DEACTIVATE: return "deactivate attack";
  }
}

void GuiBuilder::drawMiniMenu(vector<SGuiElem> elems, vector<function<void()>> callbacks,
    vector<SGuiElem> tooltips, Vec2 menuPos, int width, bool darkBg, bool exitOnCallback, int* selected, bool* exit) {
  auto lines = WL(getListBuilder, legendLineHeight);
  auto allElems = vector<SGuiElem>();
  auto selectedDefault = -1;
  if (hasController())
    for (int i : All(callbacks))
      if (!!callbacks[i]) {
        selectedDefault = i;
        break;
      }
  if (!selected)
    selected = &selectedDefault;
  bool exitDefault = false;
  if (exit == nullptr)
    exit = &exitDefault;
  for (int i : All(elems)) {
    vector<SGuiElem> stack;
    if (callbacks[i] || i < tooltips.size()) {
      if (callbacks[i])
        stack.push_back(WL(button, [exit, exitOnCallback, c = callbacks[i]] {
          c();
          if (exitOnCallback)
            *exit = true;
        }));
      stack.push_back(WL(uiHighlightMouseOver));
      stack.push_back(WL(uiHighlightFrameFilled, [selected, i] { return i == *selected; }));
      if (i < tooltips.size() && !!tooltips[i]) {
        stack.push_back(WL(conditional,
            WL(translate, WL(renderTopLayer, tooltips[i]), Vec2(0, 0), *tooltips[i]->getPreferredSize(),
                GuiFactory::TranslateCorner::TOP_RIGHT),
            [selected, i] { return i == *selected; }));
        stack.push_back(WL(tooltip2, tooltips[i],
            [](Rectangle rect) { return rect.topRight(); }));
      }
    }
    stack.push_back(std::move(elems[i]));
    allElems.push_back(WL(stack, std::move(stack)));
    lines.addElem(allElems.back());
  }
  auto content = WL(stack,
      lines.buildVerticalList(),
      WL(keyHandler, [&] {
        for (int i : Range(elems.size())) {
          auto ind = (*selected - i - 1 + elems.size()) % elems.size();
          if (!!callbacks[ind] || (ind < tooltips.size() && !!tooltips[ind])) {
            *selected = ind;
            miniMenuScroll.setRelative(allElems[ind]->getBounds().top(), clock->getRealMillis());
            break;
          }
        }
      }, {gui.getKey(C_BUILDINGS_UP)}, true),
      WL(keyHandler, [&] {
        for (int i : Range(elems.size())) {
          auto ind = (*selected + i + 1) % elems.size();
          if (!!callbacks[ind] || (ind < tooltips.size() && !!tooltips[ind])) {
            *selected = ind;
            miniMenuScroll.setRelative(allElems[ind]->getBounds().top(), clock->getRealMillis());
            break;
          }
        }
      }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
      WL(keyHandler, [&, exit] {
        if (*selected >= 0 && *selected < callbacks.size() && !!callbacks[*selected]) {
          callbacks[*selected]();
          if (exitOnCallback)
            *exit = true;
        }
      }, Keybinding("MENU_SELECT"), true)
  );
  drawMiniMenu(std::move(content), *exit, menuPos, width, darkBg);
}

void GuiBuilder::drawMiniMenu(SGuiElem elem, bool& exit, Vec2 menuPos, int width, bool darkBg, bool resetScrolling) {
  int margin = 15;
  if (resetScrolling)
    miniMenuScroll.reset();
  elem = WL(scrollable, WL(margins, std::move(elem), 5 + margin, margin, 5 + margin, margin),
      &miniMenuScroll, &scrollbarsHeld);
  elem = WL(miniWindow, std::move(elem), [&exit] { exit = true; });
  drawMiniMenu(std::move(elem), [&]{ return exit; }, menuPos, width, darkBg);
}

void GuiBuilder::drawMiniMenu(SGuiElem elem, function<bool()> done, Vec2 menuPos, int width, bool darkBg) {
  disableTooltip = true;
  int contentHeight = *elem->getPreferredHeight();
  Vec2 size(width, min(renderer.getSize().y, contentHeight));
  menuPos.y = max(0, menuPos.y);
  menuPos.x = max(0, menuPos.x);
  menuPos.y -= max(0, menuPos.y + size.y - renderer.getSize().y);
  menuPos.x -= max(0, menuPos.x + size.x - renderer.getSize().x);
  elem->setBounds(Rectangle(menuPos, menuPos + size));
  SGuiElem bg = WL(darken);
  bg->setBounds(Rectangle(renderer.getSize()));
  while (1) {
    callbacks.refreshScreen();
    if (darkBg)
      bg->render(renderer);
    elem->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {elem});
      if (done())
        return;
    }
  }
}

optional<int> GuiBuilder::chooseAtMouse(const vector<string>& elems) {
  vector<SGuiElem> list;
  vector<function<void()>> callbacks;
  optional<int> ret;
  for (int i : All(elems)) {
    list.push_back(WL(label, elems[i]));
    callbacks.push_back([i, &ret] { ret = i; });
  }
  drawMiniMenu(std::move(list), std::move(callbacks), {}, renderer.getMousePos(), 200, false);
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
  vector<SGuiElem> elems;
  vector<function<void()>> callbacks;
  int width = 120;
  optional<ItemAction> result;
  for (auto action : itemInfo.actions) {
    elems.push_back(WL(label, getActionText(action)));
    callbacks.push_back([&result, action] { result = action; });
    width = max(width, *elems.back()->getPreferredWidth());
  }
  drawMiniMenu(elems, callbacks, {}, menuPos, width, false);
  return result;
}

const Vec2 spellIconSize = Vec2(47, 47);

SGuiElem GuiBuilder::getSpellIcon(const SpellInfo& spell, int index, bool active, GenericId creatureId) {
  vector<SGuiElem> ret;
  if (!spell.timeout) {
    auto callback = getButtonCallback({UserInputId::CAST_SPELL, index});
    if (active) {
      ret.push_back(WL(mouseHighlight2, WL(standardButtonHighlight), WL(standardButton)));
      ret.push_back(WL(conditionalStopKeys, WL(stack,
          WL(standardButtonHighlight),
          WL(keyHandler, [=] {
            abilityIndex = none;
            inventoryScroll.reset();
            callback();
          }, {gui.getKey(C_BUILDINGS_CONFIRM)}, true)
      ), [this, index] { return abilityIndex == index; }));
    } else
      ret.push_back(WL(rectangleBorder, Color::WHITE));
    ret.push_back(WL(centerHoriz, WL(centerVert, WL(labelUnicode, spell.symbol, Color::WHITE))));
    if (active)
      ret.push_back(WL(button, callback));
  } else {
    ret.push_back(WL(standardButton));
    ret.push_back(WL(centerHoriz, WL(centerVert, WL(labelUnicode, spell.symbol, Color::GRAY))));
    ret.push_back(WL(darken));
    ret.push_back(WL(centeredLabel, Renderer::HOR_VER, toString(*spell.timeout)));
  }
  ret.push_back(getTooltip(concat({capitalFirst(spell.name)}, spell.help), THIS_LINE + index + creatureId));
  if (spell.highlighted)
    ret.push_back(WL(uiHighlight));
  return WL(preferredSize, spellIconSize, WL(stack, std::move(ret)));
}

SGuiElem GuiBuilder::drawSpellsList(const vector<SpellInfo>& spells, GenericId creatureId, bool active) {
  constexpr int spellsPerRow = 5;
  if (!spells.empty()) {
    auto list = WL(getListBuilder, spellIconSize.y);
    list.addElem(WL(label, "Abilities", Color::YELLOW), legendLineHeight);
    auto line = WL(getListBuilder);
    SGuiElem firstSpell;
    for (int index : All(spells)) {
      auto& elem = spells[index];
      auto icon = getSpellIcon(elem, index, active, creatureId);
      if (!firstSpell)
        firstSpell = icon;
      line.addElemAuto(std::move(icon));
      if (line.getLength() >= spellsPerRow) {
        list.addElem(line.buildHorizontalList());
        line.clear();
      }
    }
    if (!line.isEmpty())
      list.addElem(line.buildHorizontalList());
    auto ret = list.buildVerticalList();
    if (active) {
      auto getNextSpell = [spells](int curIndex, int inc) -> optional<int> {
        int module = abs(inc) == 1 ? spells.size() : (spells.size() + spellsPerRow - 1) / spellsPerRow * spellsPerRow;
        for (int i : All(spells)) {
          int index = (curIndex + (i + 1) * inc + module) % module;
          if (index < spells.size() && !spells[index].timeout)
            return index;
        }
        return none;
      };
      ret = WL(stack,
          WL(keyHandler, [this, getNextSpell, firstSpell] {
            closeOverlayWindows();
            abilityIndex = getNextSpell(-1, 1);
            if (abilityIndex)
              inventoryScroll.setRelative(firstSpell->getBounds().top(), clock->getRealMillis());
          }, {gui.getKey(C_ABILITIES)}, true),
          WL(conditionalStopKeys, WL(stack, makeVec(
              WL(keyHandler, [=, cnt = spells.size()] {
                abilityIndex = getNextSpell(*abilityIndex, 1);
              }, {gui.getKey(C_BUILDINGS_RIGHT)}, true),
              WL(keyHandler, [=, cnt = spells.size()] {
                abilityIndex = getNextSpell(*abilityIndex, -1);
              }, {gui.getKey(C_BUILDINGS_LEFT)}, true),
              WL(keyHandler, [=, cnt = spells.size()] {
                abilityIndex = getNextSpell(*abilityIndex, 5);
              }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
              WL(keyHandler, [=, cnt = spells.size()] {
                abilityIndex = getNextSpell(*abilityIndex, -5);
              }, {gui.getKey(C_BUILDINGS_UP)}, true),
              WL(keyHandler, [this] {
                abilityIndex = none;
                inventoryScroll.reset();
              }, {gui.getKey(C_BUILDINGS_CANCEL)}, true)
          )), [this] { return !!abilityIndex; }),
          std::move(ret)
      );
    }
    return ret;
  } else
    return nullptr;
}

vector<SGuiElem> GuiBuilder::drawEffectsList(const PlayerInfo& info, bool tooltip) {
  vector<SGuiElem> lines;
  for (int i : All(info.effects)) {
    auto& effect = info.effects[i];
    auto label = WL(label, effect.name, effect.bad ? Color::RED : Color::WHITE);
    if (tooltip)
      label = WL(renderInBounds, std::move(label));
    lines.push_back(WL(stack,
          tooltip ? getTooltip({effect.help}, THIS_LINE + i) : WL(empty),
          std::move(label)));
  }
  return lines;
}

SGuiElem GuiBuilder::getExpIncreaseLine(const CreatureExperienceInfo& info, ExperienceType type, bool infoOnly) {
  if (info.limit[type] == 0)
    return nullptr;
  auto line = WL(getListBuilder);
  int i = 0;
  vector<string> attrNames;
  auto attrIcons = WL(getListBuilder);
  for (auto& elem : info.attributes[type]) {
    attrIcons.addElem(WL(topMargin, -3, WL(viewObject, elem.second)), 22);
    attrNames.push_back(elem.first);
  }
  line.addElem(attrIcons.buildHorizontalList(), 80);
  if (!infoOnly)
    line.addElem(WL(label, "+" + toStringRounded(info.level[type], 0.01),
        info.warning[type] ? Color::RED : Color::WHITE), 50);
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

SGuiElem GuiBuilder::drawExperienceInfo(const CreatureExperienceInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  auto promoLevel = min<double>(info.combatExperienceCap, info.combatExperience);
  auto builder = WL(getListBuilder)
      .addElemAuto(WL(label, "Experience: ", Color::YELLOW))
      .addElemAuto(WL(label, toStringRounded(promoLevel, 0.01)));
  if (info.teamExperience > info.combatExperience)
    builder
        .addElemAuto(WL(label, " + "))
        .addElemAuto(WL(label, toStringRounded((info.teamExperience - info.combatExperience) / 2, 0.01)));
  lines.addElem(WL(stack,
      builder.buildHorizontalList(),
      getTooltip({"Experience increases every attribute that can be or has been trained. If the creature has",
          "no trainable attributes then damage and defense will be used by default.",
          "For example, Dumbug the goblin has a +1 training in archery, and a +3 training in melee.",
          "Having a +2 experience, his damage, defense and ranged damage are further increased by +2."},
          THIS_LINE)
  ));
  if (info.combatExperience > promoLevel)
    lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, "Unrealized experience: ", Color::YELLOW))
      .addElemAuto(WL(label, toStringRounded(info.combatExperience - promoLevel, 0.01)))
      .buildHorizontalList());
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawTrainingInfo(const CreatureExperienceInfo& info, bool infoOnly) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, "Training", Color::YELLOW));
  bool empty = !info.combatExperience;
  for (auto expType : ENUM_ALL(ExperienceType)) {
    if (auto elem = getExpIncreaseLine(info, expType, infoOnly)) {
      lines.addElem(std::move(elem));
      empty = false;
    }
  }
  if (!infoOnly)
    lines.addElemAuto(drawExperienceInfo(info));
  if (!empty)
    return lines.buildVerticalList();
  else
    return nullptr;
}

function<void(Rectangle)> GuiBuilder::getCommandsCallback(const vector<PlayerInfo::CommandInfo>& commands) {
  return [this, commands] (Rectangle bounds) {
    vector<SGuiElem> lines;
    vector<function<void()>> callbacks;
    vector<SGuiElem> tooltips;
    optional<UserInput> result;
    for (int i : All(commands)) {
      auto& command = commands[i];
      if (command.active)
        callbacks.push_back([i, this] {
          this->callbacks.input({UserInputId::PLAYER_COMMAND, i});
        });
      else
        callbacks.emplace_back();
      auto labelColor = command.active ? Color::WHITE : Color::GRAY;
      vector<SGuiElem> stack;
      //if (command.keybinding)
        //stack.push_back(WL(stack, std::move(button), WL(keyHandler, buttonFun, *command.keybinding)));
      if (command.tutorialHighlight)
        stack.push_back(WL(tutorialHighlight));
      auto label = WL(label, command.name, labelColor);
      if (command.keybinding)
        label = gui.getKeybindingMap()->getGlyph(std::move(label), &gui, *command.keybinding);
      stack.push_back(std::move(label));
      if (!command.description.empty())
        tooltips.push_back(WL(miniWindow, WL(margins, WL(label, command.description), 15)));
      else
        tooltips.push_back(WL(empty));
      lines.push_back(WL(stack, std::move(stack)));
    }
    drawMiniMenu(std::move(lines), std::move(callbacks), std::move(tooltips), bounds.bottomLeft(),
        290, false, true, &commandsIndex);
  };
}

bool GuiBuilder::playerInventoryFocused() const {
  return !!inventoryIndex || !!abilityIndex;
}

SGuiElem GuiBuilder::drawPlayerInventory(const PlayerInfo& info, bool withKeys) {
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
        keyElems.push_back(WL(keyHandler, getButtonCallback({UserInputId::PLAYER_COMMAND, i}), *key));
  }
  if (!info.commands.empty()) {
    auto callback = getCommandsCallback(info.commands);
    list.addElem(WL(stack,
        WL(stack, std::move(keyElems)),
        WL(conditional, WL(tutorialHighlight), [=]{ return isTutorial;}),
        WL(buttonLabel, "Commands", WL(buttonRect, callback)),
        WL(keyHandlerRect, callback, {gui.getKey(C_COMMANDS)}, true)));
  }
  for (auto& elem : drawEffectsList(info))
    list.addElem(std::move(elem));
  list.addSpace();
  if (auto spells = drawSpellsList(info.spells, info.creatureId.getGenericId(), true)) {
    list.addElemAuto(std::move(spells));
    list.addSpace();
  }
  if (info.debt > 0) {
    list.addElem(WL(label, "Debt", Color::YELLOW));
    list.addElem(WL(label, "Click on debt or on individual items to pay.", Renderer::smallTextSize(),
        Color::LIGHT_GRAY), legendLineHeight * 2 / 3);
    list.addElem(WL(stack,
        drawCost({ViewId("gold"), info.debt}),
        WL(button, getButtonCallback(UserInputId::PAY_DEBT))));
    list.addSpace();
  }
  SGuiElem firstInventoryItem;
  if (!info.inventory.empty()) {
    list.addElem(WL(label, "Inventory", Color::YELLOW));
    if (withKeys && !!inventoryIndex)
      inventoryIndex = min(*inventoryIndex, info.inventory.size() - 1);
    for (int i : All(info.inventory)) {
      auto& item = info.inventory[i];
      auto callback = [=](Rectangle butBounds) {
        if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), false))
          callbacks.input({UserInputId::INVENTORY_ITEM, InventoryItemInfo{item.ids, *choice}});
      };
      auto elem = WL(stack,
          WL(conditionalStopKeys, WL(stack,
              WL(uiHighlightFrameFilled),
              WL(keyHandlerRect, [=](Rectangle bounds) {
                if (inventoryIndex == i) {
                  callback(bounds);
                }
              }, Keybinding("MENU_SELECT"), true)
          ), [this, i] { return inventoryIndex == i; }),
          WL(conditionalStopKeys,
              WL(keyHandlerRect, [i, this, size = list.getSize()](Rectangle bounds) {
                *inventoryIndex = i;
                inventoryScroll.setRelative(bounds.top(), clock->getRealMillis());
              }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
              [this, i, cnt = info.inventory.size()] { return inventoryIndex == (i - 1 + cnt) % cnt; }),
          WL(conditionalStopKeys,
              WL(keyHandlerRect, [i, this, size = list.getSize()](Rectangle bounds) {
                *inventoryIndex = i;
                inventoryScroll.setRelative(bounds.top(), clock->getRealMillis());
              }, {gui.getKey(C_BUILDINGS_UP)}, true),
              [this, i, cnt = info.inventory.size()] { return inventoryIndex == (i + 1) % cnt; }),
          getItemLine(item, callback));
      if (!firstInventoryItem)
        firstInventoryItem = elem;
      list.addElem(std::move(elem));
    }
    double totWeight = 0;
    for (auto& item : info.inventory)
      totWeight += item.weight.value_or(0) * item.number;
    list.addElem(WL(label, "Total weight: " + getWeightString(totWeight)));
    list.addElem(WL(label, "Capacity: " +  (info.carryLimit ? getWeightString(*info.carryLimit) : "infinite"_s)));
    list.addSpace();
  } else if (withKeys && !!inventoryIndex) {
    inventoryIndex = none;
    inventoryScroll.reset();
  }
  if (!info.intrinsicAttacks.empty()) {
    list.addElem(WL(label, "Intrinsic attacks", Color::YELLOW));
    for (auto& item : info.intrinsicAttacks)
      list.addElem(getItemLine(item, [=](Rectangle butBounds) {
            if (auto choice = getItemChoice(item, butBounds.bottomLeft() + Vec2(50, 0), false))
              callbacks.input({UserInputId::INTRINSIC_ATTACK, InventoryItemInfo{item.ids, *choice}});}));
    list.addSpace();
  }
  if (auto elem = drawTrainingInfo(info.experienceInfo))
    list.addElemAuto(std::move(elem));
  return WL(stack, makeVec(
      WL(keyHandler, [this, firstInventoryItem] {
        if (!!firstInventoryItem) {
          closeOverlayWindows();
          inventoryIndex = 0;
          inventoryScroll.setRelative(firstInventoryItem->getBounds().top(), clock->getRealMillis());
        }
      }, {gui.getKey(C_INVENTORY)}, true),
      WL(conditionalStopKeys,
          WL(keyHandler, [this] {
            inventoryIndex = none;
            inventoryScroll.reset();
          }, {gui.getKey(C_BUILDINGS_CANCEL)}, true), [this] { return !!inventoryIndex;} ),
      WL(rightMargin, 5, list.buildVerticalList())
  ));
}

static const char* getControlModeName(PlayerInfo::ControlMode m) {
  switch (m) {
    case PlayerInfo::FULL: return "full";
    case PlayerInfo::LEADER: return "leader";
  }
}


SGuiElem GuiBuilder::drawRightPlayerInfo(const PlayerInfo& info) {
  if (!info.controlMode)
    return WL(margins, WL(scrollable, drawPlayerInventory(info, true), &inventoryScroll, &scrollbarsHeld), 6, 0, 15, 5);
  if (highlightedTeamMember && *highlightedTeamMember >= info.teamInfos.size())
    highlightedTeamMember = none;
  auto getIconHighlight = [&] (Color c) { return WL(topMargin, -1, WL(uiHighlight, c)); };
  auto vList = WL(getListBuilder, legendLineHeight);
  auto teamList = WL(getListBuilder);
  for (int i : All(info.teamInfos)) {
    auto& member = info.teamInfos[i];
    auto icon = WL(margins, WL(viewObject, member.viewId, 1), 1);
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
            WL(translate, WL(translate, WL(labelUnicode, u8"⬆", Color::WHITE, 14), Vec2(2, -1)),
                Vec2(6, 30), Vec2(17, 21))
        );
      if (member.isPlayerControlled)
        icon = WL(stack,
            WL(margins, WL(rectangle, Color::GREEN.transparency(1094)), 2),
            std::move(icon)
        );
    }
    if (!member.teamMemberActions.empty()) {
      auto lines = WL(getListBuilder, legendLineHeight);
      for (auto action : member.teamMemberActions)
        lines.addElem(WL(label, getText(getText(action))));
      icon = WL(stack, std::move(icon),
        WL(buttonRect, [memberId = member.creatureId, actions = member.teamMemberActions, this] (Rectangle bounds) {
              vector<SGuiElem> lines;
              vector<function<void()>> callbacks;
              optional<TeamMemberAction> ret;
              for (auto action : actions) {
                callbacks.push_back([&ret, action] {
                  ret = action;
                });
                lines.push_back(WL(label, getText(getText(action))));
              }
              drawMiniMenu(std::move(lines), std::move(callbacks), {}, bounds.bottomLeft(), 200, false);
              if (ret)
                this->callbacks.input({UserInputId::TEAM_MEMBER_ACTION, TeamMemberActionInfo{*ret, memberId}});
        }),
        getTooltip2(WL(translucentBackgroundWithBorderPassMouse, WL(margins, lines.buildVerticalList(), 15)),
            [](const Rectangle& r){ return r.bottomLeft(); })
      );
    }
    teamList.addElemAuto(std::move(icon));
    if (teamList.getLength() >= 9) {
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
                ? WL(buttonLabelSelected, "     "_s + getName(order) + "     "_s, switchFun)
                : WL(buttonLabel, "     "_s + getName(order) + "     "_s, switchFun),
            getTooltip({getDescription(order)}, THIS_LINE),
            WL(keyHandler, switchFun, getKeybinding(order))
        ));
        orderList.addSpace(15);
      }
      vList.addElem(orderList.buildHorizontalList());
      vList.addSpace(legendLineHeight / 2);
    }
    auto callback = getButtonCallback(UserInputId::TOGGLE_CONTROL_MODE);
    auto keybinding = Keybinding("TOGGLE_CONTROL_MODE");
    auto label = gui.getKeybindingMap()->getGlyph(
        WL(label, "Control mode: "_s + getControlModeName(*info.controlMode)),
        &gui, keybinding);
    vList.addElem(WL(stack,
        WL(standardButton, label, WL(button, callback)),
        WL(keyHandler, callback, keybinding)
    ));
    vList.addSpace(legendLineHeight / 2);
  }
  if (info.canExitControlMode) {
    auto callback = getButtonCallback(UserInputId::EXIT_CONTROL_MODE);
    auto keybinding = Keybinding("EXIT_CONTROL_MODE");
    auto label = gui.getKeybindingMap()->getGlyph(WL(label, "Exit control mode"_s), &gui, keybinding);
    vList.addElem(WL(stack,
        WL(standardButton, label, WL(button, callback)),
        WL(keyHandler, callback, keybinding)
    ));
  }
  vList.addSpace(10);
  vList.addElem(WL(margins, WL(sprite, GuiFactory::TexId::HORI_LINE, GuiFactory::Alignment::TOP), -6, 0, -6, 0), 10);
  vector<SGuiElem> others;
  for (int i : All(info.teamInfos)) {
    auto& elem = info.teamInfos[i];
    if (elem.creatureId != info.creatureId)
      others.push_back(WL(conditionalStopKeys, drawPlayerInventory(elem, false), [this, i]{ return highlightedTeamMember == i;}));
    else
      others.push_back(WL(conditional, drawPlayerInventory(info, true),
          [this, i]{ return !highlightedTeamMember || highlightedTeamMember == i;}));
  }
  vList.addElemAuto(WL(stack, std::move(others)));
  return WL(stopScrollEvent,
      WL(margins, WL(scrollable, vList.buildVerticalList(), &inventoryScroll, &scrollbarsHeld), 6, 0, 15, 5),
      [this] { return !!inventoryIndex || !!abilityIndex; });
}

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

static bool isGroupChosen(const CollectiveInfo& info) {
  for (auto& elem : info.minionGroups)
    if (elem.highlight)
      return true;
  for (auto& elem : info.automatonGroups)
    if (elem.highlight)
      return true;
  for (auto& elem : info.teams)
    if (elem.highlight)
      return true;
  return false;
}

SGuiElem GuiBuilder::drawTeams(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial, int& buttonCnt) {
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
          WL(conditionalStopKeys, WL(stack,
              WL(uiHighlightFrameFilled),
              WL(keyHandler, [this, hint] { callbacks.info(hint); }, {gui.getKey(C_BUILDINGS_CONFIRM) }, true)
          ), [this, buttonCnt]{ return minionsIndex == buttonCnt;} ),
          WL(conditional, WL(tutorialHighlight), [yes = isTutorialHighlight && info.teams.empty()]{ return yes; }),
          getHintCallback({hint}),
          WL(button, [this, hint] { callbacks.info(hint); }),
          WL(label, "[new team]", Color::WHITE))));
    ++buttonCnt;
  }
  bool groupChosen = isGroupChosen(info);
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
    auto callback = [=]() {
      onTutorialClicked(0, TutorialHighlight::CONTROL_TEAM);
      callbacks.input({UserInputId::SELECT_TEAM, team.id});
    };
    auto selectButton = [this, callback] (int teamIdDummy) {
      return WL(releaseLeftButton, callback);
    };
    const bool isTutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
    lines.addElemAuto(WL(stack, makeVec(
            WL(conditional, WL(tutorialHighlight),
                [=]{ return !wasTutorialClicked(0, TutorialHighlight::CONTROL_TEAM) && isTutorialHighlight; }),
            WL(uiHighlightLineConditional, [team] () { return team.highlight; }),
            WL(uiHighlightMouseOver),
            WL(conditionalStopKeys, WL(stack,
                WL(uiHighlightLine),
                info.chosenCreature ? WL(empty) : WL(uiHighlightFrame),
                WL(keyHandler, callback, {gui.getKey(C_BUILDINGS_CONFIRM), gui.getKey(C_BUILDINGS_RIGHT)}, true)
            ), [buttonCnt, groupChosen, this] {
              return hasController() && !groupChosen && minionsIndex == buttonCnt && collectiveTab == CollectiveTab::MINIONS;
            }),
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
    ++buttonCnt;
  }
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawMinions(const CollectiveInfo& info, optional<int> minionIndexDummy,
    const optional<TutorialInfo>& tutorial) {
  auto list = WL(getListBuilder, legendLineHeight);
  list.addElem(WL(label, info.monsterHeader, Color::WHITE));
  int buttonCnt = 0;
  auto groupChosen = isGroupChosen(info);
  auto addGroup = [&] (const CollectiveInfo::CreatureGroup& elem) {
    auto line = WL(getListBuilder);
    line.addElem(WL(viewObject, elem.viewId), 40);
    SGuiElem tmp = WL(label, toString(elem.count) + "   " + elem.name, Color::WHITE);
    line.addElem(WL(renderInBounds, std::move(tmp)), 200);
    auto callback = getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, elem.name});
    auto selectButton = cache->get([this](const string& group) {
      return WL(releaseLeftButton, getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, group}));
    }, THIS_LINE, elem.name);
    list.addElem(WL(stack, makeVec(
        std::move(selectButton),
        WL(dragSource, elem.name,
            [=]{ return WL(getListBuilder, 10)
                .addElemAuto(WL(label, toString(elem.count) + " "))
                .addElem(WL(viewObject, elem.viewId)).buildHorizontalList();}),
        WL(button, [this, group = elem.name, viewId = elem.viewId, id = elem.creatureId] {
            callbacks.input(UserInput(UserInputId::CREATURE_DRAG, id));
            gui.getDragContainer().put(group, WL(viewObject, viewId), Vec2(-100, -100));
        }),
        elem.highlight ? WL(uiHighlightLine) : WL(empty),
        WL(conditionalStopKeys, WL(stack,
            WL(uiHighlightLine),
            info.chosenCreature ? WL(empty) : WL(uiHighlightFrame),
            WL(keyHandler, callback, {gui.getKey(C_BUILDINGS_CONFIRM), gui.getKey(C_BUILDINGS_RIGHT)}, true)
        ), [buttonCnt, groupChosen, this] {
          return hasController() && !groupChosen && minionsIndex == buttonCnt && collectiveTab == CollectiveTab::MINIONS;
        }),
        line.buildHorizontalList()
    )));
    ++buttonCnt;
  };
  for (auto& group : info.minionGroups)
    addGroup(group);
  if (!info.automatonGroups.empty()) {
    list.addElem(WL(label, "Minions by ability: ", Color::WHITE));
    for (auto& group : info.automatonGroups)
      addGroup(group);
  }
  list.addElem(WL(label, "Teams: ", Color::WHITE));
  list.addElemAuto(drawTeams(info, tutorial, buttonCnt));
  list.addSpace();
  list.addElem(WL(stack,
            WL(conditionalStopKeys, WL(stack,
                WL(uiHighlightFrameFilled),
                WL(keyHandler, [this] { toggleBottomWindow(TASKS); }, {gui.getKey(C_BUILDINGS_CONFIRM) }, true)
            ), [this, buttonCnt]{ return minionsIndex == buttonCnt;} ),
            WL(label, "Show tasks", [=]{ return bottomWindow == TASKS ? Color::GREEN : Color::WHITE;}),
            WL(button, [this] { toggleBottomWindow(TASKS); })));
  ++buttonCnt;
  list.addElem(WL(stack,
            WL(conditionalStopKeys, WL(stack,
                WL(uiHighlightFrameFilled),
                WL(keyHandler, getButtonCallback(UserInputId::SHOW_HISTORY), {gui.getKey(C_BUILDINGS_CONFIRM) }, true)
            ), [this, buttonCnt]{ return minionsIndex == buttonCnt;} ),
            WL(label, "Show message history"),
            WL(button, getButtonCallback(UserInputId::SHOW_HISTORY))));
  ++buttonCnt;
  list.addSpace();
  return WL(stack, makeVec(
      WL(keyHandler, [this] {
        closeOverlayWindowsAndClearButton();
        minionsIndex = 0;
        setCollectiveTab(CollectiveTab::MINIONS);
      }, {gui.getKey(C_MINIONS_MENU)}, true),
      WL(stopScrollEvent,
          WL(scrollable, WL(rightMargin, 10, list.buildVerticalList()), &minionsScroll, &scrollbarsHeld),
          [this] { return !!minionsIndex; }),
      WL(conditionalStopKeys, WL(stack,
          WL(keyHandlerBool, [this] {
            if (minionsIndex) {
              minionsIndex = none;
              return true;
            }
            return false;
          }, {gui.getKey(C_BUILDINGS_CANCEL)}),
          WL(keyHandler, [this, buttonCnt] {
            if (!minionsIndex) {
              closeOverlayWindowsAndClearButton();
              minionsIndex = 0;
            } else
              minionsIndex = (*minionsIndex + 1) % buttonCnt;
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          WL(keyHandler, [this, buttonCnt] {
            if (!minionsIndex) {
              closeOverlayWindowsAndClearButton();
              minionsIndex = 0;
            } else
              minionsIndex = (*minionsIndex - 1 + buttonCnt) % buttonCnt;
          }, {gui.getKey(C_BUILDINGS_UP)}, true),
          WL(keyHandler, [this] { callbacks.input({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()}); },
              {gui.getKey(C_BUILDINGS_LEFT)}, true)
      ), [this] { return collectiveTab == CollectiveTab::MINIONS; })
  ));
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
          WL(keyHandler, [this]{bottomWindow = none;}, Keybinding("EXIT_MENU"), true),
          WL(miniWindow,
        WL(margins, WL(scrollable, WL(verticalList, std::move(lines), lineHeight), &tasksScroll, &scrollbarsHeld),
          margin))));
}

function<void(Rectangle)> GuiBuilder::getItemUpgradeCallback(const CollectiveInfo::QueuedItemInfo& elem) {
  return [=] (Rectangle bounds) {
      auto lines = WL(getListBuilder, legendLineHeight);
      lines.addElem(hasController()
          ? WL(getListBuilder)
              .addElemAuto(WL(label, "Use ", Color::YELLOW))
              .addElemAuto(WL(steamInputGlyph, C_BUILDINGS_LEFT))
              .addElemAuto(WL(label, " and ", Color::YELLOW))
              .addElemAuto(WL(steamInputGlyph, C_BUILDINGS_RIGHT))
              .addElemAuto(WL(label, " to add/remove upgrades.", Color::YELLOW))
              .buildHorizontalList()
          : WL(label, "Use left/right mouse buttons to add/remove upgrades.", Color::YELLOW));
      vector<int> increases(elem.upgrades.size(), 0);
      int totalUsed = 0;
      int totalAvailable = 0;
      bool exit = false;
      int selected = hasController() ? 0 : -1;
      vector<SGuiElem> activeElems;
      auto cnt = elem.itemInfo.number;
      for (auto& upgrade : elem.upgrades) {
        totalUsed += upgrade.used;
        totalAvailable += upgrade.count / cnt + upgrade.used;
      }
      totalAvailable = min(totalAvailable, elem.maxUpgrades.second);
      for (int i : All(elem.upgrades)) {
        auto& upgrade = elem.upgrades[i];
        auto idLine = WL(getListBuilder);
        auto colorFun = [&increases, i, upgrade, &totalUsed, cnt, max = elem.maxUpgrades.second] {
          return increases[i] + cnt > upgrade.count || totalUsed >= max ? Color::LIGHT_GRAY : Color::WHITE;
        };
        idLine.addElemAuto(WL(viewObject, upgrade.viewId));
        idLine.addElemAuto(WL(label, upgrade.name, colorFun));
        idLine.addBackElem(WL(labelFun, [&increases, i, upgrade, cnt] {
            return "(" + toString(upgrade.used * cnt + increases[i]) + "/" + toString(upgrade.used * cnt + upgrade.count) + ")  "; },
            colorFun), 70);
        auto callbackIncrease = [&increases, &totalUsed, i, upgrade, cnt, max = elem.maxUpgrades.second] {
          if (increases[i] <= upgrade.count - cnt && totalUsed < max) {
            increases[i] += cnt;
            ++totalUsed;
          }
        };
        auto callbackDecrease = [&increases, &totalUsed, i, upgrade, cnt] {
          if (increases[i] + upgrade.used * cnt >= cnt) {
            increases[i] -= cnt;
            --totalUsed;
          }
        };
        SGuiElem tooltip;
        if (!upgrade.description.empty()) {
         auto lines = WL(getListBuilder, getStandardLineHeight());
         for (auto& elem : upgrade.description)
           lines.addElem(WL(label, elem));
          tooltip = WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
        }
        auto elem = WL(stack, makeVec(
            WL(button, callbackIncrease),
            WL(buttonRightClick, callbackDecrease),
            WL(conditional, WL(uiHighlightMouseOver), [&selected] { return selected == -1; }),
            WL(conditionalStopKeys, WL(stack,
                WL(uiHighlightLine),
                WL(keyHandler, callbackIncrease, Keybinding("MENU_RIGHT"), true),
                WL(keyHandler, callbackDecrease, Keybinding("MENU_LEFT"), true),
                tooltip ? WL(translate, WL(renderTopLayer, tooltip), Vec2(0, 0), *tooltip->getPreferredSize(),
                    GuiFactory::TranslateCorner::TOP_LEFT_SHIFTED)
                    : WL(empty)
            ), [&selected, i] { return selected == i;}),
            idLine.buildHorizontalList(),
            tooltip ? WL(conditional, WL(tooltip2, tooltip, [](Rectangle r) { return r.bottomLeft(); }),
                    [&selected] { return selected == -1;})
                : WL(empty)
        ));
        activeElems.push_back(elem);
        lines.addElem(std::move(elem));
      }
      auto label = WL(labelFun, [&] {
        if (totalAvailable - totalUsed > 0)
          return "Add top " + toString((totalAvailable - totalUsed) * cnt) + " upgrades"_s;
        else
          return "Clear all upgrades"_s;
      });
      auto action = [&] {
        if (totalAvailable - totalUsed > 0)
          for (int i : All(elem.upgrades)) {
            int toAdd = min(elem.upgrades[i].count - increases[i], (totalAvailable - totalUsed) * cnt);
            totalUsed += toAdd / cnt;
            increases[i] += toAdd;
            if (totalUsed >= totalAvailable)
              break;
          }
        else {
          totalUsed = 0;
          for (int i : All(elem.upgrades)) {
            increases[i] = -elem.upgrades[i].used * cnt;
          }
        }
      };
      lines.addSpace(5);
      auto allButton = WL(getListBuilder)
          .addElem(WL(labelFun, [&totalUsed, &elem] {
              return "Used slots: " + toString(totalUsed) + "/" + toString(elem.maxUpgrades.second); }), 10)
          .addBackElemAuto(WL(setWidth, 170,
              WL(buttonLabelFocusable, std::move(label), std::move(action),
                  [&selected, myIndex = activeElems.size()] { return selected == myIndex; }, false)))
          .buildHorizontalList();
      activeElems.push_back(allButton);
      lines.addElem(std::move(allButton));
      auto content = WL(stack,
          lines.buildVerticalList(),
          getMiniMenuScrolling(activeElems, selected)
      );
      drawMiniMenu(std::move(content), exit, bounds.bottomLeft(), 500, false);
      callbacks.input({UserInputId::WORKSHOP_UPGRADE, WorkshopUpgradeInfo{elem.itemIndex, increases, cnt}});
  };
}

SGuiElem GuiBuilder::getMiniMenuScrolling(const vector<SGuiElem>& activeElems, int& selected) {
  return WL(stack,
      WL(keyHandler, [&] {
        selected = (selected + 1) % activeElems.size();
        miniMenuScroll.setRelative(activeElems[selected]->getBounds().top(), Clock::getRealMillis());
      }, Keybinding("MENU_DOWN"), true),
       WL(keyHandler, [&] {
         selected = (selected + activeElems.size() - 1) % activeElems.size();
         miniMenuScroll.setRelative(activeElems[selected]->getBounds().top(), Clock::getRealMillis());
       }, Keybinding("MENU_UP"), true)
  );
}

SGuiElem GuiBuilder::drawItemUpgradeButton(const CollectiveInfo::QueuedItemInfo& elem) {
  auto line = WL(getListBuilder);
  vector<pair<ViewId, int>> upgrades;
  auto addUpgrade = [&] (ViewId id, int cnt) {
    for (auto& elem : upgrades)
      if (elem.first == id) {
        elem.second += cnt;
        return;
      }
    upgrades.emplace_back(id, cnt);
  };
  for (int upgradeIndex : All(elem.upgrades)) {
    auto& upgrade = elem.upgrades[upgradeIndex];
    if (upgrade.used > 0)
      addUpgrade(upgrade.viewId, upgrade.used);
  }
  for (auto& elem : upgrades) {
    if (elem.second > 1)
      line.addElemAuto(WL(label, toString(elem.second)));
    line.addElemAuto(WL(viewObject, elem.first));
  }
  auto button = WL(buttonRect, getItemUpgradeCallback(elem));
  if (!upgrades.empty())
    return WL(standardButton, line.buildHorizontalList(), button);
  else
    return WL(buttonLabel, "upgrade", button);
}

optional<int> GuiBuilder::getNumber(const string& title, Vec2 position, Range range, int initial) {
  ScriptedUIState state;
  int result = initial;
  bool confirmed = false;
  while (true) {
    bool changed = false;
    ScriptedUIData data = ScriptedUIDataElems::Record {{
      {"title", title},
      {"range_begin", toString(range.getStart())},
      {"range_end", toString(range.getEnd())},
      {"current", toString(result)},
      {"confirm", ScriptedUIDataElems::Callback {
          [&confirmed] { confirmed = true; return true; }
      }},
      {"slider", ScriptedUIDataElems::SliderData {
        [range, &result, &changed] (double value) {
          int newResult = value * range.getLength() + range.getStart();
          if (newResult != result) {
            result = newResult;
            changed = true;
            return true;
          }
          return false;
        },
        double(result - range.getStart()) / range.getLength(),
        true
      }},
      {"increase", ScriptedUIDataElems::Callback {
        [range, &state, &result, &changed] {
          changed = true;
          result = min(range.getEnd(), result + 1);
          state.sliderState.clear();
          return true;
        },
      }},
      {"decrease", ScriptedUIDataElems::Callback {
        [range, &state, &result, &changed] {
          changed = true;
          state.sliderState.clear();
          result = max(range.getStart(), result - 1);
          return true;
        },
      }}
    }};
    bool exit = false;
    auto ui = gui.scripted([&]{exit = true; }, ScriptedUIId("number_menu"), data, state);
    drawMiniMenu(std::move(ui), exit, position, 450, false);
    if (!changed)
      break;
  }
  if (confirmed)
    return result;
  else
    return none;
}

function<void(Rectangle)> GuiBuilder::getItemCountCallback(const CollectiveInfo::QueuedItemInfo& elem) {
  return [=] (Rectangle bounds) {
    auto value = getNumber("Change the number of items.", bounds.bottomLeft(), Range(0, 100), elem.itemInfo.number);
    if (!!value && *value != elem.itemInfo.number)
      callbacks.input({UserInputId::WORKSHOP_CHANGE_COUNT, WorkshopCountInfo{elem.itemIndex, elem.itemInfo.number,
          *value}});
  };
}

SGuiElem GuiBuilder::drawWorkshopsOverlay(const CollectiveInfo::ChosenWorkshopInfo& info,
    const optional<TutorialInfo>& tutorial, optional<Vec2> dummyIndex) {
  int margin = 20;
  int rightElemMargin = 10;
  auto& options = info.options;
  auto& queued = info.queued;
  auto lines = WL(getListBuilder, legendLineHeight);
  if (info.resourceTabs.size() >= 2) {
    lines.addElem(WL(topMargin, 3, WL(getListBuilder)
        .addElemAuto(WL(label, "Material: "))
        .addElemAuto(WL(viewObject, info.resourceTabs[info.chosenTab]))
        .addElemAuto(WL(label, info.tabName))
        .buildHorizontalList()));
    lines.addSpace(5);
    auto line = WL(getListBuilder).addSpace(4);
    for (int i : All(info.resourceTabs))
      line.addElem(WL(setHeight, 32, WL(setWidth, 24,
          WL(buttonLabelFocusable,
              WL(viewObject, info.resourceTabs[i]),
              getButtonCallback({UserInputId::WORKSHOP_TAB, i}),
              [this, i] { return workshopIndex == Vec2(i, 2); }, false))), 36);
    lines.addElem(line.buildHorizontalList());
    lines.addSpace(5);
  }
  lines.addElem(WL(label, "Available:", Color::YELLOW));
  auto rawTooltip = [&] (const ItemInfo& itemInfo, optional<string> warning,
      const optional<ImmigrantCreatureInfo>& creatureInfo, int index, pair<string, int> maxUpgrades) {
    optional<string> upgradesTip;
    if (maxUpgrades.second > 0 && !maxUpgrades.first.empty())
      upgradesTip = "Upgradable with up to " + getPlural(maxUpgrades.first, maxUpgrades.second);
    if (creatureInfo) {
      return cache->get(
          [this](const ImmigrantCreatureInfo& creature, const optional<string>& warning, optional<string> upgradesTip) {
            auto lines = WL(getListBuilder, legendLineHeight)
                .addElemAuto(drawImmigrantCreature(creature));
            if (upgradesTip)
              lines.addElem(WL(label, *upgradesTip));
            if (warning)
              lines.addElem(WL(label, *warning, Color::RED));
            return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
          },
          index, *creatureInfo, warning, upgradesTip);
    }
    else {
      auto lines = WL(getListBuilder, legendLineHeight);
      if (!itemInfo.fullName.empty() && itemInfo.fullName != itemInfo.name)
        lines.addElem(WL(label, itemInfo.fullName));
      for (auto& elem : itemInfo.description)
        lines.addElem(WL(label, elem));
      if (upgradesTip)
        lines.addElem(WL(label, *upgradesTip));
      if (warning)
        lines.addElem(WL(label, *warning, Color::RED));
      if (lines.isEmpty())
        return SGuiElem(nullptr);
      return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
    }
  };
  auto createTooltip = [&] (SGuiElem content, bool rightSide) {
    if (content)
      return WL(conditional, WL(tooltip2, std::move(content),
                [rightSide](const Rectangle& r) {return rightSide ? r.bottomLeft() : r.topRight();}),
                [this] { return !disableTooltip && !workshopIndex;});
    else
      return WL(empty);
  };
  auto createControllerTooltip = [&] (SGuiElem content, bool rightSide) {
    if (content)
      return WL(conditional, WL(translate, WL(renderTopLayer, content), Vec2(0, 0), content->getPreferredSize(),
            rightSide ? GuiFactory::TranslateCorner::TOP_LEFT_SHIFTED : GuiFactory::TranslateCorner::TOP_RIGHT),
        [this] { return !disableTooltip;});
    else
      return WL(empty);
  };
  vector<SGuiElem> optionElems;
  for (int itemIndex : All(options)) {
    auto& elem = options[itemIndex].itemInfo;
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
    auto tooltip = rawTooltip(elem, none, options[itemIndex].creatureInfo, itemIndex + THIS_LINE,
        options[itemIndex].maxUpgrades);
    if (elem.unavailable) {
      CHECK(!elem.unavailableReason.empty());
      guiElem = WL(stack, createTooltip(tooltip, false), std::move(guiElem));
    } else
      guiElem = WL(stack,
          WL(conditional, WL(uiHighlightMouseOver), [this] { return !workshopIndex; }),
          std::move(guiElem),
          WL(button, getButtonCallback({UserInputId::WORKSHOP_ADD, itemIndex})),
          createTooltip(tooltip, false)
      );
    guiElem = WL(stack,
        WL(conditionalStopKeys, WL(stack,
            WL(uiHighlightFrameFilled),
            WL(keyHandler, [this, itemIndex, unavailable = elem.unavailable] {
              if (!unavailable)
              callbacks.input({UserInputId::WORKSHOP_ADD, itemIndex});
            }, {gui.getKey(C_BUILDINGS_CONFIRM)}, true),
            createControllerTooltip(tooltip, false)
        ), [myIndex = optionElems.size(), this] { return workshopIndex == Vec2(myIndex, 0);}),
        std::move(guiElem)
    );
    optionElems.push_back(guiElem);
    lines.addElem(WL(rightMargin, rightElemMargin, std::move(guiElem)));
  }
  auto lines2 = WL(getListBuilder, legendLineHeight);
  lines2.addElem(WL(label, info.queuedTitle, Color::YELLOW));
  vector<SGuiElem> queuedElems;
  for (int i : All(queued)) {
    auto& elem = queued[i];
    auto line = WL(getListBuilder);
    auto color = elem.paid ? Color::WHITE : Color::RED;
    line.addElem(WL(viewObject, elem.itemInfo.viewId), 35);
    auto label = WL(label, elem.itemInfo.name, color);
    if (elem.itemInfo.ingredient)
      label = WL(getListBuilder)
          .addElemAuto(std::move(label))
          .addElemAuto(WL(label, " from ", color))
          .addElemAuto(WL(viewObject, elem.itemInfo.ingredient->viewId))
          .buildHorizontalList();
    line.addMiddleElem(WL(renderInBounds, std::move(label)));
    function<void(Rectangle)> itemUpgradeCallback;
    if (!elem.upgrades.empty() && elem.maxUpgrades.second > 0) {
      line.addBackElemAuto(WL(leftMargin, 7, drawItemUpgradeButton(elem)));
      itemUpgradeCallback = getItemUpgradeCallback(elem);
    }
    if (elem.itemInfo.price)
      line.addBackElem(WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(*elem.itemInfo.price)), 80);
    auto changeCountCallback = getItemCountCallback(elem);
    if (info.allowChangeNumber && !elem.itemInfo.ingredient)
      line.addBackElemAuto(WL(leftMargin, 7, WL(buttonLabel, "+/-", WL(buttonRect, changeCountCallback))));
    auto removeCallback = getButtonCallback({UserInputId::WORKSHOP_CHANGE_COUNT,
        WorkshopCountInfo{ elem.itemIndex, elem.itemInfo.number, 0 }});
    line.addBackElemAuto(WL(topMargin, 3, WL(leftMargin, 7, WL(stack,
        WL(button, removeCallback),
        WL(labelUnicodeHighlight, u8"✘", Color::RED)))));
    auto tooltip = rawTooltip(elem.itemInfo, elem.paid ? optional<string>() : elem.itemInfo.unavailableReason,
        queued[i].creatureInfo, i + THIS_LINE, queued[i].maxUpgrades);
    auto guiElem = WL(stack, makeVec(
        WL(bottomMargin, 5, WL(progressBar, Color::DARK_GREEN.transparency(128), elem.productionState)),
        WL(conditionalStopKeys, WL(stack,
            WL(uiHighlightFrameFilled),
            WL(keyHandlerRect, [this, changeCountCallback, removeCallback, itemUpgradeCallback] (Rectangle r) {
              vector<SGuiElem> lines {
                WL(label, "Change count"),
                WL(label, "Remove")
              };
              vector<function<void()>> callbacks {
                [r, changeCountCallback] { changeCountCallback(r); },
                removeCallback,
              };
              if (itemUpgradeCallback) {
                lines.insert(1, WL(label, "Upgrade"));
                callbacks.insert(1, [r, itemUpgradeCallback] { itemUpgradeCallback(r); });
              }
              int selected = 0;
              drawMiniMenu(std::move(lines), std::move(callbacks), {}, r.bottomLeft(), 150, true, true, &selected);
            }, {gui.getKey(C_BUILDINGS_CONFIRM)}, true),
            createControllerTooltip(tooltip, true)

        ), [myIndex = queuedElems.size(), this] { return workshopIndex == Vec2(myIndex, 1);}),
        WL(rightMargin, rightElemMargin, line.buildHorizontalList()),
        createTooltip(std::move(tooltip), true)
    ));
    queuedElems.push_back(guiElem);
    lines2.addElem(guiElem);
  }
  return WL(preferredSize, 940, 600,
    WL(miniWindow, WL(stack, makeVec(
      WL(keyHandler, [this, ind = info.index] {
        callbacks.input({UserInputId::WORKSHOP, ind});
        workshopIndex = none;
      }, Keybinding("EXIT_MENU"), true),
      WL(keyHandler, [this, cnt = queued.size(), resourceCnt = info.resourceTabs.size()] {
        if (workshopIndex && workshopIndex->y == 2) {
          workshopIndex->x = (workshopIndex->x + 1) % resourceCnt;
        } else {
          workshopIndex = Vec2(0, 1);
          workshopsScroll2.setRelative(0, Clock::getRealMillis());
        }
      }, {gui.getKey(C_BUILDINGS_RIGHT)}, true),
      WL(keyHandler, [this, cnt = options.size(), resourceCnt = info.resourceTabs.size()] {
        if (workshopIndex && workshopIndex->y == 2) {
          workshopIndex->x = (workshopIndex->x + resourceCnt - 1) % resourceCnt;
        } else {
          workshopIndex = Vec2(0, 0);
          workshopsScroll.setRelative(0, Clock::getRealMillis());
        }
      }, {gui.getKey(C_BUILDINGS_LEFT)}, true),
      WL(keyHandler, [this, optionElems, queuedElems] {
        if (!workshopIndex)
          workshopIndex = Vec2(-1, 0);
        if (workshopIndex->y == 2)
          workshopIndex = Vec2(0, 0);
        else {
          auto& elems = (workshopIndex->y == 0 ? optionElems : queuedElems);
          workshopIndex->x = (workshopIndex->x + 1) % elems.size();
          (workshopIndex->y == 0 ? workshopsScroll : workshopsScroll2)
              .setRelative(elems[workshopIndex->x]->getBounds().top(), Clock::getRealMillis());
        }
      }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
      WL(keyHandler, [this, tab = info.chosenTab, optionElems, queuedElems, resourceCnt = info.resourceTabs.size()] {
        if (resourceCnt >= 2 && workshopIndex == Vec2(0, 0))
          workshopIndex = Vec2(tab, 2);
        else if (!workshopIndex)
          workshopIndex = Vec2(tab, 2);
        else if (workshopIndex->y < 2) {
          auto& elems = (workshopIndex->y == 0 ? optionElems : queuedElems);
          workshopIndex->x = (workshopIndex->x + elems.size() - 1) % elems.size();
          (workshopIndex->y == 0 ? workshopsScroll : workshopsScroll2)
              .setRelative(elems[workshopIndex->x]->getBounds().top(), Clock::getRealMillis());
        }
      }, {gui.getKey(C_BUILDINGS_UP)}, true),
      WL(getListBuilder, 470)
          .addElem(WL(margins, WL(scrollable,
                lines.buildVerticalList(), &workshopsScroll, &scrollbarsHeld), margin))
          .addElem(WL(margins,
              WL(scrollable, lines2.buildVerticalList(), &workshopsScroll2, &scrollbarsHeld2),
              margin))
          .buildHorizontalList()
  ))));
}

void GuiBuilder::updateWorkshopIndex(const CollectiveInfo::ChosenWorkshopInfo& info) {
  if (workshopIndex) {
    if (workshopIndex->y == 1 && info.queued.empty())
      workshopIndex->y = 0;
    switch (workshopIndex->y) {
      case 0: workshopIndex->x = min(workshopIndex->x, info.options.size() - 1); break;
      case 1: workshopIndex->x = min(workshopIndex->x, info.queued.size() - 1); break;
      case 2: workshopIndex->x = min(workshopIndex->x, info.resourceTabs.size() - 1); break;
    }
  }
  else if (hasController())
    workshopIndex = Vec2(0, 0);
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
  int numPromotions = collectiveInfo.minionPromotions.size();
  int numTechs = info.available.size();
  if (techIndex && *techIndex >= numPromotions + numTechs) {
    techIndex = numPromotions + numTechs - 1;
    if (*techIndex < 0)
      techIndex = none;
  }
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
    lines.addElem(WL(label, *info.warning, Renderer::smallTextSize(), Color::RED));
  vector<SGuiElem> activeElems;
  if (numPromotions > 0) {
    lines.addElem(WL(label, "Promotions:", Color::YELLOW));
    lines.addElem(WL(label, "(" + getPlural("item", collectiveInfo.availablePromotions) + " available)", Color::YELLOW));
    vector<SGuiElem> minionLabels;
    int maxWidth = 0;
    for (auto& elem : collectiveInfo.minionPromotions) {
      minionLabels.push_back(WL(getListBuilder)
          .addElemAuto(WL(viewObject, elem.viewId))
          .addElemAuto(WL(label, elem.name, elem.canAdvance ? Color::WHITE : Color::GRAY))
          .buildHorizontalList());
      maxWidth = max(maxWidth, *minionLabels.back()->getPreferredWidth());
    }
    for (int i : All(collectiveInfo.minionPromotions)) {
      auto& elem = collectiveInfo.minionPromotions[i];
      auto line = WL(getListBuilder)
          .addElem(WL(renderInBounds, std::move(minionLabels[i])), min(105, maxWidth))
          .addSpace(10);
      for (int index : All(elem.promotions)) {
        auto& info = elem.promotions[index];
        line.addElemAuto(WL(stack,
            WL(topMargin, -2, WL(viewObject, info.viewId)),
            getTooltip({makeSentence(info.description)}, i * 100 + index + THIS_LINE)));
      }
      line.addSpace(14);
      auto callback = [id = elem.id, options = elem.options, this] (Rectangle bounds) {
        vector<SGuiElem> lines = {WL(label, "Promotion type:")};
        vector<function<void()>> callbacks = { nullptr };
        optional<int> ret;
        for (int i : All(options)) {
          callbacks.push_back([&ret, i] { ret = i;});
          lines.push_back(WL(stack,
                WL(getListBuilder)
                    .addElemAuto(WL(viewObject, options[i].viewId))
                    .addSpace(10)
                    .addElemAuto(WL(label, options[i].name))
                    .buildHorizontalList(),
                WL(tooltip, {makeSentence(options[i].description)})
                ));
        }
        drawMiniMenu(std::move(lines), std::move(callbacks), {}, bounds.bottomLeft(), 200, false);
        if (ret)
          this->callbacks.input({UserInputId::CREATURE_PROMOTE, PromotionActionInfo{id, *ret}});
      };
      if (elem.canAdvance) {
        int myIndex = activeElems.size();
        activeElems.push_back(WL(stack, makeVec(
            WL(uiHighlightMouseOver),
            WL(conditional, WL(uiHighlightLine),
                [this, myIndex] { return techIndex == myIndex; }),
            line.buildHorizontalList(),
            WL(buttonRect, callback),
            WL(conditionalStopKeys,
                WL(keyHandlerRect, callback, {gui.getKey(C_BUILDINGS_CONFIRM), gui.getKey(C_BUILDINGS_RIGHT)}, true),
                [this, myIndex] { return collectiveTab == CollectiveTab::TECHNOLOGY && techIndex == myIndex; })
        )));
        lines.addElem(activeElems.back());
      } else
        lines.addElem(line.buildHorizontalList());
    }
  }
  auto emptyElem = WL(empty);
  auto getUnlocksTooltip = [&] (auto& elem) {
    return WL(translateAbsolute, [=]() { return emptyElem->getBounds().topRight() + Vec2(35, 0); },
        WL(renderTopLayer, drawTechUnlocks(elem)));
  };
  if (numTechs > 0) {
    lines.addElem(WL(label, "Research:", Color::YELLOW));
    lines.addElem(WL(label, "(" + getPlural("item", collectiveInfo.avatarLevelInfo.numAvailable) + " available)", Color::YELLOW));
    for (int i : All(info.available)) {
      auto& elem = info.available[i];
      auto line = WL(renderInBounds, WL(label, capitalFirst(getName(elem.id)), elem.active ? Color::WHITE : Color::GRAY));
      int myIndex = activeElems.size();
      line = WL(stack,
          WL(conditional, WL(mouseHighlight2, getUnlocksTooltip(elem)),
              [this] { return !techIndex;}),
          WL(conditional, WL(uiHighlightLine), [this, myIndex] { return techIndex == myIndex; }),
          std::move(line),
          WL(conditional, getUnlocksTooltip(elem), [this, myIndex] { return techIndex == myIndex;})
      );
      activeElems.push_back(line);
      if (elem.tutorialHighlight && tutorial && tutorial->highlights.contains(*elem.tutorialHighlight))
        line = WL(stack, WL(tutorialHighlight), std::move(line));
      if (i == 0)
        line = WL(stack, std::move(line), emptyElem);
      if (elem.active)
        line = WL(stack, makeVec(
            WL(uiHighlightMouseOver),
            std::move(line),
            WL(button, getButtonCallback({UserInputId::LIBRARY_ADD, elem.id})),
            WL(conditionalStopKeys,
                WL(keyHandler, getButtonCallback({UserInputId::LIBRARY_ADD, elem.id}),
                    {gui.getKey(C_BUILDINGS_CONFIRM), gui.getKey(C_BUILDINGS_RIGHT)}, true),
                [this, myIndex] {
                  return collectiveTab == CollectiveTab::TECHNOLOGY && techIndex == myIndex;
                })
        ));
      lines.addElem(WL(rightMargin, rightElemMargin, std::move(line)));
    }
    lines.addSpace(legendLineHeight * 2 / 3);
  }
  if (!info.researched.empty())
    lines.addElem(WL(label, "Already researched:", Color::YELLOW));
  for (int i : All(info.researched)) {
    auto& elem = info.researched[i];
    auto line = WL(renderInBounds, WL(label, capitalFirst(getName(elem.id)), Color::GRAY));
    line = WL(stack,
        std::move(line),
        WL(conditional, WL(mouseHighlight2, getUnlocksTooltip(elem)),
            [this] { return !techIndex;})
    );
    lines.addElem(WL(rightMargin, rightElemMargin, std::move(line)));
  }
  auto content = WL(stack,
      lines.buildVerticalList(),
      WL(keyHandler, [activeElems, this] {
        closeOverlayWindows();
        techIndex = 0;
        libraryScroll.setRelative(activeElems[*techIndex]->getBounds().top(), Clock::getRealMillis());
        setCollectiveTab(CollectiveTab::TECHNOLOGY);
      }, {gui.getKey(C_TECH_MENU)}, true),
      WL(conditionalStopKeys, WL(stack,
          WL(keyHandler, [activeElems, this] {
            if (!techIndex) {
              closeOverlayWindowsAndClearButton();
              techIndex = 0;
            } else
              techIndex = (*techIndex + 1) % activeElems.size();
            libraryScroll.setRelative(activeElems[*techIndex]->getBounds().top(), Clock::getRealMillis());
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          WL(keyHandler, [activeElems, this] {
            if (!techIndex) {
              closeOverlayWindowsAndClearButton();
              techIndex = 0;
            } else
              techIndex = (*techIndex + activeElems.size() - 1) % activeElems.size();
            libraryScroll.setRelative(activeElems[*techIndex]->getBounds().top(), Clock::getRealMillis());
          }, {gui.getKey(C_BUILDINGS_UP)}, true),
          WL(keyHandler, [this] { libraryScroll.reset(); techIndex = none; }, {gui.getKey(C_BUILDINGS_CANCEL)}, true)
      ), [this] { return collectiveTab == CollectiveTab::TECHNOLOGY; })
  );
  const int margin = 0;
  return WL(stopScrollEvent,
      WL(margins, WL(scrollable, std::move(content), &libraryScroll, &scrollbarsHeld), margin),
      [this] { return collectiveTab != CollectiveTab::TECHNOLOGY || !!techIndex; });
}

SGuiElem GuiBuilder::drawMinionsOverlay(const CollectiveInfo::ChosenCreatureInfo& chosenCreature,
    const optional<TutorialInfo>& tutorial) {
  int margin = 20;
  int minionListWidth = 220;
  setCollectiveTab(CollectiveTab::MINIONS);
  SGuiElem minionPage;
  auto& minions = chosenCreature.creatures;
  auto current = chosenCreature.chosenId;
  for (int i : All(minions))
    if (minions[i].creatureId == current)
      minionPage = WL(margins, drawMinionPage(minions[i], tutorial), 10, 15, 10, 10);
  if (!minionPage)
    return WL(empty);
  SGuiElem menu;
  SGuiElem leftSide = drawMinionButtons(minions, current, chosenCreature.teamId);
  if (chosenCreature.teamId) {
    auto list = WL(getListBuilder, legendLineHeight);
    list.addElem(
        WL(buttonLabel, "Disband team", getButtonCallback({UserInputId::CANCEL_TEAM, *chosenCreature.teamId})));
    list.addElem(WL(label, "Control a chosen minion to", Renderer::smallTextSize(),
          Color::LIGHT_GRAY), Renderer::smallTextSize() + 2);
    list.addElem(WL(label, "command the team.", Renderer::smallTextSize(),
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
  return WL(preferredSize, 720 + minionListWidth, 600,
      WL(miniWindow, WL(stack,
      WL(keyHandler, [this] { callbacks.input({UserInputId::CREATURE_BUTTON, UniqueEntity<Creature>::Id()}); minionsIndex = none; },
          Keybinding("EXIT_MENU"), true),
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
    list.addElem(WL(label, minion.description, Renderer::smallTextSize(), Color::LIGHT_GRAY));
  auto leftLines = WL(getListBuilder, legendLineHeight);
  leftLines.addElem(WL(label, "Attributes", Color::YELLOW));
  leftLines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(minion.attributes)));
  for (auto& elem : drawEffectsList(minion))
    leftLines.addElem(std::move(elem));
  leftLines.addSpace();
  if (auto elem = drawTrainingInfo(minion.experienceInfo, true))
    leftLines.addElemAuto(std::move(elem));
  if (!minion.spellSchools.empty()) {
    auto line = WL(getListBuilder)
        .addElemAuto(WL(label, "Spell schools: ", Color::YELLOW));
    for (auto& school : minion.spellSchools)
      line.addElemAuto(drawSpellSchoolLabel(school));
    leftLines.addElem(line.buildHorizontalList());
  }
  leftLines.addSpace();
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
          WL(keyHandler, [=] { toggleBottomWindow(BottomWindowId::BESTIARY); }, Keybinding("EXIT_MENU"), true),
          WL(keyHandler, [this, cnt = creatures.size()] {
            bestiaryIndex = (bestiaryIndex + 1) % cnt;
            minionButtonsScroll.set(bestiaryIndex * legendLineHeight, Clock::getRealMillis());
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          WL(keyHandler, [this, cnt = creatures.size()] {
            bestiaryIndex = (bestiaryIndex - 1 + cnt) % cnt;
            minionButtonsScroll.set(bestiaryIndex * legendLineHeight, Clock::getRealMillis());
          }, {gui.getKey(C_BUILDINGS_UP)}, true),
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
          WL(keyHandler, [=] { toggleBottomWindow(BottomWindowId::SPELL_SCHOOLS); }, Keybinding("EXIT_MENU"), true),
          WL(keyHandler, [this, cnt = schools.size()] {
            spellSchoolIndex = (spellSchoolIndex + 1) % cnt;
            minionButtonsScroll.set(spellSchoolIndex * legendLineHeight, Clock::getRealMillis());
          }, {gui.getKey(C_BUILDINGS_DOWN)}, true),
          WL(keyHandler, [this, cnt = schools.size()] {
            spellSchoolIndex = (spellSchoolIndex - 1 + cnt) % cnt;
            minionButtonsScroll.set(spellSchoolIndex * legendLineHeight, Clock::getRealMillis());
          }, {gui.getKey(C_BUILDINGS_UP)}, true),
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
          WL(keyHandler, [=] { toggleBottomWindow(BottomWindowId::ITEMS_HELP); }, Keybinding("EXIT_MENU"), true),
          WL(margins, std::move(leftSide), margin))));
}

SGuiElem GuiBuilder::drawBuildingsOverlay(const vector<CollectiveInfo::Button>& buildings,
    const optional<TutorialInfo>& tutorial) {
  vector<SGuiElem> elems;
  map<string, GuiFactory::ListBuilder> overlaysMap;
  int margin = 20;
  for (int i : All(buildings)) {
    auto& elem = buildings[i];
    if (!elem.groupName.empty()) {
      if (!overlaysMap.count(elem.groupName))
        overlaysMap.emplace(make_pair(elem.groupName, WL(getListBuilder, legendLineHeight)));
      overlaysMap.at(elem.groupName).addElem(getButtonLine(elem, i, tutorial));
      elems.push_back(WL(setWidth, 350, WL(conditional,
            WL(miniWindow, WL(margins, getButtonLine(elem, i, tutorial), margin)),
            [i, this] { return getActiveButton() == i;})));
    }
  }
  for (auto& elem : overlaysMap) {
    auto& lines = elem.second;
    string groupName = elem.first;
    elems.push_back(WL(setWidth, 350, WL(conditionalStopKeys,
          WL(miniWindow, WL(stack,
              WL(keyHandler, [=] { clearActiveButton(); }, Keybinding("EXIT_MENU"), true),
              WL(margins, lines.buildVerticalList(), margin))),
          [this, groupName] {
              return collectiveTab == CollectiveTab::BUILDINGS && activeGroup == groupName;
          })));
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
    lines.addElem(WL(label, capitalFirst(rightClickText()) + ":", Color::LIGHT_BLUE));
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

static string getLuxuryNumber(double morale) {
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
    if (auto& index = highlighted.viewIndex) {
      auto tryHighlight = [&] (HighlightType type, const char* text, Color color) {
        if (index->isHighlight(type)) {
          lines.addElem(WL(getListBuilder)
                .addElem(WL(viewObject, ViewId("magic_field", color)), 30)
                .addElemAuto(WL(label, text))
                .buildHorizontalList());
          lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
        }
      };
      tryHighlight(HighlightType::ALLIED_TOTEM, "Allied magical field", Color::GREEN);
      tryHighlight(HighlightType::HOSTILE_TOTEM, "Hostile magical field", Color::RED);
      for (auto& gas : index->getGasAmounts()) {
        lines.addElem(WL(getListBuilder)
              .addElem(WL(viewObject, ViewId("tile_gas", gas.color.transparency(254))), 30)
              .addElemAuto(WL(label, capitalFirst(gas.name)))
              .buildHorizontalList());
        lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
      }
      for (auto layer : ENUM_ALL_REVERSE(ViewLayer))
        if (index->hasObject(layer)) {
          auto& viewObject = index->getObject(layer);
          lines.addElemAuto(WL(getListBuilder)
                .addElem(WL(viewObject, viewObject.getViewIdList()), 30)
                .addElemAuto(WL(labelMultiLineWidth, viewObject.getDescription(), legendLineHeight * 2 / 3, 300))
                .buildHorizontalList());
          lines.addSpace(legendLineHeight / 3);
          if (layer == ViewLayer::CREATURE)
            lines.addElemAuto(drawLyingItemsList("Inventory: ", index->getEquipmentCounts(), 250));
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
                Renderer::textSize(), Color::RED, ','));
            lines.addSpace(legendLineHeight / 3);
          }
          if (!viewObject.getGoodAdjectives().empty()) {
            lines.addElemAuto(WL(labelMultiLineWidth, viewObject.getGoodAdjectives(), legendLineHeight * 2 / 3, 300,
                Renderer::textSize(), Color::GREEN, ','));
            lines.addSpace(legendLineHeight / 3);
          }
          if (viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
            lines.addElem(WL(label, "Can only be healed using rituals."));
          if (auto value = viewObject.getAttribute(ViewObjectAttribute::FLANKED_MOD)) {
            lines.addElem(WL(label, "Flanked: defense reduced by " + toString<int>(100 * (1 - *value)) + "%.", Color::RED));
            if (viewObject.hasModifier(ViewObject::Modifier::PLAYER))
              lines.addElem(WL(label, "Use a shield!", Color::RED));
          }
          if (auto& attributes = viewObject.getCreatureAttributes())
            lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(*attributes)));
          if (auto health = viewObject.getAttribute(ViewObjectAttribute::HEALTH))
            lines.addElem(WL(stack,
                  WL(margins, WL(progressBar, MapGui::getHealthBarColor(*health,
                      viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE)).transparency(70), *health), -2, 0, 0, 3),
                  WL(label, getHealthName(viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
                      + toString((int) (100.0f * *health)) + "%")));
          if (auto luxury = viewObject.getAttribute(ViewObjectAttribute::LUXURY))
            lines.addElem(WL(stack,
                  WL(margins, WL(progressBar, Color::GREEN.transparency(70), fabs(*luxury)), -2, 0, 0, 3),
                  WL(label, "Luxury: " + getLuxuryNumber(*luxury))));
          if (viewObject.hasModifier(ViewObjectModifier::UNPAID))
            lines.addElem(WL(label, "Cannot afford item", Color::RED));
          if (viewObject.hasModifier(ViewObjectModifier::PLANNED))
            lines.addElem(WL(label, "Planned"));
          lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
        }
      if (auto& quarters = mapGui->getQuartersInfo()) {
        lines.addElem(WL(label, "Quarters:"));
        if (quarters->viewId)
          lines.addElem(WL(getListBuilder)
              .addElemAuto(WL(viewObject, *quarters->viewId))
              .addElemAuto(WL(label, *quarters->name))
              .buildHorizontalList());
        else
          lines.addElem(WL(label, "Unassigned"));
        lines.addElem(WL(label, "Total luxury: " + getLuxuryNumber(quarters->luxury)));
        lines.addElem(WL(label, "Click to assign"));
        lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
      }
      if (index->isHighlight(HighlightType::INSUFFICIENT_LIGHT))
        lines.addElem(WL(label, "Insufficient light", Color::RED));
      if (index->isHighlight(HighlightType::TORTURE_UNAVAILABLE))
        lines.addElem(WL(label, "Torture unavailable due to population limit", Color::RED));
      if (index->isHighlight(HighlightType::PRISON_NOT_CLOSED))
        lines.addElem(WL(label, "Prison must be separated from the outdoors and from all staircases using prison bars or prison door", Color::RED));
      if (index->isHighlight(HighlightType::PIGSTY_NOT_CLOSED))
        lines.addElem(WL(label, "Animal pen must be separated from the outdoors and from all staircases using animal fence", Color::RED));
      if (index->isHighlight(HighlightType::INDOORS))
        lines.addElem(WL(label, "Indoors"));
      else
        lines.addElem(WL(label, "Outdoors"));
      lines.addElemAuto(drawLyingItemsList("Lying here: ", index->getItemCounts(), 250));
    }
    if (highlighted.tilePos)
      lines.addElem(WL(label, "Position: " + toString(*highlighted.tilePos)));
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
      WL(keyHandler, getButtonCallback(UserInputId::CANCEL_SCREENSHOT), Keybinding("EXIT_MENU"), true),
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

void GuiBuilder::drawOverlays(vector<OverlayInfo>& ret, const GameInfo& info) {
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
              info.villageInfo, collectiveInfo.nextWave, collectiveInfo.rebellionChance, villainsIndex),
          OverlayInfo::VILLAINS});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawImmigrationOverlay, this), THIS_LINE,
          collectiveInfo.immigration, info.tutorial, !collectiveInfo.allImmigration.empty()),
          OverlayInfo::IMMIGRATION});
      if (!collectiveInfo.chosenCreature)
        minionPageIndex = {};
      else
        for (auto g : Iter(collectiveInfo.minionGroups))
          if (g->highlight) {
            minionsIndex = g.index();
            break;
          }
      if (collectiveInfo.chosenCreature)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawMinionsOverlay, this), THIS_LINE,
            *collectiveInfo.chosenCreature, info.tutorial), OverlayInfo::TOP_LEFT});
      else if (collectiveInfo.chosenWorkshop) {
        updateWorkshopIndex(*collectiveInfo.chosenWorkshop);
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawWorkshopsOverlay, this), THIS_LINE,
            *collectiveInfo.chosenWorkshop, info.tutorial, workshopIndex), OverlayInfo::TOP_LEFT});
      } else if (bottomWindow == TASKS)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawTasksOverlay, this), THIS_LINE,
            collectiveInfo), OverlayInfo::TOP_LEFT});
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawBuildingsOverlay, this), THIS_LINE,
          collectiveInfo.buildings, info.tutorial), OverlayInfo::TOP_LEFT});
      if (bottomWindow == IMMIGRATION_HELP)
        ret.push_back({cache->get(bindMethod(&GuiBuilder::drawImmigrationHelp, this), THIS_LINE,
            collectiveInfo), OverlayInfo::BOTTOM_LEFT});
      break;
    }
    case GameInfo::InfoType::PLAYER: {
      auto& playerInfo = *info.playerInfo.getReferenceMaybe<PlayerInfo>();
      ret.push_back({cache->get(bindMethod(&GuiBuilder::drawPlayerOverlay, this), THIS_LINE,
          playerInfo, playerOverlayFocused), OverlayInfo::TOP_LEFT});
      break;
    }
    default:
      break;
  }
  if (bottomWindow == SCRIPTED_HELP) {
    ScriptedUIDataElems::Record data;
    for (auto& elem : info.scriptedHelp) {
      auto pageName = elem.scriptedId;
      data.elems.insert(make_pair(pageName,
        ScriptedUIDataElems::Callback{
          [pageName, this] { openScriptedHelp(pageName); return false;}
        }));
    }
    scriptedUIData = std::move(data);
    ret.push_back({
        WL(stack,
            gui.scripted([this]{ bottomWindow = none; }, scriptedHelpId, scriptedUIData, scriptedUIState),
            WL(keyHandler, [this]{ bottomWindow = none; }, Keybinding("EXIT_MENU"), true)
        ),
        OverlayInfo::TOP_LEFT
    });
  }
  if (bottomWindow == BESTIARY) {
    if (bestiaryIndex >= info.encyclopedia->bestiary.size())
      bestiaryIndex = 0;
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawBestiaryOverlay, this), THIS_LINE,
         info.encyclopedia->bestiary, bestiaryIndex), OverlayInfo::TOP_LEFT});
  }
  if (bottomWindow == SPELL_SCHOOLS) {
    if (spellSchoolIndex >= info.encyclopedia->spellSchools.size())
      spellSchoolIndex = 0;
    ret.push_back({cache->get(bindMethod(&GuiBuilder::drawSpellSchoolsOverlay, this), THIS_LINE,
         info.encyclopedia->spellSchools, spellSchoolIndex), OverlayInfo::TOP_LEFT});
  }
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

Rectangle GuiBuilder::getMenuPosition(int numElems) {
  int windowWidth = 800;
  int windowHeight = 400;
  int ySpacing = 100;
  int yOffset = 0;
  int xSpacing = (renderer.getSize().x - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing + yOffset, xSpacing + windowWidth, renderer.getSize().y - ySpacing + yOffset);
}

static vector<PlayerInfo> groupByViewId(const vector<PlayerInfo>& minions) {
  vector<vector<PlayerInfo>> groups;
  for (auto& elem : minions) [&] {
    for (auto& g : groups)
      if (g[0].viewId == elem.viewId) {
        g.push_back(elem);
        return;
      }
    groups.push_back({elem});
  }();
  vector<PlayerInfo> ret;
  for (auto& elem : groups)
    ret.append(elem);
  return ret;
}

SGuiElem GuiBuilder::drawMinionButtons(const vector<PlayerInfo>& minions1, UniqueEntity<Creature>::Id current,
    optional<TeamId> teamId) {
  CHECK(!minions1.empty());
  auto list = WL(getListBuilder, legendLineHeight);
  auto buttonCnt = 0;
  auto minions = groupByViewId(minions1);
  vector<SGuiElem> allLines;
  optional<int> currentIndex;
  for (int i : All(minions)) {
    auto& minion = minions[i];
    if (minion.creatureId == current)
      currentIndex = i;
    if (i == 0 || minions[i - 1].viewId != minion.viewId)
      list.addElem(WL(topMargin, 5, WL(viewObject, minion.viewId)), legendLineHeight + 5);
    auto minionId = minion.creatureId;
    auto line = WL(getListBuilder);
    if (teamId)
      line.addElem(WL(leftMargin, -16, WL(stack,
          WL(button, getButtonCallback({UserInputId::REMOVE_FROM_TEAM, TeamCreatureInfo{*teamId, minionId}})),
          WL(labelUnicodeHighlight, u8"✘", Color::RED))), 1);
    line.addMiddleElem(WL(rightMargin, 5, WL(renderInBounds, WL(label, minion.getFirstName()))));
    line.addBackElem(drawBestAttack(minion.bestAttack), 52);
    line.addBackSpace(5);
    auto selectButton = [this, minionId](UniqueEntity<Creature>::Id creatureId) {
      return WL(releaseLeftButton, getButtonCallback({UserInputId::CREATURE_BUTTON, minionId}));
    };
    auto lineTmp = line.buildHorizontalList();
    allLines.push_back(lineTmp);
    list.addElem(WL(stack, makeVec(
          cache->get(selectButton, THIS_LINE, minionId),
          WL(leftMargin, teamId ? -10 : 0, WL(stack,
               WL(uiHighlightLineConditional, [=] { return !teamId && mapGui->isCreatureHighlighted(minionId);}, Color::YELLOW),
               WL(uiHighlightLineConditional, [=] { return current == minionId;}),
               (current == minionId && hasController())
                   ? WL(uiHighlightFrame, [this] { return minionPageIndex == MinionPageElems::None{};})
                   : WL(empty)
          )),
          WL(dragSource, minionId, [=]{ return WL(viewObject, minion.viewId);}),
          std::move(lineTmp))));
  }
  auto getFocusFunc = [&](int inc) -> function<void()> {
    auto nextInd = (*currentIndex + inc + minions.size()) % minions.size();
    auto nextElem = allLines[nextInd];
    auto nextId = minions[nextInd].creatureId;
    return [this, nextElem, nextId] {
      callbacks.input({UserInputId::CREATURE_BUTTON, nextId});
      minionButtonsScroll.setRelative(nextElem->getBounds().top(), Clock::getRealMillis());
    };
  };
  return WL(stack,
      WL(keyHandler, getFocusFunc(1), {gui.getKey(C_BUILDINGS_DOWN), gui.getKey(SDL::SDLK_DOWN)}, true),
      WL(keyHandler, getFocusFunc(-1), {gui.getKey(C_BUILDINGS_UP), gui.getKey(SDL::SDLK_UP)}, true),
      WL(scrollable, WL(rightMargin, 10, list.buildVerticalList()), &minionButtonsScroll, &scrollbarsHeld)
  );
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
    case MinionActivity::GUARDING1: return "Guard zone 1";
    case MinionActivity::GUARDING2: return "Guard zone 2";
    case MinionActivity::GUARDING3: return "Guard zone 3";
    case MinionActivity::DISTILLATION: return "Distilling";
    case MinionActivity::BE_WHIPPED: return "Being whipped";
    case MinionActivity::BE_TORTURED: return "Being tortured";
    case MinionActivity::BE_EXECUTED: return "Being executed";
    case MinionActivity::PHYLACTERY: return "Phylactery";
  }
}

static ViewId getViewId(MinionActivity option) {
  switch (option) {
    case MinionActivity::IDLE: return ViewId("flower1");
    case MinionActivity::SLEEP: return ViewId("bed1");
    case MinionActivity::CONSTRUCTION: return ViewId("castle_wall");
    case MinionActivity::DIGGING: return ViewId("dig_icon");
    case MinionActivity::HAULING: return ViewId("fetch_icon");
    case MinionActivity::WORKING: return ViewId("imp");
    case MinionActivity::EAT: return ViewId("pig");
    case MinionActivity::EXPLORE_NOCTURNAL: return ViewId("bat");
    case MinionActivity::EXPLORE_CAVES: return ViewId("bear");
    case MinionActivity::EXPLORE: return ViewId("wolf");
    case MinionActivity::RITUAL: return ViewId("demon_shrine");
    case MinionActivity::CROPS: return ViewId("swish_wheat");
    case MinionActivity::TRAIN: return ViewId("training_wood");
    case MinionActivity::ARCHERY: return ViewId("archery_range");
    case MinionActivity::CRAFT: return ViewId("workshop");
    case MinionActivity::STUDY: return ViewId("bookcase_wood");
    case MinionActivity::POETRY: return ViewId("painting_n");
    case MinionActivity::MINION_ABUSE: return ViewId("keeper4");
    case MinionActivity::COPULATE: return ViewId("succubus");
    case MinionActivity::SPIDER: return ViewId("spider");
    case MinionActivity::GUARDING1: return getViewId(ZoneId::GUARD1);
    case MinionActivity::GUARDING2: return getViewId(ZoneId::GUARD2);
    case MinionActivity::GUARDING3: return getViewId(ZoneId::GUARD3);
    case MinionActivity::DISTILLATION: return ViewId("distillery");
    case MinionActivity::BE_WHIPPED: return ViewId("whipping_post");
    case MinionActivity::BE_TORTURED: return ViewId("torture_table");
    case MinionActivity::BE_EXECUTED: return ViewId("gallows");
    case MinionActivity::PHYLACTERY: return ViewId("phylactery");
  }
}

static Color getTaskColor(PlayerInfo::MinionActivityInfo info) {
  if (info.current)
    return Color::GREEN;
  if (info.inactive)
    return Color::GRAY;
  else
    return Color::WHITE;
}

vector<SGuiElem> GuiBuilder::drawItemMenu(const vector<ItemInfo>& items, ItemMenuCallback callback,
    bool doneBut) {
  vector<SGuiElem> lines;
  for (int i : All(items))
    lines.push_back(getItemLine(items[i], [=] (Rectangle bounds) { callback(bounds, i);}, nullptr, true));
  if (doneBut)
    lines.push_back(WL(stack,
          WL(button, [=] { callback(Rectangle(), none); }),
          WL(keyHandler, [=] { callback(Rectangle(), none); }, Keybinding("EXIT_MENU")),
          WL(centeredLabel, Renderer::HOR, "[done]", Color::LIGHT_BLUE)));
  return lines;
}

function<void(Rectangle)> GuiBuilder::getActivityButtonFun(const PlayerInfo& minion) {
  return [=] (Rectangle bounds) {
    auto tasks = WL(getListBuilder, legendLineHeight);
    auto glyph1 = WL(getListBuilder);
    if (hasController())
      glyph1.addElemAuto(WL(label, "Set current activity ", Renderer::smallTextSize()))
          .addElemAuto(WL(steamInputGlyph, C_BUILDINGS_CONFIRM));
    auto glyph2 = WL(steamInputGlyph, C_BUILDINGS_LEFT);
    auto glyph3 = WL(steamInputGlyph, C_BUILDINGS_RIGHT);
    tasks.addElem(WL(getListBuilder)
        .addElemAuto(glyph1.buildHorizontalList())
        .addBackElemAuto(WL(label, "Enable", Renderer::smallTextSize()))
        .addBackSpace(5)
        .addBackElem(glyph2, 35)
        .addBackSpace(10)
        .addBackElem(WL(getListBuilder)
            .addMiddleElem(WL(renderInBounds,
                WL(label, "Disable for all " + makePlural(minion.groupName), Renderer::smallTextSize())))
            .addBackElem(glyph3, hasController() ? 35 : 1)
            .buildHorizontalList(),
            164)
        .buildHorizontalList());
    bool exit = false;
    int selected = hasController() ? 0 : -1;
    TaskActionInfo retAction;
    retAction.creature = minion.creatureId;
    retAction.groupName = minion.groupName;
    vector<SGuiElem> activeElems;
    for (int i : All(minion.minionTasks)) {
      auto& task = minion.minionTasks[i];
      function<void()> buttonFun = [] {};
      if (!task.inactive)
        buttonFun = [&exit, &retAction, task] {
          if (retAction.lock.contains(task.task) == task.locked &&
              retAction.lockGroup.contains(task.task) == task.lockedForGroup) {
            retAction.switchTo = task.task;
            exit = true;
          }
        };
      auto lockButton = WL(rightMargin, 20, WL(conditional,
            [&retAction, task] {
              if (task.canLock && !(retAction.lockGroup.contains(task.task) ^ task.lockedForGroup)) {
                if (retAction.lock.contains(task.task) ^ task.locked)
                  return 1;
                else
                  return 2;
              } else
                return 0;
            }, {WL(empty), WL(labelUnicodeHighlight, u8"✘", Color::RED),
                 WL(labelUnicodeHighlight, u8"✓", Color::GREEN)}));
      auto lockButton2 = task.canLock
            ? WL(rightMargin, 20, WL(conditional, WL(labelUnicodeHighlight, u8"✘", Color::RED),
                 WL(labelUnicodeHighlight, u8"✘", Color::LIGHT_GRAY), [&retAction, task] {
                      return retAction.lockGroup.contains(task.task) ^ task.lockedForGroup;}))
            : WL(empty);
      auto lockCallback1 = [&retAction, task] {
        retAction.lock.toggle(task.task);
      };
      auto lockCallback2 = [&retAction, task] {
        retAction.lockGroup.toggle(task.task);
      };
      auto allName = makePlural(minion.groupName);
      activeElems.push_back(WL(stack,
          WL(conditionalStopKeys, WL(stack,
              WL(uiHighlightLine),
              WL(keyHandler, buttonFun, Keybinding("MENU_SELECT"), true),
              WL(keyHandler, lockCallback1, Keybinding("MENU_LEFT"), true),
              WL(keyHandler, lockCallback2, Keybinding("MENU_RIGHT"), true)
          ), [&selected, i] { return selected == i;}),
          WL(getListBuilder)
              .addMiddleElem(WL(stack,
                  WL(button, buttonFun),
                  WL(getListBuilder)
                      .addElem(WL(viewObject, getViewId(task.task)), 30)
                      .addElemAuto(WL(label, getTaskText(task.task), getTaskColor(task)))
                      .buildHorizontalList()
              ))
              .addBackElem(WL(stack,
                  getTooltip({"Click to turn this activity on/off for this minion."}, THIS_LINE + i,
                      milliseconds{700}, true),
                  WL(button, lockCallback1),
                  lockButton), 37)
              .addBackSpace(51)
              .addBackElemAuto(WL(stack,
                  getTooltip({"Click to turn this activity off for all " + allName}, THIS_LINE + i + 54321,
                      milliseconds{700}, true),
                  WL(button, lockCallback2),
                  lockButton2))
              .addBackSpace(130)
              .buildHorizontalList()
      ));
      tasks.addElem(activeElems.back());
    }
    auto content = WL(stack,
        getMiniMenuScrolling(activeElems, selected),
        tasks.buildVerticalList()
    );
    drawMiniMenu(std::move(content), exit, bounds.bottomLeft(), 500, true);
    callbacks.input({UserInputId::CREATURE_TASK_ACTION, retAction});
  };
}

static const char* getName(AIType t) {
  switch (t) {
    case AIType::MELEE: return "Melee";
    case AIType::RANGED: return "Avoid melee";
  }
}

static ViewId getViewId(AIType t) {
  switch (t) {
    case AIType::MELEE: return ViewId("damage_icon");
    case AIType::RANGED: return ViewId("bow");
  }
}

function<void(Rectangle)> GuiBuilder::getAIButtonFun(const PlayerInfo& minion) {
  return [=] (Rectangle bounds) {
    vector<SGuiElem> tasks;
    vector<function<void()>> callbacks;
    bool exit = false;
    AIActionInfo retAction;
    retAction.creature = minion.creatureId;
    retAction.groupName = minion.groupName;
    retAction.switchTo = minion.aiType;
    retAction.override = false;
    for (auto type : ENUM_ALL(AIType)) {
      callbacks.push_back([&, type] {
        retAction.switchTo = type;
        exit = true;
      });
      tasks.push_back(WL(getListBuilder)
          .addElem(WL(viewObject, getViewId(type)), 30)
          .addElemAuto(WL(label, getName(type),
              [&, type] { return retAction.switchTo == type ? Color::LIGHT_GREEN : Color::WHITE; }))
          .buildHorizontalList());
    }
    callbacks.push_back([&]{ retAction.override = !retAction.override; });
    tasks.push_back(WL(getListBuilder)
        .addElemAuto(WL(conditional, WL(labelUnicodeHighlight, u8"✓", Color::GREEN),
             WL(labelUnicodeHighlight, u8"✓", Color::LIGHT_GRAY), [&retAction] {
                  return retAction.override;}))
        .addElemAuto(WL(label, "Copy setting to all " + makePlural(minion.groupName)))
        .buildHorizontalList());
    drawMiniMenu(std::move(tasks), std::move(callbacks), {}, bounds.bottomLeft(), 362, true, false, nullptr, &exit);
    this->callbacks.input({UserInputId::AI_TYPE, retAction});
  };
}

SGuiElem GuiBuilder::drawAttributesOnPage(vector<SGuiElem> attrs) {
  if (attrs.empty())
    return WL(empty);
  vector<vector<SGuiElem>> lines = {{}};
  for (int i : All(attrs)) {
    lines.back().push_back(std::move(attrs[i]));
    if (lines.back().size() >= 3)
      lines.emplace_back();
  }
  auto list = WL(getListBuilder, legendLineHeight);
  for (auto& elem : lines)
    list.addElem(WL(horizontalList, std::move(elem), 100));
  return list.buildVerticalList();
}

function<void(Rectangle)> GuiBuilder::getEquipmentGroupsFun(const PlayerInfo& minion) {
  return [=] (Rectangle bounds) {
    EquipmentGroupAction ret { minion.groupName, {}};
    vector<SGuiElem> lines;
    vector<function<void()>> callbacks;
    lines.push_back(WL(label, "Setting will apply to all " + makePlural(minion.groupName),
        Renderer::smallTextSize(), Color::LIGHT_GRAY));
    callbacks.push_back(nullptr);
    for (auto& group : minion.equipmentGroups) {
      callbacks.push_back([&ret, name = group.name] {
        if (ret.flip.count(name))
          ret.flip.erase(name);
        else
        ret.flip.insert(name);
      });
      lines.push_back(WL(getListBuilder)
          .addElemAuto(WL(viewObject, group.viewId))
          .addElemAuto(WL(label, group.name))
          .addBackElem(WL(conditional, WL(labelUnicodeHighlight, u8"✘", Color::RED),
              WL(labelUnicodeHighlight, u8"✓", Color::GREEN),
              [&ret, group] { return group.locked ^ ret.flip.count(group.name); }), 30)
          .buildHorizontalList()
      );
    }
    auto chooseButton = [&]{
      for (auto& group : minion.equipmentGroups)
        if (!(group.locked ^ ret.flip.count(group.name)))
          return true;
      return false;
    };
    auto restrictButton = WL(getListBuilder)
        .addElemAuto(WL(labelUnicodeHighlight, u8"✘", Color::RED))
        .addElemAuto(WL(label, "Restrict all"))
        .buildHorizontalList();
    auto unrestrictButton = WL(getListBuilder)
        .addElemAuto(WL(labelUnicodeHighlight, u8"✓", Color::GREEN))
        .addElemAuto(WL(label, "Unrestrict all"))
        .buildHorizontalList();
    lines.push_back(WL(conditional, restrictButton, unrestrictButton, chooseButton));
    callbacks.push_back([&] {
      ret.flip.clear();
      auto type = chooseButton();
      for (auto& group : minion.equipmentGroups)
        if (!group.locked == type)
          ret.flip.insert(group.name);
    });
    drawMiniMenu(std::move(lines), std::move(callbacks), {}, bounds.bottomLeft(), 350, true, false);
    this->callbacks.input({UserInputId::EQUIPMENT_GROUP_ACTION, ret});
  };
}

SGuiElem GuiBuilder::drawEquipmentAndConsumables(const PlayerInfo& minion, bool infoOnly) {
  const vector<ItemInfo>& items = minion.inventory;
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addSpace(5);
  if (!items.empty()) {
    auto titleLine = WL(getListBuilder)
        .addElemAuto(WL(label, "Equipment", Color::YELLOW));
    lines.addElem(titleLine.buildHorizontalList());
    vector<SGuiElem> itemElems;
    if (!infoOnly)
      for (int i : All(items)) {
        SGuiElem keyElem = WL(empty);
        auto callback = [this, creatureId = minion.creatureId, item = items[i]] (Rectangle bounds) {
          if (auto choice = getItemChoice(item, bounds.bottomLeft() + Vec2(50, 0), true))
            callbacks.input({UserInputId::CREATURE_EQUIPMENT_ACTION,
                EquipmentActionInfo{creatureId, item.ids, item.slot, *choice}});
        };
        auto labelElem = WL(stack,
            WL(conditionalStopKeys, WL(stack,
                WL(keyHandlerRect, callback, {gui.getKey(C_BUILDINGS_CONFIRM)}, true),
                WL(uiHighlightLine)
            ), [this, i] { return minionPageIndex == MinionPageElems::Equipment{i}; }),
            getItemLine(items[i], callback, nullptr, false, false)
        );
        if (!items[i].ids.empty()) {
          int offset = *labelElem->getPreferredWidth();
          auto highlight = WL(leftMargin, offset, WL(labelUnicode, u8"✘", Color::RED));
          if (!items[i].locked)
            highlight = WL(stack, std::move(highlight), WL(leftMargin, -20, WL(viewObject, ViewId("key_highlight"))));
          labelElem = WL(stack, std::move(labelElem),
                      WL(mouseHighlight2, std::move(highlight), nullptr, false));
          keyElem = WL(stack,
              WL(button, getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
                  EquipmentActionInfo{minion.creatureId, items[i].ids, items[i].slot, ItemAction::LOCK}})),
              items[i].locked ? WL(viewObject, ViewId("key")) : WL(mouseHighlight2, WL(viewObject, ViewId("key_highlight"))),
              getTooltip({"Locked items won't be automatically swapped by minion."}, THIS_LINE + i)
          );
        }
        if (items[i].type == items[i].CONSUMABLE && (i == 0 || items[i].type != items[i - 1].type))
          lines.addElem(WL(label, "Consumables", Color::YELLOW));
        if (items[i].type == items[i].OTHER && (i == 0 || items[i].type != items[i - 1].type))
          lines.addElem(WL(label, "Other", Color::YELLOW));
        lines.addElem(WL(leftMargin, 3,
            WL(getListBuilder)
                .addElem(std::move(keyElem), 20)
                .addMiddleElem(std::move(labelElem))
                .buildHorizontalList()));
        if (i == items.size() - 1 || (items[i + 1].type == items[i].OTHER && (items[i].type != items[i + 1].type)))
          lines.addElem(WL(buttonLabelFocusable, "Add consumable",
              getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
                  EquipmentActionInfo{minion.creatureId, {}, none, ItemAction::REPLACE}}),
              [this, ind = items.size()] { return minionPageIndex == MinionPageElems::Equipment{ind}; }));
      }
    else
      for (int i : All(items))
        lines.addElem(getItemLine(items[i], [](Rectangle) {}, nullptr, false, false));
  }
  if (!minion.intrinsicAttacks.empty()) {
    lines.addElem(WL(label, "Intrinsic attacks", Color::YELLOW));
    for (auto& item : minion.intrinsicAttacks)
      lines.addElem(getItemLine(item, [=](Rectangle) {}));
  }
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawMinionActions(const PlayerInfo& minion, const optional<TutorialInfo>& tutorial) {
  const int buttonWidth = 110;
  const int buttonSpacing = 15;
  auto line = WL(getListBuilder, buttonWidth);
  const bool tutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
  for (auto action : Iter(minion.actions)) {
    auto focusCallback = [this, action]{ return minionPageIndex == MinionPageElems::MinionAction{action.index()};};
    switch (*action) {
      case PlayerInfo::CONTROL: {
        auto callback = getButtonCallback({UserInputId::CREATURE_CONTROL, minion.creatureId});
        line.addElem(tutorialHighlight
            ? WL(buttonLabelBlink, "Control", callback, focusCallback, false, true)
            : WL(buttonLabelFocusable, "Control", callback, focusCallback, false, true));
        break;
      }
      case PlayerInfo::RENAME:
        line.addElem(WL(buttonLabelFocusable, "Rename",
            getButtonCallback({UserInputId::CREATURE_RENAME, minion.creatureId}), focusCallback, false, true));
        break;
      case PlayerInfo::BANISH:
        line.addElem(WL(buttonLabelFocusable, "Banish",
            getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId}), focusCallback, false, true));
        break;
      case PlayerInfo::DISASSEMBLE:
        line.addElem(WL(buttonLabelFocusable, "Disassemble",
            getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId}), focusCallback, false, true));
        break;
      case PlayerInfo::CONSUME:
        line.addElem(WL(buttonLabelFocusable, "Absorb",
            getButtonCallback({UserInputId::CREATURE_CONSUME, minion.creatureId}), focusCallback, false, true));
        break;
      case PlayerInfo::LOCATE:
        line.addElem(WL(buttonLabelFocusable, "Locate",
            getButtonCallback({UserInputId::CREATURE_LOCATE, minion.creatureId}), focusCallback, false, true));
            break;
    }
    line.addSpace(buttonSpacing);
  }
  auto line2 = WL(getListBuilder, buttonWidth);
  int numSettingButtons = 0;
  auto getNextFocusPredicate = [&]() -> function<bool()> {
    ++numSettingButtons;
    return [this, numSettingButtons] { return minionPageIndex == MinionPageElems::Setting{numSettingButtons - 1}; };
  };
  line2.addElem(WL(buttonLabelFocusable,
      WL(centerHoriz, WL(getListBuilder)
          .addElemAuto(WL(label, "AI type: "))
          .addElemAuto(WL(viewObject, getViewId(minion.aiType)))
          .buildHorizontalList()),
      getAIButtonFun(minion), getNextFocusPredicate(), false, true));
  line2.addSpace(buttonSpacing);
  ViewId curTask = ViewId("unknown_monster");
  for (auto task : minion.minionTasks)
    if (task.current)
      curTask = getViewId(task.task);
  line2.addElem(WL(buttonLabelFocusable,
      WL(centerHoriz, WL(getListBuilder)
          .addElemAuto(WL(label, "Activity: "))
          .addElemAuto(WL(viewObject, curTask))
          .buildHorizontalList()),
      getActivityButtonFun(minion), getNextFocusPredicate(), false, true));
  line2.addSpace(buttonSpacing);
  if (!minion.equipmentGroups.empty())
    line2.addElem(WL(buttonLabelFocusable, "Restrict gear",
        getEquipmentGroupsFun(minion), getNextFocusPredicate(), false, true));
  return WL(getListBuilder, legendLineHeight)
      .addElem(line.buildHorizontalList())
      .addSpace(5)
      .addElem(line2.buildHorizontalList())
      .buildVerticalList();
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
  auto titleLine = WL(getListBuilder);
  titleLine.addElemAuto(WL(label, minion.title));  auto lines = WL(getListBuilder, legendLineHeight);
  for (auto& title : minion.killTitles)
    lines.addElem(WL(label, title));
  auto addLegend = [&] (const char* text) {
    lines.addElem(WL(label, text, Renderer::smallTextSize(), Color::LIGHT_GRAY), legendLineHeight * 2 / 3);
  };
  addLegend("Titles are awarded for killing tribe leaders, and increase");
  addLegend("each attribute up to a maximum of the attribute's base value.");
  if (!minion.killTitles.empty())
    titleLine.addElemAuto(WL(label, toString("+"), Color::YELLOW));
  return WL(stack,
      titleLine.buildHorizontalList(),
      minion.killTitles.empty() ? WL(empty) : WL(tooltip2, WL(miniWindow, WL(margins, lines.buildVerticalList(), 15)), [](Rectangle rect){ return rect.bottomLeft(); })
  );
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

SGuiElem GuiBuilder::drawMinionPage(const PlayerInfo& minion, const optional<TutorialInfo>& tutorial) {
  auto list = WL(getListBuilder, legendLineHeight);
  auto titleLine = WL(getListBuilder);
  titleLine.addElemAuto(drawTitleButton(minion));
  if (auto killsLabel = drawKillsLabel(minion))
    titleLine.addBackElemAuto(std::move(killsLabel));
  list.addElem(titleLine.buildHorizontalList());
  if (!minion.description.empty())
    list.addElem(WL(label, minion.description, Renderer::smallTextSize(), Color::LIGHT_GRAY));
  list.addElem(drawMinionActions(minion, tutorial), legendLineHeight * 2  );
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
  if (auto spells = drawSpellsList(minion.spells, minion.creatureId.getGenericId(), false))
    leftLines.addElemAuto(std::move(spells));
  int topMargin = list.getSize() + 20;
  int numActions = minion.actions.size();
  int numSettings = minion.equipmentGroups.empty() ? 2 : 3;
  int numEquipment = minion.inventory.size() + 1;
  return WL(stack, makeVec(
        WL(keyHandlerBool, [this] {
        if (minionPageIndex == MinionPageElems::None{}) {
          minionPageIndex = MinionPageElems::MinionAction{0};
          return true;
        }
        return false;
      }, {gui.getKey(C_BUILDINGS_CONFIRM)}),
      WL(keyHandlerBool, [this] {
        OnExit e([this] { updateMinionPageScroll(); });
        return minionPageIndex.left();
      }, {gui.getKey(C_BUILDINGS_LEFT)}),
      WL(keyHandlerBool, [this, numActions, numSettings] {
        OnExit e([this] { updateMinionPageScroll(); });
        return minionPageIndex.up(numActions, numSettings);
      }, {gui.getKey(C_BUILDINGS_UP)}),
      WL(keyHandlerBool, [this, numSettings, numEquipment] {
        OnExit e([this] { updateMinionPageScroll(); });
        return minionPageIndex.down(numSettings, numEquipment);
      }, {gui.getKey(C_BUILDINGS_DOWN)}),
      WL(keyHandlerBool, [this, numActions, numSettings] {
        OnExit e([this] { updateMinionPageScroll(); });
        return minionPageIndex.right(numActions, numSettings);
      }, {gui.getKey(C_BUILDINGS_RIGHT)}),
      WL(margin,
          list.buildVerticalList(),
          WL(scrollable, WL(horizontalListFit, makeVec(
              WL(rightMargin, 15, leftLines.buildVerticalList()),
              drawEquipmentAndConsumables(minion))), &minionPageScroll, &scrollbarsHeld2),
          topMargin, GuiFactory::TOP)));
}

void GuiBuilder::updateMinionPageScroll() {
  if (auto e = minionPageIndex.getValueMaybe<MinionPageElems::Equipment>())
    minionPageScroll.set(00 + e->index * legendLineHeight, Clock::getRealMillis());
  else
    minionPageScroll.set(0, Clock::getRealMillis());
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

static optional<Color> getHighlightColor(VillainType type) {
  switch (type) {
    case VillainType::MAIN:
      return Color::RED;
    case VillainType::LESSER:
      return Color::YELLOW;
    case VillainType::ALLY:
      return Color::GREEN;
    case VillainType::PLAYER:
      return Color::WHITE;
    default:
      return none;
  }
}

namespace {
struct LabelPlacer {
  LabelPlacer(Rectangle rect, int iconSize) : occupied(rect, false), iconSize(iconSize) {}

  void setOccupied(Vec2 pos) {
    occupied[pos] = true;
  }

  auto getLabelPosition(Vec2 pos, int textWidth) {
    int numTiles = (textWidth + iconSize) / iconSize;
    int bestOccupied = 100000;
    auto allRect = Rectangle(pos, pos + Vec2(numTiles + 1, 2));
    Vec2 bestPos;
    Vec2 bestOffset;
    for (auto startPos : {Vec2(1, -1), Vec2(1, 0), Vec2(-numTiles, 0), Vec2(-numTiles, -1)}) {
      int numOccupied = 0;
      for (auto v : allRect.translate(startPos))
        if (v != pos && (!v.inRectangle(occupied.getBounds()) || occupied[v]))
          ++numOccupied;
      if (numOccupied < bestOccupied) {
        bestPos = startPos;
        bestOffset = startPos.x > 0 ? bestPos * iconSize - Vec2(iconSize / 3, 0)
            : Vec2(-textWidth - iconSize + iconSize / 3, bestPos.y * iconSize);
        bestOccupied = numOccupied;
      }
    }
    for (auto v : allRect.translate(bestPos))
      if (v.inRectangle(occupied.getBounds()))
        occupied[v] = true;
    return bestOffset;
  }

  Table<bool> occupied;
  int iconSize;
};
}

static Vec2 maxWorldMapSize(800, 550);

void GuiBuilder::scrollWorldMap(int iconSize, Vec2 pos, Rectangle worldMapBounds) {
  auto maxSize = maxWorldMapSize / iconSize;
  scrollAreaScrollPos = {
      iconSize * max(worldMapBounds.left(), min(worldMapBounds.right() - maxSize.x, pos.x - maxSize.x / 2)),
      iconSize * max(worldMapBounds.top(), min(worldMapBounds.bottom() - maxSize.y, pos.y - maxSize.y / 2))};
}

SGuiElem GuiBuilder::drawCampaignGrid(const Campaign& c, optional<Vec2> initialPos, function<bool(Vec2)> selectable,
    function<void(Vec2)> selectCallback){
  int iconScale = c.getMapZoom();
  int iconSize = 8 * iconScale;
  int minimapScale = c.getMinimapZoom();
  auto& sites = c.getSites();
  auto rows = WL(getListBuilder, iconSize);
  auto minimapRows = WL(getListBuilder, minimapScale);
  LabelPlacer labelPlacer(sites.getBounds(), iconSize);
  auto yRange = sites.getBounds().getYRange();
  auto xRange = sites.getBounds().getXRange();
  if (initialPos)
    scrollWorldMap(iconSize, Vec2(initialPos->x, initialPos->y), sites.getBounds());
  for (int y : yRange) {
    auto columns = WL(getListBuilder, iconSize);
    auto minimapColumns = WL(getListBuilder, minimapScale);
    for (int x : xRange) {
      auto pos = Vec2(x, y);
      auto color = renderer.getTileSet().getColor(sites[x][y].viewId.back()).transparency(150);
      if (auto type = sites[x][y].getVillainType())
        if (auto c = getHighlightColor(*type))
          color = *c;
      if (!c.isInInfluence(pos))
        color = Color(0, 0, 0);
      minimapColumns.addElem(WL(stack,
          WL(button, [=, bounds = sites.getBounds()] { scrollWorldMap(iconSize, pos, bounds); }, true),
          WL(rectangle, color)));
    }
    minimapRows.addElem(minimapColumns.buildHorizontalList());
  }
  for (int y : yRange) {
    auto columns = WL(getListBuilder, iconSize);
    for (int x : xRange) {
      auto pos = Vec2(x, y);
      vector<SGuiElem> v;
      if (c.isInInfluence(pos))
        for (auto& id : sites[x][y].viewId) {
          v.push_back(WL(asciiBackground, id));
          if (startsWith(id.data(), "map_mountain_large"))
            v.push_back(WL(translate, WL(viewObject, id, iconScale), -Vec2(24, 24)));
          else
            v.push_back(WL(viewObject, id, iconScale));
        }
      columns.addElem(WL(stack, std::move(v)));
    }
    auto columns2 = WL(getListBuilder, iconSize);
    for (int x : xRange) {
      Vec2 pos(x, y);
      vector<SGuiElem> elem;
      if (auto id = sites[x][y].getDwellingViewId())
        if (c.isInInfluence(Vec2(x, y))) {
          elem.push_back(WL(asciiBackground, id->front()));
          elem.push_back(WL(viewObject, *id, iconScale));
          labelPlacer.setOccupied(pos);
        }
      if (auto desc = sites[x][y].getDwellerDescription())
        elem.push_back(WL(margins, WL(tooltip, {
            *desc,
            "+" + toString(c.getBaseLevelIncrease(Vec2(x, y))) + " difficulty"
        }, milliseconds{0}), -4));
      columns2.addElem(WL(stack, std::move(elem)));
    }
    rows.addElem(WL(stack, columns.buildHorizontalList(), columns2.buildHorizontalList()));
  }
  auto fowRows = WL(getListBuilder, iconSize);
  auto translateHighlight = [&](SGuiElem elem) {
    return WL(translate, [iconSize] { return Vec2(-iconSize, -iconSize); }, std::move(elem));
  };
  for (int y : yRange) {
    auto columns = WL(getListBuilder, iconSize);
    for (int x : xRange)
      columns.addElem(c.isInInfluence(Vec2(x, y)) ? WL(empty) : translateHighlight(WL(viewObject, ViewId("map_fow"), iconScale)));
    fowRows.addElem(columns.buildHorizontalList());
  }
  auto upperRows = WL(getListBuilder, iconSize);
  for (int y : yRange) {
    auto columns = WL(getListBuilder, iconSize);
    for (int x : xRange) {
      Vec2 pos(x, y);
      vector<SGuiElem> elem;
      if (c.isInInfluence(pos)) {
        if (auto id = sites[x][y].getDwellerViewId())
          if (auto color = getHighlightColor(*sites[pos].getVillainType()))
            elem.push_back(translateHighlight(WL(viewObject, ViewId("map_highlight"), iconScale, *color)));
        if (campaignGridPointer)
          elem.push_back(WL(conditional, translateHighlight(WL(viewObject, ViewId("map_highlight"), iconScale)),
                [this, pos] { return campaignGridPointer == pos;}));
        if (campaignGridPointer && !!selectable && selectable(pos))
          elem.push_back(WL(stack,
              WL(button, [this, pos, selectCallback] {
                if (selectCallback)
                  selectCallback(pos);
                campaignGridPointer = pos;
              }),
              WL(mouseHighlight2, translateHighlight(WL(viewObject, ViewId("map_highlight"), iconScale)), nullptr, false)
          ));
        if (auto id = sites[x][y].getDwellerViewId())
          if (c.isDefeated(pos))
            elem.push_back(WL(viewObject, ViewId("campaign_defeated"), iconScale));
        if (auto desc = sites[x][y].getDwellerName())
          if (getHighlightColor(*sites[pos].getVillainType())) {
            auto width = renderer.getTextLength(*desc, 12, FontId::MAP_FONT);
            auto color = c.isInInfluence(pos) ? Color::WHITE : Color(200, 200, 200);
            elem.push_back(WL(translate,
                WL(labelUnicode, *desc, color, 12, FontId::MAP_FONT),
            labelPlacer.getLabelPosition(Vec2(x, y), width), Vec2(width + 6, 18), GuiFactory::TranslateCorner::CENTER));
          }
      }
      columns.addElem(WL(stack, std::move(elem)));
    }
    upperRows.addElem(columns.buildHorizontalList());
  }
  auto mapContent = WL(stack, rows.buildVerticalList(), fowRows.buildVerticalList(), upperRows.buildVerticalList());
  int margin = 8;
  if (*mapContent->getPreferredWidth() > maxWorldMapSize.x || *mapContent->getPreferredHeight() > maxWorldMapSize.y)
    mapContent = WL(stack, WL(scrollArea, std::move(mapContent), scrollAreaScrollPos),
        WL(alignment, GuiFactory::Alignment::TOP_RIGHT, WL(stack,
            WL(margins, WL(rectangle, Color::BLACK), -margin),
            WL(renderInBounds, minimapRows.buildVerticalList()),
            WL(margins, WL(miniBorder2), -margin),
            WL(translate,
                [this, iconSize, minimapScale] {
                  return Vec2(scrollAreaScrollPos.first, scrollAreaScrollPos.second) * minimapScale / iconSize;
                },
                WL(alignment, GuiFactory::Alignment::TOP_LEFT, WL(rectangle, Color::TRANSPARENT, Color::WHITE),
                    maxWorldMapSize * minimapScale / iconSize)))));
  if (campaignGridPointer)
    mapContent = WL(stack, makeVec(
        std::move(mapContent),
        WL(keyHandler, [&c, iconSize, this] {
            moveCampaignGridPointer(c, iconSize, Dir::N); }, Keybinding("MENU_UP"), true),
        WL(keyHandler, [&c, iconSize, this] {
            moveCampaignGridPointer(c, iconSize, Dir::S); }, Keybinding("MENU_DOWN"), true),
        WL(keyHandler, [&c, iconSize, this] {
            moveCampaignGridPointer(c, iconSize, Dir::W); }, Keybinding("MENU_LEFT"), true),
        WL(keyHandler, [&c, iconSize, this] {
            moveCampaignGridPointer(c, iconSize, Dir::E); }, Keybinding("MENU_RIGHT"), true)
    ));
  return WL(preferredSize, maxWorldMapSize + Vec2(margin, margin) * 2, WL(stack, WL(rectangle, Color::BLACK),
    WL(miniBorder2),
    WL(margins, std::move(mapContent), margin)));
}

void GuiBuilder::moveCampaignGridPointer(const Campaign& c, int iconSize, Dir dir) {
  Vec2& cur = *campaignGridPointer;
  auto bounds = c.getSites().getBounds();
  switch (dir) {
    case Dir::N:
      if (cur.y > bounds.top())
        --cur.y;
      break;
    case Dir::S:
      if (cur.y < bounds.bottom() - 1)
        ++cur.y;
      break;
    case Dir::E:
      if (cur.x < bounds.right() - 1)
        ++cur.x;
      break;
    case Dir::W:
      if (cur.x > bounds.left())
        --cur.x;
      break;
    default:
      break;
  }
  auto visibleRect = Rectangle(maxWorldMapSize).translate(Vec2(scrollAreaScrollPos.first, scrollAreaScrollPos.second));
  if (!(cur * iconSize).inRectangle(visibleRect.minusMargin(3)))
    scrollWorldMap(iconSize, cur, bounds);
}

SGuiElem GuiBuilder::drawWorldmap(Semaphore& sem, const Campaign& campaign) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, "Map of " + campaign.getWorldName())));
  lines.addElem(WL(centerHoriz, WL(label, "Use the travel command while controlling a minion or team "
          "to travel to another site.", Renderer::smallTextSize(), Color::LIGHT_GRAY)));
  lines.addElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, none, nullptr, nullptr)));
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(centerHoriz, WL(buttonLabel, "Close", [&] { sem.v(); })));
  return WL(preferredSize, 1000, 750,
      WL(window, WL(margins, lines.buildVerticalList(), 15), [&sem] { sem.v(); }));
}

SGuiElem GuiBuilder::drawChooseSiteMenu(SyncQueue<optional<Vec2>>& queue, const string& message,
    const Campaign& campaign, Vec2 initialPos) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, message)));
  campaignGridPointer = initialPos;
  lines.addElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, initialPos,
      [&campaign](Vec2 pos){ return campaign.isInInfluence(pos); }, nullptr)));
  lines.addSpace(legendLineHeight / 2);
  auto confirmCallback = [&] {
    if (campaign.canTravelTo(*campaignGridPointer))
      queue.push(*campaignGridPointer);
  };
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(conditional, WL(stack,
              WL(buttonLabel, "Confirm", confirmCallback),
              WL(keyHandler, confirmCallback, Keybinding("MENU_SELECT"), true)
          ),
          WL(buttonLabelInactive, "Confirm"),
          [&] { return !!campaignGridPointer && campaign.isInInfluence(*campaignGridPointer); }))
      .addSpace(15)
      .addElemAuto(WL(buttonLabel, "Cancel", WL(stack,
          WL(button, [&queue] { queue.push(none); }, true),
          WL(keyHandler, [&] { queue.push(none); }, Keybinding("EXIT_MENU"), true)
      )))
      .buildHorizontalList()));
  return WL(preferredSize, 1000, 750,
      WL(window, WL(margins, lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
}

static const char* getText(AvatarMenuOption option) {
  switch (option) {
    case AvatarMenuOption::TUTORIAL:
      return "Tutorial";
    case AvatarMenuOption::CHANGE_MOD:
      return "Mods";
    case AvatarMenuOption::GO_BACK:
      return "Go back";
  }
}

SGuiElem GuiBuilder::drawGenderButtons(const vector<View::AvatarData>& avatars,
    shared_ptr<int> gender, shared_ptr<int> chosenAvatar) {
  vector<SGuiElem> genderOptions;
  for (int index : All(avatars))
    if (!avatars[index].genderNames.empty()) {
      auto& avatar = avatars[index];
      auto genderList = WL(getListBuilder);
      if (avatar.viewId.size() > 1 || !avatar.settlementNames)
        for (int i : All(avatar.viewId)) {
          auto selectFun = [i, gender] { *gender = i; };
          auto focusedFun = [this, i] { return avatarIndex == AvatarIndexElems::GenderIndex{i};};
          genderList.addElemAuto(WL(conditional,
              WL(conditional,
                  WL(buttonLabelSelectedFocusable, capitalFirst(avatar.genderNames[i]), selectFun, focusedFun,
                      false, true),
                  WL(buttonLabelFocusable, capitalFirst(capitalFirst(avatar.genderNames[i])), selectFun, focusedFun,
                      false, true),
                  [gender, i] { return *gender == i; }),
              [=] { return index == *chosenAvatar; }));
        }
      genderOptions.push_back(genderList.buildHorizontalListFit(0.2));
    }
  return WL(stack, std::move(genderOptions));
}

static int getChosenGender(shared_ptr<int> gender, shared_ptr<int> chosenAvatar,
    const vector<View::AvatarData>& avatars) {
  return min(*gender, avatars[*chosenAvatar].viewId.size() - 1);
}

const auto randomFirstNameTag = "<random>"_s;

SGuiElem GuiBuilder::drawFirstNameButtons(const vector<View::AvatarData>& avatars,
    shared_ptr<int> gender, shared_ptr<int> chosenAvatar, shared_ptr<int> chosenName) {
  vector<SGuiElem> firstNameOptions = {};
  for (int index : All(avatars)) {
    auto& avatar = avatars[index];
    for (int genderIndex : All(avatar.viewId))
      if (avatar.firstNames.size() > genderIndex) {
        auto focusedRollFun = [this] {
          return avatarIndex == AvatarIndexElems::RollNameIndex{};
        };
        auto focusedNameFun = [this] {
          return avatarIndex == AvatarIndexElems::EnterNameIndex{};
        };
        auto rollFunConfirm = [=] { options->setValue(avatar.nameOption, randomFirstNameTag); ++*chosenName; };
        auto elem = WL(getListBuilder)
            .addElemAuto(WL(label, avatar.settlementNames ? "Settlement: " : "First name: "))
            .addMiddleElem(WL(textField, maxFirstNameLength,
                [=] {
                  auto entered = options->getValueString(avatar.nameOption);
                  return entered == randomFirstNameTag ?
                      avatar.firstNames[genderIndex][*chosenName % avatar.firstNames[genderIndex].size()] :
                      entered;
                },
                [=] (string s) {
                  options->setValue(avatar.nameOption, s);
                },
                focusedNameFun))
            .addSpace(10)
            .addBackElemAuto(WL(buttonLabelFocusable, "🔄", rollFunConfirm, focusedRollFun, true, false, true))
            .buildHorizontalList();
        firstNameOptions.push_back(WL(conditionalStopKeys, std::move(elem),
            [=]{ return getChosenGender(gender, chosenAvatar, avatars) == genderIndex && index == *chosenAvatar; }));
      }
  }
  return WL(stack, std::move(firstNameOptions));
}

static const char* getName(View::AvatarRole role) {
  switch (role) {
    case View::AvatarRole::KEEPER:
      return "Keeper";
    case View::AvatarRole::ADVENTURER:
      return "Adventurer";
    case View::AvatarRole::WARLORD:
      return "Warlord";
  }
}

SGuiElem GuiBuilder::drawRoleButtons(shared_ptr<View::AvatarRole> chosenRole, shared_ptr<int> chosenAvatar,
    shared_ptr<int> avatarPage, const vector<View::AvatarData>& avatars) {
  auto roleList = WL(getListBuilder);
  for (auto role : ENUM_ALL(View::AvatarRole)) {
    auto chooseFun = [chosenRole, chosenAvatar, &avatars, role, avatarPage] {
      *chosenRole = role;
      *avatarPage = 0;
      for (int i : All(avatars))
        if (avatars[i].role == *chosenRole) {
          *chosenAvatar = i;
          break;
        }
    };
    bool hasAvatars = [&] {
      for (auto& a : avatars)
        if (a.role == role)
          return true;
      return false;
    }();
    if (hasAvatars) {
      auto focusedFun = [this, role] { return avatarIndex == AvatarIndexElems::RoleIndex{int(role)};};
      roleList.addElemAuto(
          WL(conditional,
              WL(buttonLabelSelectedFocusable, capitalFirst(getName(role)), chooseFun, focusedFun,
                  false, true),
              WL(buttonLabelFocusable, capitalFirst(getName(role)), chooseFun, focusedFun, false, true),
              [chosenRole, role] { return *chosenRole == role; })
      );
    }
  }
  return roleList.buildHorizontalListFit(0.2);
}

SGuiElem GuiBuilder::drawChosenCreatureButtons(View::AvatarRole role, shared_ptr<int> chosenAvatar, shared_ptr<int> gender,
    int page, const vector<View::AvatarData>& avatars) {
  auto allLines = WL(getListBuilder, legendLineHeight);
  auto line = WL(getListBuilder);
  const int start = page * avatarsPerPage;
  const int end = (page + 1) * avatarsPerPage;
  int processed = 0;
  for (int i : All(avatars)) {
    auto& elem = avatars[i];
    auto viewIdFun = [gender, id = elem.viewId] { return id[min(*gender, id.size() - 1)]; };
    if (elem.role == role) {
      if (processed >= start && processed < end) {
        Vec2 coord(line.getLength(), allLines.getLength());
        if (elem.unlocked) {
          auto selectFun = [i, chosenAvatar]{ *chosenAvatar = i; };
          auto icon = WL(stack,
              WL(conditionalStopKeys, WL(stack,
                  WL(uiHighlight),
                  WL(keyHandler, selectFun, Keybinding("MENU_SELECT"), true)
              ), [this, coord] { return avatarIndex == AvatarIndexElems::CreatureIndex{coord};}),
              WL(viewObject, viewIdFun, 2)
          );
          line.addElemAuto(WL(stack,
              WL(button, selectFun),
              WL(mouseHighlight2,
                  WL(rightMargin, 10, WL(topMargin, -5, icon)),
                  WL(rightMargin, 10, WL(conditional2,
                      WL(topMargin, -5, icon),
                      icon, [=](GuiElem*){ return *chosenAvatar == i;})))
          ));
        } else
          line.addElemAuto(WL(stack,
              WL(conditionalStopKeys, WL(rightMargin, 10, WL(uiHighlight)),
                  [this, coord] { return avatarIndex == AvatarIndexElems::CreatureIndex{coord};}),
              WL(rightMargin, 10, WL(viewObject, ViewId("unknown_monster", Color::GRAY), 2))
          ));
      }
      ++processed;
      if (line.getLength() >= avatarsPerPage / 2) {
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
    shared_ptr<int> chosenAvatar, shared_ptr<int> gender, shared_ptr<View::AvatarRole> chosenRole) {
  vector<SGuiElem> avatarsForRole;
  int maxAvatarPagesHeight = 0;
  for (auto role : ENUM_ALL(View::AvatarRole)) {
    int numAvatars = 0;
    for (auto& a : avatars)
      if (a.role == role)
        ++numAvatars;
    vector<SGuiElem> avatarPages;
    const int numPages = 1 + (numAvatars - 1) / avatarsPerPage;
    for (int page = 0; page < numPages; ++page)
      avatarPages.push_back(WL(conditionalStopKeys,
          drawChosenCreatureButtons(role, chosenAvatar, gender, page, avatars),
          [avatarPage, page] { return *avatarPage == page; }));
    maxAvatarPagesHeight = max(maxAvatarPagesHeight, *avatarPages[0]->getPreferredHeight());
    if (numPages > 1) {
      auto leftFocused = [this] {
        return avatarIndex == AvatarIndexElems::PageButtonsIndex{0};
      };
      auto rightFocused = [this] {
        return avatarIndex == AvatarIndexElems::PageButtonsIndex{1};
      };
      auto leftAction = [this, avatarPage, &avatars, chosenAvatar, chosenRole] {
        *avatarPage = max(0, *avatarPage - 1);
        if (*avatarPage == 0)
          avatarIndex.right(avatars, *chosenAvatar, *chosenRole, 0);
      };
      auto rightAction = [this, numPages, avatarPage, &avatars, chosenAvatar, chosenRole] {
        *avatarPage = min(numPages - 1, *avatarPage + 1);
        if (*avatarPage == numPages - 1)
          avatarIndex.left(avatars, *chosenAvatar, *chosenRole, numPages - 1);
      };
      avatarPages.push_back(WL(alignment, GuiFactory::Alignment::BOTTOM_RIGHT,
          WL(translate,
              WL(getListBuilder, 24)
                .addElem(WL(conditional2,
                    WL(buttonLabelFocusable, "<", leftAction, leftFocused),
                    WL(buttonLabelInactive, "<"),
                    [=] (GuiElem*) { return *avatarPage > 0; }))
                .addElem(WL(conditional2,
                    WL(buttonLabelFocusable, ">", rightAction, rightFocused),
                    WL(buttonLabelInactive, ">"),
                    [=] (GuiElem*) { return *avatarPage < numPages - 1; }))
                .buildHorizontalList(),
             Vec2(12, 32), Vec2(48, 30)
          )));
    }
    avatarsForRole.push_back(WL(conditionalStopKeys, WL(stack, std::move(avatarPages)),
        [chosenRole, role] { return *chosenRole == role; }));
  }
  return WL(setHeight, maxAvatarPagesHeight, WL(stack, std::move(avatarsForRole)));
}

SGuiElem GuiBuilder::drawAvatarMenu(SyncQueue<variant<View::AvatarChoice, AvatarMenuOption>>& queue,
    const vector<View::AvatarData>& avatars) {
  if (hasController())
    avatarIndex.assign(AvatarIndexElems::RoleIndex{0});
  else
    avatarIndex.assign(AvatarIndexElems::None{});
  for (auto& avatar : avatars)
    if (options->getValueString(avatar.nameOption).empty())
      options->setValue(avatar.nameOption, randomFirstNameTag);
  auto gender = make_shared<int>(0);
  auto chosenAvatar = make_shared<int>(0);
  auto chosenName = make_shared<int>(0);
  auto avatarPage = make_shared<int>(0);
  auto chosenRole = make_shared<View::AvatarRole>(View::AvatarRole::KEEPER);
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
        if (auto alignment = avatars[*chosenAvatar].alignment)
          return capitalFirst(avatars[*chosenAvatar].name) +
            ", " + getName(*alignment);
        else
          return capitalFirst(avatars[*chosenAvatar].name);
      }));
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElemAuto(WL(getListBuilder)
      .addElemAuto(WL(rightMargin, 30, leftLines.buildVerticalList()))
      .addElemAuto(rightLines.buildVerticalList())
      .buildHorizontalListFit()
  );
  vector<SGuiElem> descriptions;
  for (int avatarIndex : All(avatars)) {
    auto& avatar = avatars[avatarIndex];
    const auto maxWidth = 750;
    auto description =
        WL(labelMultiLineWidth, avatar.description, legendLineHeight, maxWidth, Renderer::textSize(), Color::LIGHT_GRAY);
    descriptions.push_back(WL(conditional,
        std::move(description),
        [avatarIndex, chosenAvatar] { return avatarIndex == *chosenAvatar; }));
  }
  lines.addBackElem(WL(stack, descriptions), 2.5 * legendLineHeight);
  lines.addBackSpace(10);
  auto startGameFun = [&queue, chosenAvatar, chosenName, gender, &avatars, this] {
    auto chosenGender = getChosenGender(gender, chosenAvatar, avatars);
    auto& avatar = avatars[*chosenAvatar];
    if (options->getValueString(avatar.nameOption).empty())
      options->setValue(avatar.nameOption, randomFirstNameTag);
    auto enteredName = options->getValueString(avatar.nameOption);
    if (enteredName == randomFirstNameTag && !avatars[*chosenAvatar].firstNames.empty()) {
      auto& firstNames = avatars[*chosenAvatar].firstNames[chosenGender];
      enteredName = firstNames[*chosenName % firstNames.size()];
    }
    queue.push(View::AvatarChoice{*chosenAvatar, chosenGender, enteredName});
  };
  auto startGameFocusedFun = [this]{ return avatarIndex == AvatarIndexElems::StartNewGameIndex{};};
  lines.addBackElem(
      WL(centerHoriz, WL(buttonLabelFocusable, "Start new game", startGameFun, startGameFocusedFun)));
  auto menuLines = WL(getListBuilder, legendLineHeight)
      .addElemAuto(
          WL(preferredSize, 800, 370, WL(window, WL(margins,
              lines.buildVerticalList(), 15), [&queue]{ queue.push(AvatarMenuOption::GO_BACK); })));
  auto othersLine = WL(getListBuilder);
  for (auto option : ENUM_ALL(AvatarMenuOption)) {
    auto bottomButtonFocusedFun = [this, option] {
      return avatarIndex == AvatarIndexElems::BottomButtonsIndex{int(option)};
    };
    auto confirmFun = [&queue, option]{ queue.push(option); };
    othersLine.addElemAuto(WL(stack,
        WL(button, confirmFun),
        WL(buttonLabelWithMargin, getText(option), bottomButtonFocusedFun),
        WL(conditionalStopKeys, WL(keyHandler, confirmFun, Keybinding("MENU_SELECT"), true),
            bottomButtonFocusedFun)
    ));
  }
  menuLines.addElem(WL(margins, othersLine.buildHorizontalListFit(), 40, 0, 40, 0), 30);
  return WL(stack, makeVec(
      WL(stopKeyEvents),
      WL(keyHandler, [this, &avatars, chosenAvatar, chosenRole, avatarPage] {
        avatarIndex.left(avatars, *chosenAvatar, *chosenRole, *avatarPage);
      }, Keybinding("MENU_LEFT"), true),
      WL(keyHandler, [this, &avatars, chosenAvatar, chosenRole, avatarPage]{
        avatarIndex.right(avatars, *chosenAvatar, *chosenRole, *avatarPage);
      }, Keybinding("MENU_RIGHT"), true),
      WL(keyHandler, [this, &avatars, chosenAvatar, chosenRole, avatarPage]{
        avatarIndex.up(avatars, *chosenAvatar, *chosenRole, *avatarPage);
      }, Keybinding("MENU_UP"), true),
      WL(keyHandler, [this, &avatars, chosenAvatar, chosenRole, avatarPage]{
        avatarIndex.down(avatars, *chosenAvatar, *chosenRole, *avatarPage);
      }, Keybinding("MENU_DOWN"), true),
      menuLines.buildVerticalList()
  ));
}

SGuiElem GuiBuilder::drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease, bool leftRight) {
  string plus = leftRight ? ">"  : "+";
  string minus = leftRight ? "<"  : "-";
  return WL(margins, WL(getListBuilder)
      .addElem(canDecrease
          ? WL(buttonLabel, minus, [callback] { callback(-1); }, false)
          : WL(buttonLabelInactive, minus, false), 10)
      .addSpace(12)
      .addElem(canIncrease
          ? WL(buttonLabel, plus, [callback] { callback(1); }, false)
          : WL(buttonLabelInactive, plus, false), 10)
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

SGuiElem GuiBuilder::drawOptionElem(OptionId id, function<void()> onChanged, function<bool()> focused) {
  string name = options->getName(id);
  SGuiElem ret;
  auto limits = options->getLimits(id);
  int value = options->getIntValue(id);
  auto changeFun = [=] (int v) { options->setValue(id, value + v); onChanged();};
  ret = WL(getListBuilder)
      .addElem(WL(labelFun, [=]{ return name + ": " + options->getValueString(id); }), renderer.getTextLength(name) + 20)
      .addBackElemAuto(drawPlusMinus(changeFun,
          value < limits->getEnd() - 1, value > limits->getStart(), options->hasChoices(id)))
      .buildHorizontalList();
  return WL(stack,
      WL(tooltip, {options->getHint(id).value_or("")}),
      WL(conditionalStopKeys, WL(stack,
          WL(margins, WL(uiHighlight), -4, -4, -15, 4),
          WL(keyHandler, [=] { if (value < limits->getEnd() - 1) changeFun(1); }, Keybinding("MENU_RIGHT"), true),
          WL(keyHandler, [=] { if (value > limits->getStart()) changeFun(-1); }, Keybinding("MENU_LEFT"), true)
      ), focused),
      std::move(ret)
  );
}

pair<GuiFactory::ListBuilder, vector<SGuiElem>> GuiBuilder::drawRetiredGames(RetiredGames& retired,
    function<void()> reloadCampaign, optional<int> maxActive, string searchString,
    function<bool(int)> isFocused) {
  auto lines = WL(getListBuilder, legendLineHeight);
  const vector<RetiredGames::RetiredGame>& allGames = retired.getAllGames();
  bool displayActive = !maxActive;
  vector<SGuiElem> added = 0;
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
          maxedOut ? Color::LIGHT_GRAY : Color::WHITE), 180);
      for (auto& minion : allGames[i].gameInfo.minions)
        header.addElem(drawMinionAndLevel(minion.viewId, minion.level, 1), 25);
      header.addSpace(20);
      if (retired.isActive(i))
        header.addBackElemAuto(WL(buttonLabelFocusable, "Remove",
            [i, reloadCampaign, &retired] { retired.setActive(i, false); reloadCampaign();},
            [isFocused, numAdded = added.size()]{ return isFocused(numAdded);}));
      else if (!maxedOut)
        header.addBackElemAuto(WL(buttonLabelFocusable, "Add",
            [i, reloadCampaign, &retired] { retired.setActive(i, true); reloadCampaign();},
            [isFocused, numAdded = added.size()]{ return isFocused(numAdded);}));
      header.addBackSpace(10);
      auto addedElem = header.buildHorizontalList();
      if (!maxedOut)
        added.push_back(addedElem);
      lines.addElem(addedElem);
      auto detailsList = WL(getListBuilder);
      if (allGames[i].numTotal > 0)
        detailsList.addElemAuto(WL(stack,
          WL(tooltip, {"Number of times this dungeon has been conquered over how many times it has been loaded."}),
          WL(label, "Conquer rate: " + toString(allGames[i].numWon) + "/" + toString(allGames[i].numTotal),
              Renderer::smallTextSize(), gui.inactiveText)));
      if (allGames[i].isFriend) {
        detailsList.addBackElemAuto(WL(label, "By your friend " + allGames[i].author, Renderer::smallTextSize(), Color::PINK));
        detailsList.addBackSpace(10);
      }
      if (!detailsList.isEmpty())
        lines.addElem(detailsList.buildHorizontalList(), legendLineHeight * 2 / 3);
      if (!allGames[i].gameInfo.spriteMods.empty()) {
        auto modsList = combine(allGames[i].gameInfo.spriteMods, true);
        lines.addElem(WL(stack,
            WL(tooltip, {"These mods may be required to successfully load this dungeon:", modsList}),
            WL(renderInBounds, WL(label, "Requires mods:" + modsList, Renderer::smallTextSize(), gui.inactiveText))),
            legendLineHeight * 2 / 3);
      }
      lines.addSpace(legendLineHeight / 3);
    }
  }
  return make_pair(std::move(lines), std::move(added));
}

SGuiElem GuiBuilder::drawRetiredDungeonsButton(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    View::CampaignMenuState& state) {
  if (campaignOptions.retired) {
    auto& retiredGames = *campaignOptions.retired;
    auto selectFun = [this, &queue, &retiredGames, campaignOptions](Rectangle r) {
      int focused = 1;
      string searchString;
      bool exit = false;
      bool clicked = false;
      auto searchField = WL(textField, 10, [&searchString] { return searchString; },
        [&](string s){
          exit = true;
          clicked = true;
          searchString = s;
        },
        [&focused] { return focused == 0; });
      while (1) {
        exit = false;
        clicked = false;
        auto retiredMenuLines = WL(getListBuilder, getStandardLineHeight());
        retiredMenuLines.addSpace(); // placeholder for the search field which needs to be stack on top
        // to catch keypress events
        auto addedDungeons = drawRetiredGames(retiredGames, [&] {
          queue.push(CampaignActionId::UPDATE_MAP);
          clicked = true;
          exit = true;
        }, none, "", [&focused] (int i) { return i == focused - 1; });
        vector<SGuiElem> dungeonElems { searchField };
        dungeonElems.append(addedDungeons.second);
        int addedHeight = addedDungeons.first.getSize();
        if (!addedDungeons.first.isEmpty()) {
          retiredMenuLines.addElem(WL(label, "Added:", Color::YELLOW));
          retiredMenuLines.addElem(addedDungeons.first.buildVerticalList(), addedHeight);
        }
        auto retiredList = drawRetiredGames(retiredGames, [&] {
          queue.push(CampaignActionId::UPDATE_MAP);
          clicked = true;
          exit = true;
        }, options->getIntValue(OptionId::MAIN_VILLAINS), searchString,
            [focusedOffset = dungeonElems.size(), &focused] (int i) {
              return i == focused - focusedOffset; });
        dungeonElems.append(retiredList.second);
        if (retiredList.first.isEmpty())
          retiredList.first.addElem(WL(label, "No retired dungeons found :("));
        else
          retiredMenuLines.addElem(WL(label, "Local:", Color::YELLOW));
        retiredMenuLines.addElemAuto(retiredList.first.buildVerticalList());
        focused = min(focused, dungeonElems.size() - 1);
        auto content = WL(stack, makeVec(
            retiredMenuLines.buildVerticalList(),
            WL(keyHandler, [&] {
              if (focused == -1)
                focused = 0;
              focused = (focused + 1) % dungeonElems.size();
              miniMenuScroll.setRelative(dungeonElems[focused]->getBounds().top(), Clock::getRealMillis());
            }, Keybinding("MENU_DOWN"), true),
            WL(keyHandler, [&] {
              if (focused == -1)
                focused = 0;
              focused = (focused + dungeonElems.size() - 1) % dungeonElems.size();
              miniMenuScroll.setRelative(dungeonElems[focused]->getBounds().top(), Clock::getRealMillis());
            }, Keybinding("MENU_UP"), true),
            WL(keyHandler, [&focused] { if (focused == 0) focused = -1; },
                Keybinding("MENU_RIGHT"), true),
            WL(keyHandler, [&focused] { if (focused == -1) focused = 0; },
                Keybinding("MENU_LEFT"), true),
            WL(setHeight, getStandardLineHeight(), WL(getListBuilder)
                .addElemAuto(WL(label, "Search: "))
                .addElem(searchField, 200)
                .addSpace(10)
                .addElemAuto(WL(buttonLabelFocusable, "X",
                    [&]{ searchString.clear(); exit = true; clicked = true;},
                    [&focused] { return focused == -1; }))
            .buildHorizontalList())
        ));
        drawMiniMenu(std::move(content), exit, r.bottomLeft(), 444, true, false);
        if (!clicked)
          break;
      }
    };
    auto focusedFun = [&state] { return state.index == CampaignMenuElems::RetiredDungeons{};};
    return WL(buttonLabelFocusable, "Add retired dungeons", selectFun, focusedFun);
  }
  return WL(buttonLabelInactive, "Add retired dungeons");
}

SGuiElem GuiBuilder::drawCampaignSettingsButton(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    View::CampaignMenuState& state) {
  auto callback = [this, &queue, campaignOptions](Rectangle rect) {
    int focused = 0;
    while (1) {
      auto optionsLines = WL(getListBuilder, getStandardLineHeight());
      bool exit = false;
      bool clicked = false;
      for (int index : All(campaignOptions.options)) {
        auto id = campaignOptions.options[index];
        optionsLines.addElem(drawOptionElem(id, [id, &queue, &clicked, &exit] {
          queue.push({CampaignActionId::UPDATE_OPTION, id});
          clicked = true;
          exit = true;
        }, [index, &focused] { return focused == index; }));
      }
      int cnt = campaignOptions.options.size();
      auto content = WL(stack,
        optionsLines.buildVerticalList(),
        WL(keyHandler, [&focused, cnt] { focused = (focused + 1) % cnt; },
            Keybinding("MENU_DOWN"), true),
        WL(keyHandler, [&focused, cnt] { focused = (focused + cnt - 1) % cnt; },
            Keybinding("MENU_UP"), true)
      );
      drawMiniMenu(std::move(content), exit, rect.bottomLeft(), 444, true);
      if (!clicked)
        break;
    }
  };
  auto focusedFun = [&state] { return state.index == CampaignMenuElems::Settings{};};
  return WL(buttonLabelFocusable, "Difficulty", callback, focusedFun);
}

SGuiElem GuiBuilder::drawGameModeButton(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    View::CampaignMenuState& menuState) {
  auto changeFun = [&queue, this, campaignOptions] (Rectangle bounds) {
    auto lines = WL(getListBuilder, legendLineHeight);
    bool exit = false;
    int focused = 0;
    for (int index : All(campaignOptions.worldMapNames)) {
      auto& name = campaignOptions.worldMapNames[index];
      lines.addElem(WL(buttonLabelFocusable, name,
          [&, index] { queue.push({CampaignActionId::CHANGE_WORLD_MAP, index}); exit = true; },
          [&focused, index] { return index == focused;}));
      lines.addSpace(legendLineHeight / 3);
    }
    int cnt = campaignOptions.worldMapNames.size();
    auto content = WL(stack,
      lines.buildVerticalList(),
      WL(keyHandler, [&focused, cnt] { focused = (focused + 1) % cnt; },
          Keybinding("MENU_DOWN"), true),
      WL(keyHandler, [&focused, cnt] { focused = (focused + cnt - 1) % cnt; },
          Keybinding("MENU_UP"), true)
    );
    drawMiniMenu(std::move(content), exit, bounds.bottomLeft(), 350, true);
  };
  auto changeFocusedFun = [&menuState] {return menuState.index == CampaignMenuElems::ChangeMode{};};
  return WL(buttonLabelFocusable, "Change", changeFun, changeFocusedFun);
}

SGuiElem GuiBuilder::drawCampaignMenu(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    View::CampaignMenuState& menuState) {
  if (menuState.index == CampaignMenuElems::None{} && hasController())
    menuState.index = CampaignMenuElems::Help{};
  const auto& campaign = campaignOptions.campaign;
  auto& retiredGames = campaignOptions.retired;
  auto lines = WL(getListBuilder, getStandardLineHeight());
  auto centerLines = WL(getListBuilder, getStandardLineHeight());
  auto rightLines = WL(getListBuilder, getStandardLineHeight());
  int optionMargin = 50;
  centerLines.addElem(WL(centerHoriz,
       WL(label, "World map style: "_s + campaignOptions.worldMapNames[campaignOptions.currentWorldMap])));
  rightLines.addElem(WL(leftMargin, -55, drawGameModeButton(queue, campaignOptions, menuState)));
  centerLines.addSpace(10);
  auto helpFocusedFun = [&menuState]{return menuState.index == CampaignMenuElems::Help{};};
  auto helpFun = [&] { menuState.helpText = !menuState.helpText; };
  centerLines.addElem(WL(centerHoriz,
      WL(buttonLabelFocusable, "Help", helpFun, helpFocusedFun)));
  lines.addElem(WL(leftMargin, optionMargin, WL(label, "World name: " + campaign.getWorldName())));
  lines.addSpace(10);
  lines.addElem(WL(leftMargin, optionMargin, drawRetiredDungeonsButton(queue, campaignOptions, menuState)));
  if (!campaignOptions.options.empty()) {
    lines.addSpace(10);
    lines.addElem(WL(leftMargin, optionMargin, drawCampaignSettingsButton(queue, campaignOptions, menuState)));
  }
  campaignGridPointer = campaign.getPlayerPos();
  lines.addBackElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, campaign.getOriginalPlayerPos(),
      [&campaign](Vec2 pos) { return campaign.isGoodStartPos(pos); },
      [&queue](Vec2 pos) { queue.push({CampaignActionId::SET_POSITION, pos}); })));
  lines.addSpace(10);
  lines.addBackElem(WL(centerHoriz, WL(getListBuilder)
        .addElemAuto(WL(buttonLabelFocusable, "Confirm", [&] { queue.push(CampaignActionId::CONFIRM); },
            [&menuState]{return menuState.index == CampaignMenuElems::Confirm{};}))
        .addSpace(20)
        .addElemAuto(WL(buttonLabelFocusable, "Re-roll map", [&] { queue.push(CampaignActionId::REROLL_MAP); },
            [&menuState]{return menuState.index == CampaignMenuElems::RollMap{};}))
        .addSpace(20)
        .addElemAuto(WL(buttonLabelFocusable, "Go back", [&] { queue.push(CampaignActionId::CANCEL); },
            [&menuState]{return menuState.index == CampaignMenuElems::Back{};}))
        .buildHorizontalList()));
  rightLines.addElem(WL(setWidth, 220, WL(label, "Home map biome: " + campaignOptions.currentBiome)));
  int retiredPosX = 640;
  vector<SGuiElem> interior {
      WL(stopKeyEvents),
      WL(keyHandler, [&menuState] {
        menuState.index.left();
      }, Keybinding("MENU_LEFT"), true),
      WL(keyHandler, [&menuState]{
        menuState.index.right();
      }, Keybinding("MENU_RIGHT"), true),
      WL(keyHandler, [&menuState] {
        menuState.index.up();
      }, Keybinding("MENU_UP"), true),
      WL(keyHandler, [&menuState]{
        menuState.index.down();
      }, Keybinding("MENU_DOWN"), true),
      WL(keyHandler, [&] { queue.push(CampaignActionId::CANCEL); }, Keybinding("EXIT_MENU"), true),
  };
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
  return
      //WL(preferredSize, 1000, 705,
         WL(window, WL(margins, WL(stack, std::move(interior)), 5),
            [&queue] { queue.push(CampaignActionId::CANCEL); });
}

const Vec2 warlordMenuSize(550, 550);

SGuiElem GuiBuilder::drawWarlordMinionsMenu(SyncQueue<variant<int, bool>>& queue,
    const vector<PlayerInfo>& minions, vector<int>& chosen, int maxCount) {
  auto lines = gui.getListBuilder(legendLineHeight);
  vector<PlayerInfo> chosenInfos;
  for (auto index : chosen)
    chosenInfos.push_back(minions[index]);
  auto minionFun = [minions, &queue](UniqueEntity<Creature>::Id id) {
    for (int i : All(minions))
      if (minions[i].creatureId == id) {
        queue.push(i);
        return;
      }
    fail();
  };
  vector<PlayerInfo> availableInfos;
  for (int i : All(minions))
    if (!chosen.contains(i))
      availableInfos.push_back(minions[i]);
  if (!availableInfos.empty()) {
    lines.addElem(WL(label, "Select a team from your minions:"));
    lines.addMiddleElem(WL(scrollable, drawCreatureList(availableInfos, minionFun, 1)));
  }
  if (!chosenInfos.empty()) {
    lines.addBackElem(WL(label, "Team:"));
    lines.addBackElem(WL(label, "Max team size: " + toString(maxCount), Renderer::smallTextSize(), Color::GRAY),
        legendLineHeight * 2 / 3);
    lines.addBackElemAuto(drawCreatureList(chosenInfos, minionFun, 1));
  }
  lines.addSpace();
  lines.addBackElem(WL(centerHoriz, WL(getListBuilder)
        .addElemAuto(WL(buttonLabel, "Go back",
            WL(button, [&queue] { queue.push(false); })))
        .addSpace(20)
        .addElemAuto(chosen.empty()
            ? WL(buttonLabelInactive, "Next")
            : WL(buttonLabel, "Next", [&queue] { queue.push(true); }))
        .buildHorizontalList()));
  return WL(preferredSize, warlordMenuSize,
      WL(window, WL(margins, lines.buildVerticalList(), 20), [&queue] { queue.push(false); }));
}

SGuiElem GuiBuilder::drawRetiredDungeonMenu(SyncQueue<variant<string, bool, none_t>>& queue,
    RetiredGames& retired, string searchString, int maxGames) {
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(WL(label, "Choose retired dungeons to attack:"));
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, "Search: "))
      .addElem(WL(textFieldFocused, 10, [ret = searchString] { return ret; },
          [&queue](string s){ queue.push(std::move(s));}), 200)
      .addSpace(10)
      .addElemAuto(WL(buttonLabel, "X",
          [&queue]{ queue.push(string());}))
      .buildHorizontalList()
  );
  auto dungeonLines = gui.getListBuilder(legendLineHeight);
  auto addedDungeons = drawRetiredGames(retired, [&queue] { queue.push(none);}, none, "",
      [](int){return false;}).first;
  int addedHeight = addedDungeons.getSize();
  if (!addedDungeons.isEmpty()) {
    dungeonLines.addElem(WL(label, "Added:", Color::YELLOW));
    dungeonLines.addElem(addedDungeons.buildVerticalList(), addedHeight);
  }
  GuiFactory::ListBuilder retiredList = drawRetiredGames(retired,
      [&queue] { queue.push(none);}, maxGames, searchString, [](int){return false;}).first;
  if (retiredList.isEmpty() && addedDungeons.isEmpty())
    retiredList.addElem(WL(label, "No retired dungeons found :("));
  if (!retiredList.isEmpty())
    dungeonLines.addElem(WL(label, "Local:", Color::YELLOW));
  dungeonLines.addElemAuto(retiredList.buildVerticalList());
  lines.addMiddleElem(WL(scrollable, dungeonLines.buildVerticalList()));
  lines.addSpace();
  lines.addBackElem(WL(centerHoriz, WL(getListBuilder)
        .addElemAuto(WL(buttonLabel, "Go back",
            WL(button, [&] { queue.push(false); })))
        .addSpace(20)
        .addElemAuto(WL(conditional,
            WL(buttonLabel, "Confirm", [&] { queue.push(true); }),
            WL(buttonLabelInactive, "Confirm"),
            [&retired] { return retired.getNumActive() >= 1; }))
        .buildHorizontalList()));
  return WL(preferredSize, warlordMenuSize,
      WL(window, WL(margins, lines.buildVerticalList(), 20), [&queue] { queue.push(false); }));
}

SGuiElem GuiBuilder::drawCreatureTooltip(const PlayerInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, info.title));
  lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(info.attributes)));
  for (auto& elem : drawEffectsList(info, false))
    lines.addElem(std::move(elem));
  ItemCounts counts;
  for (auto& item : info.inventory)
    counts[item.viewId[0]] += item.number;
  lines.addElemAuto(drawLyingItemsList("Inventory:", counts, 200));
  if (info.experienceInfo.combatExperience > 0)
    lines.addElemAuto(drawExperienceInfo(info.experienceInfo));
  return WL(miniWindow, WL(margins, lines.buildVerticalList(), 15));
}

SGuiElem GuiBuilder::drawCreatureList(const vector<PlayerInfo>& creatures,
    function<void(UniqueEntity<Creature>::Id)> button, int zoom) {
  if (hasController())
    creatureListIndex = Vec2(0, 0);
  auto minionLines = WL(getListBuilder, 20 * zoom);
  auto line = WL(getListBuilder, 30 * zoom);
  const auto numColumns = 12 / zoom + 1;
  for (auto& elem : creatures) {
    auto tooltip = drawCreatureTooltip(elem);
    auto icon = WL(stack,
        WL(tooltip2, tooltip, [](auto& r) { return r.bottomLeft(); }),
        WL(margins, WL(stack, WL(viewObject, elem.viewId, zoom),
          WL(label, toString((int) elem.bestAttack.value), 10 * zoom)), 5, 5, 5, 5));
    if (button) {
      auto tooltipSize = *tooltip->getPreferredSize();
      line.addElemAuto(WL(stack, makeVec(
          WL(conditionalStopKeys, WL(stack,
              WL(uiHighlight),
              WL(keyHandler, [id = elem.creatureId, button] { button(id); }, Keybinding("MENU_SELECT"), true),
              WL(renderTopLayer, WL(preferredSize, 5, 5, WL(translate, tooltip, Vec2(-tooltipSize.x, 0), tooltipSize)))
          ), [this, ind = Vec2(line.getLength(), minionLines.getLength() / 2)] {
            return creatureListIndex == ind;
          }),
          WL(mouseHighlight2, WL(uiHighlight)),
          WL(button, [id = elem.creatureId, button] { button(id); }),
          std::move(icon))));
    } else
      line.addElemAuto(std::move(icon));
    if (line.getLength() >= numColumns) {
      minionLines.addElemAuto(line.buildHorizontalList());
      minionLines.addSpace(10 * zoom);
      line.clear();
    }
  }
  if (!line.isEmpty()) {
    minionLines.addElemAuto(line.buildHorizontalList());
    minionLines.addSpace(10 * zoom);
  }
  const auto numRows = (creatures.size() + numColumns - 1) / numColumns;
  const auto lastRow = creatures.size() % numColumns;
  return WL(stack, makeVec(
      WL(keyHandler, [this, numRows, lastRow, numColumns] {
        if (!creatureListIndex)
          creatureListIndex = Vec2(0, 0);
        creatureListIndex->x = (creatureListIndex->x + 1) % (creatureListIndex->y == numRows - 1 ? lastRow : numColumns);
      }, Keybinding("MENU_RIGHT"), true),
      WL(keyHandler, [this, numRows, lastRow, numColumns] {
        if (!creatureListIndex)
          creatureListIndex = Vec2(0, 0);
        int mod = creatureListIndex->y == numRows - 1 ? lastRow : numColumns;
        creatureListIndex->x = (creatureListIndex->x + mod - 1) % mod;
      }, Keybinding("MENU_LEFT"), true),
      WL(keyHandler, [this, numRows, lastRow] {
        if (!creatureListIndex)
          creatureListIndex = Vec2(0, 0);
        creatureListIndex->y = (creatureListIndex->y + numRows - 1) % numRows;
        if (creatureListIndex->y == numRows - 1)
          creatureListIndex->x = min(creatureListIndex->x, lastRow - 1);
      }, Keybinding("MENU_UP"), true),
      WL(keyHandler, [this, numRows, lastRow] {
        if (!creatureListIndex)
          creatureListIndex = Vec2(0, 0);
        creatureListIndex->y = (creatureListIndex->y + 1) % numRows;
        if (creatureListIndex->y == numRows - 1)
          creatureListIndex->x = min(creatureListIndex->x, lastRow - 1);
      }, Keybinding("MENU_DOWN"), true),
      minionLines.buildVerticalList()
  ));
}

SGuiElem GuiBuilder::drawCreatureInfo(SyncQueue<bool>& queue, const string& title, bool prompt,
    const vector<PlayerInfo>& creatures) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, title)));
  const int windowWidth = 540;
  lines.addMiddleElem(WL(scrollable, WL(margins, drawCreatureList(creatures, nullptr), 10)));
  lines.addSpace(15);
  auto bottomLine = WL(getListBuilder)
      .addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, "Confirm"), &gui, C_BUILDINGS_CONFIRM, none),
          WL(button, [&queue] { queue.push(true);})));
  if (prompt) {
    bottomLine.addSpace(20);
    bottomLine.addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, "Cancel"), &gui, C_BUILDINGS_CANCEL, none),
          WL(button, [&queue] { queue.push(false);})));
  }
  lines.addBackElem(WL(centerHoriz, bottomLine.buildHorizontalList()));
  int margin = 15;
  return WL(stack,
      WL(keyHandler, [&queue] { queue.push(true);}, Keybinding("MENU_SELECT"), true),
      WL(setWidth, 2 * margin + windowWidth,
          WL(window, WL(margins, lines.buildVerticalList(), margin), [&queue] { queue.push(false); }))
  );
}

SGuiElem GuiBuilder::drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>& queue, const string& title,
      const vector<PlayerInfo>& team, const string& cancelText) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, title)));
  const int windowWidth = 480;
  lines.addMiddleElem(WL(scrollable, WL(margins, drawCreatureList(team, [&queue] (auto id) { queue.push(id); }), 10)));
  lines.addSpace(15);
  if (!cancelText.empty()) {
    lines.addBackElem(WL(centerHoriz, WL(standardButton,
        gui.getKeybindingMap()->getGlyph(WL(label, cancelText), &gui, C_BUILDINGS_CANCEL, none),
        WL(button, [&queue] { queue.push(none);}))), legendLineHeight);
  } int margin = 15;
  return WL(setWidth, 2 * margin + windowWidth,
      WL(window, WL(margins, lines.buildVerticalList(), margin), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawZLevelButton(const CurrentLevelInfo& info, Color textColor) {
  auto callback = [this, info] (Rectangle bounds) {
    vector<SGuiElem> lines;
    vector<function<void()>> callbacks;
    int maxWidth = 0;
    for (int i : All(info.zLevels)) {
      callbacks.push_back([i, this, &info] {
        this->callbacks.input(UserInput{UserInputId::SCROLL_STAIRS, i - info.levelDepth});
      });
      auto elem = WL(labelHighlight, info.zLevels[i]);
      maxWidth = max(maxWidth, *elem->getPreferredWidth());
      lines.push_back(WL(centerHoriz, std::move(elem)));
    }
    int selected = info.levelDepth;
    drawMiniMenu(std::move(lines), std::move(callbacks), {},
        Vec2(bounds.middle().x - maxWidth / 2 - 30, bounds.bottom()), maxWidth + 60, true, true, &selected);
  };
  return WL(stack,
      WL(renderInBounds, WL(centerHoriz, WL(labelHighlight, info.name, textColor))),
      info.zLevels.empty() ? WL(empty) : WL(buttonRect, callback),
      WL(keyHandlerRect, [this, callback](Rectangle bounds) {
        if (!bottomWindow)
          callback(bounds);
      }, {gui.getKey(C_BUILDINGS_CANCEL)}, true));
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
      line.addElemAuto(WL(stack,
          getButton(info->levelDepth > 0, "<", UserInput{UserInputId::SCROLL_STAIRS, -1 }),
          WL(keyHandler, getButtonCallback(UserInput{UserInputId::SCROLL_STAIRS, -1}), Keybinding("SCROLL_Z_UP"))
      ));
    line.addMiddleElem(WL(topMargin, 3, drawZLevelButton(*info, textColor)));
    if (!info->zLevels.empty())
      line.addBackElemAuto(WL(stack,
          getButton(info->levelDepth < info->zLevels.size() - 1, ">", UserInput{UserInputId::SCROLL_STAIRS, 1}),
          WL(keyHandler, getButtonCallback(UserInput{UserInputId::SCROLL_STAIRS, 1}), Keybinding("SCROLL_Z_DOWN"))
      ));
    lines.addElem(WL(stack,
        WL(stopMouseMovement),
        WL(rectangle, Color(47, 31, 0), Color::BLACK),
        line.buildHorizontalList()
    ));
  }
  auto travelButton = [&] {
    if (gameInfo.tutorial || !gameInfo.isSingleMap)
      return WL(stack, makeVec(
          getHintCallback({"Open world map."}),
          WL(mouseHighlight2, WL(icon, GuiFactory::IconId::MINIMAP_WORLD2),
             WL(icon, GuiFactory::IconId::MINIMAP_WORLD1)),
          WL(conditional, WL(blink, WL(icon, GuiFactory::IconId::MINIMAP_WORLD2)), tutorialPredicate),
          WL(button, getButtonCallback(UserInputId::DRAW_WORLD_MAP)),
          WL(keyHandler, getButtonCallback(UserInputId::DRAW_WORLD_MAP), Keybinding("OPEN_WORLD_MAP"))
      ));
    else
      return WL(icon, GuiFactory::IconId::MINIMAP_WORLD1);
  }();
  return lines.addElemAuto(
      WL(centerHoriz, WL(minimapBar,
        WL(preferredSize, 48, 48, std::move(travelButton)),
        WL(preferredSize, 48, 48, WL(stack, makeVec(
            getHintCallback({"Scroll to your character."}),
            WL(mouseHighlight2, WL(icon, GuiFactory::IconId::MINIMAP_CENTER2),
               WL(icon, GuiFactory::IconId::MINIMAP_CENTER1)),
            WL(conditional, WL(blink, WL(icon, GuiFactory::IconId::MINIMAP_CENTER2)), tutorialPredicate),
            WL(button, getButtonCallback(UserInputId::SCROLL_TO_HOME)),
            WL(keyHandler, getButtonCallback(UserInputId::SCROLL_TO_HOME), Keybinding("SCROLL_TO_PC"))
        )))
  ))).buildVerticalList();
}

SGuiElem GuiBuilder::drawTextInput(SyncQueue<optional<string>>& queue, const string& title, const string& value,
    int maxLength) {
  auto text = make_shared<string>(value);
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addSpace(20);
  lines.addElem(WL(label, title));
  lines.addElem(WL(textFieldFocused, maxLength, [text] { return *text;},
      [text](string s) { *text = std::move(s); }), 30);
  lines.addSpace(legendLineHeight);
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, "Confirm"), &gui, Keybinding("MENU_SELECT")),
          WL(button, [&queue, text] { queue.push(*text); })))
      .addSpace(15)
      .addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, "Cancel"), &gui, Keybinding("EXIT_MENU")),
          WL(button, [&queue] { queue.push(none); })))
      .buildHorizontalList()));
  return WL(stack,
      WL(window, WL(setWidth, 600, WL(margins, lines.buildVerticalList(), 50, 0, 50, 0)),
          [&queue] { queue.push(none); }),
      WL(keyHandler, [&queue] { queue.push(none); }, Keybinding("EXIT_MENU"), true),
      WL(keyHandler, [&queue, text] { queue.push(*text); }, Keybinding("MENU_SELECT"), true)
  );
}
