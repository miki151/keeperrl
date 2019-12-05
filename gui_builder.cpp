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
#include "team_order.h"
#include "lasting_effect.h"
#include "skill.h"
#include "player_role.h"
#include "tribe_alignment.h"
#include "avatar_menu_option.h"
#include "view_object_action.h"
#include "mod_info.h"
#include "special_trait.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

#define THIS_LINE __LINE__

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
  callbacks.input({UserInputId::LIBRARY_CLOSE});
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
      gui.buttonChar(std::move(buttonFun), !button.hotkeyOpensGroup ? button.hotkey : 0, true, true),
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
  int hash = combineHash(info.workshopButtons);
  if (hash != technologyHash) {
    technologyHash = hash;
    auto lines = gui.getListBuilder(legendLineHeight);
    lines.addSpace(legendLineHeight / 2);
    lines.addElem(gui.stack(
        gui.getListBuilder()
            .addElem(gui.viewObject(ViewId("book")), 35)
            .addElemAuto(gui.label("Keeperopedia", Color::WHITE))
            .buildHorizontalList(),
        gui.button(getButtonCallback(UserInputId::KEEPEROPEDIA))
    ));
    lines.addSpace(legendLineHeight / 2);
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
  auto topLine = gui.getListBuilder(resourceSpace);
  for (int i : All(info.numResource)) {
    auto res = gui.getListBuilder();
    res.addElem(gui.viewObject(info.numResource[i].viewId), 30);
    res.addElem(gui.labelFun([&info, i] { return toString<int>(info.numResource[i].count); },
          [&info, i] { return info.numResource[i].count >= 0 ? Color::WHITE : Color::RED; }), 50);
    auto tutorialHighlight = info.numResource[i].tutorialHighlight;
    auto tutorialElem = gui.conditional(gui.tutorialHighlight(), [tutorialHighlight, &gameInfo] {
        return gameInfo.tutorial && tutorialHighlight && gameInfo.tutorial->highlights.contains(*tutorialHighlight);
    });
    topLine.addElem(gui.stack(
        tutorialElem,
        getHintCallback({info.numResource[i].name}),
        res.buildHorizontalList()));
  }
  auto bottomLine = gui.getListBuilder();
  const int space = 55;
  bottomLine.addElemAuto(gui.standardButton(gui.stack(
      gui.margins(gui.progressBar(Color::DARK_GREEN.transparency(128), info.avatarLevelInfo.progress), -3, -1, 0, 7),
      gui.margins(gui.stack(
          gameInfo.tutorial && gameInfo.tutorial->highlights.contains(TutorialHighlight::RESEARCH) ?
              gui.tutorialHighlight() : gui.empty(),
          gui.uiHighlightConditional([&]{ return info.avatarLevelInfo.numAvailable > 0; })),
          3, -2, -3, 2),
      gui.getListBuilder()
          //.addSpace(10)
          .addElemAuto(gui.topMargin(-2, gui.viewObject(info.avatarLevelInfo.viewId)))
          .addElemAuto(gui.label("Level: " + toString(info.avatarLevelInfo.level)))
          .buildHorizontalList()),
      gui.button([this]() { closeOverlayWindowsAndClearButton(); callbacks.input(UserInputId::TECHNOLOGY);})
  ));
  bottomLine.addSpace(space);
  bottomLine.addElem(gui.labelFun([&info] {
        return "population: " + toString(info.minionCount) + " / " +
        toString(info.minionLimit); }), 150);
  bottomLine.addSpace(space);
  bottomLine.addElem(getTurnInfoGui(gameInfo.time), 50);
  bottomLine.addSpace(space);
  bottomLine.addElem(getSunlightInfoGui(sunlightInfo), 80);
  return gui.getListBuilder(legendLineHeight)
        .addElem(gui.centerHoriz(topLine.buildHorizontalList()))
        .addElem(gui.centerHoriz(bottomLine.buildHorizontalList()))
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
  auto getIconHighlight = [&] (Color c) { return gui.topMargin(-1, gui.uiHighlight(c)); };
  auto& collectiveInfo = *info.playerInfo.getReferenceMaybe<CollectiveInfo>();
  int hash = combineHash(collectiveInfo, info.villageInfo, info.modifiedSquares, info.totalSquares, info.tutorial);
  if (hash != rightBandInfoHash) {
    rightBandInfoHash = hash;
    vector<SGuiElem> buttons = makeVec(
        gui.icon(gui.BUILDING),
        gui.icon(gui.MINION),
        gui.icon(gui.LIBRARY),
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
        make_pair(CollectiveTab::TECHNOLOGY, drawTechnology(collectiveInfo))
    );
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
    bottomLine.addElemAuto(gui.stack(
        gui.getListBuilder()
            .addElemAuto(gui.label("speed: "))
            .addElem(gui.labelFun([this] { return getCurrentGameSpeedName();},
              [this] { return clock->isPaused() ? Color::RED : Color::WHITE; }), 70).buildHorizontalList(),
        gui.button([&] { gameSpeedDialogOpen = !gameSpeedDialogOpen; })));
    int modifiedSquares = info.modifiedSquares;
    int totalSquares = info.totalSquares;
    bottomLine.addBackElem(gui.stack(
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
        gui.button([=]() { counterMode = (CounterMode) ( ((int) counterMode + 1) % 3); })), 120);
    main = gui.margin(gui.leftMargin(10, bottomLine.buildHorizontalList()),
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
        gui.uiHighlightMouseOver(),
        gui.getListBuilder(keyMargin)
            .addElem(gui.label("pause"))
            .addElem(gui.label("[space]")).buildHorizontalList(),
        gui.button(pauseFun)));
  hotkeys.push_back(gui.keyHandler(pauseFun, {gui.getKey(SDL::SDLK_SPACE)}));
  for (GameSpeed speed : ENUM_ALL(GameSpeed)) {
    auto speedFun = [=] { gameSpeed = speed; gameSpeedDialogOpen = false; clock->cont();};
    auto colorFun = [this, speed] { return speed == gameSpeed ? Color::GREEN : Color::WHITE; };
    lines.push_back(gui.stack(
          gui.uiHighlightMouseOver(),
          gui.getListBuilder(keyMargin)
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

SGuiElem GuiBuilder::drawSpecialTrait(const SpecialTrait& trait) {
  return trait.visit(
      [&] (const ExtraTraining& t) {
        return gui.label("Extra "_s + toLower(getName(t.type)) + " training potential", Color::GREEN);
      },
      [&] (const AttrBonus& t) {
        return gui.label(toStringWithSign(t.increase) + " " + getName(t.attr), t.increase > 0 ? Color::GREEN : Color::RED);
      },
      [&] (LastingEffect effect) {
        if (auto adj = LastingEffects::getGoodAdjective(effect))
          return gui.label("Permanent trait: "_s + *adj, Color::GREEN);
        if (auto adj = LastingEffects::getBadAdjective(effect))
          return gui.label("Permanent trait: "_s + *adj, Color::RED);
        FATAL << "No adjective found: "_s + LastingEffects::getName(effect);
        return gui.empty();
      },
      [&] (SkillId skill) {
        return gui.label("Legendary skill level: " + Skill::get(skill)->getName(), Color::GREEN);
      },
      [&] (ExtraBodyPart part) {
        if (part.count == 1)
          return gui.label("Extra "_s + getName(part.part), Color::GREEN);
        else
          return gui.label(toString(part.count) + " extra "_s + getName(part.part) + "s", Color::GREEN);
      },
      [&] (const OneOfTraits&)-> SGuiElem {
        FATAL << "Can't draw traits alternative";
        return {};
      }
);
}

void GuiBuilder::toggleBottomWindow(GuiBuilder::BottomWindowId id) {
  if (bottomWindow == id)
    bottomWindow = none;
  else
    bottomWindow = id;
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
  for (auto& req : info.specialTraits)
    lines.addElem(drawSpecialTrait(req));
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
  auto continueButton = gui.setHeight(legendLineHeight, gui.buttonLabelBlink("Continue", [this] {
          callbacks.input(UserInputId::TUTORIAL_CONTINUE);
          tutorialClicks.clear();
          closeOverlayWindowsAndClearButton();
      }));
  auto backButton = gui.setHeight(legendLineHeight, gui.buttonLabel("Go back", getButtonCallback(UserInputId::TUTORIAL_GO_BACK)));
  SGuiElem warning;
  if (info.warning)
    warning = gui.label(*info.warning, Color::RED);
  else
    warning = gui.label("Press [Space] to unpause.",
        [this]{ return clock->isPaused() ? Color::RED : Color::TRANSPARENT;});
  return gui.preferredSize(520, 290, gui.stack(gui.darken(), gui.rectangleBorder(Color::GRAY),
      gui.margins(gui.stack(
        gui.labelMultiLine(info.message, legendLineHeight),
        gui.leftMargin(50, gui.alignment(GuiFactory::Alignment::BOTTOM_CENTER, gui.setHeight(legendLineHeight, warning))),
        gui.alignment(GuiFactory::Alignment::BOTTOM_RIGHT, info.canContinue ? continueButton : gui.empty()),
        gui.leftMargin(50, gui.alignment(GuiFactory::Alignment::BOTTOM_LEFT, info.canGoBack ? backButton : gui.empty()))
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
  return gui.label(getName(type), getColor(type));
}

SGuiElem GuiBuilder::drawVillainInfoOverlay(const VillageInfo::Village& info, bool showDismissHint) {
  auto lines = gui.getListBuilder(legendLineHeight);
  string name = info.tribeName;
  if (info.name)
    name  = *info.name + ", " + name;
  lines.addElem(
      gui.getListBuilder()
          .addElemAuto(gui.label(capitalFirst(name)))
          .addSpace(100)
          .addElemAuto(drawVillainType(info.type))
          .buildHorizontalList());
  if (info.attacking)
    lines.addElem(gui.label("Attacking!", Color::RED));
  if (info.access == VillageInfo::Village::NO_LOCATION)
    lines.addElem(gui.label("Location unknown", Color::LIGHT_BLUE));
  else if (info.access == VillageInfo::Village::INACTIVE)
    lines.addElem(gui.label("Outside of influence zone", Color::GRAY));
  if (info.isConquered)
    lines.addElem(gui.label("Conquered", Color::LIGHT_BLUE));
  auto triggers = info.triggers;
  sort(triggers.begin(), triggers.end(),
      [] (const VillageInfo::Village::TriggerInfo& t1, const VillageInfo::Village::TriggerInfo& t2) {
          return t1.value > t2.value;});
  if (!triggers.empty()) {
    auto line = gui.getListBuilder();
    line.addElemAuto(gui.label("Triggered by: "));
    bool addComma = false;
    for (auto& t : triggers) {
      auto name = t.name;
  /*#ifndef RELEASE
      name += " " + toString(t.value);
  #endif*/
      if (addComma)
        line.addElemAuto(gui.label(", "));
      line.addElemAuto(gui.label(name, getTriggerColor(t.value)));
      addComma = true;
    }
    lines.addElem(line.buildHorizontalList());
  }
  for (auto action : info.actions)
    if (action.disabledReason)
      lines.addElem(gui.label(*action.disabledReason, Color::ORANGE));
  if (showDismissHint)
    lines.addElem(gui.label("Right click to dismiss", Renderer::smallTextSize, Color::WHITE), legendLineHeight * 2 / 3);
  return gui.miniWindow(gui.margins(lines.buildVerticalList(), 15));
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
  auto lines = gui.getListBuilder();
  lines.addSpace(90);
  if (!info.villages.empty())
    lines.addElemAuto(gui.stack(
          gui.getListBuilder()
              .addElem(gui.icon(GuiFactory::EXPAND_UP), 12)
              .addElemAuto(gui.translate(gui.label("All villains"), Vec2(0, labelOffsetY)))
              .addSpace(10)
              .buildHorizontalList(),
          gui.conditional(gui.margins(gui.rectangle(Color(0, 255, 0, 100)), -7, 2, 2, 2),
              [this] { return bottomWindow == ALL_VILLAINS; }),
          gui.button([this] { toggleBottomWindow(ALL_VILLAINS); })
    ));
  for (int i : All(info.villages)) {
    SGuiElem label;
    string labelText;
    SGuiElem button = gui.empty();
    auto& elem = info.villages[i];
    if (elem.attacking) {
      labelText = "attacking";
      label = gui.label(labelText, Color::RED);
    }
    else if (!elem.triggers.empty()) {
      labelText = "triggered";
      label = gui.label(labelText, Color::ORANGE);
    } else
      for (auto& action : elem.actions)
        if (!action.disabledReason) {
          labelText = getVillageActionText(action.action);
          label = gui.label(labelText, Color::GREEN);
          button = gui.button(getButtonCallback({UserInputId::VILLAGE_ACTION,
              VillageActionInfo{elem.id, action.action}}));
          break;
        }
    if (!label || info.dismissedInfos.count({elem.id, labelText}))
      continue;
    label = gui.translate(std::move(label), Vec2(0, labelOffsetY));
    auto infoOverlay = drawVillainInfoOverlay(elem, true);
    int infoOverlayHeight = *infoOverlay->getPreferredHeight();
    lines.addElemAuto(gui.stack(makeVec(
        std::move(button),
        gui.releaseRightButton(getButtonCallback({UserInputId::DISMISS_VILLAGE_INFO,
            DismissVillageInfo{elem.id, labelText}})),
        gui.onMouseRightButtonHeld(gui.margins(gui.rectangle(Color(255, 0, 0, 100)), 0, 2, 2, 2)),
        gui.getListBuilder()
            .addElemAuto(gui.stack(
                 gui.setWidth(34, gui.centerVert(gui.centerHoriz(gui.bottomMargin(-3,
                     gui.viewObject(ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
                 gui.setWidth(34, gui.centerVert(gui.centerHoriz(gui.bottomMargin(5,
                     gui.viewObject(elem.viewId)))))))
            .addElemAuto(gui.rightMargin(5, gui.translate(std::move(label), Vec2(-2, 0))))
            .buildHorizontalList(),
        gui.conditional(gui.tooltip2(std::move(infoOverlay), [=](const Rectangle& r) { return r.topLeft() - Vec2(0, 0 + infoOverlayHeight);}),
            [this]{return !bottomWindow;})
    )));
  }
  return gui.setHeight(29, gui.stack(
        gui.stopMouseMovement(),
        gui.translucentBackgroundWithBorder(gui.topMargin(0, lines.buildHorizontalList()))));
}

SGuiElem GuiBuilder::drawAllVillainsOverlay(const VillageInfo& info) {
  auto lines = gui.getListBuilder(legendLineHeight);
  if (info.numMainVillains > 0) {
    lines.addElem(gui.label(toString(info.numConqueredMainVillains) + "/" + toString(info.numMainVillains) +
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
    auto label = gui.label(capitalFirst(name), labelColor);
    if (elem.isConquered)
      label = gui.stack(std::move(label), gui.crossOutText(Color::RED));
    for (auto& action : elem.actions)
      if (!action.disabledReason) {
        label = gui.stack(
            gui.getListBuilder()
                .addElemAuto(std::move(label))
                .addBackElemAuto(gui.label(" ["_s + getVillageActionText(action.action) + "]", Color::GREEN))
                .buildHorizontalList(),
            gui.button(getButtonCallback({UserInputId::VILLAGE_ACTION,
                  VillageActionInfo{elem.id, action.action}}))
        );
        break;
      }
    lines.addElem(gui.stack(
        gui.getListBuilder()
            .addElemAuto(gui.setWidth(34, gui.centerVert(gui.centerHoriz(gui.stack(
                 gui.bottomMargin(-3, gui.viewObject(ViewId("round_shadow"), 1, Color(255, 255, 255, 160))),
                 gui.bottomMargin(5, gui.viewObject(elem.viewId))
            )))))
            .addElemAuto(gui.rightMargin(5, gui.translate(gui.renderInBounds(std::move(label)), Vec2(0, 0))))
            .buildHorizontalList(),
        gui.tooltip2(std::move(infoOverlay), [=](const Rectangle& r) { return r.topRight();})));
  }
  return gui.setWidth(350, gui.miniWindow(gui.margins(gui.stack(
        gui.stopMouseMovement(),
        gui.scrollable(lines.buildVerticalList(), &tasksScroll, &scrollbarsHeld)), 15), [this] { bottomWindow = none; }, true));
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
    if (!elem.specialTraits.empty())
      button = gui.stack(
          std::move(button),
          gui.icon(GuiFactory::IconId::SPECIAL_IMMIGRANT)
      );
    if (tutorial && elem.tutorialHighlight && tutorial->highlights.contains(*elem.tutorialHighlight))
        button = gui.stack(std::move(button), gui.blink(makeHighlight(Color(255, 255, 0, 100))));
    auto initTime = elem.generatedTime;
    lines.addElem(gui.translate([=]() { return Vec2(0, initTime ? -getImmigrantAnimationOffset(*initTime) : 0);},
        gui.stack(
            std::move(button),
            gui.tooltip2(drawImmigrantInfo(elem), [](const Rectangle& r) { return r.topRight();}),
            gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(-3,
                gui.viewObject(ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
            gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(5,
                elem.count ? drawMinionAndLevel(elem.viewId, *elem.count, 1) : gui.viewObject(elem.viewId)))))
    )));
  }
  lines.addElem(gui.stack(makeVec(
      gui.sprite(GuiFactory::TexId::IMMIGRANT_BG, GuiFactory::Alignment::CENTER),
      gui.conditional(makeHighlight(Color(0, 255, 0, 100)), [this] { return bottomWindow == IMMIGRATION_HELP; }),
      gui.button([this] { toggleBottomWindow(IMMIGRATION_HELP); }),
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
          icon = gui.stack(std::move(icon), gui.viewObject(ViewId("accept_immigrant"), iconScale));
          break;
        case ImmigrantAutoState::AUTO_REJECT:
          icon = gui.stack(std::move(icon), gui.viewObject(ViewId("reject_immigrant"), iconScale));
          break;
      }
    line.addElem(gui.stack(makeVec(
        gui.button(getButtonCallback({UserInputId::IMMIGRANT_AUTO_ACCEPT, elem.id})),
        gui.buttonRightClick(getButtonCallback({UserInputId::IMMIGRANT_AUTO_REJECT, elem.id})),
        gui.tooltip2(drawImmigrantInfo(elem), [](const Rectangle& r) { return r.bottomLeft();}),
        gui.setWidth(elemWidth, gui.centerVert(gui.centerHoriz(gui.bottomMargin(-3,
            gui.viewObject(ViewId("round_shadow"), 1, Color(255, 255, 255, 160)))))),
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
        lines.buildVerticalList()), 15), [this] { bottomWindow = none; }, true);
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
      attrText += toStringWithSign(elem.bonus);
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
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
  return gui.getListBuilder(legendLineHeight)
      .addElem(gui.centerHoriz(gui.horizontalList(
              drawPlayerAttributes(gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>()->attributes), resourceSpace)))
      .addElem(gui.centerHoriz(gui.getListBuilder()
             .addElemAuto(info.avatarLevelInfo ? gui.standardButton(gui.stack(
                 gui.margins(gui.progressBar(Color::DARK_GREEN.transparency(128), info.avatarLevelInfo->progress), -3, -1, 0, 7),
                 info.avatarLevelInfo->numAvailable > 0 ? gui.margins(
                     gui.uiHighlightLine(), 3, -2, -3, 2) : gui.empty(),
                 gui.getListBuilder()
//                     .addSpace(10)
                     .addElemAuto(gui.topMargin(-2, gui.viewObject(info.avatarLevelInfo->viewId)))
                     .addElemAuto(gui.label("Level: " + toString(info.avatarLevelInfo->level)))
                     .buildHorizontalList()),
                 gui.button([this]() { closeOverlayWindowsAndClearButton(); callbacks.input(UserInputId::TECHNOLOGY);})
             ) : gui.empty())
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
  out.append(item.description);
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
    line.addElem(gui.viewObject(ViewId("key")), viewObjectWidth);
    leftMargin -= viewObjectWidth - 3;
  }
  auto viewId = gui.viewObject(item.viewId);
  if (item.viewIdModifiers.contains(ViewObjectModifier::AURA))
    viewId = gui.stack(std::move(viewId), gui.viewObject(ViewId("item_aura")));
  line.addElem(std::move(viewId), viewObjectWidth);
  Color color = item.equiped ? Color::GREEN : (item.pending || item.unavailable) ?
      Color::GRAY : Color::WHITE;
  if (item.intrinsicState)
    color = getIntrinsicStateColor(*item.intrinsicState);
  if (item.number > 1)
    line.addElemAuto(gui.rightMargin(0, gui.label(toString(item.number) + " ", color)));
  line.addMiddleElem(gui.label(item.name, color));
  auto mainLine = gui.stack(
      gui.buttonRect(onClick),
      gui.renderInBounds(line.buildHorizontalList()),
      getTooltip(getItemHint(item), (int) item.ids.getHash()));
  line.clear();
  line.addMiddleElem(std::move(mainLine));
  line.addBackSpace(5);
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
        getTooltip({"Click to choose how many to pick up."}, int(item.ids.begin()->getGenericId()))), 25);
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
  if (itemIndex.value_or(-1) >= totalElems)
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
    case ItemAction::REMOVE: return "remove item";
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
    lines.push_back(gui.label("Skills", Color::YELLOW));
    for (auto& elem : info.skills)
      lines.push_back(gui.stack(getTooltip({elem.help}, THIS_LINE),
            gui.label(capitalFirst(elem.name), Color::WHITE)));
    lines.push_back(gui.empty());
  }
  return lines;
}

SGuiElem GuiBuilder::getSpellIcon(const PlayerInfo::Spell& spell, int index, bool active, UniqueEntity<Creature>::Id id) {
  vector<SGuiElem> ret;
  if (!spell.timeout) {
    ret.push_back(gui.mouseHighlight2(gui.standardButtonHighlight(), gui.standardButton()));
    ret.push_back(gui.centerHoriz(gui.centerVert(gui.labelUnicode(spell.symbol, Color::WHITE))));
    if (active)
      ret.push_back(gui.button(getButtonCallback({UserInputId::CAST_SPELL, index})));
  } else {
    ret.push_back(gui.standardButton());
    ret.push_back(gui.centerHoriz(gui.centerVert(gui.labelUnicode(spell.symbol, Color::GRAY))));
    ret.push_back(gui.darken());
    ret.push_back(gui.centeredLabel(Renderer::HOR_VER, toString(*spell.timeout)));
  }
  ret.push_back(getTooltip(concat({capitalFirst(spell.name)}, spell.help), THIS_LINE + index + id.getGenericId()));
  return gui.stack(std::move(ret));
}

const Vec2 spellIconSize = Vec2(47, 47);

SGuiElem GuiBuilder::drawSpellsList(const PlayerInfo& info, bool active) {
  constexpr int spellsPerRow = 5;
  if (!info.spells.empty()) {
    auto list = gui.getListBuilder(spellIconSize.y);
    list.addElem(gui.label("Abilities", Color::YELLOW), legendLineHeight);
    auto line = gui.getListBuilder(spellIconSize.x);
    for (int index : All(info.spells)) {
      auto& elem = info.spells[index];
      line.addElem(getSpellIcon(elem, index, active, info.creatureId));
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

static string toStringRounded(double value, double precision) {
  return toString(precision * round(value / precision));
}

SGuiElem GuiBuilder::getExpIncreaseLine(const CreatureExperienceInfo& info, ExperienceType type,
    function<void()> increaseCallback) {
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
  line.addElem(gui.label("+" + toStringRounded(info.level[type], 0.01),
      info.warning[type] ? Color::RED : Color::WHITE), 50);
  if (increaseCallback && info.numAvailableUpgrades > 0) {
    line.addElemAuto(info.level[type] < info.limit[type]
        ? gui.buttonLabel("+", increaseCallback)
        : gui.buttonLabelInactive("+"));
    line.addSpace(15);
  }
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

SGuiElem GuiBuilder::drawTrainingInfo(const CreatureExperienceInfo& info,
    function<void(optional<ExperienceType>)> increaseCallback) {
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(gui.label("Training", Color::YELLOW));
  if (increaseCallback) {
    lines.addElem(gui.label(getPlural("point", info.numAvailableUpgrades) + " available"));
  }
  bool empty = true;
  for (auto expType : ENUM_ALL(ExperienceType)) {
    if (auto elem = getExpIncreaseLine(info, expType,
        [increaseCallback, expType] { increaseCallback(expType); })) {
      lines.addElem(std::move(elem));
      empty = false;
    }
  }
  lines.addElem(gui.getListBuilder()
      .addElemAuto(gui.label("Combat experience: ", Color::YELLOW))
      .addElemAuto(gui.label(toStringRounded(info.combatExperience, 0.01)))
      .buildHorizontalList());
  if (increaseCallback) {
    lines.addSpace(15);
    lines.addElem(gui.centerHoriz(
        gui.buttonLabel("Dismiss", [increaseCallback] { increaseCallback(none); })));
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
        gui.conditional(gui.tutorialHighlight(), [=]{ return isTutorial;}),
        gui.buttonLabel("Commands",
          gui.buttonRect([this, commands = info.commands] (Rectangle bounds) {
              auto lines = gui.getListBuilder(legendLineHeight);
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
         }))));
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
        drawCost({ViewId("gold"), info.debt}),
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
          gui.mouseHighlight2(getIconHighlight(Color::GREEN)),
          gui.mouseOverAction([this, i] { highlightedTeamMember = i;},
              [this, i] { if (highlightedTeamMember == i) highlightedTeamMember = none; }),
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
                      gui.label(getText(getText(action)))));
              }
              drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 200, false);
              if (ret)
                callbacks.input({UserInputId::TEAM_MEMBER_ACTION, TeamMemberActionInfo{*ret, memberId}});
        }));
    }
    teamList.addElemAuto(std::move(icon));
    if (teamList.getLength() >= 5) {
      vList.addElemAuto(gui.margins(teamList.buildHorizontalList(), 0, 25, 0, 0));
      teamList.clear();
    }
  }
  if (!teamList.isEmpty())
    vList.addElemAuto(gui.margins(teamList.buildHorizontalList(), 0, 25, 0, 3));
  vList.addSpace(22);
  if (info.teamOrders) {
    auto orderList = gui.getListBuilder();
    for (auto order : ENUM_ALL(TeamOrder)) {
      auto switchFun = getButtonCallback({UserInputId::TOGGLE_TEAM_ORDER, order});
      orderList.addElemAuto(gui.stack(
          info.teamOrders->contains(order)
              ? gui.buttonLabelSelected(getName(order), switchFun)
              : gui.buttonLabel(getName(order), switchFun),
          getTooltip({getDescription(order)}, THIS_LINE)
      ));
      orderList.addSpace(15);
    }
    vList.addElem(orderList.buildHorizontalList());
    vList.addSpace(legendLineHeight / 2);
  }
  if (info.teamInfos.size() > 1) {
    vList.addElem(gui.buttonLabel("Control mode: "_s + getControlModeName(info.controlMode) + "",
        gui.button(getButtonCallback(UserInputId::TOGGLE_CONTROL_MODE), gui.getKey(SDL::SDLK_g))));
    vList.addSpace(legendLineHeight / 2);
  }
  vList.addElem(gui.buttonLabel("Exit control mode",
        gui.button(getButtonCallback(UserInputId::EXIT_CONTROL_MODE), gui.getKey(SDL::SDLK_u))));
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
  vList.addElemAuto(gui.stack(std::move(others)));
  return gui.margins(gui.scrollable(vList.buildVerticalList(), &inventoryScroll, &scrollbarsHeld), 6, 0, 15, 5);
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

SGuiElem GuiBuilder::drawTeams(const CollectiveInfo& info, const optional<TutorialInfo>& tutorial) {
  const int elemWidth = 30;
  auto lines = gui.getListBuilder(legendLineHeight);
  for (int i : All(info.teams)) {
    auto& team = info.teams[i];
    const int numPerLine = 7;
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
            gui.conditional(gui.tutorialHighlight(),
                [=]{ return !wasTutorialClicked(0, TutorialHighlight::CONTROL_TEAM) && isTutorialHighlight; }),
            gui.uiHighlightConditional([team] () { return team.highlight; }),
            gui.uiHighlightMouseOver(),
            gui.mouseOverAction([team, this] { mapGui->highlightTeam(team.members); },
                [team, this] { mapGui->unhighlightTeam(team.members); }),
            cache->get(selectButton, THIS_LINE, team.id),
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
      makeSetting(mapGui->hideFullHealthBars, "Hide full health bars", "")
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
      list.addElem(gui.stack(
          cache->get(selectButton, THIS_LINE, elem.creatureId),
          gui.dragSource({DragContentId::CREATURE_GROUP, elem.creatureId},
              [=]{ return gui.getListBuilder(10)
                  .addElemAuto(gui.label(toString(elem.count) + " "))
                  .addElem(gui.viewObject(elem.viewId)).buildHorizontalList();}),
          gui.uiHighlightConditional([highlight = elem.highlight]{return highlight;}),
          line.buildHorizontalList()
       ));
    }
    list.addElem(gui.label("Teams: ", Color::WHITE));
    list.addElemAuto(drawTeams(info, tutorial));
    list.addSpace();
    list.addElem(gui.stack(
              gui.label("Show tasks", [=]{ return bottomWindow == TASKS ? Color::GREEN : Color::WHITE;}),
              gui.button([this] { toggleBottomWindow(TASKS); })));
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
          gui.keyHandler([this]{bottomWindow = none;}, {gui.getKey(SDL::SDLK_ESCAPE)}, true),
          gui.miniWindow(
        gui.margins(gui.scrollable(gui.verticalList(std::move(lines), lineHeight), &tasksScroll, &scrollbarsHeld),
          margin))), [this] { return bottomWindow == TASKS; }));
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

SGuiElem GuiBuilder::drawWorkshopItemActionButton(const CollectiveInfo::QueuedItemInfo& elem, int itemIndex) {
  return gui.buttonRect([=] (Rectangle bounds) {
      auto lines = gui.getListBuilder(legendLineHeight);
      disableTooltip = true;
      DestructorFunction dFun([this] { disableTooltip = false; });
      bool exit = false;
      optional<ItemAction> ret;
      for (auto action : elem.itemInfo.actions) {
        auto buttonFun = [&exit, &ret, action] {
            ret = action;
            exit = true;
        };
        lines.addElem(gui.stack(
              gui.button(buttonFun),
              gui.uiHighlightMouseOver(),
              gui.label(getActionText(action))));
      }
      drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 200, false);
      if (ret)
        callbacks.input({UserInputId::WORKSHOP_ITEM_ACTION,
            WorkshopQueuedActionInfo{itemIndex, *ret}});
  });
}

SGuiElem GuiBuilder::drawItemUpgradeButton(const CollectiveInfo::QueuedItemInfo& elem, int itemIndex) {
  auto buttonHandler = gui.buttonRect([=] (Rectangle bounds) {
      auto lines = gui.getListBuilder(legendLineHeight);
      disableTooltip = true;
      DestructorFunction dFun([this] { disableTooltip = false; });
      bool exit = false;
      optional<WorkshopUpgradeInfo> ret;
      for (int i : All(elem.added)) {
        auto buttonFun = [&exit, &ret, i, itemIndex] {
            ret = WorkshopUpgradeInfo{ itemIndex,  i, true};
            exit = true;
        };
        auto& upgrade = elem.added[i];
        auto idLine = gui.getListBuilder();
        idLine.addElemAuto(gui.label("Remove "));
        idLine.addElemAuto(gui.viewObject(upgrade.viewId));
        idLine.addElemAuto(gui.label(upgrade.name));
        lines.addElem(gui.stack(
              gui.button(buttonFun),
              gui.uiHighlightMouseOver(),
              idLine.buildHorizontalList(),
              gui.tooltip({upgrade.description})
        ));
      }
      if (elem.added.size() < elem.maxUpgrades)
        for (int i : All(elem.available)) {
          auto buttonFun = [&exit, &ret, i, itemIndex] {
              ret = WorkshopUpgradeInfo{ itemIndex,  i, false};
              exit = true;
          };
          auto& upgrade = elem.available[i];
          auto idLine = gui.getListBuilder();
          idLine.addElemAuto(gui.label("Add "));
          idLine.addElemAuto(gui.viewObject(upgrade.viewId));
          idLine.addElemAuto(gui.label(upgrade.name + " (" + toString(upgrade.count) + " available)"));
          lines.addElem(gui.stack(
                gui.button(buttonFun),
                gui.uiHighlightMouseOver(),
                idLine.buildHorizontalList(),
                gui.tooltip({upgrade.description})
          ));
        }
      lines.addElem(gui.label("Available slots: " + toString(elem.maxUpgrades - elem.added.size())));
      lines.addElem(gui.label("Upgraded items can only be crafted by a craftsman of legendary skills.",
          Renderer::smallTextSize, Color::LIGHT_GRAY));
      drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 450, false);
      if (ret)
        callbacks.input({UserInputId::WORKSHOP_UPGRADE, *ret});
  });
  auto line = gui.getListBuilder();
  for (int upgradeIndex : All(elem.added)) {
    auto& upgrade = elem.added[upgradeIndex];
    line.addElemAuto(gui.viewObject(upgrade.viewId));
  }
  if (!elem.added.empty())
    return gui.standardButton(line.buildHorizontalList(), std::move(buttonHandler));
  else
    return gui.buttonLabel("upgrade", std::move(buttonHandler));
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
  for (int itemIndex : All(options)) {
    auto& elem = options[itemIndex];
    if (elem.hidden)
      continue;
    auto line = gui.getListBuilder();
    line.addElem(gui.viewObject(elem.viewId), 35);
    line.addMiddleElem(gui.renderInBounds(gui.label(elem.name, elem.unavailable ? Color::GRAY : Color::WHITE)));
    if (elem.price)
      line.addBackElem(gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(*elem.price)), 80);
    SGuiElem guiElem = line.buildHorizontalList();
    if (elem.tutorialHighlight)
      guiElem = gui.stack(gui.tutorialHighlight(), std::move(guiElem));
    if (elem.unavailable) {
      CHECK(!elem.unavailableReason.empty());
      guiElem = gui.stack(getTooltip(concat({elem.unavailableReason}, elem.description), THIS_LINE + itemIndex), std::move(guiElem));
    }
    else
      guiElem = gui.stack(
          gui.uiHighlightMouseOver(),
          std::move(guiElem),
          gui.button(getButtonCallback({UserInputId::WORKSHOP_ADD, itemIndex})),
          getTooltip({elem.description}, THIS_LINE)
      );
    lines.addElem(gui.rightMargin(rightElemMargin, std::move(guiElem)));
  }
  auto lines2 = gui.getListBuilder(legendLineHeight);
  lines2.addElem(gui.label("In production:", Color::YELLOW));
  for (int itemIndex : All(queued)) {
    auto& elem = queued[itemIndex];
    auto line = gui.getListBuilder();
    line.addMiddleElem(gui.stack(
        drawWorkshopItemActionButton(elem, itemIndex),
        gui.uiHighlightMouseOver(),
        gui.getListBuilder()
            .addElem(gui.viewObject(elem.itemInfo.viewId), 35)
            .addMiddleElem(gui.renderInBounds(gui.label(elem.itemInfo.name)))
            .buildHorizontalList()
    ));
    if ((!elem.available.empty() || !elem.added.empty()) && elem.maxUpgrades > 0) {
      line.addBackElemAuto(gui.leftMargin(7, drawItemUpgradeButton(elem, itemIndex)));
    }
    if (elem.itemInfo.price)
      line.addBackElem(gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(*elem.itemInfo.price)), 80);
    lines2.addElem(gui.stack(
        gui.bottomMargin(5,
            gui.progressBar(Color::DARK_GREEN.transparency(128), elem.productionState)),
        gui.rightMargin(rightElemMargin, line.buildHorizontalList()),
        getTooltip(elem.itemInfo.description, THIS_LINE)
    ));
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

SGuiElem GuiBuilder::drawCreatureUpgradeMenu(SyncQueue<optional<ExperienceType>>& queue,
    const CreatureExperienceInfo& info) {
  const int margin = 20;
  auto lines = gui.getListBuilder(legendLineHeight);
  if (auto elem = drawTrainingInfo(info, [&queue](optional<ExperienceType> exp) { queue.push(exp); }))
    lines.addElemAuto(std::move(elem));
  else
    return nullptr;
  return gui.preferredSize(500, 320,
      gui.window(gui.margins(lines.buildVerticalList(), margin), [&queue] { queue.push(none); }));
}

static string getName(TechId id) {
  return id.data();
}

SGuiElem GuiBuilder::drawLibraryOverlay(const CollectiveInfo& collectiveInfo, const optional<TutorialInfo>& tutorial) {
  if (!collectiveInfo.libraryInfo)
    return gui.empty();
  auto& info = *collectiveInfo.libraryInfo;
  const int margin = 20;
  const int rightElemMargin = 10;
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addSpace(5);
  lines.addElem(gui.getListBuilder()
      .addElemAuto(gui.topMargin(-2, gui.viewObject(collectiveInfo.avatarLevelInfo.viewId)))
      .addSpace(4)
      .addElemAuto(gui.label(toString(collectiveInfo.avatarLevelInfo.title)))
      .buildHorizontalList());
  lines.addElem(gui.label("Level " + toString(collectiveInfo.avatarLevelInfo.level)));
  lines.addElem(gui.label("Next level progress: " +
      toString(info.currentProgress) + "/" + toString(info.totalProgress)));
  //lines.addElem(gui.rightMargin(rightElemMargin, gui.alignment(GuiFactory::Alignment::RIGHT, drawCost(info.resource))));
  if (info.warning)
    lines.addElem(gui.label(*info.warning, Color::RED));
  if (!info.available.empty()) {
    lines.addElem(gui.label("Research:", Color::YELLOW));
    lines.addElem(gui.label("(" + getPlural("item", collectiveInfo.avatarLevelInfo.numAvailable) + " available)", Color::YELLOW));
    for (int i : All(info.available)) {
      auto& elem = info.available[i];
      auto line = gui.getListBuilder()
          .addElem(gui.label(getName(elem.id), elem.active ? Color::WHITE : Color::GRAY), 10)
          .buildHorizontalList();
      line = gui.stack(std::move(line), getTooltip({elem.description}, THIS_LINE));
      if (elem.tutorialHighlight && tutorial && tutorial->highlights.contains(*elem.tutorialHighlight))
        line = gui.stack(gui.tutorialHighlight(), std::move(line));
      if (elem.active)
        line = gui.stack(
            gui.uiHighlightMouseOver(Color::GREEN),
            std::move(line),
            gui.button(getButtonCallback({UserInputId::LIBRARY_ADD, elem.id})));
      lines.addElem(gui.rightMargin(rightElemMargin, std::move(line)));
    }
    lines.addSpace(legendLineHeight * 2 / 3);
  }
  if (!info.researched.empty())
  lines.addElem(gui.label("Already researched:", Color::YELLOW));
  for (int i : All(info.researched)) {
    auto& elem = info.researched[i];
    auto line = gui.getListBuilder()
        .addElem(gui.label(getName(elem.id), Color::GRAY), 10)
        .buildHorizontalList();
    line = gui.stack(std::move(line), getTooltip({elem.description}, THIS_LINE));
    lines.addElem(gui.rightMargin(rightElemMargin, std::move(line)));
  }
  int height = lines.getSize();
  return gui.preferredSize(500, height + 2 * margin + 2,
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
    list.addElem(
        gui.buttonLabel("Disband team", getButtonCallback({UserInputId::CANCEL_TEAM, *info.chosenCreature->teamId})));
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
  return gui.preferredSize(640 + minionListWidth, 600,
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
      elems.push_back(gui.setWidth(350, gui.conditional(
            gui.miniWindow(gui.margins(getButtonLine(elem, i, CollectiveTab::BUILDINGS, tutorial), margin)),
            [i, this] { return getActiveButton(CollectiveTab::BUILDINGS) == i;})));
    }
  }
  for (auto& elem : overlaysMap) {
    auto& lines = elem.second;
    string groupName = elem.first;
    elems.push_back(gui.setWidth(350, gui.conditionalStopKeys(
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
  if (auto action = object.getClickAction()) {
    lines.addElem(gui.label(getText(*action)));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!object.getExtendedActions().isEmpty()) {
    lines.addElem(gui.label("Right click:", Color::LIGHT_BLUE));
    for (auto action : object.getExtendedActions())
      lines.addElem(gui.label(getText(action), Color::LIGHT_GRAY));
    lines.addSpace(legendLineHeight / 3);
  }
  if (!lines.isEmpty())
    return lines.buildVerticalList();
  else
    return nullptr;
}

SGuiElem GuiBuilder::drawLyingItemsList(const string& title, const ItemCounts& itemCounts, int maxWidth) {
  auto lines = gui.getListBuilder(legendLineHeight);
  auto line = gui.getListBuilder();
  int currentWidth = 0;
  for (auto& elemPair : itemCounts) {
    auto cnt = elemPair.second;
    auto id = elemPair.first;
    if (line.isEmpty() && lines.isEmpty() && !title.empty()) {
      line.addElemAuto(gui.label(title));
      currentWidth = line.getSize();
    }
    auto elem = cnt > 1
        ? drawMinionAndLevel(id, cnt, 1)
        : gui.viewObject(id);
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
  auto lines = gui.getListBuilder(legendLineHeight);
  vector<SGuiElem> allElems;
  if (!hint.empty()) {
    for (auto& line : hint)
      if (!line.empty())
        lines.addElem(gui.label(line));
  } else {
    auto& highlighted = mapGui->getLastHighlighted();
    auto& index = highlighted.viewIndex;
    for (auto layer : ENUM_ALL_REVERSE(ViewLayer))
      if (index.hasObject(layer)) {
        auto& viewObject = index.getObject(layer);
        lines.addElem(gui.getListBuilder()
              .addElem(gui.viewObject(viewObject.id()), 30)
              .addElemAuto(gui.label(viewObject.getDescription()))
              .buildHorizontalList());
        if (layer == ViewLayer::CREATURE)
          lines.addElemAuto(drawLyingItemsList("Inventory: ", highlighted.viewIndex.getEquipmentCounts(), 250));
        if (viewObject.hasModifier(ViewObject::Modifier::HOSTILE))
          lines.addElem(gui.label("Hostile", Color::ORANGE));
        for (auto status : viewObject.getCreatureStatus()) {
          lines.addElem(gui.label(getName(status), getColor(status)));
          if (auto desc = getDescription(status))
            lines.addElem(gui.label(*desc, getColor(status)));
          break;
        }
        if (!disableClickActions)
          if (auto actions = getClickActions(viewObject))
            if (highlighted.tileScreenPos)
              allElems.push_back(gui.absolutePosition(gui.translucentBackgroundWithBorderPassMouse(gui.margins(
                  gui.setHeight(*actions->getPreferredHeight(), actions), 5, 1, 5, -2)),
                  highlighted.creaturePos.value_or(*highlighted.tileScreenPos) + Vec2(60, 60)));
        if (!viewObject.getBadAdjectives().empty()) {
          lines.addElemAuto(gui.labelMultiLineWidth(viewObject.getBadAdjectives(), legendLineHeight * 2 / 3, 300,
              Renderer::textSize, Color::RED, ','));
          lines.addSpace(legendLineHeight / 3);
        }
        if (!viewObject.getGoodAdjectives().empty()) {
          lines.addElemAuto(gui.labelMultiLineWidth(viewObject.getGoodAdjectives(), legendLineHeight * 2 / 3, 300,
              Renderer::textSize, Color::GREEN, ','));
          lines.addSpace(legendLineHeight / 3);
        }
        if (viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
          lines.addElem(gui.label("Can only be healed using rituals."));
        if (auto& attributes = viewObject.getCreatureAttributes())
          lines.addElemAuto(drawAttributesOnPage(drawPlayerAttributes(*attributes)));
        if (auto health = viewObject.getAttribute(ViewObjectAttribute::HEALTH))
          lines.addElem(gui.stack(
                gui.margins(gui.progressBar(MapGui::getHealthBarColor(*health,
                    viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE)).transparency(70), *health), -2, 0, 0, 3),
                gui.label(getHealthName(viewObject.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE))
                    + toString((int) (100.0f * *health)) + "%")));
        if (auto morale = viewObject.getAttribute(ViewObjectAttribute::MORALE))
          lines.addElem(gui.stack(
                gui.margins(gui.progressBar((*morale >= 0 ? Color::GREEN : Color::RED).transparency(70), fabs(*morale)), -2, 0, 0, 3),
                gui.label("Morale: " + getMoraleNumber(*morale))));
        if (auto luxury = viewObject.getAttribute(ViewObjectAttribute::LUXURY))
          lines.addElem(gui.stack(
                gui.margins(gui.progressBar(Color::GREEN.transparency(70), fabs(*luxury)), -2, 0, 0, 3),
                gui.label("Luxury: " + getMoraleNumber(*luxury))));
        if (viewObject.hasModifier(ViewObjectModifier::PLANNED))
          lines.addElem(gui.label("Planned"));
        lines.addElem(gui.margins(gui.rectangle(Color::DARK_GRAY), -9, 2, -9, 8), 12);
      }
    if (highlighted.viewIndex.isHighlight(HighlightType::INSUFFICIENT_LIGHT))
      lines.addElem(gui.label("Insufficient light", Color::RED));
    if (highlighted.viewIndex.isHighlight(HighlightType::INDOORS))
      lines.addElem(gui.label("Indoors"));
    else
      lines.addElem(gui.label("Outdoors"));
    if (highlighted.tilePos)
      lines.addElem(gui.label("Position: " + toString(*highlighted.tilePos)));
    lines.addElemAuto(drawLyingItemsList("Lying here: ", highlighted.viewIndex.getItemCounts(), 250));
  }
  if (!lines.isEmpty())
    allElems.push_back(gui.margins(gui.translucentBackgroundWithBorderPassMouse(
        gui.margins(lines.buildVerticalList(), 10, 10, 10, 22)), 0, 0, -2, -2));
  return gui.stack(allElems);
}

SGuiElem GuiBuilder::drawScreenshotOverlay() {
  const int width = 600;
  const int height = 360;
  const int margin = 20;
  return gui.preferredSize(width, height, gui.stack(
      gui.keyHandler(getButtonCallback(UserInputId::CANCEL_SCREENSHOT), {gui.getKey(SDL::SDLK_ESCAPE)}, true),
      gui.rectangle(Color::TRANSPARENT, Color::LIGHT_GRAY),
      gui.translate(gui.centerHoriz(gui.miniWindow(gui.margins(gui.getListBuilder(legendLineHeight)
              .addElem(gui.labelMultiLineWidth("Your dungeon will be shared in Steam Workshop with an attached screenshot. "
                  "Steer the rectangle below to a particularly pretty or representative area of your dungeon and confirm.",
                  legendLineHeight, width - 2 * margin), legendLineHeight * 4)
              .addElem(gui.centerHoriz(gui.getListBuilder()
                  .addElemAuto(gui.buttonLabel("Confirm", getButtonCallback({UserInputId::TAKE_SCREENSHOT, Vec2(width, height)})))
                  .addSpace(15)
                  .addElemAuto(gui.buttonLabel("Cancel", getButtonCallback(UserInputId::CANCEL_SCREENSHOT)))
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
    GuiFactory::ListBuilder line(gui);
    for (auto& message : messages[i]) {
      string text = (line.isEmpty() ? "" : " ") + message.getText();
      cutToFit(renderer, text, maxMessageLength - 2 * hMargin);
      if (message.isClickable()) {
        line.addElemAuto(gui.stack(
              gui.button(getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
              gui.labelHighlight(text, getMessageColor(message))));
        line.addElemAuto(gui.labelUnicodeHighlight(u8"➚", getMessageColor(message)));
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
      ret.push_back(gui.label(s, fontSize, color));
    else
      ret.push_back(gui.mainMenuLabelBg(s, menuLabelVPadding));

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
      return gui.stack(std::move(line),
          gui.mouseHighlight(gui.mainMenuLabel(label, menuLabelVPadding), numActive, highlight));
    default:
      return gui.stack(gui.mouseHighlight(
          gui.leftMargin(0, gui.translate(gui.uiHighlightLine(), Vec2(0, 0))),
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
    return gui.stack(
        gui.button([url] { openUrl(url); }),
        gui.getListBuilder()
          .addElem(gui.viewObject(ViewId(viewId)), 30)
          .addElemAuto(gui.labelHighlight(label))
          .buildHorizontalList()
    );
  };
  auto makeWindow = [&](SGuiElem elem) {
    return gui.centerHoriz(gui.translucentBackgroundWithBorder(gui.margins(std::move(elem), 15, 4, 15, 5)));
  };
  auto buttonsLine = makeWindow(gui.getListBuilder()
      .addElemAuto(getButton("keeper4", "News", "https://keeperrl.com/category/News/"))
      .addSpace(100)
      .addElemAuto(getButton("elementalist", "Wiki", "http://keeperrl.com/wiki/index.php?title=Main_Page"))
      .addSpace(100)
      .addElemAuto(getButton("jester", "Discord", "https://discordapp.com/invite/XZfCCs5"))
      .buildHorizontalList());
  if (!personalMessage.empty())
    buttonsLine = gui.getListBuilder(legendLineHeight)
        .addElemAuto(makeWindow(gui.label(personalMessage)))
        .addSpace(10)
        .addElem(std::move(buttonsLine))
        .buildVerticalList();
  return gui.stack(std::move(elem),
      gui.fullScreen(gui.alignment(GuiFactory::Alignment::BOTTOM, gui.bottomMargin(10, std::move(buttonsLine)))));

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
      line = gui.verticalList(std::move(label1), getLineHeight(options[i].getMod()));
    else
      line = std::move(label1.getOnlyElement());
    if (!options[i].getTip().empty())
      line = gui.stack(std::move(line),
          gui.tooltip({options[i].getTip()}));
    if (!options[i].getSecondColumn().empty())
      line = gui.horizontalList(makeVec(std::move(line),
            gui.label(options[i].getSecondColumn(), color)), columnWidth + 80);
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
  auto minionMap = groupByViewId(minions);
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
            gui.labelUnicodeHighlight(u8"✘", Color::RED))), 1);
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
    case MinionActivity::POETRY: return "Poetry";
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
                    .addElem(gui.label(toString(i + 1)))
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
                  ? gui.rightMargin(20, gui.conditional(gui.labelUnicodeHighlight(u8"✓", Color::LIGHT_GRAY),
                       gui.labelUnicodeHighlight(u8"✓", Color::GREEN), [&retAction, task] {
                            return retAction.lock.contains(task.task) ^ *task.locked;}))
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

SGuiElem GuiBuilder::drawAttributesOnPage(vector<SGuiElem> attrs) {
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
  lines.addElem(gui.buttonLabel("Add consumable",
      getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION,
          EquipmentActionInfo{minion.creatureId, {}, none, ItemAction::REPLACE}})));
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
      case PlayerInfo::CONTROL: {
        auto callback = getButtonCallback({UserInputId::CREATURE_CONTROL, minion.creatureId});
        line.push_back(tutorialHighlight
            ? gui.buttonLabelBlink("Control", callback)
            : gui.buttonLabel("Control", callback));
        break;
      }
      case PlayerInfo::RENAME:
        line.push_back(gui.buttonLabel("Rename", [=] {
            if (auto name = getTextInput("Rename minion", minion.firstName, maxFirstNameLength, "Press escape to cancel."))
              callbacks.input({UserInputId::CREATURE_RENAME, RenameActionInfo{minion.creatureId, *name}}); }));
        break;
      case PlayerInfo::BANISH:
        line.push_back(gui.buttonLabel("Banish",
            getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId})));
        break;
      case PlayerInfo::CONSUME:
        line.push_back(gui.buttonLabel("Absorb",
            getButtonCallback({UserInputId::CREATURE_CONSUME, minion.creatureId})));
        break;
      case PlayerInfo::LOCATE:
        line.push_back(gui.buttonLabel("Locate",
            getButtonCallback({UserInputId::CREATURE_LOCATE, minion.creatureId})));
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
  if (auto elem = drawTrainingInfo(minion.experienceInfo))
    leftLines.addElemAuto(std::move(elem));
  if (!minion.spellSchools.empty())
    leftLines.addElem(gui.getListBuilder()
        .addElemAuto(gui.label("Spell schools: ", Color::YELLOW))
        .addElemAuto(gui.label(combine(minion.spellSchools, true)))
        .buildHorizontalList());
  leftLines.addSpace();
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

SGuiElem GuiBuilder::drawChooseNumberMenu(SyncQueue<optional<int>>& queue, const string& title,
    Range range, int initial, int increments) {
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(gui.centerHoriz(gui.label(title)));
  lines.addSpace(legendLineHeight / 2);
  auto currentChoice = make_shared<int>((initial - range.getStart()) / increments);
  auto getCurrent = [range, currentChoice, increments] {
    return range.getStart() + *currentChoice * increments;
  };
  const int sideMargin = 50;
  lines.addElem(gui.leftMargin(sideMargin, gui.rightMargin(sideMargin, gui.stack(
      gui.margins(gui.rectangle(Color::ALMOST_DARK_GRAY), 0, legendLineHeight / 3, 0, legendLineHeight / 3),
      gui.slider(gui.preferredSize(10, 25, gui.rectangle(Color::WHITE)), currentChoice,
          range.getLength() / increments))
  )));
  const int width = 380;
  lines.addElem(gui.stack(
      gui.centerHoriz(gui.labelFun([getCurrent]{ return toString(getCurrent()); }), 30),
      gui.translate(gui.centeredLabel(Renderer::HOR, toString(range.getStart())), Vec2(sideMargin, 0), Vec2(1, 0)),
      gui.translate(gui.centeredLabel(Renderer::HOR, toString(range.getEnd())), Vec2(-sideMargin, 0), Vec2(1, 0),
          GuiFactory::TranslateCorner::TOP_RIGHT)
  ));
  auto confirmFun = [&queue, getCurrent] { queue.push(getCurrent());};
  lines.addElem(gui.centerHoriz(gui.getListBuilder()
      .addElemAuto(gui.stack(
          gui.keyHandler(confirmFun, {gui.getKey(SDL::SDLK_RETURN)}, true),
          gui.buttonLabel("Confirm", confirmFun)))
      .addSpace(15)
      .addElemAuto(gui.buttonLabel("Cancel", [&queue] { queue.push(none);}))
      .buildHorizontalList()
  ));
  return gui.setWidth(width,
      gui.miniWindow(gui.margins(lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawTickBox(shared_ptr<bool> value, const string& title) {
  return gui.stack(
      gui.button([value]{ *value = !*value; }),
      gui.getListBuilder()
          .addElemAuto(
              gui.conditional(gui.labelUnicodeHighlight(u8"✓", Color::GREEN), [value] { return *value; }))
          .addElemAuto(gui.label(title))
          .buildHorizontalList());
}

SGuiElem GuiBuilder::drawBugreportMenu(bool saveFile, function<void(optional<BugReportInfo>)> callback) {
  auto lines = gui.getListBuilder(legendLineHeight);
  const int width = 300;
  const int windowMargin = 15;
  lines.addElem(gui.centerHoriz(gui.label("Submit bug report")));
  auto text = make_shared<string>();
  auto withScreenshot = make_shared<bool>(true);
  auto withSavefile = make_shared<bool>(saveFile);
  lines.addElem(gui.label("Enter bug desciption:"));
  lines.addElemAuto(gui.stack(
        gui.rectangle(Color::GRAY),
        gui.margins(gui.textInput(width - 10, 5, text), 5)));
  lines.addSpace();
  lines.addElem(drawTickBox(withScreenshot, "Include screenshot"));
  if (saveFile)
    lines.addElem(drawTickBox(withSavefile, "Include save file"));
  lines.addElem(gui.centerHoriz(gui.getListBuilder()
      .addElemAuto(gui.buttonLabel("Upload",
          [=] { callback(BugReportInfo{*text, *withSavefile, *withScreenshot});}))
      .addSpace(10)
      .addElemAuto(gui.buttonLabel("Cancel", [callback] { callback(none);}))
      .buildHorizontalList()
  ));
  return gui.setWidth(width + 2 * windowMargin,
      gui.miniWindow(gui.margins(lines.buildVerticalList(), windowMargin), [callback]{ callback(none); }));
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
          if (sites[x][y].viewId[i] == ViewId("canif_tree") || sites[x][y].viewId[i] == ViewId("decid_tree"))
            v.push_back(gui.topMargin(1 * iconScale,
                  gui.viewObject(ViewId("round_shadow"), iconScale, Color(255, 255, 255, 160))));
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
          elem.push_back(gui.viewObject(ViewId("square_highlight"), iconScale,
              getHighlightColor(*sites[pos].getVillainType())));
        if (c.getPlayerPos() == pos && (!marked || !*marked)) // hacky way of checking this is adventurer embark position
          elem.push_back(gui.viewObject(ViewId("square_highlight"), iconScale));
        if (activeFun(pos))
          elem.push_back(gui.stack(
                gui.button([pos, clickFun] { clickFun(pos); }),
                gui.mouseHighlight2(gui.viewObject(ViewId("square_highlight"), iconScale))));
        elem.push_back(gui.topMargin(1 * iconScale,
              gui.viewObject(ViewId("round_shadow"), iconScale, Color(255, 255, 255, 160))));
        if (marked)
          elem.push_back(gui.conditional(gui.viewObject(ViewId("square_highlight"), iconScale),
                [marked, pos] { return *marked == pos;}));
        elem.push_back(gui.topMargin(-2 * iconScale, gui.viewObject(*id, iconScale)));
        if (c.isDefeated(pos))
          elem.push_back(gui.viewObject(ViewId("campaign_defeated"), iconScale));
      } else {
        if (activeFun(pos))
          elem.push_back(gui.stack(
                gui.button([pos, clickFun] { clickFun(pos); }),
                gui.mouseHighlight2(gui.viewObject(ViewId("square_highlight"), iconScale))));
        if (marked)
          elem.push_back(gui.conditional(gui.viewObject(ViewId("square_highlight"), iconScale),
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
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(gui.centerHoriz(gui.buttonLabel("Close", [&] { sem.v(); })));
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
  lines.addSpace(legendLineHeight / 2);
  lines.addElem(gui.centerHoriz(gui.getListBuilder()
        .addElemAuto(gui.conditional(
            gui.buttonLabel("Confirm", [&] { queue.push(*sitePos); }),
            gui.buttonLabelInactive("Confirm"),
            [&] { return !!sitePos; }))
        .addSpace(15)
        .addElemAuto(gui.buttonLabel("Cancel",
            gui.button([&queue] { queue.push(none); }, gui.getKey(SDL::SDLK_ESCAPE), true)))
        .buildHorizontalList()));
  return gui.preferredSize(1000, 600,
                           gui.window(gui.margins(lines.buildVerticalList(), 15), [&queue] { queue.push(none); }));
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
    auto genderList = gui.getListBuilder();
    for (int i : All(avatar.viewId)) {
      auto selectFun = [i, gender] { *gender = i; };
      genderList.addElemAuto(gui.conditional(
          gui.conditional(gui.buttonLabelSelected(capitalFirst(avatar.genderNames[i]), selectFun, false, true),
              gui.buttonLabel(capitalFirst(capitalFirst(avatar.genderNames[i])), selectFun, false, true),
             [gender, i] { return *gender == i; }),
          [=] { return avatarIndex == *chosenAvatar; }));
    }
    genderOptions.push_back(genderList.buildHorizontalListFit(0.2));
  }
  return gui.stack(std::move(genderOptions));
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
      auto elem = gui.getListBuilder()
          .addElemAuto(gui.label("Name: "))
          .addMiddleElem(gui.textField(maxFirstNameLength,
              [=] {
                auto entered = options->getValueString(OptionId::PLAYER_NAME);
                return entered.empty() ?
                    avatar.firstNames[genderIndex][*chosenName % avatar.firstNames[genderIndex].size()] :
                    entered;
              },
              [=] (string s) {
                options->setValue(OptionId::PLAYER_NAME, s);
              }))
          .addSpace(10)
          .addBackElemAuto(gui.buttonLabel("random", [=] { options->setValue(OptionId::PLAYER_NAME, ""_s); ++*chosenName; }))
          .buildHorizontalList();
      firstNameOptions.push_back(gui.conditional(std::move(elem),
          [=]{ return getChosenGender(gender, chosenAvatar, avatars) == genderIndex && avatarIndex == *chosenAvatar; }));
    }
  }
  return gui.stack(std::move(firstNameOptions));
}

SGuiElem GuiBuilder::drawRoleButtons(shared_ptr<PlayerRole> chosenRole, shared_ptr<int> chosenAvatar, shared_ptr<int> avatarPage,
    const vector<View::AvatarData>& avatars) {
  auto roleList = gui.getListBuilder();
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
        gui.conditional(gui.buttonLabelSelected(capitalFirst(getName(role)), chooseFun, false, true),
            gui.buttonLabel(capitalFirst(getName(role)), chooseFun, false, true),
            [chosenRole, role] { return *chosenRole == role; })
    );
  }
  return roleList.buildHorizontalListFit(0.2);
}

constexpr int avatarsPerPage = 10;

SGuiElem GuiBuilder::drawChosenCreatureButtons(PlayerRole role, shared_ptr<int> chosenAvatar, shared_ptr<int> gender, int page,
    const vector<View::AvatarData>& avatars) {
  auto allLines = gui.getListBuilder(legendLineHeight);
  auto line = gui.getListBuilder();
  const int start = page * avatarsPerPage;
  const int end = (page + 1) * avatarsPerPage;
  int processed = 0;
  for (int i : All(avatars)) {
    auto& elem = avatars[i];
    auto viewIdFun = [gender, id = elem.viewId] { return id[min(*gender, id.size() - 1)].back(); };
    if (elem.role == role) {
      if (processed >= start && processed < end)
        line.addElemAuto(gui.stack(
            gui.button([i, chosenAvatar]{ *chosenAvatar = i; }),
            gui.mouseHighlight2(
                gui.rightMargin(10, gui.topMargin(-5, gui.viewObject(viewIdFun, 2))),
                gui.rightMargin(10, gui.conditional2(
                    gui.topMargin(-5, gui.viewObject(viewIdFun, 2)),
                    gui.viewObject(viewIdFun, 2), [=](GuiElem*){ return *chosenAvatar == i;})))
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
      avatarPages.push_back(gui.conditional(
          drawChosenCreatureButtons(role, chosenAvatar, gender, page, avatars),
          [avatarPage, page] { return *avatarPage == page; }));
    maxAvatarPagesHeight = max(maxAvatarPagesHeight, *avatarPages[0]->getPreferredHeight());
    if (numPages > 1) {
      avatarPages.push_back(gui.alignment(GuiFactory::Alignment::BOTTOM_RIGHT,
          gui.translate(
              gui.getListBuilder(24)
                .addElem(gui.conditional2(
                    gui.buttonLabel("<", [avatarPage]{ *avatarPage = max(0, *avatarPage - 1); }),
                    gui.buttonLabelInactive("<"),
                    [=] (GuiElem*) { return *avatarPage > 0; }))
                .addElem(gui.conditional2(
                    gui.buttonLabel(">", [=]{ *avatarPage = min(numPages - 1, *avatarPage + 1); }),
                    gui.buttonLabelInactive(">"),
                    [=] (GuiElem*) { return *avatarPage < numPages - 1; }))
                .buildHorizontalList(),
             Vec2(12, 32), Vec2(48, 30)
          )));
    }
    avatarsForRole.push_back(gui.conditional(gui.stack(std::move(avatarPages)),
        [chosenRole, role] { return *chosenRole == role; }));
  }
  return gui.setHeight(maxAvatarPagesHeight, gui.stack(std::move(avatarsForRole)));
}

SGuiElem GuiBuilder::drawAvatarMenu(SyncQueue<variant<View::AvatarChoice, AvatarMenuOption>>& queue,
    const vector<View::AvatarData>& avatars) {
  auto gender = make_shared<int>(0);
  auto chosenAvatar = make_shared<int>(0);
  auto entered = options->getValueString(OptionId::PLAYER_NAME);
  auto chosenName = make_shared<int>(0);
  auto avatarPage = make_shared<int>(0);
  auto chosenRole = make_shared<PlayerRole>(PlayerRole::KEEPER);
  auto leftLines = gui.getListBuilder(legendLineHeight);
  auto rightLines = gui.getListBuilder(legendLineHeight);
  leftLines.addElem(drawRoleButtons(chosenRole, chosenAvatar, avatarPage, avatars));
  leftLines.addSpace(15);
  leftLines.addElem(drawGenderButtons(avatars, gender, chosenAvatar));
  leftLines.addSpace(15);
  leftLines.addElem(drawFirstNameButtons(avatars, gender, chosenAvatar, chosenName));
  leftLines.addSpace(15);
  rightLines.addElemAuto(drawAvatarsForRole(avatars, avatarPage, chosenAvatar, gender, chosenRole));
  rightLines.addSpace(12);
  rightLines.addElem(gui.labelFun([&avatars, chosenAvatar] {
      return capitalFirst(avatars[*chosenAvatar].name) + ", " + getName(avatars[*chosenAvatar].alignment);}));
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElemAuto(gui.getListBuilder()
      .addElemAuto(gui.rightMargin(30, leftLines.buildVerticalList()))
      .addElemAuto(rightLines.buildVerticalList())
      .buildHorizontalListFit()
  );
  vector<SGuiElem> descriptions;
  for (int avatarIndex : All(avatars)) {
    auto& avatar = avatars[avatarIndex];
    descriptions.push_back(gui.conditional(
        gui.labelMultiLineWidth(avatar.description, legendLineHeight, 530, Renderer::textSize, Color::LIGHT_GRAY),
        [avatarIndex, chosenAvatar] { return avatarIndex == *chosenAvatar; }));
  }
  lines.addBackElem(gui.stack(descriptions), 2 * legendLineHeight);
  lines.addBackElem(gui.centerHoriz(gui.buttonLabel("Start new game",
      [&queue, chosenAvatar, chosenName, gender, &avatars, this] {
        auto chosenGender = getChosenGender(gender, chosenAvatar, avatars);
        auto enteredName = options->getValueString(OptionId::PLAYER_NAME);
        auto& firstNames = avatars[*chosenAvatar].firstNames[chosenGender];
        queue.push(View::AvatarChoice{*chosenAvatar, chosenGender, enteredName.empty() ?
            firstNames[*chosenName % firstNames.size()] :
            enteredName});
      })));
  auto menuLines = gui.getListBuilder(legendLineHeight)
      .addElemAuto(
          gui.preferredSize(600, 350, gui.window(gui.margins(
              lines.buildVerticalList(), 15), [&queue]{ queue.push(AvatarMenuOption::GO_BACK); })));
  auto othersLine = gui.getListBuilder();
  for (auto option : ENUM_ALL(AvatarMenuOption)) {
    othersLine.addElemAuto(
        gui.stack(
              gui.button([&queue, option]{ queue.push(option); }),
              gui.buttonLabelWithMargin(getText(option), false)));
  }
  menuLines.addElem(gui.margins(othersLine.buildHorizontalListFit(), 40, 0, 40, 0), 30);
  return menuLines.buildVerticalList();

}

SGuiElem GuiBuilder::drawPlusMinus(function<void(int)> callback, bool canIncrease, bool canDecrease, bool leftRight) {
  string plus = leftRight ? "<"  : "+";
  string minus = leftRight ? ">"  : "-";
  return gui.margins(gui.getListBuilder()
      .addElem(canIncrease
          ? gui.buttonLabel(plus, [callback] { callback(1); }, false)
          : gui.buttonLabelInactive(plus, false), 10)
      .addSpace(12)
      .addElem(canDecrease
          ? gui.buttonLabel(minus, [callback] { callback(-1); }, false)
          : gui.buttonLabelInactive(minus, false), 10)
      .buildHorizontalList(), 0, 2, 0, 2);
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
  switch (options->getType(id)) {
    case Options::STRING:
      ret = gui.getListBuilder()
          .addElemAuto(gui.label(name + ":"))
          .addMiddleElem(gui.textField(maxFirstNameLength, getValue, [=] (string s) {
            options->setValue(id, s);
            onChanged();
          }))
          .buildHorizontalList();
      break;
    case Options::INT: {
      auto limits = options->getLimits(id);
      int value = options->getIntValue(id);
      ret = gui.getListBuilder()
          .addElem(gui.labelFun([=]{ return name + ": " + getValue(); }), renderer.getTextLength(name) + 20)
          .addBackElemAuto(drawPlusMinus([=] (int v) { options->setValue(id, value + v); onChanged();},
              value < limits->second, value > limits->first, options->hasChoices(id)))
          .buildHorizontalList();
      break;
    }
    case Options::BOOL: {
      bool value = options->getBoolValue(id);
      ret = gui.getListBuilder()
          .addElemAuto(gui.label(name + ": "))
          .addElem(gui.stack(
              gui.button([=]{options->setValue(id, int(!value)); onChanged();}),
              gui.labelFun([getValue]{ return "[" + getValue() + "]";}, Color::LIGHT_BLUE)), 50)
          .buildHorizontalList();
      break;
    }
  }
  return gui.stack(
      gui.tooltip({options->getHint(id).value_or("")}),
      std::move(ret)
  );
}

GuiFactory::ListBuilder GuiBuilder::drawRetiredGames(RetiredGames& retired, function<void()> reloadCampaign,
    optional<int> maxActive, string searchString) {
  auto lines = gui.getListBuilder(legendLineHeight);
  const vector<RetiredGames::RetiredGame>& allGames = retired.getAllGames();
  bool displayActive = !maxActive;
  for (int i : All(allGames)) {
    if (i == retired.getNumLocal() && !displayActive)
      lines.addElem(gui.label(allGames[i].subscribed ? "Subscribed:" : "Online:", Color::YELLOW));
    else
      if (i > 0 && !allGames[i].subscribed && allGames[i - 1].subscribed && !displayActive)
        lines.addElem(gui.label("Online:", Color::YELLOW));
    if (searchString != "" && !contains(toLower(allGames[i].gameInfo.name), toLower(searchString)))
      continue;
    if (retired.isActive(i) == displayActive) {
      auto header = gui.getListBuilder();
      bool maxedOut = !displayActive && retired.getNumActive() >= *maxActive;
      header.addElem(gui.label(allGames[i].gameInfo.name,
          maxedOut ? Color::LIGHT_GRAY : Color::WHITE), 170);
      for (auto& minion : allGames[i].gameInfo.minions)
        header.addElem(drawMinionAndLevel(minion.viewId, minion.level, 1), 25);
      header.addSpace(20);
      if (retired.isActive(i))
        header.addElemAuto(gui.buttonLabel("Remove", [i, reloadCampaign, &retired] { retired.setActive(i, false); reloadCampaign();}));
      else if (!maxedOut)
        header.addElemAuto(gui.buttonLabel("Add", [i, reloadCampaign, &retired] { retired.setActive(i, true); reloadCampaign();}));
      lines.addElem(header.buildHorizontalList());
      if (allGames[i].numTotal > 0)
        lines.addElem(gui.stack(
          gui.tooltip({"Number of times this dungeon has been conquered over how many times it has been loaded."}),
          gui.label("Conquer rate: " + toString(allGames[i].numWon) + "/" + toString(allGames[i].numTotal),
              Renderer::smallTextSize, gui.inactiveText)), legendLineHeight * 2 / 3);
      if (!allGames[i].gameInfo.spriteMods.empty()) {
        lines.addElem(gui.stack(
            gui.tooltip({"These mods may be required to successfully load this dungeon."}),
            gui.label("Requires mods:" + combine(allGames[i].gameInfo.spriteMods, true), Renderer::smallTextSize, gui.inactiveText)),
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
      return gui.labelMultiLine("Warning: you won't be able to retire your dungeon in this mode.",
              legendLineHeight, Renderer::textSize, Color::RED);
  }
}

SGuiElem GuiBuilder::drawCampaignMenu(SyncQueue<CampaignAction>& queue, View::CampaignOptions campaignOptions,
    View::CampaignMenuState& menuState) {
  const auto& campaign = campaignOptions.campaign;
  auto& retiredGames = campaignOptions.retired;
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  GuiFactory::ListBuilder centerLines(gui, getStandardLineHeight());
  GuiFactory::ListBuilder rightLines(gui, getStandardLineHeight());
  int optionMargin = 50;
  centerLines.addElem(gui.centerHoriz(
       gui.label("Game mode: "_s + getGameTypeName(campaign.getType()))));
  rightLines.addElem(gui.leftMargin(-55,
      gui.buttonLabel("Change",
      gui.buttonRect([&queue, this, campaignOptions] (Rectangle bounds) {
          auto lines = gui.getListBuilder(legendLineHeight);
          bool exit = false;
          for (auto& info : campaignOptions.availableTypes) {
            lines.addElem(gui.buttonLabel(getGameTypeName(info.type),
                [&, info] { queue.push({CampaignActionId::CHANGE_TYPE, info.type}); exit = true; }));
            for (auto& desc : info.description)
              lines.addElem(gui.leftMargin(0, gui.label("- " + desc, Color::LIGHT_GRAY, Renderer::smallTextSize)),
                  legendLineHeight * 2 / 3);
            lines.addSpace(legendLineHeight / 3);
          }
          drawMiniMenu(std::move(lines), exit, bounds.bottomLeft(), 350, true);
      }))));
  centerLines.addSpace(10);
  centerLines.addElem(gui.centerHoriz(
            gui.buttonLabel("Help", [&] { menuState.helpText = !menuState.helpText; })));
  lines.addElem(gui.leftMargin(optionMargin, gui.label("World name: " + campaign.getWorldName())));
  auto getDefaultString = [&](OptionId id) -> optional<string> {
    switch(id) {
      case OptionId::PLAYER_NAME:
        return campaignOptions.player->getName().firstOrBare();
      default:
        return none;
    }
  };
  lines.addSpace(10);
  GuiFactory::ListBuilder retiredMenuLines(gui, getStandardLineHeight());
  if (retiredGames) {
    retiredMenuLines.addElem(gui.getListBuilder()
        .addElemAuto(gui.label("Search: "))
        .addElem(gui.textField(10, [ret = campaignOptions.searchString] { return ret; },
            [&queue](string s){ queue.push(CampaignAction(CampaignActionId::SEARCH_RETIRED, std::move(s)));}), 200)
        .addSpace(10)
        .addElemAuto(gui.buttonLabel("X",
            [&queue]{ queue.push(CampaignAction(CampaignActionId::SEARCH_RETIRED, string()));}))
        .buildHorizontalList()
    );
    auto addedDungeons = drawRetiredGames(
        *retiredGames, [&queue] { queue.push(CampaignActionId::UPDATE_MAP);}, none, "");
    int addedHeight = addedDungeons.getSize();
    if (!addedDungeons.isEmpty()) {
      retiredMenuLines.addElem(gui.label("Added:", Color::YELLOW));
      retiredMenuLines.addElem(addedDungeons.buildVerticalList(), addedHeight);
    }
    GuiFactory::ListBuilder retiredList = drawRetiredGames(*retiredGames,
        [&queue] { queue.push(CampaignActionId::UPDATE_MAP);}, options->getIntValue(OptionId::MAIN_VILLAINS), campaignOptions.searchString);
    if (retiredList.isEmpty())
      retiredList.addElem(gui.label("No retired dungeons found :("));
    else
      retiredMenuLines.addElem(gui.label("Local:", Color::YELLOW));
    retiredMenuLines.addElemAuto(retiredList.buildVerticalList());
    lines.addElem(gui.leftMargin(optionMargin,
        gui.buttonLabel("Add retired dungeons", [&menuState] { menuState.retiredWindow = !menuState.retiredWindow;})));
  }
  GuiFactory::ListBuilder optionsLines(gui, getStandardLineHeight());
  if (!campaignOptions.options.empty()) {
    lines.addSpace(10);
    lines.addElem(gui.leftMargin(optionMargin,
        gui.buttonLabel("Customize", [&menuState] { menuState.options = !menuState.options;})));
    for (OptionId id : campaignOptions.options)
      optionsLines.addElem(
          drawOptionElem(id, [&queue, id] { queue.push({CampaignActionId::UPDATE_OPTION, id});},
              getDefaultString(id)));
  }
  lines.addBackElemAuto(gui.centerHoriz(drawCampaignGrid(campaign, nullptr,
        [&campaign](Vec2 pos) { return campaign.canEmbark(pos); }, [](Vec2) {})));
  lines.addSpace(10);
  lines.addBackElem(gui.centerHoriz(gui.getListBuilder()
        .addElemAuto(gui.conditional(
            gui.buttonLabel("Confirm", [&] { queue.push(CampaignActionId::CONFIRM); }),
            gui.buttonLabelInactive("Confirm"),
            [&campaign] { return !!campaign.getPlayerPos(); }))
        .addSpace(20)
        .addElemAuto(gui.buttonLabel("Re-roll map", [&queue] { queue.push(CampaignActionId::REROLL_MAP);}))
        .addSpace(20)
        .addElemAuto(gui.buttonLabel("Go back",
            gui.button([&queue] { queue.push(CampaignActionId::CANCEL); }, gui.getKey(SDL::SDLK_ESCAPE))))
        .buildHorizontalList()));
  if (campaignOptions.warning)
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
  auto addOverlay = [&] (GuiFactory::ListBuilder builder, bool& visible) {
    int size = 500;
    if (!builder.isEmpty())
      interior.push_back(
            gui.conditionalStopKeys(gui.translate(gui.miniWindow2(
                gui.margins(gui.scrollable(gui.topMargin(3, builder.buildVerticalList())), 10),
                [&] { visible = false;}), Vec2(30, 70),
                    Vec2(444, 50 + size)),
            [&visible] { return visible;}));
  };
  addOverlay(std::move(retiredMenuLines), menuState.retiredWindow);
  addOverlay(std::move(optionsLines), menuState.options);
  return
      gui.preferredSize(1000, 705,
      gui.window(gui.margins(gui.stack(std::move(interior)), 5), [&queue] { queue.push(CampaignActionId::CANCEL); }));
}

SGuiElem GuiBuilder::drawCreatureList(const vector<CreatureInfo>& creatures, function<void(UniqueEntity<Creature>::Id)> button) {
  auto minionLines = gui.getListBuilder(getStandardLineHeight() + 10);
  auto line = gui.getListBuilder(60);
  for (auto& elem : creatures) {
    auto icon = gui.margins(gui.stack(gui.viewObject(elem.viewId, 2),
        gui.label(toString((int) elem.bestAttack.value), 20)), 5, 5, 5, 5);
    if (button)
      line.addElemAuto(gui.stack(
            gui.mouseHighlight2(gui.uiHighlight()),
            gui.button([id = elem.uniqueId, button] { button(id); }),
            std::move(icon)));
    else
      line.addElemAuto(std::move(icon));
    if (line.getLength() > 6) {
      minionLines.addElemAuto(gui.centerHoriz(line.buildHorizontalList()));
      minionLines.addSpace(20);
      line.clear();
    }
  }
  if (!line.isEmpty()) {
    minionLines.addElemAuto(gui.centerHoriz(line.buildHorizontalList()));
    minionLines.addSpace(20);
  }
  return minionLines.buildVerticalList();
}

SGuiElem GuiBuilder::drawCreatureInfo(SyncQueue<bool>& queue, const string& title, bool prompt,
    const vector<CreatureInfo>& creatures) {
  auto lines = gui.getListBuilder(getStandardLineHeight());
  lines.addElem(gui.centerHoriz(gui.label(title)));
  const int windowWidth = 540;
  lines.addMiddleElem(gui.scrollable(gui.margins(drawCreatureList(creatures, nullptr), 10)));
  lines.addSpace(15);
  auto bottomLine = gui.getListBuilder()
      .addElemAuto(gui.buttonLabel("Confirm", [&queue] { queue.push(true);}));
  if (prompt) {
    bottomLine.addSpace(20);
    bottomLine.addElemAuto(gui.buttonLabel("Cancel", [&queue] { queue.push(false);}));
  }
  lines.addBackElem(gui.centerHoriz(bottomLine.buildHorizontalList()));
  int margin = 15;
  return gui.setWidth(2 * margin + windowWidth,
      gui.window(gui.margins(lines.buildVerticalList(), margin), [&queue] { queue.push(false); }));
}

SGuiElem GuiBuilder::drawChooseCreatureMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>& queue, const string& title,
      const vector<CreatureInfo>& team, const string& cancelText) {
  auto lines = gui.getListBuilder(getStandardLineHeight());
  lines.addElem(gui.centerHoriz(gui.label(title)));
  const int windowWidth = 480;
  lines.addMiddleElem(gui.scrollable(gui.margins(drawCreatureList(team, [&queue] (auto id) { queue.push(id); }), 10)));
  lines.addSpace(15);
  if (!cancelText.empty())
    lines.addBackElem(gui.centerHoriz(gui.buttonLabel(cancelText, gui.button([&queue] { queue.push(none);}))), legendLineHeight);
  int margin = 15;
  return gui.setWidth(2 * margin + windowWidth,
      gui.window(gui.margins(lines.buildVerticalList(), margin), [&queue] { queue.push(none); }));
}

SGuiElem GuiBuilder::drawModMenu(SyncQueue<optional<ModAction>>& queue, int highlighted, const vector<ModInfo>& mods) {
  auto localItems = gui.getListBuilder(legendLineHeight);
  auto subscribedItems = gui.getListBuilder(legendLineHeight);
  auto onlineItems = gui.getListBuilder(legendLineHeight);
  SGuiElem activeItem;
  shared_ptr<int> chosenMod = make_shared<int>(highlighted);
  vector<SGuiElem> modPages;
  const int margin = 15;
  const int pageWidth = 480;
  const int listWidth = 200;
  for (int i : All(mods)) {
    auto name = mods[i].name;
    auto itemLabel =
    gui.stack(
        gui.conditional(gui.label(name, Color::GREEN), gui.label(name),
            [chosenMod, i] { return *chosenMod == i; }),
        gui.button([chosenMod, i] { *chosenMod = i; })
    );
    if (mods[i].isActive)
      activeItem = std::move(itemLabel);
    else if (mods[i].isLocal)
      localItems.addElem(std::move(itemLabel));
    else if (mods[i].isSubscribed)
      subscribedItems.addElem(std::move(itemLabel));
    else
      onlineItems.addElem(std::move(itemLabel));
    auto lines = gui.getListBuilder(legendLineHeight);
    auto stars = gui.getListBuilder();
    if (mods[i].upvotes + mods[i].downvotes > 0) {
      const int maxStars = 5;
      double rating = double(mods[i].upvotes) / (mods[i].downvotes + mods[i].upvotes);
      for (int j = 0; j < 5; ++j)
        stars.addElemAuto(gui.labelUnicode(j < rating * maxStars ? "★" : "☆", Color::YELLOW));
    }
    lines.addElem(gui.getListBuilder()
        .addElemAuto(gui.label(mods[i].name))
        .addBackElemAuto(stars.buildHorizontalList())
        .buildHorizontalList());
    lines.addElem(gui.label(!mods[i].details.author.empty() ? ("by " + mods[i].details.author) : "",
        Renderer::smallTextSize, Color::LIGHT_GRAY));
    lines.addElemAuto(gui.labelMultiLineWidth(mods[i].details.description, legendLineHeight, pageWidth - 2 * margin));
    auto buttons = gui.getListBuilder();
    for (int j : All(mods[i].actions)) {
      buttons.addElemAuto(
          gui.buttonLabel(mods[i].actions[j], [&queue, i, j] { queue.push(ModAction{i, j}); })
      );
      buttons.addSpace(15);
    }
    if (mods[i].versionInfo.steamId != 0) {
      buttons.addElemAuto(
          gui.buttonLabel("Show in Steam Workshop", [id = mods[i].versionInfo.steamId] {
            openUrl("https://steamcommunity.com/sharedfiles/filedetails/?id=" + toString(id));
          })
      );
      buttons.addSpace(15);
    }
    lines.addBackElem(buttons.buildHorizontalList());
    modPages.push_back(gui.conditional(
        lines.buildVerticalList(),
        [chosenMod, i] { return *chosenMod == i; }
    ));
  }
  auto allItems = gui.getListBuilder(legendLineHeight);
  allItems.addElem(gui.label("Currently active:", Color::YELLOW));
  CHECK(!!activeItem);
  allItems.addElem(std::move(activeItem));
  auto addList = [&] (GuiFactory::ListBuilder& list, const char* title) {
    if (!list.isEmpty()) {
      allItems.addSpace(legendLineHeight / 2);
      allItems.addElem(gui.label(title, Color::YELLOW));
      allItems.addElemAuto(list.buildVerticalList());
    }
  };
  addList(localItems, "Installed:");
  addList(subscribedItems, "Subscribed:");
  addList(onlineItems, "Online:");
  allItems.addElem(gui.buttonLabel("Create new", [&queue] { queue.push(ModAction{-1, 0}); }));
  const int windowWidth = 2 * margin + pageWidth + listWidth;
  return gui.preferredSize(windowWidth, 400,
      gui.window(gui.margins(gui.getListBuilder()
          .addElem(gui.scrollable(allItems.buildVerticalList()), listWidth)
          .addSpace(25)
          .addMiddleElem(gui.stack(std::move(modPages)))
          .buildHorizontalList(), margin), [&queue] { queue.push(none); }));
}

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

SGuiElem GuiBuilder::drawMinimapIcons(const GameInfo& gameInfo) {
  auto tutorialPredicate = [&gameInfo] {
    return gameInfo.tutorial && gameInfo.tutorial->highlights.contains(TutorialHighlight::MINIMAP_BUTTONS);
  };
  Color textColor(209, 181, 130);
  auto lines = gui.getListBuilder(legendLineHeight);
  if (auto& info = gameInfo.currentLevel) {
    auto getButton = [&](bool enabled, string label, UserInputId inputId) {
      auto ret = gui.preferredSize(legendLineHeight, legendLineHeight, gui.stack(
          gui.margins(gui.rectangle(Color(56, 36, 0), Color(57, 41, 0)), 2),
          gui.centerHoriz(gui.topMargin(-2, gui.mouseHighlight2(
                                        gui.label(label, 24, enabled ? Color::YELLOW : Color::DARK_GRAY),
                                        gui.label(label, 24, enabled ? textColor : Color::DARK_GRAY))))
      ));
      if (enabled)
        ret = gui.stack(
            std::move(ret),
            gui.button(getButtonCallback(inputId)));
      return ret;
    };
    lines.addElem(gui.stack(
        gui.stopMouseMovement(),
        gui.rectangle(Color(47, 31, 0), Color::BLACK),
        gui.getListBuilder()
          .addElemAuto(getButton(info->canScrollUp, "<", UserInputId::SCROLL_UP_STAIRS))
          .addMiddleElem(gui.topMargin(3, gui.centerHoriz(gui.label(info->levelName, textColor))))
          .addBackElemAuto(getButton(info->canScrollDown, ">", UserInputId::SCROLL_DOWN_STAIRS))
          .buildHorizontalList()
    ));
  }
  return lines.addElemAuto(
      gui.centerHoriz(gui.minimapBar(
        gui.preferredSize(48, 48, gui.stack(
            getHintCallback({"Open world map. You can also press 't'."}),
            gui.mouseHighlight2(gui.icon(GuiFactory::IconId::MINIMAP_WORLD2), gui.icon(GuiFactory::IconId::MINIMAP_WORLD1)),
            gui.conditional(gui.blink(gui.icon(GuiFactory::IconId::MINIMAP_WORLD2)), tutorialPredicate),
            gui.button(getButtonCallback(UserInputId::DRAW_WORLD_MAP), gui.getKey(SDL::SDLK_t))
        )),
        gui.preferredSize(48, 48, gui.stack(
            getHintCallback({"Scroll to your character. You can also press 'k'."}),
            gui.mouseHighlight2(gui.icon(GuiFactory::IconId::MINIMAP_CENTER2), gui.icon(GuiFactory::IconId::MINIMAP_CENTER1)),
            gui.conditional(gui.blink(gui.icon(GuiFactory::IconId::MINIMAP_CENTER2)), tutorialPredicate),
            gui.button(getButtonCallback(UserInputId::SCROLL_TO_HOME), gui.getKey(SDL::SDLK_k))
            ))
  ))).buildVerticalList();
}

Rectangle GuiBuilder::getTextInputPosition() {
  Vec2 center = renderer.getSize() / 2;
  return Rectangle(center - Vec2(300, 129), center + Vec2(300, 129));
}

SGuiElem GuiBuilder::getTextContent(const string& title, const string& value, const string& hint) {
  auto lines = gui.getListBuilder(legendLineHeight);
  lines.addElem(gui.label(capitalFirst(title)));
  lines.addElem(
      gui.variableLabel([&] { return value + "_"; }, legendLineHeight), 2 * legendLineHeight);
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
  auto levelBounds = view->getCreatureViewLevel()->getBounds();
  miniMap->update(levelBounds, view, renderer);
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
