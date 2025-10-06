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
#include "tribe_alignment.h"
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
  options->addTrigger(OptionId::LANGUAGE, [this] (int on) {
    clearCache();
  });
}

void GuiBuilder::clearCache() {
  cache = CallCache<SGuiElem>(1000);
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

SGuiElem GuiBuilder::getHintCallback(const vector<TString>& s) {
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

void GuiBuilder::setActiveGroup(const TString& group, optional<TutorialHighlight> tutorial) {
  closeOverlayWindowsAndClearButton();
  activeGroup = group;
  if (tutorial)
    onTutorialClicked(combineHash(group), *tutorial);
}

void GuiBuilder::setActiveButton(int num, ViewId viewId, optional<TString> group,
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
  return WL(getListBuilder)
      .addElemAuto(WL(rightMargin, 5, WL(label, TString(cost.second), color)))
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
// Caching messes button overlays on the deck, so disable
//  auto getValue = [this] (CollectiveInfo::Button button, int num, const optional<TutorialInfo>& tutorial) {
    auto line = WL(getListBuilder);
    line.addElem(WL(viewObject, button.viewId), 35);
    auto tutorialHighlight = button.tutorialHighlight;
    if (button.state != CollectiveInfo::Button::ACTIVE)
      line.addElemAuto(WL(label, TSentence("ITEM_COUNT", button.name, button.count), Color::GRAY));
    else {
      line.addElemAuto(WL(label, TSentence("ITEM_COUNT", button.name, button.count), Color::WHITE));
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
//  };
//  return cache->get(getValue, THIS_LINE, button, num, tutorial);
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
  if (buttons.empty())
    return WL(empty);
  vector<SGuiElem> keypressOnly;
  auto elems = WL(getListBuilder, legendLineHeight);
  elems.addSpace(5);
  TString lastGroup;
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
            optional<TString> group;
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
  auto addBuiltinButton = [this, &lines, &buttonCnt] (ViewId viewId, TStringId name, BottomWindowId windowId) {
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
  addBuiltinButton(ViewId("special_bmbw"), TStringId("BESTIARY_HELP_BUTTON"), BESTIARY);
  addBuiltinButton(ViewId("scroll"), TStringId("ITEMS_HELP_BUTTON"), ITEMS_HELP);
  addBuiltinButton(ViewId("book"), TStringId("SPELL_SCHOOLS_HELP_BUTTON"), SPELL_SCHOOLS);
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
    maxMoreName = max(maxMoreName, renderer.getTextLength(gui.translate(numResource[i].name)));
  for (int i : All(numResource)) {
    auto res = WL(getListBuilder);
    if ( i >= numResourcesShown)
      res.addElem(WL(label, numResource[i].name), maxMoreName + 20);
    res.addElem(WL(viewObject, numResource[i].viewId), 30);
    res.addElem(WL(labelFun, [&numResource, i] { return TString(numResource[i].count); },
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
          WL(label, TStringId("MORE_BUTTON")),
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
      return TSentence("MINIONS_HEADER", {capitalFirst(info.populationString), TString(info.minionCount),
        TString(info.minionLimit)}); }), 150);
  bottomLine.addSpace(space);
  bottomLine.addElem(getTurnInfoGui(gameInfo.time), 50);
  bottomLine.addSpace(space);
  bottomLine.addElem(getSunlightInfoGui(sunlightInfo), 80);
  return WL(getListBuilder, legendLineHeight)
        .addElem(WL(centerHoriz, drawResources(info.numResource, gameInfo.tutorial, width)))
        .addElem(WL(centerHoriz, bottomLine.buildHorizontalList()))
        .buildVerticalList();
}

TStringId GuiBuilder::getGameSpeedName(GuiBuilder::GameSpeed gameSpeed) const {
  switch (gameSpeed) {
    case GameSpeed::SLOW: return TStringId("SLOW_GAME_SPEED");
    case GameSpeed::NORMAL: return TStringId("NORMAL_GAME_SPEED");
    case GameSpeed::FAST: return TStringId("FAST_GAME_SPEED");
    case GameSpeed::VERY_FAST: return TStringId("VERY_FAST_GAME_SPEED");
  }
}

TStringId GuiBuilder::getCurrentGameSpeedName() const {
  if (clock->isPaused())
    return TStringId("PAUSED_GAME_SPEED");
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
      lines.push_back(WL(label, TStringId("PAUSE_BUTTON")));
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
            .addElemAuto(WL(label, TStringId("SPEED_LABEL")))
            .addElem(WL(labelFun, [this] { return getCurrentGameSpeedName();},
              [this] { return clock->isPaused() ? Color::RED : Color::WHITE; }), 70).buildHorizontalList(),
        WL(buttonRect, speedMenu),
        getGameSpeedHotkeys()
    ));
    int modifiedSquares = info.modifiedSquares;
    int totalSquares = info.totalSquares;
    bottomLine.addBackElem(WL(stack,
        WL(labelFun, [=]()->TString {
          switch (counterMode) {
            case CounterMode::NONE:
              return TString();
            case CounterMode::FPS:
              return TString("FPS " + toString(fpsCounter.getFps()) + " / " + toString(upsCounter.getFps()));
            case CounterMode::LAT:
              return TString("LAT " + toString(fpsCounter.getMaxLatency()) + "ms / " + toString(upsCounter.getMaxLatency()) + "ms");
            case CounterMode::SMOD:
              return TString("SMOD " + toString(modifiedSquares) + "/" + toString(totalSquares));
          }
        }, Color::WHITE),
        WL(button, [=]() { counterMode = (CounterMode) ( ((int) counterMode + 1) % 4); })), 120);
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
        lines.addElem(WL(label, TStringId("IMMIGRANT_AUTO_ACCEPTED"), Color::GREEN));
        break;
      case ImmigrantAutoState::AUTO_REJECT:
        lines.addElem(WL(label, TStringId("IMMIGRANT_AUTO_REJECTED"), Color::RED));
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
    lines.addElem(WL(label, TSentence("IMMIGRANT_TURNS_LEFT", TString(*info.timeLeft))));
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
  auto continueButton = WL(setHeight, legendLineHeight, WL(buttonLabelBlink, TStringId("CONTINUE_BUTTON"),
      continueCallback));
  auto backButton = WL(setHeight, legendLineHeight, WL(buttonLabel, TStringId("GO_BACK_BUTTON"),
      getButtonCallback(UserInputId::TUTORIAL_GO_BACK)));
  SGuiElem warning;
  warning = WL(label, TSentence("PRESS_TO_UNPAUSE", hasController() ? TString("[Y]"_s) : TStringId("SPACE_KEY")),
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

static TStringId getVillageActionText(VillageAction action) {
  switch (action) {
    case VillageAction::TRADE:
      return TStringId("VILLAGE_ACTION_TRADE");
    case VillageAction::PILLAGE:
      return TStringId("VILLAGE_ACTION_PILLAGE");
  }
}

SGuiElem GuiBuilder::drawVillainInfoOverlay(const VillageInfo::Village& info, bool showDismissHint) {
  auto lines = WL(getListBuilder, legendLineHeight);
  auto name = [&] {
    if (!info.tribeName.empty())
      return combineWithCommas({info.name, info.tribeName});
    return info.name;
  }();
 lines.addElem(
      WL(getListBuilder)
          .addElemAuto(WL(label, capitalFirst(name)))
          .addSpace(100)
          .addElemAuto(drawVillainType(info.type))
          .buildHorizontalList());
  if (info.attacking)
    lines.addElem(WL(label, TStringId("VILLAIN_ATTACKING"), Color::RED));
  if (info.access == VillageInfo::Village::NO_LOCATION)
    lines.addElem(WL(label, TStringId("VILLAIN_LOCATION_UNKNOWN"), Color::LIGHT_BLUE));
  else if (info.access == VillageInfo::Village::INACTIVE)
    lines.addElem(WL(label, TStringId("VILLAIN_OUTSIDE_INFLUENCE_ZONE"), Color::GRAY));
  if (info.isConquered)
    lines.addElem(WL(label, TStringId("VILLAIN_CONQUERED"), Color::LIGHT_BLUE));
  auto triggers = info.triggers;
  sort(triggers.begin(), triggers.end(),
      [] (const VillageInfo::Village::TriggerInfo& t1, const VillageInfo::Village::TriggerInfo& t2) {
          return t1.value > t2.value;});
  if (!triggers.empty()) {
    auto line = WL(getListBuilder);
    line.addElemAuto(WL(label, TStringId("VILLAIN_TRIGGERED_BY")));
    bool addComma = false;
    for (auto& t : triggers) {
      auto name = t.name;
  /*#ifndef RELEASE
      name += " " + toString(t.value);
  #endif*/
      if (addComma)
        line.addElemAuto(WL(label, TString(", "_s)));
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
    lines.addElem(WL(label, TStringId("RIGHT_CLICK_TO_CLEAR"), Renderer::smallTextSize()), legendLineHeight * 2 / 3);
  else if (!villainsIndex)
    lines.addElem(WL(label, TStringId("LEFT_TRIGGER_TO_CLEAR"), Renderer::smallTextSize()), legendLineHeight * 2 / 3);
  else {
    auto line = WL(getListBuilder)
        .addElemAuto(gui.steamInputGlyph(C_ZOOM, 16))
        .addElemAuto(WL(label, TStringId("X_TO_CLEAR"), Renderer::smallTextSize()));
    if (!!action)
      line.addSpace(30)
          .addElemAuto(gui.steamInputGlyph(C_BUILDINGS_CONFIRM, 16))
          .addElemAuto(WL(label, TSentence("X_TO_VILLAGE_ACTION", getVillageActionText(*action)), Renderer::smallTextSize()));
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
  lines.addElem(WL(label, TSentence("ATTACKING_IN", TString(info.numTurns)),
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

static TStringId getRebellionChanceText(CollectiveInfo::RebellionChance chance) {
  switch (chance) {
    case CollectiveInfo::RebellionChance::HIGH:
      return TStringId("REBELLION_CHANCE_HIGH");
    case CollectiveInfo::RebellionChance::MEDIUM:
      return TStringId("REBELLION_CHANCE_MEDIUM");
    case CollectiveInfo::RebellionChance::LOW:
      return TStringId("REBELLION_CHANCE_LOW");
  }
}

SGuiElem GuiBuilder::drawRebellionOverlay(const CollectiveInfo::RebellionChance& rebellionChance) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, TStringId("CHANCE_OF_PRISONER_ESCAPE")))
      .addElemAuto(WL(label, getRebellionChanceText(rebellionChance), getRebellionChanceColor(rebellionChance)))
      .buildHorizontalList());
  lines.addElem(WL(label, TStringId("REMOVE_PRISONERS_OR_INCREASE_FORCES")));
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
            .addElemAuto(WL(translate, WL(label, TStringId("ALL_VILLAINS")), Vec2(0, labelOffsetY)))
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
        WL(label, TStringId("REBELLION_RISK"), getRebellionChanceColor(*rebellionChance)),
        {ViewId("prisoner")},
        drawRebellionOverlay(*rebellionChance)
    );
  if (nextWave)
    addVillainButton(
        getButtonCallback({UserInputId::DISMISS_NEXT_WAVE}),
        []{},
        WL(label, TStringId("NEXT_ENEMY_WAVE"), getNextWaveColor(*nextWave)),
        nextWave->viewId,
        drawNextWaveOverlay(*nextWave)
    );
  for (int i : All(info.villages)) {
    SGuiElem label;
    TStringId labelText;
    function<void()> actionCallback = [](){};
    auto& elem = info.villages[i];
    if (elem.attacking) {
      labelText = TStringId("VILLAIN_ATTACKING");
      label = WL(label, labelText, Color::RED);
    } else if (!elem.triggers.empty()) {
      labelText = TStringId("VILLAIN_TRIGGERED");
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
    lines.push_back(WL(label, TSentence("MAIN_VILLAINS_CONQUERED", TString(info.numConqueredMainVillains),
        TString(info.numMainVillains))));
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
    auto name = [&] {
      if (!elem.tribeName.empty())
        return combineWithCommas({elem.name, elem.tribeName});
      return elem.name;
    }();
    auto label = WL(label, capitalFirst(name), labelColor);
    if (elem.isConquered)
      label = WL(stack, std::move(label), WL(crossOutText, Color::RED));
    if (!!elem.action) {
      label = WL(getListBuilder)
          .addElemAuto(std::move(label))
          .addBackElemAuto(WL(label, TSentence("VILLAINS_MENU_ACTION", getVillageActionText(*elem.action)), Color::GREEN))
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
        WL(setWidth, elemWidth, WL(topMargin, -2, WL(centerHoriz, WL(label, TString("?"_s), 32, Color::GREEN))))
    )));
  return WL(setWidth, elemWidth, WL(stack,
        WL(stopMouseMovement),
        lines.buildVerticalList()));
}

TStringId GuiBuilder::leftClickText() {
  return hasController() ? TStringId("IMMIGRATION_HELP_CONTROLLER1") : TStringId("IMMIGRATION_HELP_MOUSE1");
}

TStringId GuiBuilder::rightClickText() {
  return hasController() ? TStringId("IMMIGRATION_HELP_CONTROLLER2") : TStringId("IMMIGRATION_HELP_MOUSE2");
}

SGuiElem GuiBuilder::getImmigrationHelpText() {
  return WL(labelMultiLine, TSentence("IMMIGRATION_HELP", leftClickText(), rightClickText()), legendLineHeight);
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
          getHintCallback({TStringId("TIME_REMAINING_TILL_NIGHTFALL")}),
          getHintCallback({TStringId("TIME_REMAINING_TILL_DAY")}),
          [&] { return sunlightInfo.description == TStringId("DAY");}),
      WL(labelFun, [&] { return TSentence("SUNLIGHT_INFO_DESCRIPTION_AND_TIMEOUT", sunlightInfo.description, TString(sunlightInfo.timeRemaining)); },
          [&] { return sunlightInfo.description == TStringId("DAY") ? Color::WHITE : Color::LIGHT_BLUE;}));
}

SGuiElem GuiBuilder::getTurnInfoGui(const GlobalTime& turn) {
  return WL(stack, getHintCallback({TStringId("CURRENT_TURN")}),
      WL(labelFun, [&turn] { return TString("T: " + toString(turn)); }, Color::WHITE));
}

vector<SGuiElem> GuiBuilder::drawPlayerAttributes(const vector<AttributeInfo>& attr) {
  vector<SGuiElem> ret;
  for (auto& elem : attr) {
    auto getValue = [this](const AttributeInfo& elem) {
      auto attrText = TString(elem.value);
      if (elem.bonus != 0)
        attrText = combineWithNoSpace(std::move(attrText), toStringWithSign(elem.bonus));
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
            WL(margins, WL(label, TString((int) attr.second)), 0, 2, 0, 0)), 30));
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

static optional<TStringId> getIntrinsicStateText(const ItemInfo& item) {
  if (auto state = item.intrinsicAttackState) {
    if (state == item.INACTIVE)
      return TStringId("INTRINSIC_ATTACK_INACTIVE");
    return item.intrinsicExtraAttack ? TStringId("EXTRA_INTRINSIC_ATTACK") : TStringId("USED_WHEN_NO_WEAPON");
  }
  return none;
}

static optional<Color> getIntrinsicStateColor(const ItemInfo& item) {
  if (auto state = item.intrinsicAttackState) {
    if (state == item.INACTIVE)
      return Color::LIGHT_GRAY;
    return item.intrinsicExtraAttack ? Color::GREEN : Color::WHITE;
  }
  return none;
}

vector<TString> GuiBuilder::getItemHint(const ItemInfo& item) {
  vector<TString> out { capitalFirst(item.fullName)};
  out.append(item.description);
  if (item.equiped)
    out.push_back(item.representsSteed() ? TStringId("CURRENTLY_RIDING_STEED") : TStringId("ITEM_EQUIPED"));
  if (item.pending)
    out.push_back(item.representsSteed() ? TStringId("CURRENTLY_NOT_RIDING_STEED") : TStringId("ITEM_NOT_EQUIPED"));
  if (auto text = getIntrinsicStateText(item))
    out.push_back(*text);
  if (!item.unavailableReason.empty())
    out.push_back(item.unavailableReason);
  return out;
}

SGuiElem GuiBuilder::drawBestAttack(const BestAttack& info) {
  return WL(getListBuilder, 30)
      .addElem(WL(viewObject, info.viewId))
      .addElem(WL(topMargin, 2, WL(label, TString((int) info.value))))
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
  if (item.number > 1) {
    line.addElemAuto(WL(rightMargin, 0, WL(label, TString(item.number), color)));
    line.addSpace(6);
  } else
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
      line.addBackElemAuto(WL(label, TString(item.owner->second)));
    line.addBackElem(WL(viewObject, item.owner->first.viewId), viewObjectWidth);
    line.addBackElem(drawBestAttack(item.owner->first.bestAttack), getItemLineOwnerMargin() - viewObjectWidth);
  }
  if (item.price)
    line.addBackElemAuto(drawCost(*item.price));
  if (item.weight)
    line.addBackElemAuto(WL(label, TString("[" + getWeightString(*item.weight * item.number) + "]")));
  if (onMultiClick && item.number > 1) {
    line.addBackElem(WL(stack,
        WL(label, TString("[#]"_s)),
        WL(button, onMultiClick),
        getTooltip({TStringId("CLICK_TO_CHOOSE_COUNT")}, int(item.ids.front().getGenericId()))), 25);
  }
  auto elem = line.buildHorizontalList();
  if (item.tutorialHighlight)
    elem = WL(stack, WL(tutorialHighlight), std::move(elem));
  return WL(margins, std::move(elem), leftMargin, 0, 0, 0);
}

SGuiElem GuiBuilder::getTooltip(const vector<TString>& text, int id, milliseconds delay, bool forceEnableTooltip) {
  return cache->get(
      [this, delay, forceEnableTooltip](const vector<TString>& text) {
        return forceEnableTooltip
            ? WL(tooltip, text, delay)
            : WL(conditional, WL(tooltip, text, delay),
                [this] { return !disableTooltip && !mouseGone;}); },
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
      lines.addElem(WL(label, TStringId("TRAINING_POTENTIAL_LABEL"), Color::YELLOW));
    trainingHeader = true;
    lines.addElem(WL(getListBuilder)
        .addElem(WL(topMargin, -3, WL(viewObject, info.attribute)), 22)
        .addSpace(15)
        .addElemAuto(WL(label, TString("+" + toString(info.limit))))
        .buildHorizontalList());
  }
  if (!creature.spellSchools.empty())
    lines.addElem(WL(getListBuilder)
        .addElemAuto(WL(label, TStringId("SPELL_SCHOOLS_LABEL"), Color::YELLOW))
        .addSpace(8)
        .addElemAuto(WL(label, combineWithCommas(creature.spellSchools)))
        .buildHorizontalList());
  if (!creature.preInstalledParts.empty()) {
    lines.addElem(WL(label, TStringId("PREINSTALLED_PARTS"), Color::YELLOW));
    for (auto& part : creature.preInstalledParts)
      lines.addElem(WL(getListBuilder)
          .addElemAuto(WL(viewObject, part.first))
          .addElemAuto(WL(label, part.second))
          .buildHorizontalList());
  }
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::getTooltip2(SGuiElem elem, GuiFactory::PositionFun fun) {
  return WL(conditional, WL(tooltip2, std::move(elem), std::move(fun)),
      [this] { return !disableTooltip && !mouseGone;});
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
  auto title = gui.getKeybindingMap()->getGlyph(WL(label, TStringId("LYING_HERE_LABEL"), Color::YELLOW), &gui,
      C_BUILDINGS_CONFIRM, TString(TStringId("KEY_ENTER")));
  title = WL(leftMargin, 3, std::move(title));
  int numElems = min<int>(maxElems, info.lyingItems.size());
  Vec2 size = Vec2(380, (1 + numElems) * legendLineHeight);
  if (!info.lyingItems.empty()) {
    for (int i : All(info.lyingItems)) {
      size.x = max(size.x, 40 + viewObjectWidth + renderer.getTextLength(gui.translate(info.lyingItems[i].name)));
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

static TString getActionText(ItemAction a) {
   switch (a) {
    case ItemAction::DROP: return TStringId("DROP_ITEM_ACTION");
    case ItemAction::DROP_MULTI: return TStringId("DROP_MULTI_ITEM_ACTION");
    case ItemAction::DROP_STEED: return TStringId("DROP_STEED_ITEM_ACTION");
    case ItemAction::GIVE: return TStringId("GIVE_ITEM_ACTION");
    case ItemAction::PAY: return TStringId("PAY_ITEM_ACTION");
    case ItemAction::EQUIP: return TStringId("EQUIP_ITEM_ACTION");
    case ItemAction::THROW: return TStringId("THROW_ITEM_ACTION");
    case ItemAction::UNEQUIP: return TStringId("UNEQUIP_ITEM_ACTION");
    case ItemAction::APPLY: return TStringId("APPLY_ITEM_ACTION");
    case ItemAction::REPLACE: return TStringId("REPLACE_ITEM_ACTION");
    case ItemAction::REPLACE_STEED: return TStringId("REPLACE_STEED_ITEM_ACTION");
    case ItemAction::LOCK: return TStringId("LOCK_ITEM_ACTION");
    case ItemAction::REMOVE: return TStringId("REMOVE_ITEM_ACTION");
    case ItemAction::CHANGE_NUMBER: return TStringId("CHANGE_NUMBER_ITEM_ACTION");
    case ItemAction::NAME: return TStringId("NAME_ITEM_ACTION");
    case ItemAction::INTRINSIC_ACTIVATE: return TStringId("INTRINSIC_ACTIVATE_ITEM_ACTION");
    case ItemAction::INTRINSIC_DEACTIVATE: return TStringId("INTRINSIC_DEACTIVATE_ITEM_ACTION");
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

optional<int> GuiBuilder::chooseAtMouse(const vector<TString>& elems) {
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
    ret.push_back(WL(centerHoriz, WL(centerVert, WL(labelUnicode, TString(spell.symbol), Color::WHITE))));
    if (active)
      ret.push_back(WL(button, callback));
  } else {
    ret.push_back(WL(standardButton));
    ret.push_back(WL(centerHoriz, WL(centerVert, WL(labelUnicode, TString(spell.symbol), Color::GRAY))));
    ret.push_back(WL(darken));
    ret.push_back(WL(centeredLabel, Renderer::HOR_VER, TString(*spell.timeout)));
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
    list.addElem(WL(label, TStringId("ABILITIES_LABEL"), Color::YELLOW), legendLineHeight);
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
    auto label = WL(label, TSentence("ALL_BUFF_ADJECTIVES_HELPER", effect.name, info.title), effect.bad ? Color::RED : Color::WHITE);
    if (tooltip)
      label = WL(renderInBounds, std::move(label));
    lines.push_back(WL(stack,
          tooltip ? getTooltip({effect.help}, THIS_LINE + i) : WL(empty),
          std::move(label)));
  }
  return lines;
}

SGuiElem GuiBuilder::getExpIncreaseLine(const CreatureExperienceInfo::TrainingInfo& info, bool infoOnly) {
  auto line = WL(getListBuilder);
  int i = 0;
  auto attrIcons = WL(getListBuilder);
  attrIcons.addElem(WL(topMargin, -3, WL(viewObject, info.viewId)), 22);
  line.addElem(attrIcons.buildHorizontalList(), 80);
  if (!infoOnly)
    line.addElem(WL(label, TString("+" + toStringRounded(info.level, 0.01)),
        info.warning ? Color::RED : Color::WHITE), 50);
  auto limit = TString(info.limit);
  line.addElemAuto(WL(label, TSentence("TRAINING_LIMIT_LABEL", limit)));
  vector<TString> tooltip {
      capitalFirst(TSentence("TRAINING_TYPE_TOOLTIP", info.name)),
      TSentence("TRAINING_LIMIT_TOOLTIP", limit)};
  if (info.warning)
    tooltip.push_back(*info.warning);
  return WL(stack,
             getTooltip(tooltip, THIS_LINE),
             line.buildHorizontalList());
}

SGuiElem GuiBuilder::drawExperienceInfo(const CreatureExperienceInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  auto promoLevel = min<double>(info.combatExperienceCap, info.combatExperience);
  auto builder = WL(getListBuilder)
      .addElemAuto(WL(label, TStringId("EXPERIENCE_LABEL"), Color::YELLOW))
      .addElemAuto(WL(label, TString(" " + toStringRounded(promoLevel, 0.01))));
  if (info.teamExperience > 0)
    builder
        .addElemAuto(WL(label, TString(info.teamExperience > 0 ? " + "_s  : " - "_s)))
        .addElemAuto(WL(label, TString(toStringRounded(info.teamExperience, 0.01))));
  lines.addElem(WL(stack,
      builder.buildHorizontalList(),
      getTooltip({TStringId("EXPERIENCE_EXPLANATION")},
          THIS_LINE)
  ));
  if (info.combatExperience > promoLevel)
    lines.addElem(WL(stack,
        WL(getListBuilder)
          .addElemAuto(WL(label, TStringId("UNREALIZED_EXPERIENCE_LABEL"), Color::YELLOW))
          .addElemAuto(WL(label, TString(toStringRounded(info.combatExperience - promoLevel, 0.01))))
          .buildHorizontalList(),
        getTooltip({info.requiredLuxury == 0 ? TSentence("CREATURE_REQUIRES_QUARTERS")
            : TSentence("CREATURE_REQUIRES_QUARTERS_WITH_LUXURY", TString(info.requiredLuxury))},
            THIS_LINE)));
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawTrainingInfo(const CreatureExperienceInfo& info, bool infoOnly) {
  if (!info.combatExperience && info.training.empty())
    return nullptr;
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, TStringId("TRAINING_INFO_LABEL"), Color::YELLOW));
  for (auto& elem : info.training) {
    lines.addElem(getExpIncreaseLine(elem, infoOnly));
  }
  if (!infoOnly)
    lines.addElemAuto(drawExperienceInfo(info));
  return lines.buildVerticalList();
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
        WL(buttonLabel, TStringId("COMMANDS_BUTTON"), WL(buttonRect, callback)),
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
    list.addElem(WL(label, TStringId("DEBT_BUTTON"), Color::YELLOW));
    list.addElemAuto(WL(labelMultiLineWidth, TStringId("CLICK_ON_DEBT_BUTTON"), legendLineHeight * 2 / 3, 300, Renderer::smallTextSize(),
        Color::LIGHT_GRAY));
    list.addElem(WL(stack,
        drawCost({ViewId("gold"), info.debt}),
        WL(button, getButtonCallback(UserInputId::PAY_DEBT))));
    list.addSpace();
  }
  SGuiElem firstInventoryItem;
  if (!info.inventory.empty()) {
    list.addElem(WL(label, TStringId("INVENTORY_LABEL"), Color::YELLOW));
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
    list.addElem(WL(label, TSentence("TOTAL_WEIGHT_LABEL", TString(getWeightString(totWeight)))));
    list.addElem(WL(label, info.carryLimit ? TSentence("CAPACITY_LABEL", TString(getWeightString(*info.carryLimit))) :
        TStringId("INFINITE_CAPACITY_LABEL")));
    list.addSpace();
  } else if (withKeys && !!inventoryIndex) {
    inventoryIndex = none;
    inventoryScroll.reset();
  }
  if (!info.intrinsicAttacks.empty()) {
    list.addElem(WL(label, TStringId("INTRINSIC_ATTACKS_LABEL"), Color::YELLOW));
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

static TStringId getControlModeLabel(PlayerInfo::ControlMode m) {
  switch (m) {
    case PlayerInfo::FULL: return TStringId("CONTROL_MODE_FULL");
    case PlayerInfo::LEADER: return TStringId("CONTROL_MODE_LEADER");
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
            WL(translate, WL(translate, WL(labelUnicode, TString(u8"⬆"_s), Color::WHITE, 14), Vec2(2, -1)),
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
                ? WL(buttonLabelSelected, getName(order), switchFun)
                : WL(buttonLabel, getName(order), switchFun),
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
        WL(label, getControlModeLabel(*info.controlMode)),
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
    auto label = gui.getKeybindingMap()->getGlyph(WL(label, TStringId("EXIT_CONTROL_MODE")), &gui, keybinding);
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
        WL(label, TString(level), 12 * iconMult)));
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
  auto hint = TStringId("NEW_TEAM_BUTTON_HINT");
  if (!tutorial || info.teams.empty()) {
    const bool isTutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::NEW_TEAM);
    lines.addElem(WL(stack, makeVec(
          WL(dragListener, [this](DragContent content) {
              content.visit<void>(
                  [&](UniqueEntity<Creature>::Id id) {
                    callbacks.input({UserInputId::CREATE_TEAM, id});
                  },
                  [&](const TString& group) {
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
          WL(label, TStringId("NEW_TEAM_BUTTON"), Color::WHITE))));
    ++buttonCnt;
  }
  bool groupChosen = isGroupChosen(info);
  for (int i : All(info.teams)) {
    auto& team = info.teams[i];
    const int numPerLine = 7;
    auto teamLine = WL(getListBuilder, legendLineHeight);
    vector<SGuiElem> currentLine;
    for (auto member : team.members)
      if (auto memberInfo = info.getMinion(member)) {
        currentLine.push_back(drawMinionAndLevel(memberInfo->viewId, (int) memberInfo->bestAttack.value, 1));
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
                    [&](const TString& group) {
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
  list.addElem(WL(label, TSentence("MINIONS_HEADER", {capitalFirst(info.populationString),
        TString(info.minionCount), TString(info.minionLimit)}), Color::WHITE));
  int buttonCnt = 0;
  auto groupChosen = isGroupChosen(info);
  auto addGroup = [&] (const CollectiveInfo::CreatureGroup& elem) {
    auto line = WL(getListBuilder);
    line.addElem(WL(viewObject, elem.viewId), 40);
    SGuiElem tmp = WL(label, TSentence("MINION_GROUP", TString(elem.count),
        elem.count > 1 ? makePlural(elem.name) : elem.name), Color::WHITE);
    line.addElem(WL(renderInBounds, std::move(tmp)), 200);
    auto callback = getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, elem.name});
    auto selectButton = cache->get([this](const TString& group) {
      return WL(releaseLeftButton, getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, group}));
    }, THIS_LINE, elem.name);
    list.addElem(WL(stack, makeVec(
        std::move(selectButton),
        WL(dragSource, elem.name,
            [=]{ return WL(getListBuilder, 10)
                .addElemAuto(WL(label, TString(elem.count)))
                .addSpace(6)
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
    list.addElem(WL(label, TStringId("MINIONS_BY_ABILITY_LABEL"), Color::WHITE));
    for (auto& group : info.automatonGroups)
      addGroup(group);
  }
  list.addElem(WL(label, TStringId("TEAMS_LABEL"), Color::WHITE));
  list.addElemAuto(drawTeams(info, tutorial, buttonCnt));
  list.addSpace();
  list.addElem(WL(stack,
            WL(conditionalStopKeys, WL(stack,
                WL(uiHighlightFrameFilled),
                WL(keyHandler, [this] { toggleBottomWindow(TASKS); }, {gui.getKey(C_BUILDINGS_CONFIRM) }, true)
            ), [this, buttonCnt]{ return minionsIndex == buttonCnt;} ),
            WL(label, TStringId("SHOW_TASKS"), [=]{ return bottomWindow == TASKS ? Color::GREEN : Color::WHITE;}),
            WL(button, [this] { toggleBottomWindow(TASKS); })));
  ++buttonCnt;
  list.addElem(WL(stack,
            WL(conditionalStopKeys, WL(stack,
                WL(uiHighlightFrameFilled),
                WL(keyHandler, getButtonCallback(UserInputId::SHOW_HISTORY), {gui.getKey(C_BUILDINGS_CONFIRM) }, true)
            ), [this, buttonCnt]{ return minionsIndex == buttonCnt;} ),
            WL(label, TStringId("SHOW_MESSAGE_HISTORY")),
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
    lines.push_back(WL(label, TStringId("NO_TASKS_PRESENT")));
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

bool GuiBuilder::yesOrNo(const TString& question) {
  bool ret = false;
  bool exit = false;
  ScriptedUIData data = ScriptedUIDataElems::Record {{
    {"callback"_s, ScriptedUIDataElems::Callback{[&ret, &exit] { ret = true; exit = true; return true; }}},
    {"message"_s, question},
  }};
  ScriptedUIState state;
  auto ui = gui.scripted([&exit]{ exit = true; }, ScriptedUIId("yes_or_no"), data, state);
  drawMiniMenu(std::move(ui), [&]{ return exit; }, renderer.getSize() / 2, 650, false);
  return ret;
}

function<void(Rectangle)> GuiBuilder::getItemUpgradeCallback(const CollectiveInfo::QueuedItemInfo& elem) {
  return [=] (Rectangle bounds) {
      auto lines = WL(getListBuilder, legendLineHeight);
      lines.addElem(WL(label, TStringId("UPGRADES_NEED_TO_BE_PLACED_IN_STORAGE"), Renderer::smallTextSize()));
      lines.addElem(hasController()
          ? WL(getListBuilder)
              .addElemAuto(WL(label, TStringId("ADD_REMOVE_UPGRADES_CONTROLLER_HINT"), Color::YELLOW))
              .addElemAuto(WL(steamInputGlyph, C_BUILDINGS_LEFT))
              .addElemAuto(WL(steamInputGlyph, C_BUILDINGS_RIGHT))
              .buildHorizontalList()
          : WL(label, TStringId("ADD_REMOVE_UPGRADES_MOUSE_HINT"), Color::YELLOW));
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
        auto colorFun = [upgrade, &totalUsed, max = elem.maxUpgrades.second] {
          return totalUsed >= max ? Color::LIGHT_GRAY : Color::WHITE;
        };
        idLine.addElemAuto(WL(viewObject, upgrade.viewId));
        idLine.addElemAuto(WL(label, upgrade.name, colorFun));
        idLine.addBackElem(WL(labelFun, [&increases, i, upgrade, cnt] {
            return TString("(" + toString(upgrade.used * cnt + increases[i]) + "/" + toString(upgrade.used * cnt + upgrade.count) + ")  "); },
            colorFun), 70);
        auto callbackIncrease = [this, &exit, &increases, &totalUsed, i, upgrade, cnt, max = elem.maxUpgrades.second] {
          int toAdd = min(cnt, upgrade.count - increases[i]);
          bool mustSplit = cnt > upgrade.count - increases[i];
          if (toAdd > 0 && totalUsed < max && (!mustSplit ||
              yesOrNo(TSentence("NOT_ENOUGH_UPGRADES", TString(upgrade.count - increases[i]), TString(cnt))))) {
            increases[i] += toAdd;
            ++totalUsed;
            if (mustSplit)
              exit = true;
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
      auto label = WL(labelFun, [&]() -> TString {
        if (totalAvailable - totalUsed > 0)
          return TSentence("ADD_TOP_UPGRADES", TString((totalAvailable - totalUsed) * cnt));
        else
          return TStringId("CLEAR_ALL_UPGRADES");
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
              return TSentence("USED_SLOTS", TString(totalUsed), TString(elem.maxUpgrades.second)); }), 10)
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
      line.addElemAuto(WL(label, TString(elem.second)));
    line.addElemAuto(WL(viewObject, elem.first));
  }
  auto button = WL(buttonRect, getItemUpgradeCallback(elem));
  if (!upgrades.empty())
    return WL(standardButton, line.buildHorizontalList(), button);
  else
    return WL(buttonLabel, TStringId("ITEM_UPGRADE_BUTTON"), button);
}

optional<int> GuiBuilder::getNumber(const TString& title, Vec2 position, Range range, int initial) {
  ScriptedUIState state;
  int result = initial;
  while (initial > range.getEnd())
    range = Range(range.getStart(), range.getStart() + range.getLength() * 10);
  bool confirmed = false;
  while (true) {
    bool changed = false;
    ScriptedUIData data = ScriptedUIDataElems::Record {{
      {"title", title},
      {"range_begin", TString(toString(range.getStart()))},
      {"range_end", TString(toString(range.getEnd()))},
      {"range_inc", ScriptedUIDataElems::Callback { [&range, &changed, &state] {
        if (range.getLength() <= 1000)
          range = Range(range.getStart(), range.getStart() + range.getLength() * 10);
        changed = true;
        state.sliderState.clear();
        return true;
      }}},
      {"range_dec", ScriptedUIDataElems::Callback { [&range, &changed, &state, &result] {
        if (range.getLength() >= 100)
          range = Range(range.getStart(), range.getStart() + range.getLength() / 10);
        result = min(range.getEnd(), result);
        changed = true;
        state.sliderState.clear();
        return true;
      }}},
      {"current", TString(toString(result))},
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
    drawMiniMenu(std::move(ui), exit, position, 650, false);
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
    auto value = getNumber(TStringId("CHANGE_THE_NUMBER_OF_ITEMS"), bounds.bottomLeft(), Range(0, 100), elem.itemInfo.number);
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
  lines.addElem(WL(getListBuilder)
      .addElemAuto(WL(label, TStringId("ITEM_REQUIRES_SKILL_LEVEL")))
      .addElemAuto(WL(viewObject, info.attr))
      .addElemAuto(WL(label, TString(info.minAttrValue)))
      .buildHorizontalList());
  if (info.resourceTabs.size() >= 2) {
    lines.addElem(WL(topMargin, 3, WL(getListBuilder)
        .addElemAuto(WL(label, TStringId("ITEM_MATERIAL_LABEL")))
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
  lines.addElem(WL(label, TStringId("AVAILABLE_WORKSHOP_ITEMS"), Color::YELLOW));
  auto rawTooltip = [&] (const ItemInfo& itemInfo, optional<TString> warning,
      const optional<ImmigrantCreatureInfo>& creatureInfo, int index, pair<TString, int> maxUpgrades) {
    optional<TString> upgradesTip;
    if (maxUpgrades.second > 0 && !maxUpgrades.first.empty())
      upgradesTip = TString(TSentence("UPGRADABLE_WITH_UP_TO", TString(maxUpgrades.second),
          maxUpgrades.second > 1 ? makePlural(maxUpgrades.first) : maxUpgrades.first));
    if (creatureInfo) {
      return cache->get(
          [this](const ImmigrantCreatureInfo& creature, const optional<TString>& warning, optional<TString> upgradesTip) {
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
          .addElemAuto(WL(label, TStringId("ITEM_FROM_INGREDIENT")))
          .addElemAuto(WL(viewObject, elem.ingredient->viewId))
          .buildHorizontalList();
    line.addMiddleElem(WL(renderInBounds, std::move(label)));
    if (elem.price)
      line.addBackElem(WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(*elem.price)), 80);
    SGuiElem guiElem = line.buildHorizontalList();
    if (elem.tutorialHighlight)
      guiElem = WL(stack, WL(tutorialHighlight), std::move(guiElem));
    auto tooltip = rawTooltip(elem, elem.unavailable ? elem.unavailableReason : optional<TString>(),
        options[itemIndex].creatureInfo, itemIndex + THIS_LINE, options[itemIndex].maxUpgrades);
    if (elem.unavailable) {
      CHECK(!elem.unavailableReason.empty());
      guiElem = WL(stack, createTooltip(tooltip, false), std::move(guiElem));
    } else
      guiElem = WL(stack,
          WL(conditional, WL(uiHighlightMouseOver), [this] { return !workshopIndex; }),
          std::move(guiElem),
          WL(button,
              [this, itemIndex]() {
                int count = 1;
                if (renderer.isKeypressed(SDL::SDL_SCANCODE_LSHIFT) || renderer.isKeypressed(SDL::SDL_SCANCODE_RSHIFT))
                  count = 5;
                for (int i : Range(count))
                  callbacks.input({UserInputId::WORKSHOP_ADD, itemIndex});
              }),
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
          .addElemAuto(WL(label, TStringId("ITEM_FROM_INGREDIENT"), color))
          .addElemAuto(WL(viewObject, elem.itemInfo.ingredient->viewId))
          .buildHorizontalList();
    line.addMiddleElem(WL(renderInBounds, std::move(label)));
    function<void(Rectangle)> itemUpgradeCallback;
    if (/*!elem.upgrades.empty() && */elem.maxUpgrades.second > 0) {
      line.addBackElemAuto(WL(leftMargin, 7, drawItemUpgradeButton(elem)));
      line.addBackSpace(5);
      itemUpgradeCallback = getItemUpgradeCallback(elem);
    }
    if (elem.itemInfo.price)
      line.addBackElem(WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(*elem.itemInfo.price)), 80);
    auto changeCountCallback = getItemCountCallback(elem);
    if (info.allowChangeNumber && !elem.itemInfo.ingredient)
      line.addBackElemAuto(WL(leftMargin, 7, WL(buttonLabel, TString("+/-"_s), WL(buttonRect, changeCountCallback))));
    auto removeCallback = getButtonCallback({UserInputId::WORKSHOP_CHANGE_COUNT,
        WorkshopCountInfo{ elem.itemIndex, elem.itemInfo.number, 0 }});
    line.addBackElemAuto(WL(topMargin, 3, WL(leftMargin, 7, WL(stack,
        WL(button, removeCallback),
        WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::RED)))));
    auto tooltip = rawTooltip(elem.itemInfo, elem.paid ? optional<TString>() : elem.itemInfo.unavailableReason,
        queued[i].creatureInfo, i + THIS_LINE, queued[i].maxUpgrades);
    auto guiElem = WL(stack, makeVec(
        WL(bottomMargin, 5, WL(progressBar, Color::DARK_GREEN.transparency(128), elem.productionState)),
        WL(conditionalStopKeys, WL(stack,
            WL(uiHighlightFrameFilled),
            WL(keyHandlerRect, [this, changeCountCallback, removeCallback, itemUpgradeCallback] (Rectangle r) {
              vector<SGuiElem> lines {
                WL(label, TStringId("WORKSHOP_CHANGE_COUNT_BUTTON")),
                WL(label, TStringId("WORKSHOP_REMOVE_BUTTON"))
              };
              vector<function<void()>> callbacks {
                [r, changeCountCallback] { changeCountCallback(r); },
                removeCallback,
              };
              if (itemUpgradeCallback) {
                lines.insert(1, WL(label, TStringId("WORKSHOP_UPGRADE_BUTTON")));
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

SGuiElem GuiBuilder::drawTechUnlocks(const CollectiveInfo::LibraryInfo::TechInfo& tech) {
  auto lines = WL(getListBuilder, legendLineHeight);
  const int width = 550;
  const int margin = 15;
  lines.addElemAuto(WL(labelMultiLineWidth, tech.description, legendLineHeight * 2 / 3, width - 2 * margin));
  lines.addSpace(legendLineHeight);
  for (int i : All(tech.unlocks)) {
    auto& info = tech.unlocks[i];
    if (i == 0 || info.type != tech.unlocks[i - 1].type) {
      if (i > 0)
        lines.addSpace(legendLineHeight / 2);
      lines.addElem(WL(label, TSentence("UNLOCKS_ITEM_TYPE", info.type)));
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
      .addElemAuto(WL(label, collectiveInfo.avatarLevelInfo.title))
      .buildHorizontalList()));
  lines.addElem(WL(label, TSentence("RESEARCH_LEVEL", TString(collectiveInfo.avatarLevelInfo.level))));
  lines.addElem(WL(stack,
      WL(margins, WL(progressBar, Color::DARK_GREEN.transparency(128), collectiveInfo.avatarLevelInfo.progress), -4, 0, -1, 6),
      WL(label, TSentence("NEXT_LEVEL_PROGRESS", TString(info.currentProgress), TString(info.totalProgress)))
  ));
  //lines.addElem(WL(rightMargin, rightElemMargin, WL(alignment, GuiFactory::Alignment::RIGHT, drawCost(info.resource))));
  if (info.warning)
    lines.addElem(WL(label, *info.warning, Renderer::smallTextSize(), Color::RED));
  vector<SGuiElem> activeElems;
  if (numPromotions > 0) {
    lines.addElem(WL(label, TSentence("PROMOTIONS_LABEL"), Color::YELLOW));
    if (collectiveInfo.availablePromotions == 1)
      lines.addElem(WL(label, TStringId("ONE_ITEM_AVAILABLE"), Color::YELLOW));
    else
      lines.addElem(WL(label, TSentence("COUNT_ITEM_AVAILABLE", TString(collectiveInfo.availablePromotions)), Color::YELLOW));
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
      auto minionElem = WL(getListBuilder, legendLineHeight);
      auto line = WL(getListBuilder)
          .addElem(WL(renderInBounds, std::move(minionLabels[i])), min(105, maxWidth))
          .addSpace(10);
      for (int index : All(elem.promotions)) {
        auto& info = elem.promotions[index];
        line.addElem(WL(stack,
            WL(topMargin, -2, WL(viewObject, info.viewId)),
            getTooltip({TSentence("MAKE_SENTENCE", info.description)}, i * 100 + index + THIS_LINE)), 24);
        if (line.getSize() > 250) {
          minionElem.addElem(line.buildHorizontalList());
          line.clear();
        }
      }
      if (!line.isEmpty())
        minionElem.addElem(line.buildHorizontalList());
      minionElem.addSpace(14);
      auto callback = [id = elem.id, options = elem.options, this] (Rectangle bounds) {
        vector<SGuiElem> lines = {WL(label, TSentence("PROMOTION_TYPE_LABEL"))};
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
                WL(tooltip, {TSentence("MAKE_SENTENCE", options[i].description)})
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
            minionElem.buildVerticalList(),
            WL(buttonRect, callback),
            WL(conditionalStopKeys,
                WL(keyHandlerRect, callback, {gui.getKey(C_BUILDINGS_CONFIRM), gui.getKey(C_BUILDINGS_RIGHT)}, true),
                [this, myIndex] { return collectiveTab == CollectiveTab::TECHNOLOGY && techIndex == myIndex; })
        )));
        lines.addElemAuto(activeElems.back());
      } else
        lines.addElemAuto(minionElem.buildVerticalList());
    }
  }
  auto emptyElem = WL(empty);
  auto getUnlocksTooltip = [&] (auto& elem) {
    return WL(translateAbsolute, [=]() { return emptyElem->getBounds().topRight() + Vec2(35, 0); },
        WL(renderTopLayer, drawTechUnlocks(elem)));
  };
  if (numTechs > 0) {
    lines.addElem(WL(label, TStringId("RESEARCH_LABEL"), Color::YELLOW));
    if (collectiveInfo.avatarLevelInfo.numAvailable == 1)
      lines.addElem(WL(label, TStringId("ONE_ITEM_AVAILABLE"), Color::YELLOW));
    else
      lines.addElem(WL(label, TSentence("COUNT_ITEM_AVAILABLE",
          TString(collectiveInfo.avatarLevelInfo.numAvailable)), Color::YELLOW));
    for (int i : All(info.available)) {
      auto& elem = info.available[i];
      auto line = WL(renderInBounds, WL(label, capitalFirst(elem.name), elem.active ? Color::WHITE : Color::GRAY));
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
    lines.addElem(WL(label, TStringId("ALREADY_RESEARCHED_LABEL"), Color::YELLOW));
  for (int i : All(info.researched)) {
    auto& elem = info.researched[i];
    auto line = WL(renderInBounds, WL(label, capitalFirst(elem.name), Color::GRAY));
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
        WL(buttonLabel, TStringId("DISBAND_TEAM_BUTTON"), getButtonCallback({UserInputId::CANCEL_TEAM, *chosenCreature.teamId})));
    list.addElem(WL(labelMultiLine, TStringId("CONTROL_MINION_TO_COMMAND_TEAM"), Renderer::smallTextSize() + 2,
        Renderer::smallTextSize(), Color::LIGHT_GRAY));
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
  leftLines.addElem(WL(label, TStringId("ATTRIBUTES_LABEL"), Color::YELLOW));
  leftLines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(minion.attributes)));
  for (auto& elem : drawEffectsList(minion))
    leftLines.addElem(std::move(elem));
  leftLines.addSpace();
  if (auto elem = drawTrainingInfo(minion.experienceInfo, true))
    leftLines.addElemAuto(std::move(elem));
  if (!minion.spellSchools.empty()) {
    auto line = WL(getListBuilder)
        .addElemAuto(WL(label, TStringId("SPELL_SCHOOLS_LABEL"), Color::YELLOW))
        .addSpace(8);
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

SGuiElem GuiBuilder::drawBestiaryButtons(const vector<PlayerInfo>& minions, int index, const string& searchString) {
  CHECK(!minions.empty());
  auto list = WL(getListBuilder, legendLineHeight);
  for (int i : All(minions)) {
    auto& minion = minions[i];
    if (!searchString.empty()) {
      auto name = gui.translate(minion.name);
      if (name.find(searchString) == string::npos)
        continue;
    }
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
  auto searchBox = cache->get([this] {
    return WL(textField, 15, [this]{ return bestiarySearchString; }, [this](string s) { bestiarySearchString = s;}, []{ return false; });
  }, 0);
  return WL(getListBuilder, legendLineHeight)
      .addElem(
          WL(getListBuilder)
              .addElemAuto(WL(label, TStringId("SEARCH_DUNGEONS")))
              .addElem(std::move(searchBox), 200)
              .addSpace(10)
              .addElemAuto(WL(buttonLabelFocusable, TString("X"_s),
                    [this]{ bestiarySearchString.clear();},
                    [] { return false; }))
              .buildHorizontalList()
      )
      .addSpace(10)
      .addMiddleElem(
          WL(scrollable, gui.margins(list.buildVerticalList(), 0, 5, 10, 0), &minionButtonsScroll, &scrollbarsHeld2)
      )
      .buildVerticalList();
}

SGuiElem GuiBuilder::drawBestiaryOverlay(const vector<PlayerInfo>& creatures, int index, const string& searchString) {
  int margin = 20;
  int minionListWidth = 330;
  SGuiElem menu;
  SGuiElem leftSide = drawBestiaryButtons(creatures, index, searchString);
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
  map<TString, GuiFactory::ListBuilder> overlaysMap;
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
    TString groupName = elem.first;
    elems.push_back(WL(setWidth, 350, WL(conditionalStopKeys,
          WL(miniWindow, WL(stack,
              WL(keyHandler, [=] { clearActiveButton(); }, Keybinding("EXIT_MENU"), true),
              WL(margins, WL(scrollable, lines.buildVerticalList(), &minionButtonsScroll, &scrollbarsHeld, 3), margin))),
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
    lines.addElem(WL(label, capitalFirst(rightClickText()), Color::LIGHT_BLUE));
    for (auto action : object.getExtendedActions())
      lines.addElem(WL(label, getText(action), Color::LIGHT_GRAY));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!lines.isEmpty())
    return lines.buildVerticalList();
  else
    return nullptr;
}

SGuiElem GuiBuilder::drawLyingItemsList(const TString& title, const ItemCounts& itemCounts, int maxWidth) {
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

static TString getLuxuryNumber(double morale) {
#ifndef RELEASE
  return TString(toString(morale));
#else
  return TString(toString(round(10.0 * morale) / 10));
#endif
}

static TStringId getHealthName(bool spirit) {
  return spirit ? TStringId("MATERIALIZATION_LABEL") : TStringId("HEALTH_LABEL");
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
          auto& objectName = viewObject.getDescription();
          lines.addElemAuto(WL(getListBuilder)
                .addElem(WL(viewObject, viewObject.getViewIdList()), 30)
                .addElemAuto(WL(labelMultiLineWidth, objectName, legendLineHeight * 2 / 3, 300))
                .buildHorizontalList());
          if (viewObject.hasModifier(ViewObject::Modifier::DOUBLE_CLICK_RESOURCE) && activeButton == 0) {
            lines.addSpace(legendLineHeight / 3);
            lines.addElem(WL(label, TStringId("RESOURCE_DOUBLE_CLICK_HINT")));
          }
          lines.addSpace(legendLineHeight / 3);
          if (layer == ViewLayer::CREATURE)
            lines.addElemAuto(drawLyingItemsList(TStringId("INVENTORY_LABEL"), index->getEquipmentCounts(), 250));
          if (viewObject.hasModifier(ViewObject::Modifier::HOSTILE))
            lines.addElem(WL(label, TSentence("HOSTILE_LABEL", objectName), Color::ORANGE));
          for (auto status : viewObject.getCreatureStatus()) {
            lines.addElem(WL(label, TSentence(getName(status), objectName), getColor(status)));
            if (auto desc = getDescription(status))
              lines.addElem(WL(label, TSentence(*desc, objectName), getColor(status)));
            break;
          }
          if (!disableClickActions)
            if (auto actions = getClickActions(viewObject))
              if (highlighted.tileScreenPos)
                allElems.push_back(WL(absolutePosition, WL(translucentBackgroundWithBorderPassMouse, WL(margins,
                    WL(setHeight, *actions->getPreferredHeight(), actions), 5, 1, 5, -2)),
                    highlighted.creaturePos.value_or(*highlighted.tileScreenPos) + Vec2(60, 60)));
          if (!viewObject.getBadAdjectives().empty()) {
            lines.addElemAuto(WL(labelMultiLineWidth,
                TSentence("ALL_BUFF_ADJECTIVES_HELPER", capitalFirst(viewObject.getBadAdjectives()), objectName),
                legendLineHeight * 2 / 3, 300,
                Renderer::textSize(), Color::RED, ','));
            lines.addSpace(legendLineHeight / 3);
          }
          if (!viewObject.getGoodAdjectives().empty()) {
            lines.addElemAuto(WL(labelMultiLineWidth,
                TSentence("ALL_BUFF_ADJECTIVES_HELPER", capitalFirst(viewObject.getGoodAdjectives()), objectName),
                legendLineHeight * 2 / 3, 300,
                Renderer::textSize(), Color::GREEN, ','));
            lines.addSpace(legendLineHeight / 3);
          }
          if (viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
            lines.addElem(WL(label, TStringId("HEALED_BY_RITUALS_HINT")));
          if (auto value = viewObject.getAttribute(ViewObjectAttribute::FLANKED_MOD)) {
            lines.addElem(WL(label, TSentence("FLANKED_HINT", TString((int)(100 * (1 - *value)))), Color::RED));
            if (viewObject.hasModifier(ViewObject::Modifier::PLAYER))
              lines.addElem(WL(label, TStringId("USE_A_SHIELD_HINT"), Color::RED));
          }
          if (auto& attributes = viewObject.getCreatureAttributes())
            lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(*attributes)));
          if (auto health = viewObject.getAttribute(ViewObjectAttribute::HEALTH))
            lines.addElem(WL(stack,
                  WL(margins, WL(progressBar, MapGui::getHealthBarColor(*health,
                      viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE)).transparency(70), *health), -2, 0, 0, 3),
                  WL(label, TSentence(getHealthName(viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE)),
                      toPercentage(*health)))));
          if (auto luxury = viewObject.getAttribute(ViewObjectAttribute::LUXURY))
            lines.addElem(WL(stack,
                  WL(margins, WL(progressBar, Color::GREEN.transparency(70), fabs(*luxury)), -2, 0, 0, 3),
                  WL(label, TSentence("LUXURY_HINT", getLuxuryNumber(*luxury)))));
          if (auto efficiency = viewObject.getAttribute(ViewObjectAttribute::EFFICIENCY))
            if (*efficiency >= 0.4) // Loading saves from before this change, non-relevant furniture would have this initialized to 0 so skip them
              lines.addElem(WL(stack,
                    WL(margins, WL(progressBar, Color::GREEN.transparency(70), fabs(*efficiency / 2)), -2, 0, 0, 3),
                    WL(label, TSentence("WORK_EFFICIENCY_MULTIPLIER_HINT", getLuxuryNumber(*efficiency)))));
          if (viewObject.hasModifier(ViewObjectModifier::UNPAID))
            lines.addElem(WL(label, TStringId("CANT_AFFORD_ITEM_HINT"), Color::RED));
          if (viewObject.hasModifier(ViewObjectModifier::DANGEROUS))
            lines.addElem(WL(label, TStringId("LOCATION_TOO_DANGEROUS_HINT"), Color::RED));
          if (viewObject.hasModifier(ViewObjectModifier::PLANNED))
            lines.addElem(WL(label, TStringId("PLANNED_HINT")));
          lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
        }
      for (auto highlight : ENUM_ALL(HighlightType))
        if (index->isHighlight(highlight))
          if (auto desc = getDescription(highlight)) {
            auto viewId = getViewId(highlight, true);
            auto line = WL(getListBuilder)
                .addElem(WL(viewObject, viewId, 1, viewId.getColor()), 30)
                .addElemAuto(WL(labelMultiLineWidth, *desc, legendLineHeight * 2 / 3, 300))
                .buildHorizontalList();
            int height = max(legendLineHeight, *line->getPreferredHeight() + 6);
            lines.addElem(line, height);
            lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
          }
      if (auto& quarters = mapGui->getQuartersInfo()) {
        lines.addElem(WL(label, TStringId("QUARTERS_LABEL")));
        if (quarters->viewId)
          lines.addElem(WL(getListBuilder)
              .addElemAuto(WL(viewObject, *quarters->viewId))
              .addElemAuto(WL(label, *quarters->name))
              .buildHorizontalList());
        else
          lines.addElem(WL(label, TStringId("QUARTERS_UNASSIGNED_LABEL")));
        lines.addElem(WL(label, TSentence("QUARTERS_LUXURY", getLuxuryNumber(quarters->luxury))));
        lines.addElem(WL(label, TStringId("CLICK_TO_ASSIGN_QUARTERS_LABEL")));
        lines.addElem(WL(margins, WL(rectangle, Color::DARK_GRAY), -9, 2, -9, 8), 12);
      }
      if (index->isHighlight(HighlightType::INDOORS))
        lines.addElem(WL(label, TStringId("INDOORS_LABEL")));
      else
        lines.addElem(WL(label, TStringId("OUTDOORS_LABEL")));
      lines.addElemAuto(drawLyingItemsList(TStringId("LYING_HERE_LABEL"), index->getItemCounts(), 250));
    }
    if (highlighted.tilePos)
      lines.addElem(WL(label, TSentence("GRID_POSITION", TString(toString(*highlighted.tilePos)))));
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
              .addElem(WL(labelMultiLineWidth, TStringId("RETIRED_SCREENSHOT_TEXT"),
                  legendLineHeight, width - 2 * margin), legendLineHeight * 4)
              .addElem(WL(centerHoriz, WL(getListBuilder)
                  .addElemAuto(WL(buttonLabel, TStringId("CONFIRM_BUTTON"), getButtonCallback({UserInputId::TAKE_SCREENSHOT, Vec2(width, height)})))
                  .addSpace(15)
                  .addElemAuto(WL(buttonLabel, TStringId("CANCEL_BUTTON"), getButtonCallback(UserInputId::CANCEL_SCREENSHOT)))
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
      if (!info.tutorial)
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
         info.encyclopedia->bestiary, bestiaryIndex, bestiarySearchString), OverlayInfo::TOP_LEFT});
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

static int getMessageLength(Renderer& renderer, GuiFactory* guiFactory, const PlayerMessage& msg) {
  return renderer.getTextLength(msg.getTranslatedText(guiFactory)) + (msg.isClickable() ? messageArrowLength : 0);
}

static int getNumFitting(Renderer& renderer, GuiFactory* guiFactory, const vector<PlayerMessage>& messages,
    int maxLength, int numLines) {
  int currentWidth = 0;
  int currentLine = 0;
  for (int i = messages.size() - 1; i >= 0; --i) {
    int length = getMessageLength(renderer, guiFactory, messages[i]);
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

static vector<vector<PlayerMessage>> fitMessages(Renderer& renderer, GuiFactory* guiFactory,
    const vector<PlayerMessage>& messages, int maxLength, int numLines) {
  int currentWidth = 0;
  vector<vector<PlayerMessage>> ret {{}};
  int numFitting = getNumFitting(renderer, guiFactory, messages, maxLength, numLines);
  for (int i : Range(messages.size() - numFitting, messages.size())) {
    int length = getMessageLength(renderer, guiFactory, messages[i]);
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
/*
static void cutToFit(Renderer& renderer, string& s, int maxLength) {
  while (!s.empty() && renderer.getTextLength(s) > maxLength)
    s.pop_back();
}
*/
SGuiElem GuiBuilder::drawMessages(const vector<PlayerMessage>& messageBuffer, int maxMessageLength) {
  int hMargin = 10;
  int vMargin = 5;
  vector<vector<PlayerMessage>> messages = fitMessages(renderer, &gui, messageBuffer, maxMessageLength - 2 * hMargin,
      getNumMessageLines());
  int lineHeight = 20;
  vector<SGuiElem> lines;
  for (int i : All(messages)) {
    auto line = WL(getListBuilder);
    for (auto& message : messages[i]) {
      if (!line.isEmpty())
        line.addSpace(6);
      auto text = message.getText();
//      cutToFit(renderer, text, maxMessageLength - 2 * hMargin);
      if (message.isClickable()) {
        line.addElemAuto(WL(stack,
              WL(button, getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
              WL(labelHighlight, text, getMessageColor(message))));
        line.addElemAuto(WL(labelUnicodeHighlight, TString(u8"➚"_s), getMessageColor(message)));
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
          WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::RED))), 1);
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

static TString getTaskText(MinionActivity option) {
  switch (option) {
    case MinionActivity::IDLE: return TStringId("IDLE_MINION_ACTIVITY");
    case MinionActivity::SLEEP: return TStringId("SLEEP_MINION_ACTIVITY");
    case MinionActivity::CONSTRUCTION: return TStringId("CONSTRUCTION_MINION_ACTIVITY");
    case MinionActivity::DIGGING: return TStringId("DIGGING_MINION_ACTIVITY");
    case MinionActivity::HAULING: return TStringId("HAULING_MINION_ACTIVITY");
    case MinionActivity::WORKING: return TStringId("WORKING_MINION_ACTIVITY");
    case MinionActivity::EAT: return TStringId("EAT_MINION_ACTIVITY");
    case MinionActivity::EXPLORE_NOCTURNAL: return TStringId("EXPLORE_NOCTURNAL_MINION_ACTIVITY");
    case MinionActivity::EXPLORE_CAVES: return TStringId("EXPLORE_CAVES_MINION_ACTIVITY");
    case MinionActivity::EXPLORE: return TStringId("EXPLORE_MINION_ACTIVITY");
    case MinionActivity::RITUAL: return TStringId("RITUAL_MINION_ACTIVITY");
    case MinionActivity::CROPS: return TStringId("CROPS_MINION_ACTIVITY");
    case MinionActivity::TRAIN: return TStringId("TRAIN_MINION_ACTIVITY");
    case MinionActivity::ARCHERY: return TStringId("ARCHERY_MINION_ACTIVITY");
    case MinionActivity::CRAFT: return TStringId("CRAFT_MINION_ACTIVITY");
    case MinionActivity::STUDY: return TStringId("STUDY_MINION_ACTIVITY");
    case MinionActivity::POETRY: return TStringId("POETRY_MINION_ACTIVITY");
    case MinionActivity::MINION_ABUSE: return TStringId("MINION_ABUSE_MINION_ACTIVITY");
    case MinionActivity::COPULATE: return TStringId("COPULATE_MINION_ACTIVITY");
    case MinionActivity::SPIDER: return TStringId("SPIDER_MINION_ACTIVITY");
    case MinionActivity::GUARDING1: return TStringId("GUARDING1_MINION_ACTIVITY");
    case MinionActivity::GUARDING2: return TStringId("GUARDING2_MINION_ACTIVITY");
    case MinionActivity::GUARDING3: return TStringId("GUARDING3_MINION_ACTIVITY");
    case MinionActivity::DISTILLATION: return TStringId("DISTILLATION_MINION_ACTIVITY");
    case MinionActivity::BE_WHIPPED: return TStringId("BE_WHIPPED_MINION_ACTIVITY");
    case MinionActivity::BE_TORTURED: return TStringId("BE_TORTURED_MINION_ACTIVITY");
    case MinionActivity::BE_EXECUTED: return TStringId("BE_EXECUTED_MINION_ACTIVITY");
    case MinionActivity::PHYLACTERY: return TStringId("PHYLACTERY_MINION_ACTIVITY");
    case MinionActivity::PREACHING: return TStringId("PREACHING_MINION_ACTIVITY");
    case MinionActivity::MASS: return TStringId("MASS_MINION_ACTIVITY");
    case MinionActivity::PRAYER: return TStringId("PRAYER_MINION_ACTIVITY");
    case MinionActivity::HEARING_CONFESSION: return TStringId("HEARING_CONFESSION_MINION_ACTIVITY");
    case MinionActivity::CONFESSION: return TStringId("CONFESSION_MINION_ACTIVITY");
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
    case MinionActivity::GUARDING1: return getViewId(getHighlight(ZoneId::GUARD1), true);
    case MinionActivity::GUARDING2: return getViewId(getHighlight(ZoneId::GUARD2), true);
    case MinionActivity::GUARDING3: return getViewId(getHighlight(ZoneId::GUARD3), true);
    case MinionActivity::DISTILLATION: return ViewId("distillery");
    case MinionActivity::BE_WHIPPED: return ViewId("whipping_post");
    case MinionActivity::BE_TORTURED: return ViewId("torture_table");
    case MinionActivity::BE_EXECUTED: return ViewId("gallows");
    case MinionActivity::PHYLACTERY: return ViewId("phylactery");
    case MinionActivity::PREACHING: return ViewId("rostrum_wood");
    case MinionActivity::MASS: return ViewId("pew_wood");
    case MinionActivity::PRAYER: return ViewId("prayer_bench_wood");
    case MinionActivity::HEARING_CONFESSION:
    case MinionActivity::CONFESSION: return ViewId("confessional");
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
          WL(centeredLabel, Renderer::HOR, TStringId("DONE_LABEL"), Color::LIGHT_BLUE)));
  return lines;
}

function<void(Rectangle)> GuiBuilder::getActivityButtonFun(const PlayerInfo& minion) {
  return [=] (Rectangle bounds) {
    auto tasks = WL(getListBuilder, legendLineHeight);
    auto glyph1 = WL(getListBuilder);
    if (hasController())
      glyph1.addElemAuto(WL(label, TStringId("SET_CURRENT_ACTIVITY_LABEL"), Renderer::smallTextSize()))
          .addElemAuto(WL(steamInputGlyph, C_BUILDINGS_CONFIRM));
    auto glyph2 = WL(steamInputGlyph, C_BUILDINGS_LEFT);
    auto glyph3 = WL(steamInputGlyph, C_BUILDINGS_RIGHT);
    tasks.addElem(WL(getListBuilder)
        .addElemAuto(glyph1.buildHorizontalList())
        .addBackElemAuto(WL(label, TStringId("ENABLE_LABEL"), Renderer::smallTextSize()))
        .addBackSpace(5)
        .addBackElem(glyph2, 35)
        .addBackSpace(10)
        .addBackElem(WL(getListBuilder)
            .addMiddleElem(WL(renderInBounds,
                WL(label, TSentence("DISABLE_FOR_ALL_GROUP", makePlural(minion.groupName)),
                    Renderer::smallTextSize())))
            .addBackElem(glyph3, hasController() ? 35 : 1)
            .buildHorizontalList(),
            234)
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
            }, {WL(empty), WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::RED),
                 WL(labelUnicodeHighlight, TString(u8"✓"_s), Color::GREEN)}));
      auto lockButton2 = task.canLock
            ? WL(rightMargin, 20, WL(conditional, WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::RED),
                 WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::LIGHT_GRAY), [&retAction, task] {
                      return retAction.lockGroup.contains(task.task) ^ task.lockedForGroup;}))
            : WL(empty);
      auto lockCallback1 = [&retAction, task] {
        retAction.lock.toggle(task.task);
      };
      auto lockCallback2 = [&retAction, task] {
        retAction.lockGroup.toggle(task.task);
      };
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
                  getTooltip({TStringId("ACTIVITY_ON_OFF_HINT")}, THIS_LINE + i,
                      milliseconds{700}, true),
                  WL(button, lockCallback1),
                  lockButton), 37)
              .addBackSpace(51)
              .addBackElemAuto(WL(stack,
                  getTooltip({TSentence("ACTIVITY_ON_OFF_HINT_FOR_GROUP", makePlural(minion.groupName))}, THIS_LINE + i + 54321,
                      milliseconds{700}, true),
                  WL(button, lockCallback2),
                  lockButton2))
              .addBackSpace(200)
              .buildHorizontalList()
      ));
      tasks.addElem(activeElems.back());
    }
    auto content = WL(stack,
        getMiniMenuScrolling(activeElems, selected),
        tasks.buildVerticalList()
    );
    drawMiniMenu(std::move(content), exit, bounds.bottomLeft(), 700, true);
    callbacks.input({UserInputId::CREATURE_TASK_ACTION, retAction});
  };
}

static TStringId getName(AIType t) {
  switch (t) {
    case AIType::MELEE: return TStringId("MELEE_AI_NAME");
    case AIType::RANGED: return TStringId("AVOID_MELEE_AI_NAME");
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
        .addElemAuto(WL(conditional, WL(labelUnicodeHighlight, TString(u8"✓"_s), Color::GREEN),
             WL(labelUnicodeHighlight, TString(u8"✓"_s), Color::LIGHT_GRAY), [&retAction] {
                  return retAction.override;}))
        .addElemAuto(WL(label, TSentence("COPY_SETTING_TO_GROUP", makePlural(minion.groupName))))
        .buildHorizontalList());
    drawMiniMenu(std::move(tasks), std::move(callbacks), {}, bounds.bottomLeft(), 420, true, false, nullptr, &exit);
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
    lines.push_back(WL(label, TSentence("SETTING_WILL_APPLY_TO_ALL", makePlural(minion.groupName)),
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
          .addBackElem(WL(conditional, WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::RED),
              WL(labelUnicodeHighlight, TString(u8"✓"_s), Color::GREEN),
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
        .addElemAuto(WL(labelUnicodeHighlight, TString(u8"✘"_s), Color::RED))
        .addElemAuto(WL(label, TStringId("RESTRICT_ALL_EQUIPMENT")))
        .buildHorizontalList();
    auto unrestrictButton = WL(getListBuilder)
        .addElemAuto(WL(labelUnicodeHighlight, TString(u8"✓"_s), Color::GREEN))
        .addElemAuto(WL(label, TStringId("UNRESTRICT_ALL_EQUIPMENT")))
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
        .addElemAuto(WL(label, TStringId("EQUIPMENT_LABEL"), Color::YELLOW));
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
          auto highlight = WL(leftMargin, offset, WL(labelUnicode, TString(u8"✘"_s), Color::RED));
          if (!items[i].locked)
            highlight = WL(stack, std::move(highlight), WL(leftMargin, -20, WL(viewObject, ViewId("key_highlight"))));
          labelElem = WL(stack, std::move(labelElem),
                      WL(mouseHighlight2, std::move(highlight), nullptr, false));
          keyElem = WL(stack,
              WL(button, getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
                  EquipmentActionInfo{minion.creatureId, items[i].ids, items[i].slot, ItemAction::LOCK}})),
              items[i].locked ? WL(viewObject, ViewId("key")) : WL(mouseHighlight2, WL(viewObject, ViewId("key_highlight"))),
              getTooltip({TStringId("LOCKED_ITEMS_HINT")}, THIS_LINE + i)
          );
        }
        if (items[i].type == items[i].CONSUMABLE && (i == 0 || items[i].type != items[i - 1].type))
          lines.addElem(WL(label, TStringId("CONSUMABLES_LABEL"), Color::YELLOW));
        if (items[i].type == items[i].OTHER && (i == 0 || items[i].type != items[i - 1].type))
          lines.addElem(WL(label, TStringId("OTHER_EQUIPMENT_LABEL"), Color::YELLOW));
        lines.addElem(WL(leftMargin, 3,
            WL(getListBuilder)
                .addElem(std::move(keyElem), 20)
                .addMiddleElem(std::move(labelElem))
                .buildHorizontalList()));
        if (i == items.size() - 1 || (items[i + 1].type == items[i].OTHER && (items[i].type != items[i + 1].type)))
          lines.addElem(WL(buttonLabelFocusable, TStringId("ADD_CONSUMABLE"),
              getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
                  EquipmentActionInfo{minion.creatureId, {}, none, ItemAction::REPLACE}}),
              [this, ind = items.size()] { return minionPageIndex == MinionPageElems::Equipment{ind}; }));
      }
    else
      for (int i : All(items))
        lines.addElem(getItemLine(items[i], [](Rectangle) {}, nullptr, false, false));
  }
  if (!minion.intrinsicAttacks.empty()) {
    lines.addElem(WL(label, TStringId("INTRINSIC_ATTACKS_LABEL"), Color::YELLOW));
    for (auto& item : minion.intrinsicAttacks)
      lines.addElem(getItemLine(item, [=](Rectangle) {}));
  }
  return lines.buildVerticalList();
}

SGuiElem GuiBuilder::drawMinionActions(const PlayerInfo& minion, const optional<TutorialInfo>& tutorial) {
  const int buttonWidth = 110;
  const int buttonSpacing = 15;
  auto line = WL(getListBuilder, buttonWidth);
  auto lineExtra = WL(getListBuilder, buttonWidth);
  const bool tutorialHighlight = tutorial && tutorial->highlights.contains(TutorialHighlight::CONTROL_TEAM);
  for (auto action : Iter(minion.actions)) {
    auto focusCallback = [this, action]{ return minionPageIndex == MinionPageElems::MinionAction{action.index()};};
    auto input = UserInput{UserInputId::MINION_ACTION, MinionActionInfo{minion.creatureId, *action}};
    auto& whichLine = action.index() <= 4 ? line : lineExtra;
    switch (*action) {
      case PlayerInfo::Action::CONTROL: {
        auto callback = getButtonCallback(input);
        whichLine.addElem(tutorialHighlight
            ? WL(buttonLabelBlink, TStringId("CONTROL_MINION_BUTTON"), callback, focusCallback, false, true)
            : WL(buttonLabelFocusable, TStringId("CONTROL_MINION_BUTTON"), callback, focusCallback, false, true));
        break;
      }
      case PlayerInfo::Action::RENAME:
        whichLine.addElem(WL(buttonLabelFocusable, TStringId("RENAME_MINION_BUTTON"), getButtonCallback(input), focusCallback, false, true));
        break;
      case PlayerInfo::Action::BANISH:
        whichLine.addElem(WL(buttonLabelFocusable, TStringId("BANISH_MINION_BUTTON"), getButtonCallback(input), focusCallback, false, true));
        break;
      case PlayerInfo::Action::DISASSEMBLE:
        whichLine.addElem(WL(buttonLabelFocusable, TStringId("DISASSEMBLE_MINION_BUTTON"), getButtonCallback(input), focusCallback, false, true));
        break;
      case PlayerInfo::Action::CONSUME:
        whichLine.addElem(WL(buttonLabelFocusable, TStringId("ABSORB_MINION_BUTTON"), getButtonCallback(input), focusCallback, false, true));
        break;
      case PlayerInfo::Action::LOCATE:
        whichLine.addElem(WL(buttonLabelFocusable, TStringId("LOCATE_MINION_BUTTON"), getButtonCallback(input), focusCallback, false, true));
        break;
      case PlayerInfo::Action::ASSIGN_EQUIPMENT:
        whichLine.addElem(WL(buttonLabelFocusable, TStringId("ASSIGN_GEAR_MINION_BUTTON"), getButtonCallback(input), focusCallback, false, true));
        break;
      case PlayerInfo::Action::LOCK_POSITION:
          whichLine.addElem(WL(stack,
              WL(buttonLabelFocusable, TStringId("LOCK_POSITION_BUTTON"), getButtonCallback(input), focusCallback, false, true),
              getTooltip({TStringId("LOCK_POSITION_TOOLTIP")}, THIS_LINE)
          ));
          break;
    }
    whichLine.addSpace(buttonSpacing);
  }
  int numSettingButtons = 0;
  auto getNextFocusPredicate = [&]() -> function<bool()> {
    ++numSettingButtons;
    return [this, numSettingButtons] { return minionPageIndex == MinionPageElems::Setting{numSettingButtons - 1}; };
  };
  auto line2 = WL(getListBuilder, buttonWidth);
  line2.addElem(WL(buttonLabelFocusable,
      WL(centerHoriz, WL(getListBuilder)
          .addElemAuto(WL(label, TStringId("AI_TYPE_LABEL")))
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
          .addElemAuto(WL(label, TStringId("ACTIVITY_LABEL")))
          .addElemAuto(WL(viewObject, curTask))
          .buildHorizontalList()),
      getActivityButtonFun(minion), getNextFocusPredicate(), false, true));
  line2.addSpace(buttonSpacing);
  if (!minion.equipmentGroups.empty())
    line2.addElem(WL(buttonLabelFocusable, TStringId("RESTRICT_GEAR_BUTTON"),
        getEquipmentGroupsFun(minion), getNextFocusPredicate(), false, true));
  if (!lineExtra.isEmpty()) {
    line2.addBackElemAuto(lineExtra.buildHorizontalList());
    line2.addBackSpace(buttonSpacing);
  }
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
        WL(label, TSentence("KILLS_LABEL", TString(minion.kills.size()))),
        WL(tooltip2, WL(miniWindow, WL(margins, lines.buildVerticalList(), 15)), [](Rectangle rect){ return rect.bottomLeft(); })
    );
  } else
    return nullptr;
}

SGuiElem GuiBuilder::drawTitleButton(const PlayerInfo& minion) {
  auto titleLine = WL(getListBuilder);
  titleLine.addElemAuto(WL(label, capitalFirst(minion.title)));
  auto lines = WL(getListBuilder, legendLineHeight);
  for (auto& title : minion.killTitles)
    lines.addElem(WL(label, capitalFirst(title)));
  lines.addElem(WL(labelMultiLineWidth,
      TStringId("KILL_TITLES_HINT"), legendLineHeight * 2 / 3, 400, Renderer::smallTextSize(), Color::LIGHT_GRAY));
  if (!minion.killTitles.empty())
    titleLine.addElemAuto(WL(label, TString("+"_s), Color::YELLOW));
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
      .addBackElemAuto(WL(label, TSentence("SPELL_LEVEL", TString(*spell.level)), color))
      .buildHorizontalList();
}

SGuiElem GuiBuilder::drawSpellSchoolLabel(const SpellSchoolInfo& school) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, TSentence("EXPERIENCE_TYPE", school.experienceType)));
  for (auto& spell : school.spells) {
    lines.addElem(drawSpellLabel(spell));
  }
  return WL(stack,
      WL(label, school.name),
      WL(tooltip2, WL(setWidth, 470, WL(miniWindow, WL(margins, lines.buildVerticalList(), 15))),
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
  leftLines.addElem(WL(label, TStringId("ATTRIBUTES_LABEL"), Color::YELLOW));
  leftLines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(minion.attributes)));
  for (auto& elem : drawEffectsList(minion))
    leftLines.addElem(std::move(elem));
  leftLines.addSpace();
  if (auto elem = drawTrainingInfo(minion.experienceInfo))
    leftLines.addElemAuto(std::move(elem));
  if (!minion.spellSchools.empty()) {
    auto line = WL(getListBuilder)
        .addElemAuto(WL(label, TStringId("SPELL_SCHOOLS_LABEL"), Color::YELLOW))
        .addSpace(8);
    for (int i : All(minion.spellSchools)) {
      if (i > 0)
        line.addElemAuto(WL(label, TString(", "_s)));
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

SGuiElem GuiBuilder::drawTickBox(shared_ptr<bool> value, const TString& title) {
  return WL(stack,
      WL(button, [value]{ *value = !*value; }),
      WL(getListBuilder)
          .addElemAuto(
              WL(conditional, WL(labelUnicodeHighlight, TString(u8"✓"_s), Color::GREEN), [value] { return *value; }))
          .addElemAuto(WL(label, title))
          .buildHorizontalList());
}

SGuiElem GuiBuilder::drawBugreportMenu(bool saveFile, function<void(optional<BugReportInfo>)> callback) {
  auto lines = WL(getListBuilder, legendLineHeight);
  const int width = 300;
  const int windowMargin = 15;
  lines.addElem(WL(centerHoriz, WL(label, TStringId("BUG_REPORT_BUTTON"))));
  auto text = make_shared<string>();
  auto withScreenshot = make_shared<bool>(true);
  auto withSavefile = make_shared<bool>(saveFile);
  lines.addElem(WL(label, TStringId("ENTER_BUG_DESCRIPTION")));
  lines.addElemAuto(WL(stack,
        WL(rectangle, Color::GRAY),
        WL(margins, WL(textInput, width - 10, 5, text), 5)));
  lines.addSpace();
  lines.addElem(drawTickBox(withScreenshot, TStringId("INCLUDE_SCREENSHOT")));
  if (saveFile)
    lines.addElem(drawTickBox(withSavefile, TStringId("INCLUDE_SAVE_FILE")));
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(buttonLabel, TStringId("UPLOAD_BUTTON"),
          [=] { callback(BugReportInfo{*text, *withSavefile, *withScreenshot});}))
      .addSpace(10)
      .addElemAuto(WL(buttonLabel, TStringId("CANCEL_BUTTON"), [callback] { callback(none);}))
      .buildHorizontalList()
  ));
  return WL(setWidth, width + 2 * windowMargin,
      WL(miniWindow, WL(margins, lines.buildVerticalList(), windowMargin), [callback]{ callback(none); }));
}

static optional<Color> getHighlightColor(VillainType type) {
  switch (type) {
    case VillainType::RETIRED:
      return Color::PURPLE;
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
      iconSize * (max(worldMapBounds.left(), min(worldMapBounds.right(), pos.x))  - maxSize.x / 2),
      iconSize * (max(worldMapBounds.top(), min(worldMapBounds.bottom(), pos.y) - maxSize.y / 2))};
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
          if (startsWith(id.data(), "map_mountains_large"))
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
      if (c.isInInfluence(Vec2(x, y))) {
        if (auto id = sites[x][y].getDwellingViewId())
          if (!c.isDefeated(Vec2(x, y))) {
            elem.push_back(WL(asciiBackground, id->front()));
            elem.push_back(WL(viewObject, *id, iconScale));
            labelPlacer.setOccupied(pos);
          }
        if (auto desc = sites[x][y].getDwellerDescription()) {
          auto minions = WL(getListBuilder);
          for (auto& m : sites[x][y].inhabitants)
            minions.addElemAuto(drawMinionAndLevel(m.viewId, m.level, 1));
          auto lines = WL(getListBuilder, legendLineHeight).addElem(WL(label, *desc));
          if (c.isDefeated(pos))
            lines.addElem(WL(label, TStringId("CAMPAIGN_VILLAIN_DEFEATED")));
          auto exp = c.getBaseLevelIncrease(Vec2(x, y));
          if (exp > 0)
            lines.addElem(WL(label, TSentence("CAMPAIGN_VILLAIN_EXP", TString(exp))));
          if (!minions.isEmpty() && !sites[x][y].getDwellingViewId()->contains(ViewId("map_unknown")))
            lines.addElem(minions.buildHorizontalList());
          elem.push_back(WL(margins, WL(tooltip2, WL(miniWindow, WL(margins,
              lines.buildVerticalList(), 15)), [](auto& r) { return r.bottomLeft(); }), -4));
        }
      }
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
        if (!!sites[x][y].getVillainType() && !c.isDefeated(Vec2(x, y)))
          if (auto color = getHighlightColor(*sites[pos].getVillainType()))
            elem.push_back(translateHighlight(WL(viewObject, ViewId("map_highlight"), iconScale, *color)));
        if (campaignGridPointer)
          elem.push_back(WL(conditional, translateHighlight(WL(viewObject, ViewId("map_highlight"), iconScale)),
                [this, pos] { return campaignGridPointer == pos;}));
        if (campaignGridPointer && !!selectable && selectable(pos)) {
          auto marginSize = [&] {
            for (auto v : pos.neighbors8())
              if (v.inRectangle(sites.getBounds()) && selectable(v))
                return 0;
            return 4;
          }();
          elem.push_back(WL(margins, WL(stack,
              WL(button, [this, pos, selectCallback] {
                if (selectCallback)
                  selectCallback(pos);
                campaignGridPointer = pos;
              }),
              WL(mouseHighlight2, WL(margins,
                  translateHighlight(WL(viewObject, ViewId("map_highlight"), iconScale)), marginSize), nullptr, false)),
                  -marginSize
          ));
        }
        if (!!sites[x][y].getVillainType() && c.isDefeated(pos))
          elem.push_back(WL(viewObject, ViewId("campaign_defeated"), iconScale));
        if (auto desc = sites[x][y].getDwellerName())
          if (!c.isDefeated(pos) && getHighlightColor(*sites[pos].getVillainType())) {
            auto width = renderer.getTextLength(gui.translate(*desc), 12, FontId::MAP_FONT);
            elem.push_back(WL(translate,
                WL(labelUnicode, *desc, Color::WHITE, 12, FontId::MAP_FONT),
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
            WL(renderInBounds, WL(translate,
                [this, iconSize, minimapScale] {
                  return Vec2(scrollAreaScrollPos.first, scrollAreaScrollPos.second) * minimapScale / iconSize;
                },
                WL(alignment, GuiFactory::Alignment::TOP_LEFT, WL(rectangle, Color::TRANSPARENT, Color::WHITE),
                    maxWorldMapSize * minimapScale / iconSize))))));
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

SGuiElem GuiBuilder::drawWorldmap(Semaphore& sem, const Campaign& campaign, Vec2 current) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  campaignGridPointer = none;
  lines.addElem(WL(centerHoriz, WL(label, TSentence("WORLD_MAP_OF", TString(campaign.getWorldName())))));
  lines.addElem(WL(centerHoriz, WL(label, TStringId("TRAVEL_COMMAND_TIP"), Renderer::smallTextSize(), Color::LIGHT_GRAY)));
  lines.addElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, current, nullptr, nullptr)));
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(WL(centerHoriz, WL(buttonLabel, TStringId("CLOSE_BUTTON"), [&] { sem.v(); })));
  return WL(preferredSize, 1000, 750,
      WL(window, WL(margins, lines.buildVerticalList(), 15), [&sem] { sem.v(); }));
}

SGuiElem GuiBuilder::drawChooseSiteMenu(SyncQueue<optional<Vec2>>& queue, const TString& message,
    const Campaign& campaign, Vec2 initialPos) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, message)));
  campaignGridPointer = initialPos;
  lines.addElemAuto(WL(centerHoriz, drawCampaignGrid(campaign, initialPos,
      [&campaign](Vec2 pos){ return campaign.canTravelTo(pos); }, nullptr)));
  lines.addSpace(legendLineHeight / 2);
  auto confirmCallback = [&] {
    if (campaign.canTravelTo(*campaignGridPointer))
      queue.push(*campaignGridPointer);
  };
  lines.addElem(WL(centerHoriz, WL(getListBuilder)
      .addElemAuto(WL(conditional, WL(stack,
              WL(buttonLabel, TStringId("CONFIRM_BUTTON"), confirmCallback),
              WL(keyHandler, confirmCallback, Keybinding("MENU_SELECT"), true)
          ),
          WL(buttonLabelInactive, TStringId("CONFIRM_BUTTON")),
          [&] { return !!campaignGridPointer && campaign.isInInfluence(*campaignGridPointer); }))
      .addSpace(15)
      .addElemAuto(WL(buttonLabel, TStringId("CANCEL_BUTTON"), WL(stack,
          WL(button, [&queue] { queue.push(none); }, true),
          WL(keyHandler, [&] { queue.push(none); }, Keybinding("EXIT_MENU"), true)
      )))
      .buildHorizontalList()));
  return WL(preferredSize, 1000, 750,
      WL(window, WL(margins, lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease, bool leftRight) {
  auto plus = TString(leftRight ? ">"_s  : "+"_s);
  auto minus = TString(leftRight ? "<"_s  : "-"_s);
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
      WL(labelUnicode, TString(u8"✓"_s), Color::GREEN),
      WL(labelUnicode, TString(u8"✘"_s), Color::RED),
      [this, id]{ return options->getBoolValue(id); })
  );
  line.addElemAuto(WL(label, TString(name)));
  return WL(stack,
      WL(button, [this, id] { options->setValue(id, int(!options->getBoolValue(id))); }),
      line.buildHorizontalList()
  );
}

SGuiElem GuiBuilder::drawOptionElem(OptionId id, function<void()> onChanged, function<bool()> focused) {
  auto name = options->getName(id);
  SGuiElem ret;
  auto limits = options->getLimits(id);
  int value = options->getIntValue(id);
  auto changeFun = [=] (int v) { options->setValue(id, value + v); onChanged();};
  ret = WL(getListBuilder)
      .addElem(WL(labelFun, [=]{ return TSentence("OPTION_NAME_AND_VALUE", name, TString(options->getValueString(id))); }), renderer.getTextLength(gui.translate(name)) + 20)
      .addBackElemAuto(drawPlusMinus(changeFun,
          value < limits->getEnd() - 1, value > limits->getStart(), options->hasChoices(id)))
      .buildHorizontalList();
  return WL(stack,
      WL(tooltip, {options->getHint(id).value_or(TString())}),
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
      lines.addElem(WL(label, allGames[i].subscribed ?
          TStringId("SUBSCRIBED_DUNGEONS") : TStringId("ONLINE_DUNGEONS"), Color::YELLOW));
    else
      if (i > 0 && !allGames[i].subscribed && allGames[i - 1].subscribed && !displayActive)
        lines.addElem(WL(label, TStringId("ONLINE_DUNGEONS"), Color::YELLOW));
    if (searchString != "" && !contains(toLower(allGames[i].gameInfo.name), toLower(searchString)))
      continue;
    if (retired.isActive(i) == displayActive) {
      auto header = WL(getListBuilder);
      bool maxedOut = !displayActive && retired.getNumActive() >= *maxActive;
      header.addElem(WL(label, TString(allGames[i].gameInfo.name),
          maxedOut ? Color::LIGHT_GRAY : Color::WHITE), 180);
      for (auto& minion : allGames[i].gameInfo.minions)
        header.addElem(drawMinionAndLevel(minion.viewId, minion.level, 1), 25);
      header.addSpace(20);
      if (retired.isActive(i))
        header.addBackElemAuto(WL(buttonLabelFocusable, TStringId("REMOVE_DUNGEON"),
            [i, reloadCampaign, &retired] { retired.setActive(i, false); reloadCampaign();},
            [isFocused, numAdded = added.size()]{ return isFocused(numAdded);}));
      else if (!maxedOut)
        header.addBackElemAuto(WL(buttonLabelFocusable, TStringId("ADD_DUNGEON"),
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
          WL(tooltip, {TStringId("DUNGEON_CONQUER_RATE_TIP")}),
          WL(label, TSentence("DUNGEON_CONQUER_RATE", TString(allGames[i].numWon), TString(allGames[i].numTotal)),
              Renderer::smallTextSize(), gui.inactiveText)));
      if (allGames[i].isFriend) {
        detailsList.addBackElemAuto(WL(label, TSentence("MOD_MADE_BY_STEAM_FRIEND", TString(allGames[i].author)),
            Renderer::smallTextSize(), Color::PINK));
        detailsList.addBackSpace(10);
      }
      if (!detailsList.isEmpty())
        lines.addElem(detailsList.buildHorizontalList(), legendLineHeight * 2 / 3);
      if (!allGames[i].gameInfo.spriteMods.empty()) {
        auto modsList = combine(allGames[i].gameInfo.spriteMods, true);
        lines.addElem(WL(stack,
            WL(tooltip, {TStringId("DUNGEON_MODS_TIP"), TString(modsList)}),
            WL(renderInBounds, WL(label, TSentence("DUNGEON_REQUIRED_MODS", TString(modsList)), Renderer::smallTextSize(), gui.inactiveText))),
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
          retiredMenuLines.addElem(WL(label, TStringId("ADDED_DUNGEONS"), Color::YELLOW));
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
          retiredList.first.addElem(WL(label, TStringId("NO_RETIRED_DUNGEONS")));
        else
          retiredMenuLines.addElem(WL(label, TStringId("LOCAL_DUNGEONS"), Color::YELLOW));
        retiredMenuLines.addElemAuto(retiredList.first.buildVerticalList());
        focused = min(focused, dungeonElems.size() - 1);
        auto content = WL(stack, makeVec(
            retiredMenuLines.buildVerticalList(),
            WL(keyHandler, [&] {
              if (focused == -1)
                focused = 0;
              focused = (focused + 1) % dungeonElems.size();
              miniMenuScroll.setRelative(dungeonElems[focused]->getBounds().top(), Clock::getRealMillis());
            }, Keybinding("MENU_DOWN"), SoundId("MENU_TRAVEL")),
            WL(keyHandler, [&] {
              if (focused == -1)
                focused = 0;
              focused = (focused + dungeonElems.size() - 1) % dungeonElems.size();
              miniMenuScroll.setRelative(dungeonElems[focused]->getBounds().top(), Clock::getRealMillis());
            }, Keybinding("MENU_UP"), SoundId("MENU_TRAVEL")),
            WL(keyHandler, [&focused] { if (focused == 0) focused = -1; },
                Keybinding("MENU_RIGHT"), SoundId("MENU_TRAVEL")),
            WL(keyHandler, [&focused] { if (focused == -1) focused = 0; },
                Keybinding("MENU_LEFT"), SoundId("MENU_TRAVEL")),
            WL(setHeight, getStandardLineHeight(), WL(getListBuilder)
                .addElemAuto(WL(label, TStringId("SEARCH_DUNGEONS")))
                .addElem(searchField, 200)
                .addSpace(10)
                .addElemAuto(WL(buttonLabelFocusable, TString("X"_s),
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
    return WL(buttonLabelFocusable, TStringId("ADD_RETIRED_DUNGEONS"), selectFun, focusedFun);
  }
  return WL(buttonLabelInactive, TStringId("ADD_RETIRED_DUNGEONS"));
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
            Keybinding("MENU_DOWN"), SoundId("MENU_TRAVEL")),
        WL(keyHandler, [&focused, cnt] { focused = (focused + cnt - 1) % cnt; },
            Keybinding("MENU_UP"), SoundId("MENU_TRAVEL"))
      );
      drawMiniMenu(std::move(content), exit, rect.bottomLeft(), 520, true);
      if (!clicked)
        break;
    }
  };
  auto focusedFun = [&state] { return state.index == CampaignMenuElems::Settings{};};
  return WL(buttonLabelFocusable, TStringId("CAMPAIGN_DIFFICULTY"), callback, focusedFun);
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
          Keybinding("MENU_DOWN"), SoundId("MENU_TRAVEL")),
      WL(keyHandler, [&focused, cnt] { focused = (focused + cnt - 1) % cnt; },
          Keybinding("MENU_UP"), SoundId("MENU_TRAVEL"))
    );
    drawMiniMenu(std::move(content), exit, bounds.bottomLeft(), 350, true);
  };
  auto changeFocusedFun = [&menuState] {return menuState.index == CampaignMenuElems::ChangeMode{};};
  return WL(buttonLabelFocusable, TStringId("CHANGE_CAMPAIGN_MODE"), changeFun, changeFocusedFun);
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
       WL(label, TSentence("WORLD_MAP_STYLE", campaignOptions.worldMapNames[campaignOptions.currentWorldMap]))));
  rightLines.addElem(WL(leftMargin, -55, drawGameModeButton(queue, campaignOptions, menuState)));
  centerLines.addSpace(10);
  auto helpFocusedFun = [&menuState]{return menuState.index == CampaignMenuElems::Help{};};
  auto helpFun = [&] { menuState.helpText = !menuState.helpText; };
  centerLines.addElem(WL(centerHoriz,
      WL(buttonLabelFocusable, TStringId("CAMPAIGN_HELP_BUTTON"), helpFun, helpFocusedFun)));
  lines.addElem(WL(leftMargin, optionMargin, WL(label, TSentence("CAMPAIGN_WORLD_NAME", TString(campaign.getWorldName())))));
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
        .addElemAuto(WL(buttonLabelFocusable, TStringId("CONFIRM_BUTTON"), [&] { queue.push(CampaignActionId::CONFIRM); },
            [&menuState]{return menuState.index == CampaignMenuElems::Confirm{};}))
        .addSpace(20)
        .addElemAuto(WL(buttonLabelFocusable, TStringId("REROLL_MAP_BUTTON"), [&] { queue.push(CampaignActionId::REROLL_MAP); },
            [&menuState]{return menuState.index == CampaignMenuElems::RollMap{};}))
        .addSpace(20)
        .addElemAuto(WL(buttonLabelFocusable, TStringId("GO_BACK_BUTTON"), [&] { queue.push(CampaignActionId::CANCEL); },
            [&menuState]{return menuState.index == CampaignMenuElems::Back{};}))
        .buildHorizontalList()));
  rightLines.addElem(WL(setWidth, 220, WL(label, TSentence("HOME_MAP_BIOME", campaignOptions.currentBiome))));
  int retiredPosX = 640;
  vector<SGuiElem> interior {
      WL(stopKeyEvents),
      WL(keyHandler, [&menuState] {
        menuState.index.left();
      }, Keybinding("MENU_LEFT"), SoundId("MENU_TRAVEL")),
      WL(keyHandler, [&menuState]{
        menuState.index.right();
      }, Keybinding("MENU_RIGHT"), SoundId("MENU_TRAVEL")),
      WL(keyHandler, [&menuState] {
        menuState.index.up();
      }, Keybinding("MENU_UP"), SoundId("MENU_TRAVEL")),
      WL(keyHandler, [&menuState]{
        menuState.index.down();
      }, Keybinding("MENU_DOWN"), SoundId("MENU_TRAVEL")),
      WL(keyHandler, [&] { queue.push(CampaignActionId::CANCEL); }, Keybinding("EXIT_MENU"), SoundId("MENU_TRAVEL")),
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

SGuiElem GuiBuilder::drawCreatureTooltip(const PlayerInfo& info) {
  auto lines = WL(getListBuilder, legendLineHeight);
  lines.addElem(WL(label, info.title));
  if (!info.description.empty())
    lines.addElem(WL(label, info.description, Color::RED));
  lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(info.attributes)));
  for (auto& elem : drawEffectsList(info, false))
    lines.addElem(std::move(elem));
  ItemCounts counts;
  for (auto& item : info.inventory)
    counts[item.viewId[0]] += item.number;
  lines.addElemAuto(drawLyingItemsList(TStringId("INVENTORY_LABEL"), counts, 200));
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
          WL(label, TString((int) elem.bestAttack.value), 10 * zoom)), 5, 5, 5, 5));
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
  const auto lastRow = (creatures.size() - 1) % numColumns + 1;
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

SGuiElem GuiBuilder::drawCreatureInfo(SyncQueue<bool>& queue, const TString& title, bool prompt,
    const vector<PlayerInfo>& creatures) {
  auto lines = WL(getListBuilder, getStandardLineHeight());
  lines.addElem(WL(centerHoriz, WL(label, title)));
  const int windowWidth = 590;
  lines.addMiddleElem(WL(scrollable, WL(margins, drawCreatureList(creatures, nullptr), 10)));
  lines.addSpace(15);
  auto bottomLine = WL(getListBuilder)
      .addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, TStringId("CONFIRM_BUTTON")), &gui, C_BUILDINGS_CONFIRM, none),
          WL(button, [&queue] { queue.push(true);})));
  if (prompt) {
    bottomLine.addSpace(20);
    bottomLine.addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, TStringId("CANCEL_BUTTON")), &gui, C_BUILDINGS_CANCEL, none),
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

SGuiElem GuiBuilder::drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>& queue, const TString& title,
      const vector<PlayerInfo>& team, const TString& cancelText) {
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
    auto getButton = [&](bool enabled, TString label, UserInput inputId, int margin) {
      auto ret = WL(preferredSize, legendLineHeight, legendLineHeight, WL(stack,
          WL(margins, WL(rectangle, Color(56, 36, 0), Color(57, 41, 0)), 2),
          WL(centerHoriz, WL(topMargin, margin, WL(mouseHighlight2,
              WL(labelUnicode, label, enabled ? Color::YELLOW : Color::DARK_GRAY, 22),
              WL(labelUnicode, label, enabled ? textColor : Color::DARK_GRAY, 22))))
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
          getButton(info->levelDepth > 0, TString(u8"⮝"_s), UserInput{UserInputId::SCROLL_STAIRS, -1 }, 3),
          WL(keyHandler, getButtonCallback(UserInput{UserInputId::SCROLL_STAIRS, -1}), Keybinding("SCROLL_Z_UP"))
      ));
    line.addMiddleElem(WL(topMargin, 3, drawZLevelButton(*info, textColor)));
    if (!info->zLevels.empty())
      line.addBackElemAuto(WL(stack,
          getButton(info->levelDepth < info->zLevels.size() - 1, TString(u8"⮟"_s), UserInput{UserInputId::SCROLL_STAIRS, 1}, 1),
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
          getHintCallback({TStringId("OPEN_WORLD_MAP_HINT")}),
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
            getHintCallback({TStringId("SCROLL_TO_KEEPER_HINT")}),
            WL(mouseHighlight2, WL(icon, GuiFactory::IconId::MINIMAP_CENTER2),
               WL(icon, GuiFactory::IconId::MINIMAP_CENTER1)),
            WL(conditional, WL(blink, WL(icon, GuiFactory::IconId::MINIMAP_CENTER2)), tutorialPredicate),
            WL(button, getButtonCallback(UserInputId::SCROLL_TO_HOME)),
            WL(keyHandler, getButtonCallback(UserInputId::SCROLL_TO_HOME), Keybinding("SCROLL_TO_PC"))
        )))
  ))).buildVerticalList();
}

SGuiElem GuiBuilder::drawTextInput(SyncQueue<optional<string>>& queue, const TString& title, const string& value,
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
          gui.getKeybindingMap()->getGlyph(WL(label, TStringId("CONFIRM_BUTTON")), &gui, Keybinding("MENU_SELECT")),
          WL(button, [&queue, text] { queue.push(*text); })))
      .addSpace(15)
      .addElemAuto(WL(standardButton,
          gui.getKeybindingMap()->getGlyph(WL(label, TStringId("CANCEL_BUTTON")), &gui, Keybinding("EXIT_MENU")),
          WL(button, [&queue] { queue.push(none); })))
      .buildHorizontalList()));
  return WL(stack,
      WL(window, WL(setWidth, 600, WL(margins, lines.buildVerticalList(), 50, 0, 50, 0)),
          [&queue] { queue.push(none); }),
      WL(keyHandler, [&queue] { queue.push(none); }, Keybinding("EXIT_MENU"), true),
      WL(keyHandler, [&queue, text] { queue.push(*text); }, Keybinding("MENU_SELECT"), true)
  );
}
