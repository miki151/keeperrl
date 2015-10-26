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

using sf::Color;
using sf::String;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Text;
using sf::Font;
using sf::Event;
using sf::RectangleShape;
using sf::CircleShape;
using sf::Vector2f;
using sf::Vector2u;
using sf::Image;
using sf::Sprite;
using sf::Texture;
using sf::Keyboard;
using sf::Mouse;

GuiBuilder::GuiBuilder(Renderer& r, GuiFactory& g, Clock* c, Options* o, Callbacks call)
    : renderer(r), gui(g), clock(c), options(o), callbacks(call), gameSpeed(GameSpeed::NORMAL) {
}

void GuiBuilder::reset() {
  activeBuilding = 0;
  activeLibrary = -1;
  gameSpeed = GameSpeed::NORMAL;
  numSeenVillains = -1;
}

void GuiBuilder::setMapGui(MapGui* g) {
  mapGui = g;
}

const int legendLineHeight = 30;
const int titleLineHeight = legendLineHeight + 8;

int GuiBuilder::getStandardLineHeight() const {
  return legendLineHeight;
}

PGuiElem GuiBuilder::getHintCallback(const vector<string>& s) {
  return gui.mouseOverAction([this, s]() { callbacks.hint({s}); });
}

function<void()> GuiBuilder::getButtonCallback(UserInput input) {
  return [this, input]() { callbacks.input(input); };
}

void GuiBuilder::setTilesOk(bool ok) {
  tilesOk = ok;
}

void GuiBuilder::setCollectiveTab(CollectiveTab t) {
  collectiveTab = t;
  if (t != CollectiveTab::MINIONS) {
    closeOverlayWindows();
  }
}

GuiBuilder::CollectiveTab GuiBuilder::getCollectiveTab() const {
  return collectiveTab;
}

void GuiBuilder::closeOverlayWindows() {
  callbacks.input({UserInputId::CREATURE_BUTTON, -1});
}

int GuiBuilder::getActiveBuilding() const {
  return activeBuilding;
}

int GuiBuilder::getActiveLibrary() const {
  return activeLibrary;
}

PGuiElem GuiBuilder::drawCost(pair<ViewId, int> cost, ColorId color) {
  string costText = toString(cost.second);
  return GuiFactory::ListBuilder(gui)
      .addElemAuto(gui.rightMargin(5, gui.label(costText, colors[color])))
      .addElem(gui.viewObject(cost.first, tilesOk), 25)
      .buildHorizontalList();
}

static Keyboard::Key getKey(char c) {
  switch (c) {
    case 'a': return Keyboard::A;
    case 'b': return Keyboard::B;
    case 'c': return Keyboard::C;
    case 'd': return Keyboard::D;
    case 'e': return Keyboard::E;
    case 'f': return Keyboard::F;
    case 'g': return Keyboard::G;
    case 'h': return Keyboard::H;
    case 'i': return Keyboard::I;
    case 'j': return Keyboard::J;
    case 'k': return Keyboard::K;
    case 'l': return Keyboard::L;
    case 'm': return Keyboard::M;
    case 'n': return Keyboard::N;
    case 'o': return Keyboard::O;
    case 'p': return Keyboard::P;
    case 'q': return Keyboard::Q;
    case 'r': return Keyboard::R;
    case 's': return Keyboard::S;
    case 't': return Keyboard::T;
    case 'u': return Keyboard::U;
    case 'v': return Keyboard::V;
    case 'w': return Keyboard::W;
    case 'x': return Keyboard::X;
    case 'y': return Keyboard::Y;
    case 'z': return Keyboard::Z;
    case 0: return Keyboard::KeyCount;
  }
  FAIL << "Unrecognized key " << c;
  return Keyboard::F13;
}

Event::KeyEvent GuiBuilder::getHotkeyEvent(char c) {
  return Event::KeyEvent{getKey(c), options->getBoolValue(OptionId::WASD_SCROLLING), false, false, false};
}

PGuiElem GuiBuilder::getButtonLine(CollectiveInfo::Button button, int num, int& active,
    CollectiveTab tab) {
  GuiFactory::ListBuilder line(gui);
  line.addElem(gui.viewObject(button.viewId, tilesOk), 35);
  if (button.state != CollectiveInfo::Button::ACTIVE)
    line.addElem(gui.label(button.name + " " + button.count, colors[ColorId::GRAY], button.hotkey), 100);
  else
    line.addElem(gui.conditional(
          gui.label(button.name + " " + button.count, colors[ColorId::GREEN], button.hotkey),
          gui.label(button.name + " " + button.count, colors[ColorId::WHITE], button.hotkey),
          [=, &active] (GuiElem*) { return active == num; }), 100);
  if (button.cost)
    line.addBackElemAuto(drawCost(*button.cost));
  function<void()> buttonFun;
  if (button.state != CollectiveInfo::Button::INACTIVE)
    buttonFun = [this, &active, num, tab] {
      active = num;
      hideBuildingOverlay = false;
      setCollectiveTab(tab);
    };
  else {
    buttonFun = [] {};
  }
  return gui.stack(
      getHintCallback({capitalFirst(button.help)}),
      gui.button(buttonFun, getHotkeyEvent(button.hotkey)),
      line.buildHorizontalList());
}

vector<PGuiElem> GuiBuilder::drawButtons(vector<CollectiveInfo::Button> buttons, int& active,
    CollectiveTab tab) {
  vector<PGuiElem> elems;
  vector<PGuiElem> invisible;
  vector<CollectiveInfo::Button> groupedButtons;
  for (int i = 0; i <= buttons.size(); ++i) {
    if (!groupedButtons.empty() && (i == buttons.size() || buttons[i].groupName != groupedButtons[0].groupName)) {
      int firstItem = i - groupedButtons.size();
      int lastItem = i - 1;
      CollectiveInfo::Button button1 = groupedButtons.front();
      function<void()> buttonFun = [=, &active] {
        active = firstItem;
        hideBuildingOverlay = false;
      };
      vector<PGuiElem> line;
      line.push_back(gui.viewObject(button1.viewId, tilesOk));
      line.push_back(gui.label(groupedButtons.front().groupName,
            active <= lastItem && active >= firstItem ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
      elems.push_back(gui.stack(
            gui.button(buttonFun),
            gui.horizontalList(std::move(line), 35)));
      for (int groupedInd : All(groupedButtons))
        invisible.push_back(getButtonLine(groupedButtons[groupedInd],
              firstItem + groupedInd, active, tab));
      groupedButtons.clear();
    }
    if (i == buttons.size())
      break;
    if (!buttons[i].groupName.empty())
      groupedButtons.push_back(buttons[i]);
    else
      elems.push_back(getButtonLine(buttons[i], i, active, tab));
  }
  elems.push_back(gui.invisible(gui.stack(std::move(invisible))));
  return elems;
}

PGuiElem GuiBuilder::drawBuildings(CollectiveInfo& info) {
  return gui.scrollable(gui.verticalList(drawButtons(info.buildings, activeBuilding, CollectiveTab::BUILDINGS),
      legendLineHeight), &buildingsScroll, &scrollbarsHeld);
}

PGuiElem GuiBuilder::getStandingGui(double standing) {
  sf::Color color = standing >= 0 ? sf::Color((1. - standing) * 255, 255, (1. - standing) * 255)
    : sf::Color(255, (1. + standing) * 255, (1. + standing) * 255);
  if (standing < -0.33)
    return gui.label("bad", color);
  if (standing < 0.33)
    return gui.label("neutral", color);
  else
    return gui.label("good", color);
}

PGuiElem GuiBuilder::drawDeities(CollectiveInfo& info) {
  vector<PGuiElem> lines;
  for (int i : All(info.deities)) {
    lines.push_back(gui.stack(
          gui.button(getButtonCallback(UserInput(UserInputId::DEITIES, i))),
          gui.label(capitalFirst(info.deities[i].name), colors[ColorId::WHITE])));
    lines.push_back(gui.margins(gui.horizontalList(makeVec<PGuiElem>(
              gui.label("standing: ", colors[ColorId::WHITE]),
              getStandingGui(info.deities[i].standing)), 85), 40, 0, 0, 0));
  }
  return gui.verticalList(std::move(lines), legendLineHeight);
}

PGuiElem GuiBuilder::drawTechnology(CollectiveInfo& info) {
  vector<PGuiElem> lines = drawButtons(info.libraryButtons, activeLibrary, CollectiveTab::TECHNOLOGY);
  for (int i : All(info.techButtons)) {
    vector<PGuiElem> line;
    line.push_back(gui.viewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, ""), tilesOk));
    line.push_back(gui.label(info.techButtons[i].name, colors[ColorId::WHITE], info.techButtons[i].hotkey));
    lines.push_back(gui.stack(gui.button(
          getButtonCallback(UserInput(UserInputId::TECHNOLOGY, i)), getHotkeyEvent(info.techButtons[i].hotkey)),
          gui.horizontalList(std::move(line), 35)));
  }
  return gui.verticalList(std::move(lines), legendLineHeight);
}

PGuiElem GuiBuilder::drawKeeperHelp() {
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
  vector<PGuiElem> lines;
  for (string line : helpText)
    lines.push_back(gui.label(line, colors[ColorId::LIGHT_BLUE]));
  return gui.verticalList(std::move(lines), legendLineHeight);
}

int GuiBuilder::FpsCounter::getSec() {
  return clock.getElapsedTime().asSeconds();
}

void GuiBuilder::FpsCounter::addTick() {
  if (getSec() == curSec)
    ++curFps;
  else {
    lastFps = curFps;
    curSec = getSec();
    curFps = 0;
  }
}

int GuiBuilder::FpsCounter::getFps() {
  return lastFps;
}

void GuiBuilder::addFpsCounterTick() {
  fpsCounter.addTick();
}

void GuiBuilder::addUpsCounterTick() {
  upsCounter.addTick();
}

const int resourceSpace = 110;

PGuiElem GuiBuilder::drawBottomBandInfo(GameInfo& gameInfo) {
  CollectiveInfo& info = gameInfo.collectiveInfo;
  GameSunlightInfo& sunlightInfo = gameInfo.sunlightInfo;
  vector<PGuiElem> topLine;
  for (int i : All(info.numResource)) {
    vector<PGuiElem> res;
    res.push_back(gui.viewObject(info.numResource[i].viewId, tilesOk));
    res.push_back(gui.label(toString<int>(info.numResource[i].count),
          info.numResource[i].count >= 0 ? colors[ColorId::WHITE] : colors[ColorId::RED]));
    topLine.push_back(gui.stack(getHintCallback({info.numResource[i].name}),
          gui.horizontalList(std::move(res), 30)));
  }
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(getTurnInfoGui(gameInfo.time));
  bottomLine.push_back(getSunlightInfoGui(sunlightInfo));
  bottomLine.push_back(gui.label("population: " + toString(gameInfo.collectiveInfo.minionCount) + " / " +
        toString(gameInfo.collectiveInfo.minionLimit)));
  int numTop = topLine.size();
  int numBottom = bottomLine.size();
  return gui.verticalList(makeVec<PGuiElem>(
      gui.centerHoriz(gui.horizontalList(std::move(topLine), resourceSpace), numTop * resourceSpace),
      gui.centerHoriz(gui.horizontalList(std::move(bottomLine), 140, 3), numBottom * 140)), 28);
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

PGuiElem GuiBuilder::drawRightBandInfo(CollectiveInfo& info, VillageInfo& villageInfo) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
      gui.icon(gui.BUILDING),
      gui.icon(gui.MINION),
      gui.icon(gui.LIBRARY),
      gui.stack(gui.icon(gui.DIPLOMACY),
          numSeenVillains < villageInfo.villages.size() ? gui.icon(gui.CLICK_TAB) : gui.empty()),
      gui.icon(gui.HELP));
  for (int i : All(buttons)) {
    buttons[i] = gui.stack(
        std::move(buttons[i]),
        gui.button([this, i]() { setCollectiveTab(CollectiveTab(i)); }),
        gui.conditional(gui.border2(), [this, i] (GuiElem*) { return int(collectiveTab) == i;}));
  }
  vector<pair<CollectiveTab, PGuiElem>> elems = makeVec<pair<CollectiveTab, PGuiElem>>(
      make_pair(CollectiveTab::MINIONS, drawMinions(info)),
      make_pair(CollectiveTab::BUILDINGS, drawBuildings(info)),
      make_pair(CollectiveTab::KEY_MAPPING, drawKeeperHelp()),
      make_pair(CollectiveTab::TECHNOLOGY, drawTechnology(info)),
      make_pair(CollectiveTab::VILLAGES, drawVillages(villageInfo)));
  vector<PGuiElem> tabs;
  for (auto& elem : elems) {
    auto tab = elem.first;
    tabs.push_back(gui.conditional(std::move(elem.second), [tab, this] (GuiElem*) { return tab == collectiveTab;}));
  }
  PGuiElem main = gui.stack(std::move(tabs));
  main = gui.margins(std::move(main), 15, 15, 15, 5);
  int numButtons = buttons.size();
  PGuiElem butGui = gui.margins(
      gui.centerHoriz(gui.horizontalList(std::move(buttons), 50), numButtons * 50), 0, 5, 0, 5);
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(gui.stack(
      gui.horizontalList(makeVec<PGuiElem>(
          gui.label("speed:"),
          gui.label([this] { return getCurrentGameSpeedName();},
              [this] { return colors[clock->isPaused() ? ColorId::RED : ColorId::WHITE]; })), 60),
      gui.button([&] { gameSpeedDialogOpen = !gameSpeedDialogOpen; })));
  bottomLine.push_back(
      gui.label([this]()->string { return "FPS " + toString(fpsCounter.getFps()) + " / " + toString(upsCounter.getFps()); },
      colors[ColorId::WHITE]));
  PGuiElem bottomElem = gui.horizontalList(std::move(bottomLine), 160);
  main = gui.margin(gui.margins(std::move(bottomElem), 25, 0, 0, 0),
      std::move(main), 18, gui.BOTTOM);
  return gui.margin(std::move(butGui), std::move(main), 55, gui.TOP);
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

static Event::KeyEvent getHotkey(GuiBuilder::GameSpeed speed) {
  switch (speed) {
    case GuiBuilder::GameSpeed::SLOW: return Event::KeyEvent{Keyboard::Num1};
    case GuiBuilder::GameSpeed::NORMAL: return Event::KeyEvent{Keyboard::Num2};
    case GuiBuilder::GameSpeed::FAST: return Event::KeyEvent{Keyboard::Num3};
    case GuiBuilder::GameSpeed::VERY_FAST: return Event::KeyEvent{Keyboard::Num4};
  }
}

void GuiBuilder::drawGameSpeedDialog(vector<OverlayInfo>& overlays) {
  vector<PGuiElem> lines;
  int keyMargin = 95;
  lines.push_back(gui.stack(
        gui.horizontalList(makeVec<PGuiElem>(
            gui.label("pause"),
            gui.label("[space]")), keyMargin),
        gui.button([=] {
            if (clock->isPaused()) 
              clock->cont();
            else
              clock->pause();
            gameSpeedDialogOpen = false;
            }, Event::KeyEvent{Keyboard::Space})));
  for (GameSpeed speed : ENUM_ALL(GameSpeed)) {
    Color color = colors[speed == gameSpeed ? ColorId::GREEN : ColorId::WHITE];
    lines.push_back(gui.stack(gui.horizontalList(makeVec<PGuiElem>(
             gui.label(getGameSpeedName(speed), color),
             gui.label("'" + string(1, getHotkeyChar(speed)) + "' ", color)), keyMargin),
          gui.button([=] { gameSpeed = speed; gameSpeedDialogOpen = false; clock->cont();},
            getHotkey(speed))));
  }
  reverse(lines.begin(), lines.end());
  int margin = 20;
  Vec2 size(150 + 2 * margin, legendLineHeight * lines.size() - 10 + 2 * margin);
  PGuiElem dialog = gui.miniWindow(
      gui.margins(gui.verticalList(std::move(lines), legendLineHeight), margin, margin, margin, margin));
  overlays.push_back({gui.conditional(std::move(dialog), [this] { return gameSpeedDialogOpen; }), size,
      OverlayInfo::GAME_SPEED});
}

PGuiElem GuiBuilder::getSunlightInfoGui(GameSunlightInfo& sunlightInfo) {
  vector<PGuiElem> line;
  Color color = sunlightInfo.description == "day" ? colors[ColorId::WHITE] : colors[ColorId::LIGHT_BLUE];
  line.push_back(gui.label(sunlightInfo.description, color));
  line.push_back(gui.label("[" + toString(sunlightInfo.timeRemaining) + "]", color));
  return gui.stack(
    getHintCallback({sunlightInfo.description == "day"
      ? "Time remaining till nightfall." : "Time remaining till day."}),
    gui.horizontalList(std::move(line), renderer.getTextLength(sunlightInfo.description) + 5));
}

PGuiElem GuiBuilder::getTurnInfoGui(int turn) {
  return gui.stack(getHintCallback({"Current turn."}),
      gui.label("T: " + toString(turn), colors[ColorId::WHITE]));
}

static Color getBonusColor(int bonus) {
  if (bonus < 0)
    return colors[ColorId::RED];
  if (bonus > 0)
    return colors[ColorId::GREEN];
  return colors[ColorId::WHITE];
}

typedef PlayerInfo::AttributeInfo::Id AttrId;

static GuiFactory::IconId getAttrIcon(AttrId id) {
  switch (id) {
    case AttrId::ATT: return GuiFactory::STAT_ATT;
    case AttrId::DEF: return GuiFactory::STAT_DEF;
    case AttrId::STR: return GuiFactory::STAT_STR;
    case AttrId::DEX: return GuiFactory::STAT_DEX;
    case AttrId::ACC: return GuiFactory::STAT_ACC;
    case AttrId::SPD: return GuiFactory::STAT_SPD;
  }
}

vector<PGuiElem> GuiBuilder::drawPlayerAttributes(const vector<PlayerInfo::AttributeInfo>& attr) {
  vector<PGuiElem> ret;
  for (auto& elem : attr)
    ret.push_back(gui.stack(getTooltip({elem.name, elem.help}),
        gui.horizontalList(makeVec<PGuiElem>(
          gui.icon(getAttrIcon(elem.id)),
          gui.margins(gui.label(toString(elem.value), getBonusColor(elem.bonus)), 0, 2, 0, 0)), 30)));
  return ret;
}

PGuiElem GuiBuilder::drawBottomPlayerInfo(GameInfo& gameInfo) {
  PlayerInfo& info = gameInfo.playerInfo;
  vector<PGuiElem> topLine = drawPlayerAttributes(info.attributes);
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(getTurnInfoGui(gameInfo.time));
  bottomLine.push_back(getSunlightInfoGui(gameInfo.sunlightInfo));
  int numTop = topLine.size();
  int numBottom = bottomLine.size();
  return gui.verticalList(makeVec<PGuiElem>(
      gui.centerHoriz(gui.horizontalList(std::move(topLine), resourceSpace), numTop * resourceSpace),
      gui.centerHoriz(gui.horizontalList(std::move(bottomLine), 140, 3), numBottom * 140)), 28);
}

static int viewObjectWidth = 27;
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
  return out;
}

PGuiElem GuiBuilder::getItemLine(const ItemInfo& item, function<void(Rectangle)> onClick,
    function<void()> onMultiClick) {
  GuiFactory::ListBuilder line(gui);
  int leftMargin = -4;
  if (item.locked) {
    line.addElem(gui.viewObject(ViewId::KEY, tilesOk), viewObjectWidth);
    leftMargin -= viewObjectWidth - 3;
  }
  line.addElem(gui.viewObject(item.viewId, tilesOk), viewObjectWidth);
  Color color = colors[item.equiped ? ColorId::GREEN : (item.pending || item.unavailable) ?
      ColorId::GRAY : ColorId::WHITE];
  if (item.number > 1)
    line.addElemAuto(gui.rightMargin(8, gui.label(toString(item.number), color)));
  if (!item.name.empty())
    line.addElemAuto(gui.label(item.name, color));
  else
    line.addElem(gui.empty(), 130);
  for (auto& elem : line.getAllElems())
    elem = gui.stack(gui.button(onClick), std::move(elem), getTooltip(getItemHint(item)));
  if (item.owner) {
    line.addBackElem(gui.viewObject(item.owner->viewId, tilesOk), viewObjectWidth);
    line.addBackElem(gui.label("L:" + toString(item.owner->expLevel)), 60);
  }
  if (item.price)
    line.addBackElemAuto(drawCost(*item.price));
  if (onMultiClick && item.number > 1) {
    line.addBackElem(gui.stack(
        gui.label("[#]"),
        gui.button(onMultiClick),
        getTooltip({"Click to choose how many to pick up."})), 25);
  }
  return gui.margins(line.buildHorizontalList(), leftMargin, 0, 0, 0);
}

PGuiElem GuiBuilder::getTooltip(const vector<string>& text) {
  return gui.conditional(gui.tooltip(text), [this] (GuiElem*) { return !disableTooltip;});
}

const int listLineHeight = 30;
const int listBrokenLineHeight = 24;

int GuiBuilder::getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

void GuiBuilder::drawPlayerOverlay(vector<OverlayInfo>& ret, PlayerInfo& info) {
  if (info.lyingItems.empty()) {
    playerOverlayFocused = false;
    itemIndex = -1;
    return;
  }
  if (lastPlayerPositionHash && lastPlayerPositionHash != info.positionHash) {
    playerOverlayFocused = false;
    itemIndex = -1;
  }
  lastPlayerPositionHash = info.positionHash;
  vector<PGuiElem> lines;
  const int maxElems = 6;
  const string title = "Click or press [Enter]:";
  int numElems = min<int>(maxElems, info.lyingItems.size());
  Vec2 size = Vec2(60 + renderer.getTextLength(title), (1 + numElems) * legendLineHeight);
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
  PGuiElem content;
  if (totalElems == 1 && !playerOverlayFocused)
    content = gui.stack(
        gui.margin(
          gui.leftMargin(3, gui.label(title, colors[ColorId::YELLOW])),
          gui.scrollable(gui.verticalList(std::move(lines), legendLineHeight), &lyingItemsScroll),
          legendLineHeight, GuiFactory::TOP),
        gui.keyHandler([=] { callbacks.input({UserInputId::PICK_UP_ITEM, 0});},
            {{Keyboard::Return}, {Keyboard::Numpad5}}, true));
  else
    content = gui.stack(makeVec<PGuiElem>(
          gui.focusable(gui.stack(
              gui.keyHandler([=] { callbacks.input({UserInputId::PICK_UP_ITEM, itemIndex});},
                {{Keyboard::Return}, {Keyboard::Numpad5}}, true),
              gui.keyHandler([=] { itemIndex = (itemIndex + 1) % totalElems;
                lyingItemsScroll = getScrollPos(itemIndex, totalElems - 1);},
                {{Keyboard::Down}, {Keyboard::Numpad2}}, true),
              gui.keyHandler([=] { itemIndex = (itemIndex + totalElems - 1) % totalElems;
                lyingItemsScroll = getScrollPos(itemIndex, totalElems - 1); },
                {{Keyboard::Up}, {Keyboard::Numpad8}}, true)),
            {{Keyboard::Return}, {Keyboard::Numpad5}}, {{Keyboard::Escape}}, playerOverlayFocused),
          gui.keyHandler([=] { if (!playerOverlayFocused) itemIndex = 0; },
            {{Keyboard::Return}, {Keyboard::Numpad5}}),
          gui.keyHandler([=] { itemIndex = -1; }, {{Keyboard::Escape}}),
          gui.margin(
            gui.leftMargin(3, gui.label(title, colors[ColorId::YELLOW])),
            gui.scrollable(gui.verticalList(std::move(lines), legendLineHeight), &lyingItemsScroll),
            legendLineHeight, GuiFactory::TOP)));
  int margin = 14;
  content = gui.stack(
      gui.conditional(gui.stack(gui.fullScreen(gui.darken()), gui.miniWindow()), gui.translucentBackground(),
        [=] (GuiElem*) { return playerOverlayFocused;}),
      gui.margins(std::move(content), margin, margin, margin, margin));
  ret.push_back({std::move(content), size + Vec2(margin, margin) * 2, OverlayInfo::TOP_RIGHT});
}

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

PGuiElem GuiBuilder::drawPlayerHelp(PlayerInfo& info) {
  vector<PGuiElem> lines;
  vector<KeyInfo> bottomKeys = {
      { "Enter", "Interact or pick up", {Keyboard::Return}},
      { "U", "Leave minion", {Keyboard::U}},
      { "C", "Chat with someone", {Keyboard::C}},
      { "H", "Hide", {Keyboard::H}},
      { "P", "Pay debt", {Keyboard::P}},
      { "M", "Message history", {Keyboard::M}},
      { "Space", "Wait", {Keyboard::Space}},
  };
  vector<string> help = {
      "Move around with number pad.",
      "Extended attack: ctrl + arrow.",
      "Fast travel: ctrl + arrow.",
      "Fire arrows: alt + arrow.",
      "",
  };
  for (auto& elem : help)
    lines.push_back(gui.label(elem, colors[ColorId::LIGHT_BLUE]));
  for (int i : All(bottomKeys)) {
    string text = "[" + bottomKeys[i].keyDesc + "] " + bottomKeys[i].action;
    Event::KeyEvent key = bottomKeys[i].event;
    lines.push_back(gui.stack(
          gui.label(text, colors[ColorId::LIGHT_BLUE]),
          gui.button([this, key]() { callbacks.keyboard(key);})));
  }
  return gui.verticalList(std::move(lines), legendLineHeight);
}

static string getActionText(ItemAction a) {
  switch (a) {
    case ItemAction::DROP: return "drop";
    case ItemAction::DROP_MULTI: return "drop some";
    case ItemAction::GIVE: return "give";
    case ItemAction::EQUIP: return "equip";
    case ItemAction::THROW: return "throw";
    case ItemAction::UNEQUIP: return "remove";
    case ItemAction::APPLY: return "apply";
    case ItemAction::REPLACE: return "replace";
    case ItemAction::LOCK: return "lock";
    case ItemAction::UNLOCK: return "unlock";
  }
}

void GuiBuilder::drawMiniMenu(GuiFactory::ListBuilder elems, bool& exit, Vec2 menuPos, int width) {
  if (elems.isEmpty())
    return;
  int contentHeight = elems.getSize();
  int margin = 15;
  PGuiElem menu = gui.stack(
      gui.reverseButton([&exit] { exit = true; }, {{Keyboard::Escape}}),
      gui.miniWindow(gui.leftMargin(margin, gui.topMargin(margin, elems.buildVerticalList()))));
  menu->setBounds(Rectangle(menuPos, menuPos + Vec2(width + 2 * margin, contentHeight + 2 * margin)));
  PGuiElem bg = gui.darken();
  bg->setBounds(renderer.getSize());
  while (1) {
    callbacks.refreshScreen();
    bg->render(renderer);
    menu->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {menu.get()});
      if (exit)
        return;
    }
  }

}

optional<ItemAction> GuiBuilder::getItemChoice(const ItemInfo& itemInfo, Vec2 menuPos,
    bool autoDefault) {
  if (itemInfo.actions.empty())
    return none;
  if (itemInfo.actions.size() == 1 && autoDefault)
    return itemInfo.actions[0];
  renderer.flushEvents(Event::KeyPressed);
  int contentHeight;
  int choice = -1;
  int index = 0;
  disableTooltip = true;
  DestructorFunction dFun([this] { disableTooltip = false; });
  vector<string> options = transform2<string>(itemInfo.actions, bindFunction(getActionText));
  options.push_back("cancel");
  int count = options.size();
  PGuiElem stuff = drawListGui("", ListElem::convert(options), MenuType::NORMAL, &contentHeight, &index, &choice);
  stuff = gui.miniWindow(gui.margins(std::move(stuff), 0, 0, 0, 0));
  Vec2 size(150, options.size() * listLineHeight + 35);
  menuPos.x = min(menuPos.x, renderer.getSize().x - size.x);
  menuPos.y = min(menuPos.y, renderer.getSize().y - size.y);
  while (1) {
    callbacks.refreshScreen();
    stuff->setBounds(Rectangle(menuPos, menuPos + size));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {stuff.get()});
      if (choice > -1 && index > -1) {
        if (index < itemInfo.actions.size())
          return itemInfo.actions[index];
        else
          return none;
      }
      if (choice == -100)
        return none;
      if (event.type == Event::MouseButtonPressed && 
          !Vec2(event.mouseButton.x, event.mouseButton.x).inRectangle(stuff->getBounds()))
        return none;
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::Numpad8:
          case Keyboard::Up: index = (index - 1 + count) % count; break;
          case Keyboard::Numpad2:
          case Keyboard::Down: index = (index + 1 + count) % count; break;
          case Keyboard::Numpad5:
          case Keyboard::Return: 
              if (index > -1) {
                if (index < itemInfo.actions.size())
                  return itemInfo.actions[index];
              }
          case Keyboard::Escape: return none;
          default: break;
        }
    }
  }
}

vector<PGuiElem> GuiBuilder::drawSkillsList(const PlayerInfo& info) {
  vector<PGuiElem> lines;
  if (!info.skills.empty()) {
    lines.push_back(gui.label("Skills", colors[ColorId::YELLOW]));
    for (auto& elem : info.skills)
      lines.push_back(gui.stack(getTooltip({elem.help}),
            gui.label(capitalFirst(elem.name), colors[ColorId::WHITE])));
    lines.push_back(gui.empty());
  }
  return lines;
}

const int spellsPerRow = 5;
const Vec2 spellIconSize = Vec2(47, 47);

PGuiElem GuiBuilder::getSpellIcon(const PlayerInfo::Spell& spell, bool active) {
  vector<PGuiElem> ret = makeVec<PGuiElem>(
      gui.spellIcon(spell.id));
  if (spell.timeout) {
    ret.push_back(gui.darken());
    ret.push_back(gui.centeredLabel(Renderer::HOR_VER, toString(*spell.timeout)));
  } else
  if (active)
    ret.push_back(gui.button(getButtonCallback({UserInputId::CAST_SPELL, spell.id})));
  ret.push_back(getTooltip({spell.name, spell.help}));
  return gui.stack(std::move(ret));
}

vector<PGuiElem> GuiBuilder::drawSpellsList(const PlayerInfo& info, bool active) {
  vector<PGuiElem> list;
  if (!info.spells.empty()) {
    vector<PGuiElem> line;
    for (auto& elem : info.spells) {
      line.push_back(getSpellIcon(elem, active));
      if (line.size() >= spellsPerRow)
        list.push_back(gui.horizontalList(std::move(line), spellIconSize.x));
    }
    if (!line.empty())
      list.push_back(gui.horizontalList(std::move(line), spellIconSize.x));
  }
  return list;
}

vector<PGuiElem> GuiBuilder::drawEffectsList(const PlayerInfo& info) {
  vector<PGuiElem> lines;
  for (auto effect : info.effects)
    lines.push_back(gui.stack(
          getTooltip({effect.help}),
          gui.label(effect.name, effect.bad ? colors[ColorId::RED] : colors[ColorId::WHITE])));
  return lines;
}

PGuiElem GuiBuilder::drawPlayerInventory(PlayerInfo& info) {
  GuiFactory::ListBuilder list(gui, legendLineHeight);
  list.addElem(gui.label(info.getTitle(), colors[ColorId::WHITE]));
  list.addElem(gui.horizontalList(makeVec<PGuiElem>(
      gui.label("Level " + toString(info.level), colors[ColorId::WHITE]),
      gui.stack(gui.button(getButtonCallback(UserInputId::UNPOSSESS)),
          gui.label("[U] Leave control", colors[ColorId::LIGHT_BLUE]))), 150, 1));
  for (auto& elem : drawEffectsList(info))
    list.addElem(std::move(elem));
  list.addElem(gui.empty());
  if (!info.team.empty()) {
    const int numPerLine = 6;
    vector<int> widths { 60 };
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        gui.label("Team: ", colors[ColorId::WHITE]));
    for (auto& elem : info.team) {
      currentLine.push_back(gui.stack(
            gui.viewObject(elem.viewId, tilesOk),
            gui.label(toString(elem.expLevel), 12)));
      widths.push_back(30);
      if (currentLine.size() >= numPerLine) {
        list.addElem(gui.horizontalList(std::move(currentLine), widths));
        widths.clear();
      }
    }
    if (!currentLine.empty())
      list.addElem(gui.horizontalList(std::move(currentLine), widths));
    list.addElem(gui.empty());
  }
  for (auto& elem : drawSkillsList(info))
    list.addElem(std::move(elem));
  vector<PGuiElem> spells = drawSpellsList(info, true);
  if (!spells.empty()) {
    list.addElem(gui.label("Spells", colors[ColorId::YELLOW]));
    for (auto& elem : spells)
      list.addElem(std::move(elem), spellIconSize.y);
    list.addElem(gui.empty());
  }
  if (!info.inventory.empty()) {
    list.addElem(gui.label("Inventory", colors[ColorId::YELLOW]));
    for (auto& item : info.inventory)
      list.addElem(getItemLine(item, [=](Rectangle butBounds) {
            if (auto choice = getItemChoice(item, butBounds.getBottomLeft() + Vec2(50, 0), false))
              callbacks.input({UserInputId::INVENTORY_ITEM,
                  InventoryItemInfo{item.ids, *choice}});}));
  }
  return gui.margins(
      gui.scrollable(list.buildVerticalList(), &inventoryScroll), -5, 0, 0, 0);
}

PGuiElem GuiBuilder::drawRightPlayerInfo(PlayerInfo& info) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
    gui.icon(gui.DIPLOMACY),
    gui.icon(gui.HELP));
  for (int i : All(buttons)) {
    buttons[i] = gui.stack(
        std::move(buttons[i]),
        gui.button([this, i]() { minionTab = MinionTab(i); }),
        gui.conditional(gui.border2(), [this, i] (GuiElem*) { return int(minionTab) == i;}));
  }
  PGuiElem main;
  vector<pair<MinionTab, PGuiElem>> elems = makeVec<pair<MinionTab, PGuiElem>>(
    make_pair(MinionTab::INVENTORY, drawPlayerInventory(info)),
    make_pair(MinionTab::HELP, drawPlayerHelp(info)));
  for (auto& elem : elems)
    if (elem.first == minionTab)
      main = std::move(elem.second);
  main = gui.margins(std::move(main), 15, 24, 15, 5);
  int numButtons = buttons.size();
  PGuiElem butGui = gui.margins(
      gui.centerHoriz(gui.horizontalList(std::move(buttons), 50), numButtons * 50), 0, 5, 0, 5);
  return gui.margin(std::move(butGui), std::move(main), 55, gui.TOP);
}

typedef CreatureInfo CreatureInfo;

struct CreatureMapElem {
  ViewId viewId;
  int count;
  CreatureInfo any;
};

static map<string, CreatureMapElem> getCreatureMap(const vector<CreatureInfo>& creatures) {
  map<string, CreatureMapElem> creatureMap;
  for (int i : All(creatures)) {
    auto elem = creatures[i];
    if (!creatureMap.count(elem.speciesName)) {
      creatureMap.insert(make_pair(elem.speciesName, CreatureMapElem({elem.viewId, 1, elem})));
    } else
      ++creatureMap.at(elem.speciesName).count;
  }
  return creatureMap;
}

PGuiElem GuiBuilder::drawTeams(CollectiveInfo& info) {
  int newHash = info.getHash();
  if (newHash != teamHash) {
    const int elemWidth = 30;
    auto lines = gui.getListBuilder(legendLineHeight);
    for (int i : All(info.teams)) {
      auto& team = info.teams[i];
      const int numPerLine = 8;
      auto teamLine = gui.getListBuilder(legendLineHeight);
      vector<PGuiElem> currentLine;
      for (auto member : team.members) {
        auto& memberInfo = info.getMinion(member);
        currentLine.push_back(gui.stack(makeVec<PGuiElem>(
                gui.viewObject(memberInfo.viewId, tilesOk),
                gui.label(toString(memberInfo.expLevel), 12))));
        if (currentLine.size() >= numPerLine)
          teamLine.addElem(gui.horizontalList(std::move(currentLine), elemWidth));
      }
      if (!currentLine.empty())
        teamLine.addElem(gui.horizontalList(std::move(currentLine), elemWidth));
      lines.addElemAuto(gui.stack(makeVec<PGuiElem>(
              gui.conditional(gui.margins(
                  gui.rectangle(colors[ColorId::TRANSPARENT], colors[ColorId::GREEN]), -3, -2, 3, 2),
                  [team] (GuiElem*) { return team.highlight; }),
              gui.mouseOverAction([team, this] { mapGui->highlightTeam(team.members); },
                [team, this] { mapGui->unhighlightTeam(team.members); }),
              gui.button(getButtonCallback({UserInputId::SELECT_TEAM, team.id})),
              gui.mouseHighlight2(gui.margins(
                  gui.rectangle(colors[ColorId::TRANSPARENT], colors[ColorId::WHITE]), -3, -2, 3, 2)),
              gui.dragListener([this, team](DragContent content) {
                callbacks.input({UserInputId::ADD_TO_TEAM, TeamCreatureInfo{team.id, content.get<int>()}});}),
              gui.getListBuilder(22)
              .addElem(gui.topMargin(8, gui.icon(GuiFactory::TEAM_BUTTON, GuiFactory::Alignment::TOP_CENTER)))
              .addElemAuto(teamLine.buildVerticalList()).buildHorizontalList())));
    }
    lines.addElem(gui.stack(
          gui.dragListener([this](DragContent content) {
            callbacks.input({UserInputId::CREATE_TEAM, content.get<int>() });}),
          gui.mouseHighlight2(gui.margins(
              gui.rectangle(colors[ColorId::TRANSPARENT], colors[ColorId::WHITE]), -3, -2, 3, 2)),
          gui.label("[new team]", colors[ColorId::WHITE])));
    teamCache = lines.buildVerticalList();
    teamHash = newHash;
  }
  return gui.external(teamCache.get());
}

PGuiElem GuiBuilder::drawMinions(CollectiveInfo& info) {
  auto list = gui.getListBuilder(legendLineHeight);
  list.addElem(gui.label(info.monsterHeader, colors[ColorId::WHITE]));
  for (int i : All(info.minionGroups)) {
    auto& elem = info.minionGroups[i];
    vector<PGuiElem> line;
    vector<int> widths;
    line.push_back(gui.viewObject(elem.viewId, tilesOk));
    widths.push_back(40);
    Color col = elem.highlight ? colors[ColorId::GREEN] : colors[ColorId::WHITE];
    line.push_back(gui.label(toString(elem.count) + "   " + elem.name, col));
    widths.push_back(200);
    list.addElem(gui.leftMargin(20, gui.stack(
        gui.button(getButtonCallback({UserInputId::CREATURE_GROUP_BUTTON, elem.creatureId})),
        gui.horizontalList(std::move(line), widths))));
  }
  list.addElem(gui.label("Teams: ", colors[ColorId::WHITE]));
  list.addElemAuto(drawTeams(info));
  list.addElem(gui.empty());
  list.addElem(gui.horizontalList(makeVec<PGuiElem>(
          gui.stack(
            gui.label("Show tasks", colors[showTasks ? ColorId::GREEN : ColorId::WHITE]),
            gui.button([this] { closeOverlayWindows(); showTasks = !showTasks; })),
          gui.stack(
            getHintCallback({"Morale affects minion's productivity and chances of fleeing from battle."}),
            gui.label("Show morale", colors[morale ? ColorId::GREEN : ColorId::WHITE]),
            gui.button([this] { morale = !morale; }))
      ), 120));
  list.addElem(gui.empty());
  if (!info.enemyGroups.empty()) {
    list.addElem(gui.label("Enemies:", colors[ColorId::WHITE]));
    for (auto& elem : info.enemyGroups){
      vector<PGuiElem> line;
      line.push_back(gui.viewObject(elem.viewId, tilesOk));
      line.push_back(gui.label(toString(elem.count) + "   " + elem.name, colors[ColorId::WHITE]));
      list.addElem(gui.horizontalList(std::move(line), 20));
    }
  }
  return gui.scrollable(list.buildVerticalList(), &minionsScroll, &scrollbarsHeld);
}

bool GuiBuilder::showMorale() const {
  return morale;
}

const int taskMapWindowWidth = 400;

void GuiBuilder::drawTasksOverlay(vector<OverlayInfo>& ret, CollectiveInfo& info) {
  if (info.taskMap.empty())
    return;
  vector<PGuiElem> lines;
  vector<PGuiElem> freeLines;
  for (auto& elem : info.taskMap)
    if (elem.creature)
      lines.push_back(gui.horizontalList(makeVec<PGuiElem>(
            gui.viewObject(info.getMinion(*elem.creature).viewId, tilesOk),
            gui.label(elem.name, colors[elem.priority ? ColorId::GREEN : ColorId::WHITE])), 35));
    else
      freeLines.push_back(gui.horizontalList(makeVec<PGuiElem>(
            gui.empty(),
            gui.label(elem.name, colors[elem.priority ? ColorId::GREEN : ColorId::WHITE])), 35));
  int lineHeight = 25;
  int margin = 20;
  append(lines, std::move(freeLines));
  ret.push_back({gui.miniWindow(gui.stack(
        gui.keyHandler([=] { showTasks = false; }, {{Keyboard::Escape}}, true),
        gui.margins(gui.verticalList(std::move(lines), lineHeight), margin, margin, margin, margin))),
      Vec2(taskMapWindowWidth, info.taskMap.size() * lineHeight + 2 * margin),
      OverlayInfo::TOP_RIGHT});
}

void GuiBuilder::drawRansomOverlay(vector<OverlayInfo>& ret, const CollectiveInfo::Ransom& ransom) {
  GuiFactory::ListBuilder lines(gui, legendLineHeight);
  lines.addElem(gui.label(ransom.attacker + " demand " + toString(ransom.amount.second) 
        + " gold for not attacking. Agree?"));
  if (ransom.canAfford)
    lines.addElem(gui.leftMargin(25, gui.stack(
          gui.mouseHighlight2(gui.highlight(legendLineHeight)),
          gui.button(getButtonCallback(UserInputId::PAY_RANSOM)),
          gui.label("Yes"))));
  else
    lines.addElem(gui.leftMargin(25, gui.label("Yes", colors[ColorId::GRAY])));
  lines.addElem(gui.leftMargin(25, gui.stack(
        gui.mouseHighlight2(gui.highlight(legendLineHeight)),
        gui.button(getButtonCallback(UserInputId::IGNORE_RANSOM)),
        gui.label("No"))));
  int margin = 20;
  ret.push_back({gui.miniWindow(
        gui.margins(lines.buildVerticalList(), margin)),
      Vec2(600 + 2 * margin, lines.getSize() + 2 * margin),
      OverlayInfo::TOP_RIGHT});
}

void GuiBuilder::drawMinionsOverlay(vector<OverlayInfo>& ret, CollectiveInfo& info) {
  if (showTasks) {
    drawTasksOverlay(ret, info);
    return;
  }
  int margin = 20;
  Vec2 size(600, 600);
  int minionListWidth = 220;
  size.x += minionListWidth;
  if (info.chosen) {
    int newHash = info.getHash();
    if (newHash != minionsOverlayHash) {
      PGuiElem minionPage;
      auto& minions = info.chosen->creatures;
      auto current = info.chosen->chosenId;
      for (int i : All(minions))
        if (minions[i].creatureId == current)
          minionPage = gui.margins(drawMinionPage(minions[i]), 10, 15, 10, 10);
      CHECK(!!minionPage);
      PGuiElem menu;
      PGuiElem leftSide = drawMinionButtons(minions, current, info.chosen->teamId);
      if (info.chosen->teamId) {
        auto list = gui.getListBuilder(legendLineHeight);
        list.addElem(gui.stack(
            gui.button(getButtonCallback({UserInputId::CANCEL_TEAM, *info.chosen->teamId})),
            gui.label("[Disband team]", colors[ColorId::LIGHT_BLUE])));
        list.addElem(gui.empty(), legendLineHeight);
        leftSide = gui.marginAuto(list.buildVerticalList(), std::move(leftSide), GuiFactory::TOP);
      }
      menu = gui.stack(
          gui.keyHandler(getButtonCallback({UserInputId::CREATURE_BUTTON, -1}),
            {{Keyboard::Escape}, {Keyboard::Return}}),
          gui.horizontalList(makeVec<PGuiElem>(
              gui.margins(std::move(leftSide), 8, 15, 5, 0),
              gui.margins(gui.sprite(GuiFactory::TexId::VERT_BAR_MINI, GuiFactory::Alignment::LEFT),
                0, -15, 0, -15)), minionListWidth),
          gui.leftMargin(minionListWidth + 20, std::move(minionPage)));
      minionsOverlayCache = gui.miniWindow(gui.stack(
            gui.keyHandler(getButtonCallback({UserInputId::CREATURE_BUTTON, -1}), {{Keyboard::Escape}}, true),
            gui.margins(std::move(menu), margin)));
      minionsOverlayHash = newHash;
    }
    ret.push_back({gui.external(minionsOverlayCache.get()), size, OverlayInfo::MINIONS});
  }
}

void GuiBuilder::drawBuildingsOverlay(vector<OverlayInfo>& ret, CollectiveInfo& info) {
  if (activeBuilding == -1 || info.buildings[activeBuilding].groupName.empty() || hideBuildingOverlay)
    return;
  string groupName = info.buildings[activeBuilding].groupName;
  vector<PGuiElem> lines;
  for (int i : All(info.buildings)) {
    auto& elem = info.buildings[i];
    if (elem.groupName == groupName)
      lines.push_back(getButtonLine(elem, i, activeBuilding, collectiveTab));
  }
  lines.push_back(gui.stack(
        gui.centeredLabel(Renderer::HOR, "[close]", colors[ColorId::LIGHT_BLUE]),
        gui.button([=] { hideBuildingOverlay = true;})));
  int margin = 20;
  int height = lines.size() * legendLineHeight - 8;
  ret.push_back({gui.miniWindow(gui.stack(
          gui.keyHandler([=] { hideBuildingOverlay = true; }, {{Keyboard::Escape}}, true),
          gui.margins(gui.verticalList(std::move(lines), legendLineHeight), margin))),
      Vec2(300 + 2 * margin, height + 2 * margin),
      OverlayInfo::TOP_RIGHT});
}

void GuiBuilder::drawBandOverlay(vector<OverlayInfo>& ret, CollectiveInfo& info) {
  if (info.ransom) {
    drawRansomOverlay(ret, *info.ransom);
    return;
  }
  switch (collectiveTab) {
    case CollectiveTab::MINIONS: return drawMinionsOverlay(ret, info);
    case CollectiveTab::BUILDINGS: return drawBuildingsOverlay(ret, info);
    default: break;
  }
}

int GuiBuilder::getNumMessageLines() const {
  return 3;
}

static Color makeBlack(const Color& col, double freshness) {
  double amount = 0.5 + freshness / 2;
  return Color(col.r * amount, col.g * amount, col.b * amount);
}

static Color getMessageColor(const PlayerMessage& msg) {
  Color color;
  switch (msg.getPriority()) {
    case PlayerMessage::NORMAL: color = colors[ColorId::WHITE]; break;
    case PlayerMessage::HIGH: color = colors[ColorId::ORANGE]; break;
    case PlayerMessage::CRITICAL: color = colors[ColorId::RED]; break;
  }
  return makeBlack(color, msg.getFreshness());
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

void GuiBuilder::drawMessages(vector<OverlayInfo>& ret,
    const vector<PlayerMessage>& messageBuffer, int maxMessageLength) {
  int hMargin = 10;
  int vMargin = 5;
  vector<vector<PlayerMessage>> messages = fitMessages(renderer, messageBuffer, maxMessageLength - 2 * hMargin,
      getNumMessageLines());
  int lineHeight = 20;
  vector<PGuiElem> lines;
  for (int i : All(messages)) {
    GuiFactory::ListBuilder line(gui);
    for (auto& message : messages[i]) {
      string text = (line.isEmpty() ? "" : " ") + message.getText();
      cutToFit(renderer, text, maxMessageLength - 2 * hMargin);
      line.addElemAuto(gui.stack(
            gui.button(getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
            gui.label(text, getMessageColor(message))));
      if (message.isClickable())
        line.addElemAuto(gui.labelUnicode(String(L'➚'), getMessageColor(message)));
    }
    if (!messages[i].empty())
      lines.push_back(line.buildHorizontalList());
  }
  if (!lines.empty())
    ret.push_back({gui.translucentBackground(
        gui.margins(gui.verticalList(std::move(lines), lineHeight), hMargin, vMargin, hMargin, vMargin)),
        Vec2(maxMessageLength, lineHeight * messages.size() + 15), OverlayInfo::MESSAGES});
}

PGuiElem GuiBuilder::getVillageStateLabel(VillageInfo::Village::State state) {
  switch (state) {
    case VillageInfo::Village::FRIENDLY: return gui.label("Friendly", colors[ColorId::GREEN]);
    case VillageInfo::Village::HOSTILE: return gui.label("Hostile", colors[ColorId::ORANGE]);
    case VillageInfo::Village::CONQUERED: return gui.label("Conquered", colors[ColorId::LIGHT_BLUE]);
  }
}

PGuiElem GuiBuilder::getVillageActionButton(int villageIndex, VillageAction action) {
  switch (action) {
    case VillageAction::RECRUIT: 
      return gui.stack(
          gui.label("Recruit", colors[ColorId::GREEN]),
          gui.button(getButtonCallback({UserInputId::VILLAGE_ACTION, VillageActionInfo{villageIndex, action}})));
    case VillageAction::TRADE: 
      return gui.stack(
          gui.label("Trade", colors[ColorId::GREEN]),
          gui.button(getButtonCallback({UserInputId::VILLAGE_ACTION, VillageActionInfo{villageIndex, action}})));
  }
}

static string getTriggerLabel(const AttackTrigger& trigger) {
  switch (trigger.getId()) {
    case AttackTriggerId::SELF_VICTIMS: return "Killed tribe members";
    case AttackTriggerId::GOLD: return "Gold";
    case AttackTriggerId::STOLEN_ITEMS: return "Item theft";
    case AttackTriggerId::ROOM_BUILT:
      switch (trigger.get<SquareType>().getId()) {
        case SquareId::THRONE: return "Throne";
        case SquareId::IMPALED_HEAD: return "Impaled heads";
        default: FAIL << "Unsupported ROOM_BUILT type"; return "";
      }
    case AttackTriggerId::POWER: return "Keeper's power";
    case AttackTriggerId::ENEMY_POPULATION: return "Dungeon population";
    case AttackTriggerId::TIMER: return "Time";
    case AttackTriggerId::ENTRY: return "Entry";
  }
}

static sf::Color getTriggerColor(double value) {
  return sf::Color(255, max<int>(0, 255 - value * 500 * 255), max<int>(0, 255 - value * 500 * 255));
}

void GuiBuilder::showAttackTriggers(const vector<TriggerInfo>& triggers, Vec2 pos) {
  vector<PGuiElem> elems;
  for (auto& trigger : triggers)
#ifdef RELEASE
    if (trigger.value > 0)
#endif
    elems.push_back(gui.label(getTriggerLabel(trigger.trigger)
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
    drawMiniMenu(std::move(list), exit, pos, 300);
  }
}

PGuiElem GuiBuilder::drawVillages(VillageInfo& info) {
  auto lines = gui.getListBuilder(legendLineHeight);
  int titleMargin = -11;
  lines.addElem(gui.leftMargin(titleMargin, gui.label(toString(info.numConquered) + "/" +
          toString(info.totalMain) + " main villains conquered.")), titleLineHeight);
  if (info.numMainVillains > 0)
    lines.addElem(gui.leftMargin(titleMargin, gui.label("Main villains:")), titleLineHeight);
  for (int i : All(info.villages)) {
    if (i == info.numMainVillains) {
      lines.addElem(gui.empty());
      lines.addElem(gui.leftMargin(titleMargin, gui.label("Lesser villains:")), titleLineHeight);
    }
    auto& elem = info.villages[i];
    PGuiElem header = gui.label(capitalFirst(elem.name) + (elem.tribeName.empty() ?
          string() : " (" + elem.tribeName + ")"), colors[ColorId::WHITE]);
    if (info.villages[i].knownLocation)
      header = gui.stack(gui.button(getButtonCallback({UserInputId::GO_TO_VILLAGE, i})),
        gui.getListBuilder()
            .addElemAuto(std::move(header))
            .addElem(gui.empty(), 7)
            .addElemAuto(gui.labelUnicode(String(L'➚'))).buildHorizontalList());
    lines.addElem(std::move(header));
    if (!info.villages[i].knownLocation)
      lines.addElem(gui.leftMargin(40, gui.label("Location unknown", colors[ColorId::LIGHT_BLUE])));
    GuiFactory::ListBuilder line(gui);
    line.addElemAuto(gui.margins(getVillageStateLabel(elem.state), 40, 0, 40, 0));
    vector<TriggerInfo> triggers = elem.triggers;
    sort(triggers.begin(), triggers.end(),
        [] (const TriggerInfo& t1, const TriggerInfo& t2) { return t1.value > t2.value;});
#ifdef RELEASE
    triggers = filter(triggers, [](const TriggerInfo& t) { return t.value > 0;});
#endif
    if (!triggers.empty())
      line.addElemAuto(gui.stack(
          gui.label("Triggers", colors[ColorId::RED]),
          gui.button([this, triggers](Rectangle bounds) {
              showAttackTriggers(triggers, bounds.getTopRight() + Vec2(20, 0));})));
    lines.addElem(line.buildHorizontalList());
    for (auto action : elem.actions)
      lines.addElem(gui.margins(getVillageActionButton(i, action), 40, 0, 0, 0));

  }
  if (lines.isEmpty())
    return gui.label("No foreign tribes discovered.");
  int numVillains = info.villages.size();
  if (numSeenVillains == -1)
    numSeenVillains = numVillains;
  return gui.stack(
    gui.onRenderedAction([this, numVillains] { numSeenVillains = numVillains;}),
    gui.scrollable(lines.buildVerticalList(), &villagesScroll, &scrollbarsHeld));
}

const double menuLabelVPadding = 0.15;

Rectangle GuiBuilder::getMinionMenuPosition() {
  Vec2 center = renderer.getSize() / 2;
  Vec2 dim(800, 600);
  return Rectangle(center - dim / 2, center + dim / 2);
}

Rectangle GuiBuilder::getEquipmentMenuPosition(int height) {
  Rectangle minionMenu = getMinionMenuPosition();
  int width = 340;
  Vec2 origin = minionMenu.getTopRight() + Vec2(-100, 200);
  origin.x = min(origin.x, renderer.getSize().x - width);
  return Rectangle(origin, origin + Vec2(width, height)).intersection(Rectangle(Vec2(0, 0), renderer.getSize()));
}

Rectangle GuiBuilder::getMenuPosition(MenuType type) {
  int windowWidth = 800;
  int windowHeight = 400;
  int ySpacing;
  int yOffset = 0;
  switch (type) {
    case MenuType::YES_NO:
      ySpacing = (renderer.getSize().y - 200) / 2;
      yOffset = - ySpacing + 100;
      break;
    case MenuType::MAIN_NO_TILES:
      ySpacing = (renderer.getSize().y - windowHeight) / 2;
      break;
    case MenuType::MAIN:
      windowWidth = 0.41 * renderer.getSize().y;
      ySpacing = renderer.getSize().y / 3;
      break;
    case MenuType::GAME_CHOICE:
      windowWidth = 0.41 * renderer.getSize().y;
      ySpacing = renderer.getSize().y * 0.28;
      yOffset = renderer.getSize().y * 0.05;
      break;
    default: ySpacing = 100; break;
  }
  int xSpacing = (renderer.getSize().x - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing + yOffset, xSpacing + windowWidth, renderer.getSize().y - ySpacing + yOffset);
}


vector<string> GuiBuilder::breakText(const string& text, int maxWidth) {
  if (text.empty())
    return {""};
  vector<string> rows;
  for (string line : split(text, {'\n'})) {
    rows.push_back("");
    for (string word : split(line, {' '}))
      if (renderer.getTextLength(rows.back() + ' ' + word) <= maxWidth)
        rows.back().append((rows.back().size() > 0 ? " " : "") + word);
      else
        rows.push_back(word);
  }
  return rows;
}


vector<PGuiElem> GuiBuilder::getMultiLine(const string& text, Color color, MenuType menuType, int maxWidth) {
  vector<PGuiElem> ret;
  for (const string& s : breakText(text, maxWidth)) {
    if (menuType != MenuType::MAIN)
      ret.push_back(gui.label(s, color));
    else
      ret.push_back(gui.mainMenuLabelBg(s, menuLabelVPadding));

  }
  return ret;
}

PGuiElem GuiBuilder::menuElemMargins(PGuiElem elem) {
  return gui.margins(std::move(elem), 10, 3, 10, 0);
}

PGuiElem GuiBuilder::getHighlight(MenuType type, const string& label, int height) {
  switch (type) {
    case MenuType::MAIN: return menuElemMargins(gui.mainMenuLabel(label, menuLabelVPadding));
    default: return gui.highlight(height);
  }
}

PGuiElem GuiBuilder::drawListGui(const string& title, const vector<ListElem>& options,
    MenuType menuType, int* height, int* highlight, int* choice) {
  vector<PGuiElem> lines;
  vector<int> heights;
  int leftMargin = 30;
  if (!title.empty()) {
    lines.push_back(gui.leftMargin(leftMargin, gui.label(capitalFirst(title), colors[ColorId::WHITE])));
    heights.push_back(listLineHeight);
    lines.push_back(gui.empty());
    heights.push_back(listLineHeight);
  } else {
    lines.push_back(gui.empty());
    heights.push_back(listLineHeight / 2);
  }
  int numActive = 0;
  int secColumnWidth = 0;
  int columnWidth = 300;
  for (auto& elem : options) {
    columnWidth = max(columnWidth, renderer.getTextLength(elem.getText()) + 50);
    if (!elem.getSecondColumn().empty())
      secColumnWidth = max(secColumnWidth, 80 + renderer.getTextLength(elem.getSecondColumn()));
  }
  columnWidth = min(columnWidth, getMenuPosition(menuType).getW() - secColumnWidth - 140);
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
    vector<PGuiElem> label1 = getMultiLine(options[i].getText(), color, menuType, columnWidth);
    if (options.size() == 1 && label1.size() > 1) { // hacky way of checking that we display a wall of text
      append(heights, vector<int>(label1.size(), listLineHeight));
      for (auto& line : label1)
        line = gui.margins(std::move(line), leftMargin, 0, 0, 0);
      append(lines, std::move(label1));
      break;
    }
    heights.push_back((label1.size() - 1) * listBrokenLineHeight + listLineHeight);
    PGuiElem line;
    if (menuType != MenuType::MAIN)
      line = gui.verticalList(std::move(label1), listBrokenLineHeight);
    else
      line = std::move(getOnlyElement(label1));
    if (!options[i].getTip().empty())
      line = gui.stack(std::move(line),
          gui.tooltip({options[i].getTip()}));
    if (!options[i].getSecondColumn().empty())
      line = gui.horizontalList(makeVec<PGuiElem>(std::move(line),
            gui.label(options[i].getSecondColumn())), columnWidth + 80);
    lines.push_back(menuElemMargins(std::move(line)));
    if (highlight && options[i].getMod() == ListElem::NORMAL) {
      lines.back() = gui.stack(makeVec<PGuiElem>(
          gui.button([=]() { *choice = numActive; }),
          std::move(lines.back()),
          gui.mouseHighlight(getHighlight(menuType, options[i].getText(), listLineHeight), numActive, highlight)));
      ++numActive;
    }
    lines.back() = gui.margins(std::move(lines.back()), leftMargin, 0, 0, 0);
  }
  *height = accumulate(heights.begin(), heights.end(), 0);
  if (menuType != MenuType::MAIN)
    return gui.verticalList(std::move(lines), heights);
  else
    return gui.verticalListFit(std::move(lines), 0.0);
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

PGuiElem GuiBuilder::drawMinionButtons(const vector<PlayerInfo>& minions, UniqueEntity<Creature>::Id current,
    optional<TeamId> teamId) {
  CHECK(!minions.empty());
  int cache = combineHash(minions, current);
  if (cache != minionButtonsHash) {
    map<ViewId, vector<PlayerInfo>> minionMap = groupByViewId(minions);
    auto list = gui.getListBuilder(legendLineHeight);
    for (auto& elem : minionMap) {
      list.addElem(gui.topMargin(5, gui.viewObject(elem.first, tilesOk)), legendLineHeight + 5);
      for (auto& minion : elem.second) {
        auto minionId = minion.creatureId;
        Color color = colors[current == minionId ? ColorId::GREEN : ColorId::WHITE];
        function<Color()> colorFun = [this, color, minionId] {
          if (auto hColor = mapGui->getCreatureHighlight(minionId, clock->getRealMillis()))
            return *hColor;
          else
            return color;
        };
        GuiFactory::ListBuilder line(gui);
        if (teamId)
          line.addElem(gui.leftMargin(-16, gui.stack(
              gui.button(getButtonCallback({UserInputId::REMOVE_FROM_TEAM, TeamCreatureInfo{*teamId, minionId}})),
              gui.labelUnicode(String(L'✘'), colors[ColorId::RED]))), 1);
        line.addElemAuto(gui.rightMargin(5, gui.label(minion.getFirstName(), colorFun)));
        if (auto icon = getMoraleIcon(minion.morale))
          line.addElem(gui.topMargin(-2, gui.icon(*icon)), 20);
        line.addBackElem(gui.label("L:" + toString(minion.level), colorFun), 42);
        list.addElem(gui.stack(
              gui.button(getButtonCallback({UserInputId::CREATURE_BUTTON, minionId})),
              gui.dragSource({DragContentId::CREATURE, minionId},
                [=]{ return gui.viewObject(minion.viewId, tilesOk);}),
              line.buildHorizontalList()));
      }
    }
    minionButtonsCache = gui.scrollable(list.buildVerticalList());
    cache = minionButtonsHash;
  }
  return gui.external(minionButtonsCache.get());
}

static string getTaskText(MinionTask option) {
  switch (option) {
    case MinionTask::GRAVE:
    case MinionTask::LAIR:
    case MinionTask::SLEEP: return "Sleeping";
    case MinionTask::EAT: return "Eating";
    case MinionTask::EXPLORE_NOCTURNAL: return "Exploring (night)";
    case MinionTask::EXPLORE_CAVES: return "Exploring caves";
    case MinionTask::EXPLORE: return "Exploring";
    case MinionTask::RITUAL: return "Rituals";
    case MinionTask::CROPS: return "Crops";
    case MinionTask::PRISON: return "Prison";
    case MinionTask::TORTURE: return "Torture";
    case MinionTask::EXECUTE: return "Execution";
    case MinionTask::TRAIN: return "Training";
    case MinionTask::WORKSHOP: return "Workshop";
    case MinionTask::FORGE: return "Forge";
    case MinionTask::LABORATORY: return "Laboratory";
    case MinionTask::JEWELER: return "Jeweler";
    case MinionTask::STUDY: return "Studying";
    case MinionTask::COPULATE: return "Copulating";
    case MinionTask::CONSUME: return "Absorbing";
    case MinionTask::SPIDER: return "Spider";
  }
}

static ColorId getTaskColor(PlayerInfo::MinionTaskInfo info) {
  if (info.inactive)
    return ColorId::GRAY;
  if (info.current)
    return ColorId::GREEN;
  else
    return ColorId::WHITE;
}

vector<PGuiElem> GuiBuilder::drawItemMenu(const vector<ItemInfo>& items, ItemMenuCallback callback,
    bool doneBut) {
  vector<PGuiElem> lines;
  for (int i : All(items))
    lines.push_back(getItemLine(items[i], [=] (Rectangle bounds) { callback(bounds, i);} ));
  if (doneBut)
    lines.push_back(gui.stack(
          gui.button([=] { callback(Rectangle(), none); }, {Keyboard::Escape}),
          gui.centeredLabel(Renderer::HOR, "[done]", colors[ColorId::LIGHT_BLUE])));
  return lines;
}

PGuiElem GuiBuilder::drawActivityButton(const PlayerInfo& minion) {
  string curTask = "(none)";
  for (auto task : minion.minionTasks)
    if (task.current)
      curTask = getTaskText(task.task);
  return gui.stack(
      gui.horizontalList(makeVec<PGuiElem>(
          gui.label(curTask), gui.label("[change]", colors[ColorId::LIGHT_BLUE])),
        renderer.getTextLength(curTask) + 20),
      gui.button([=] (Rectangle bounds) {
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
            tasks.addElem(GuiFactory::ListBuilder(gui)
                .addElemAuto(gui.stack(
                    gui.button(buttonFun),
                    gui.label(getTaskText(task.task), colors[getTaskColor(task)])))
                .addBackElemAuto(gui.stack(
                    getTooltip({"Click to turn this task on/off."}),
                    gui.button([&exit, &retAction, task] {
                      retAction.lock.toggle(task.task);
                    }),
                    gui.rightMargin(20, gui.labelUnicode(String(L'✓'), [&retAction, task] {
                        return colors[(retAction.lock[task.task] ^ task.locked) ?
                            ColorId::LIGHT_GRAY : ColorId::GREEN];})))).buildHorizontalList());
          }
          drawMiniMenu(std::move(tasks), exit, bounds.getBottomLeft(), 200);
          callbacks.input({UserInputId::CREATURE_TASK_ACTION, retAction});
        }));
}

vector<PGuiElem> GuiBuilder::drawAttributesOnPage(vector<PGuiElem>&& attrs) {
  vector<PGuiElem> lines[2];
  for (int i : All(attrs))
    lines[i % 2].push_back(std::move(attrs[i]));
  int elemWidth = 80;
  return makeVec<PGuiElem>(
        gui.horizontalList(std::move(lines[0]), elemWidth),
        gui.horizontalList(std::move(lines[1]), elemWidth));
}

vector<PGuiElem> GuiBuilder::drawEquipmentAndConsumables(const PlayerInfo& minion) {
  const vector<ItemInfo>& items = minion.inventory;
  if (items.empty())
    return {};
  vector<PGuiElem> lines;
  vector<PGuiElem> itemElems = drawItemMenu(items,
      [=](Rectangle butBounds, optional<int> index) {
        const ItemInfo& item = items[*index];
        if (auto choice = getItemChoice(item, butBounds.getBottomLeft() + Vec2(50, 0), true))
          callbacks.input({UserInputId::CREATURE_EQUIPMENT_ACTION,
              EquipmentActionInfo{minion.creatureId, item.ids, item.slot, *choice}});
      });
  lines.push_back(gui.label("Equipment", colors[ColorId::YELLOW]));
  for (int i : All(itemElems))
    if (items[i].type == items[i].EQUIPMENT)
      lines.push_back(gui.leftMargin(3, std::move(itemElems[i])));
  lines.push_back(gui.label("Consumables", colors[ColorId::YELLOW]));
  for (int i : All(itemElems))
    if (items[i].type == items[i].CONSUMABLE)
      lines.push_back(gui.leftMargin(3, std::move(itemElems[i])));
  lines.push_back(gui.stack(
      gui.label("[add consumable]", colors[ColorId::LIGHT_BLUE]),
      gui.button(getButtonCallback({UserInputId::CREATURE_EQUIPMENT_ACTION, 
              EquipmentActionInfo{minion.creatureId, {}, none, ItemAction::REPLACE}}))));
  for (int i : All(itemElems))
    if (items[i].type == items[i].OTHER) {
      lines.push_back(gui.label("Other", colors[ColorId::YELLOW]));
      break;
    }
  for (int i : All(itemElems))
    if (items[i].type == items[i].OTHER)
      lines.push_back(gui.leftMargin(3, std::move(itemElems[i])));
  return lines;
}

vector<PGuiElem> GuiBuilder::drawMinionActions(const PlayerInfo& minion) {
  vector<PGuiElem> line;
  for (auto action : minion.actions)
    switch (action) {
      case PlayerInfo::CONTROL:
        line.push_back(gui.stack(
            gui.label("[Control]", colors[ColorId::LIGHT_BLUE]),
            gui.button(getButtonCallback({UserInputId::CREATURE_CONTROL, minion.creatureId}))));
        break;
      case PlayerInfo::RENAME:
        line.push_back(gui.stack(
            gui.label("[Rename]", colors[ColorId::LIGHT_BLUE]),
            gui.button([=] { 
                if (auto name = getTextInput("Rename minion", minion.firstName, 10, "Press escape to cancel."))
                callbacks.input({UserInputId::CREATURE_RENAME,
                    RenameActionInfo{minion.creatureId, *name}}); })));
        break;
      case PlayerInfo::BANISH:
        line.push_back(gui.stack(
            gui.label("[Banish]", colors[ColorId::LIGHT_BLUE]),
            gui.button(getButtonCallback({UserInputId::CREATURE_BANISH, minion.creatureId}))));
        break;
      case PlayerInfo::WHIP:
        line.push_back(gui.stack(
            gui.label("[Whip]", colors[ColorId::LIGHT_BLUE]),
            gui.button(getButtonCallback({UserInputId::CREATURE_WHIP, minion.creatureId}))));
        break;
      case PlayerInfo::EXECUTE:
        line.push_back(gui.stack(
            gui.label("[Execute]", colors[ColorId::LIGHT_BLUE]),
            gui.button(getButtonCallback({UserInputId::CREATURE_EXECUTE, minion.creatureId}))));
        break;
      case PlayerInfo::TORTURE:
        line.push_back(gui.stack(
            gui.label("[Torture]", colors[ColorId::LIGHT_BLUE]),
            gui.button(getButtonCallback({UserInputId::CREATURE_TORTURE, minion.creatureId}))));
        break;
    }
  return line;
}

vector<PGuiElem> GuiBuilder::joinLists(vector<PGuiElem>&& v1, vector<PGuiElem>&& v2) {
  vector<PGuiElem> ret;
  for (int i : Range(max(v1.size(), v2.size())))
    ret.push_back(gui.horizontalListFit(makeVec<PGuiElem>(
            i < v1.size() ? std::move(v1[i]) : gui.empty(),
            i < v2.size() ? gui.leftMargin(10, std::move(v2[i])) : gui.empty())));
  return ret;
}

PGuiElem GuiBuilder::drawMinionPage(const PlayerInfo& minion) {
  GuiFactory::ListBuilder list(gui, legendLineHeight);
  list.addElem(gui.label(minion.getTitle()));
  list.addElem(gui.horizontalList(drawMinionActions(minion), 140));
  vector<PGuiElem> leftLines;
  leftLines.push_back(gui.label("Attributes", colors[ColorId::YELLOW]));
  for (auto& elem : drawAttributesOnPage(drawPlayerAttributes(minion.attributes)))
    leftLines.push_back(std::move(elem));
  for (auto& elem : drawEffectsList(minion))
    leftLines.push_back(std::move(elem));
  leftLines.push_back(gui.empty());
  leftLines.push_back(gui.label("Activity", colors[ColorId::YELLOW]));
  leftLines.push_back(drawActivityButton(minion));
  leftLines.push_back(gui.empty());
  for (auto& elem : drawSkillsList(minion))
    leftLines.push_back(std::move(elem));
  vector<PGuiElem> spells = drawSpellsList(minion, false);
  if (!spells.empty()) {
    leftLines.push_back(gui.label("Spells", colors[ColorId::YELLOW]));
    for (auto& elem : spells) {
      leftLines.push_back(std::move(elem));
      leftLines.push_back(gui.empty());
    }
  }
  int topMargin = list.getSize() + 20;
  return gui.margin(list.buildVerticalList(),
      gui.scrollable(gui.verticalList(joinLists(
          std::move(leftLines),
          drawEquipmentAndConsumables(minion)), legendLineHeight)),
      topMargin, GuiFactory::TOP);
}

static vector<CreatureMapElem> getRecruitStacks(const vector<CreatureInfo>& creatures) {
  map<string, CreatureMapElem> creatureMap;
  for (int i : All(creatures)) {
    auto elem = creatures[i];
    string key = elem.speciesName + " " + toString(elem.cost->second) + " " + toString(elem.expLevel);
    if (!creatureMap.count(key)) {
      creatureMap.insert(make_pair(key, CreatureMapElem({elem.viewId, 1, elem})));
    } else
      ++creatureMap.at(key).count;
  }
  return getValues(creatureMap);
}

PGuiElem GuiBuilder::drawTradeItemMenu(SyncQueue<optional<UniqueEntity<Item>::Id>>& queue, const string& title,
    pair<ViewId, int> budget, const vector<ItemInfo>& items, double* scrollPos) {
  int titleExtraSpace = 10;
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  lines.addElem(GuiFactory::ListBuilder(gui)
      .addElemAuto(gui.label(title))
      .addBackElemAuto(drawCost(budget)).buildHorizontalList(),
     getStandardLineHeight() + titleExtraSpace);
  for (PGuiElem& elem : drawItemMenu(items,
        [&queue, items] (Rectangle, optional<int> index) { if (index) queue.push(items[*index].ids.front());}))
    lines.addElem(std::move(elem));
  int menuHeight = lines.getSize() + 30;
  return gui.stack(
      gui.preferredSize(330, menuHeight),
      gui.reverseButton([&queue] { queue.push(none); }, {{Keyboard::Escape}}),
      gui.miniWindow(gui.margins(gui.scrollable(lines.buildVerticalList(), scrollPos),
          15, 15, 15, 15)));
}

PGuiElem GuiBuilder::drawRecruitMenu(SyncQueue<optional<UniqueEntity<Creature>::Id>>& queue, const string& title,
    const string& warning, pair<ViewId, int> budget, const vector<CreatureInfo>& creatures, double* scrollPos) {
  GuiFactory::ListBuilder lines(gui, getStandardLineHeight());
  lines.addElem(GuiFactory::ListBuilder(gui)
      .addElemAuto(gui.label(title))
      .addBackElemAuto(drawCost(budget)).buildHorizontalList());
  if (!warning.empty()) {
    lines.addElem(gui.label(warning, colors[ColorId::RED]));
    budget.second = -1;
  }
  lines.addElem(gui.empty(), 10);
  for (PGuiElem& elem : drawRecruitList(creatures,
        [&queue] (optional<UniqueEntity<Creature>::Id> a) { queue.push(a);}, budget.second))
    lines.addElem(std::move(elem));
  int menuHeight = lines.getSize() + 30;
  return gui.stack(
      gui.preferredSize(330, menuHeight),
      gui.reverseButton([&queue] { queue.push(none); }, {{Keyboard::Escape}}),
      gui.miniWindow(gui.margins(gui.scrollable(lines.buildVerticalList(), scrollPos),
          15, 15, 15, 15)));
}

vector<PGuiElem> GuiBuilder::drawRecruitList(const vector<CreatureInfo>& creatures,
    CreatureMenuCallback callback, int budget) {
  vector<CreatureMapElem> stacks = getRecruitStacks(creatures);
  vector<PGuiElem> lines;
  for (auto& elem : stacks) {
    bool canAfford = elem.any.cost->second <= budget;
    ColorId color = canAfford ? ColorId::WHITE : ColorId::GRAY;
    lines.push_back(gui.stack(
          gui.keyHandler([callback] { callback(none); }, {{Keyboard::Escape}, {Keyboard::Return}}),
          canAfford ? gui.button([callback, elem] { callback(elem.any.uniqueId); }) : gui.empty(),
          gui.leftMargin(25, gui.stack(
              canAfford ? gui.mouseHighlight2(gui.highlight(listLineHeight)) : gui.empty(),
              GuiFactory::ListBuilder(gui)
                  .addElemAuto(gui.rightMargin(10, gui.label(toString(elem.count), colors[color])))
                  .addElem(gui.viewObject(elem.viewId, tilesOk), 50)
                  .addElem(gui.label("level " + toString(elem.any.expLevel), colors[color]), 50)
                  .addBackElemAuto(drawCost(*elem.any.cost, color)).buildHorizontalList()))));
  }
  return lines;
}

PGuiElem GuiBuilder::drawHighscorePage(const HighscoreList& page, double *scrollPos) {
  GuiFactory::ListBuilder lines(gui, legendLineHeight);
  for (auto& elem : page.scores) {
    GuiFactory::ListBuilder line(gui);
    ColorId color = elem.highlight ? ColorId::GREEN : ColorId::WHITE;
    line.addElemAuto(gui.label(elem.text, colors[color]));
    if (page.sortBy == page.SCORE)
      line.addBackElem(gui.label(toString(elem.score) + " points", colors[color]), 130);
    else
      line.addBackElem(gui.label(toString(elem.turns) + " turns", colors[color]), 130);
    lines.addElem(gui.leftMargin(30, line.buildHorizontalList()));
  }
  return gui.scrollable(lines.buildVerticalList(), scrollPos);
}

PGuiElem GuiBuilder::drawHighscores(const vector<HighscoreList>& list, Semaphore& sem, int& tabNum,
    vector<double>& scrollPos, bool& online) {
  vector<PGuiElem> pages;
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
          [&tabNum, i, &online, numTabs] (GuiElem*) { return (online && tabNum == i - numTabs) ||
              (!online && tabNum == i) ; }));
    if (i < numTabs)
    topLine.addElem(gui.stack(
        gui.margins(gui.mouseHighlightClick(gui.mainMenuHighlight(), i, &tabNum), 32, 0, 32, 0),
        gui.centeredLabel(Renderer::HOR, list[i].name),
        gui.button([&tabNum, i] { tabNum = i;})));
  }
  PGuiElem onlineBut = gui.stack(
      gui.label("Online", [&online] { return colors[online ? ColorId::GREEN : ColorId::WHITE];}),
      gui.button([&online] { online = !online; }));
  Vec2 size = getMenuPosition(MenuType::NORMAL).getSize();
  return gui.stack(makeVec<PGuiElem>(gui.preferredSize(size.x, size.y),
      gui.keyHandler([&tabNum, numTabs] { tabNum = (tabNum + 1) % numTabs; }, {{Keyboard::Right}}),
      gui.keyHandler([&tabNum, numTabs] { tabNum = (tabNum + numTabs - 1) % numTabs; }, {{Keyboard::Left}}),
      gui.window(
        gui.margin(gui.leftMargin(25, std::move(onlineBut)),
        gui.topMargin(30, gui.margin(gui.leftMargin(5, topLine.buildHorizontalListFit()),
            gui.margins(gui.stack(std::move(pages)), 25, 60, 0, 30), legendLineHeight, GuiFactory::TOP)), legendLineHeight, GuiFactory::TOP),
        [&] { sem.v(); })));
  
}

Rectangle GuiBuilder::getTextInputPosition() {
  Vec2 center = renderer.getSize() / 2;
  return Rectangle(center - Vec2(300, 129), center + Vec2(300, 129));
}

PGuiElem GuiBuilder::getTextContent(const string& title, const string& value, const string& hint) {
  vector<PGuiElem> lines = makeVec<PGuiElem>(
      gui.variableLabel([&] { return title + ":  " + value + "_"; }));
  if (!hint.empty())
    lines.push_back(gui.label(hint, gui.inactiveText));
  return gui.verticalList(std::move(lines), 40);
}

optional<string> GuiBuilder::getTextInput(const string& title, const string& value, int maxLength,
    const string& hint) {
  bool dismiss = false;
  string text = value;
  PGuiElem dismissBut = gui.margins(gui.stack(makeVec<PGuiElem>(
        gui.button([&](){ dismiss = true; }),
        gui.mouseHighlight2(gui.mainMenuHighlight()),
        gui.centerHoriz(
            gui.label("Dismiss", colors[ColorId::WHITE]), renderer.getTextLength("Dismiss")))), 0, 5, 0, 0);
  PGuiElem stuff = gui.margins(getTextContent(title, text, hint), 30, 50, 0, 0);
  stuff = gui.margin(gui.centerHoriz(std::move(dismissBut), renderer.getTextLength("Dismiss") + 100),
    std::move(stuff), 30, gui.BOTTOM);
  stuff = gui.window(std::move(stuff), [&dismiss] { dismiss = true; });
  PGuiElem bg = gui.darken();
  bg->setBounds(renderer.getSize());
  while (1) {
    callbacks.refreshScreen();
    bg->render(renderer);
    stuff->setBounds(getTextInputPosition());
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      gui.propagateEvent(event, {stuff.get()});
      if (dismiss)
        return none;
      if (event.type == Event::TextEntered)
        if ((isalnum(event.text.unicode) || event.text.unicode == ' ') && text.size() < maxLength)
          text += event.text.unicode;
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::BackSpace:
              if (!text.empty())
                text.pop_back();
              break;
          case Keyboard::Return: return text;
          case Keyboard::Escape: return none;
          default: break;
        }
    }
  }
}


