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

GuiBuilder::GuiBuilder(Renderer& r, GuiFactory& g, Clock* c, Callbacks call)
    : renderer(r), gui(g), clock(c), callbacks(call), gameSpeed(GameSpeed::NORMAL) {
}

void GuiBuilder::reset() {
  chosenCreature = "";
  activeBuilding = 0;
  activeLibrary = -1;
  gameSpeed = GameSpeed::NORMAL;
}

const int legendLineHeight = 30;

int GuiBuilder::getStandardLineHeight() const {
  return legendLineHeight;
}

PGuiElem GuiBuilder::getHintCallback(const vector<string>& s) {
  return gui.mouseOverAction([this, s]() { callbacks.hintCallback({s}); });
}

function<void()> GuiBuilder::getButtonCallback(UserInput input) {
  return [this, input]() { callbacks.inputCallback(input); };
}

void GuiBuilder::setTilesOk(bool ok) {
  tilesOk = ok;
}


void GuiBuilder::setCollectiveTab(CollectiveTab t) {
  collectiveTab = t;
  if (t != CollectiveTab::MINIONS)
    callbacks.inputCallback(UserInputId::CONFIRM_TEAM);
}

GuiBuilder::CollectiveTab GuiBuilder::getCollectiveTab() const {
  return collectiveTab;
}

void GuiBuilder::closeOverlayWindows() {
  chosenCreature = "";
//  activeBuilding = 0;
}

int GuiBuilder::getActiveBuilding() const {
  return activeBuilding;
}

int GuiBuilder::getActiveLibrary() const {
  return activeLibrary;
}

PGuiElem GuiBuilder::getButtonLine(GameInfo::BandInfo::Button button, int num, int& active,
    CollectiveTab tab) {
  vector<PGuiElem> line;
  line.push_back(gui.viewObject(button.viewId, tilesOk));
  vector<int> widths { 35 };
  if (button.state != GameInfo::BandInfo::Button::ACTIVE)
    line.push_back(gui.label(button.name + " " + button.count, colors[ColorId::GRAY], button.hotkey));
  else
    line.push_back(gui.conditional(
          gui.label(button.name + " " + button.count, colors[ColorId::GREEN], button.hotkey),
          gui.label(button.name + " " + button.count, colors[ColorId::WHITE], button.hotkey),
          [=, &active] (GuiElem*) { return active == num; }));
  widths.push_back(100);
  if (button.cost) {
    string costText = toString(button.cost->second);
    line.push_back(gui.label(costText, colors[ColorId::WHITE]));
    widths.push_back(renderer.getTextLength(costText) + 8);
    line.push_back(gui.viewObject(button.cost->first, tilesOk));
    widths.push_back(25);
  }
  function<void()> buttonFun;
  if (button.state != GameInfo::BandInfo::Button::INACTIVE)
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
      gui.button(buttonFun, button.hotkey),
      gui.horizontalList(std::move(line), widths, button.cost ? 2 : 0));
}

vector<PGuiElem> GuiBuilder::drawButtons(vector<GameInfo::BandInfo::Button> buttons, int& active,
    CollectiveTab tab) {
  vector<PGuiElem> elems;
  vector<PGuiElem> invisible;
  vector<GameInfo::BandInfo::Button> groupedButtons;
  for (int i = 0; i <= buttons.size(); ++i) {
    if (!groupedButtons.empty() && (i == buttons.size() || buttons[i].groupName != groupedButtons[0].groupName)) {
      int firstItem = i - groupedButtons.size();
      int lastItem = i - 1;
      GameInfo::BandInfo::Button button1 = groupedButtons.front();
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

PGuiElem GuiBuilder::drawBuildings(GameInfo::BandInfo& info) {
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

PGuiElem GuiBuilder::drawDeities(GameInfo::BandInfo& info) {
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

PGuiElem GuiBuilder::drawTechnology(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines = drawButtons(info.libraryButtons, activeLibrary, CollectiveTab::TECHNOLOGY);
  for (int i : All(info.techButtons)) {
    vector<PGuiElem> line;
    line.push_back(gui.viewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, ""), tilesOk));
    line.push_back(gui.label(info.techButtons[i].name, colors[ColorId::WHITE], info.techButtons[i].hotkey));
    lines.push_back(gui.stack(gui.button(
          getButtonCallback(UserInput(UserInputId::TECHNOLOGY, i)), info.techButtons[i].hotkey),
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
    "[z] or mouse wheel: zoom",
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
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  GameInfo::SunlightInfo& sunlightInfo = gameInfo.sunlightInfo;
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
  bottomLine.push_back(gui.label("population: " + toString(gameInfo.bandInfo.minionCount) + " / " + toString(gameInfo.bandInfo.minionLimit)));
  int numTop = topLine.size();
  int numBottom = bottomLine.size();
  return gui.verticalList(makeVec<PGuiElem>(
      gui.centerHoriz(gui.horizontalList(std::move(topLine), resourceSpace), numTop * resourceSpace),
      gui.centerHoriz(gui.horizontalList(std::move(bottomLine), 140, 3), numBottom * 140)), 28);
}

string GuiBuilder::getGameSpeedName(GuiBuilder::GameSpeed gameSpeed) const {
  switch (gameSpeed) {
    case GameSpeed::SLOW: return "slow";
    case GameSpeed::NORMAL: return "normal";
    case GameSpeed::FAST: return "fast";
    case GameSpeed::VERY_FAST: return "very fast";
  }
}

string GuiBuilder::getCurrentGameSpeedName() const {
  if (clock->isPaused())
    return "paused";
  else
    return getGameSpeedName(gameSpeed);
}

PGuiElem GuiBuilder::drawRightBandInfo(GameInfo::BandInfo& info, GameInfo::VillageInfo& villageInfo) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
      gui.icon(gui.BUILDING),
      gui.icon(gui.MINION),
      gui.icon(gui.LIBRARY),
      gui.icon(gui.DIPLOMACY),
      gui.icon(gui.HELP));
  for (int i : All(buttons)) {
    if (int(collectiveTab) == i)
      buttons[i] = gui.border2(std::move(buttons[i]));
    else
      buttons[i] = gui.margins(std::move(buttons[i]), 6, 6, 6, 6);
    buttons[i] = gui.stack(std::move(buttons[i]),
        gui.button([this, i]() { setCollectiveTab(CollectiveTab(i)); }));
  }
  if (collectiveTab != CollectiveTab::MINIONS)
    chosenCreature = "";
  vector<pair<CollectiveTab, PGuiElem>> elems = makeVec<pair<CollectiveTab, PGuiElem>>(
      make_pair(CollectiveTab::MINIONS, drawMinions(info)),
      make_pair(CollectiveTab::BUILDINGS, drawBuildings(info)),
      make_pair(CollectiveTab::KEY_MAPPING, drawKeeperHelp()),
      make_pair(CollectiveTab::TECHNOLOGY, drawTechnology(info)),
      make_pair(CollectiveTab::VILLAGES, drawVillages(villageInfo)));
  vector<PGuiElem> tabs;
  for (auto& elem : elems)
    if (elem.first == collectiveTab)
      tabs.push_back(std::move(elem.second));
    else
      tabs.push_back(gui.invisible(std::move(elem.second)));
  PGuiElem main = gui.stack(std::move(tabs));
  main = gui.margins(std::move(main), 15, 15, 15, 5);
  int numButtons = buttons.size();
  PGuiElem butGui = gui.margins(
      gui.centerHoriz(gui.horizontalList(std::move(buttons), 50), numButtons * 50), 0, 5, 0, 5);
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(gui.stack(
      gui.horizontalList(makeVec<PGuiElem>(
          gui.label("speed:"),
          gui.label(getCurrentGameSpeedName(),
              colors[clock->isPaused() ? ColorId::RED : ColorId::WHITE])), 60),
      gui.button([&] { gameSpeedDialogOpen = !gameSpeedDialogOpen; })));
  bottomLine.push_back(
      gui.label("FPS " + toString(fpsCounter.getFps()) + " / " + toString(upsCounter.getFps()),
      colors[ColorId::WHITE]));
  PGuiElem bottomElem = gui.horizontalList(std::move(bottomLine), 160);
  bottomElem = gui.verticalList(makeVec<PGuiElem>(
      gui.label(renderer.setViewCount > 1000 ? ("Warning: " + toString(renderer.setViewCount)) : "",
          colors[ColorId::RED]),
      std::move(bottomElem)), 30);
  main = gui.margin(gui.margins(std::move(bottomElem), 25, 0, 0, 0),
      std::move(main), 48, gui.BOTTOM);
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
            }, ' ')));
  for (GameSpeed speed : ENUM_ALL(GameSpeed)) {
    Color color = colors[speed == gameSpeed ? ColorId::GREEN : ColorId::WHITE];
    lines.push_back(gui.stack(gui.horizontalList(makeVec<PGuiElem>(
             gui.label(getGameSpeedName(speed), color),
             gui.label("'" + string(1, getHotkeyChar(speed)) + "' ", color)), keyMargin),
          gui.button2([=] { gameSpeed = speed; gameSpeedDialogOpen = false; clock->cont();},
            getHotkey(speed))));
  }
  reverse(lines.begin(), lines.end());
  int margin = 20;
  Vec2 size(150 + 2 * margin, legendLineHeight * lines.size() - 10 + 2 * margin);
  PGuiElem dialog = gui.miniWindow(
      gui.margins(gui.verticalList(std::move(lines), legendLineHeight), margin, margin, margin, margin));
  overlays.push_back({std::move(dialog), size,
      gameSpeedDialogOpen ? OverlayInfo::GAME_SPEED : OverlayInfo::INVISIBLE});
}

PGuiElem GuiBuilder::getSunlightInfoGui(GameInfo::SunlightInfo& sunlightInfo) {
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

typedef GameInfo::PlayerInfo::AttributeInfo::Id AttrId;

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

vector<PGuiElem> GuiBuilder::drawPlayerAttributes(const vector<GameInfo::PlayerInfo::AttributeInfo>& attr) {
  vector<PGuiElem> ret;
  for (auto& elem : attr)
    ret.push_back(gui.stack(getTooltip({elem.name, elem.help}),
        gui.horizontalList(makeVec<PGuiElem>(
          gui.icon(getAttrIcon(elem.id)),
          gui.margins(gui.label(toString(elem.value), getBonusColor(elem.bonus)), 0, 2, 0, 0)), 30)));
  return ret;
}

PGuiElem GuiBuilder::drawBottomPlayerInfo(GameInfo& gameInfo) {
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
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
vector<string> GuiBuilder::getItemHint(const GameInfo::ItemInfo& item) {
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

PGuiElem GuiBuilder::getItemLine(const GameInfo::ItemInfo& item, function<void(Rectangle)> onClick,
    function<void()> onMultiClick) {
  vector<PGuiElem> line;
  vector<int> widths;
  int leftMargin = -4;
  if (item.locked) {
    line.push_back(gui.viewObject(ViewId::KEY, tilesOk));
    widths.push_back(viewObjectWidth);
    leftMargin -= viewObjectWidth - 3;
  }
  line.push_back(gui.viewObject(item.viewId, tilesOk));
  widths.push_back(viewObjectWidth);
  Color color = colors[item.equiped ? ColorId::GREEN : item.pending ? ColorId::GRAY : ColorId::WHITE];
  if (item.number > 1) {
    line.push_back(gui.label(toString(item.number), color));
    widths.push_back(renderer.getTextLength(toString(item.number) + " "));
  }
  line.push_back(gui.label(item.name, color));
  widths.push_back(100);
  for (auto& elem : line)
    elem = gui.stack(gui.button(onClick), std::move(elem), getTooltip(getItemHint(item)));
  int numAlignRight = 0;
  if (item.owner) {
    line.push_back(gui.viewObject(item.owner->viewId, tilesOk));
    widths.push_back(viewObjectWidth);
    line.push_back(gui.label("L:" + toString(item.owner->expLevel)));
    widths.push_back(60);
    numAlignRight = 2;
  }
  if (onMultiClick && item.number > 1) {
    line.push_back(gui.stack(
        gui.label("[#]"),
        gui.button(onMultiClick),
        getTooltip({"Click to choose how many to pick up."})));
    widths.push_back(25);
    numAlignRight = 1;
  }
  return gui.margins(gui.horizontalList(std::move(line), widths, numAlignRight), leftMargin, 0, 0, 0);
}

PGuiElem GuiBuilder::getTooltip(const vector<string>& text) {
  return gui.conditional(gui.tooltip(text), [this] (GuiElem*) { return !disableTooltip;});
}

const int listLineHeight = 30;
const int listBrokenLineHeight = 24;

int GuiBuilder::getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

void GuiBuilder::drawPlayerOverlay(vector<OverlayInfo>& ret, GameInfo::PlayerInfo& info) {
  if (info.lyingItems.empty()) {
    playerOverlayFocused = false;
    itemIndex = -1;
    return;
  }
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
          [this, i](Rectangle) { callbacks.inputCallback({UserInputId::PICK_UP_ITEM, i}); },
          [this, i]() { callbacks.inputCallback({UserInputId::PICK_UP_ITEM_MULTI, i}); })))));
    }
  }
  int totalElems = info.lyingItems.size();
  if (itemIndex >= totalElems)
    itemIndex = totalElems - 1;
  PGuiElem content = gui.stack(makeVec<PGuiElem>(
        gui.focusable(gui.stack(
            gui.keyHandler([=] { callbacks.inputCallback({UserInputId::PICK_UP_ITEM, itemIndex});},
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

PGuiElem GuiBuilder::drawPlayerHelp(GameInfo::PlayerInfo& info) {
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
          gui.button([this, key]() { callbacks.keyboardCallback(key);})));
  }
  return gui.verticalList(std::move(lines), legendLineHeight);
}

static string getActionText(GameInfo::ItemInfo::Action a) {
  switch (a) {
    case GameInfo::ItemInfo::DROP: return "drop";
    case GameInfo::ItemInfo::DROP_MULTI: return "drop some";
    case GameInfo::ItemInfo::EQUIP: return "equip";
    case GameInfo::ItemInfo::THROW: return "throw";
    case GameInfo::ItemInfo::UNEQUIP: return "remove";
    case GameInfo::ItemInfo::APPLY: return "apply";
    case GameInfo::ItemInfo::REPLACE: return "replace";
    case GameInfo::ItemInfo::LOCK: return "lock";
    case GameInfo::ItemInfo::UNLOCK: return "unlock";
  }
}

void GuiBuilder::drawMiniMenu(vector<PGuiElem> elems, function<bool(int)> callback, Vec2 menuPos, int width) {
  if (elems.empty())
    return;
  bool exit = false;
  int numElems = elems.size();
  for (int i : All(elems))
    elems[i] = gui.stack(gui.button([&exit, callback, i] { exit = callback(i); }), std::move(elems[i]));
  int margin = 15;
  PGuiElem menu = gui.stack(
      gui.reverseButton([&exit] { exit = true; }, {{Keyboard::Escape}}),
      gui.miniWindow(gui.leftMargin(margin, gui.topMargin(margin,
            gui.verticalList(std::move(elems), legendLineHeight)))));
  menu->setBounds(Rectangle(menuPos, menuPos + Vec2(width + 2 * margin, numElems * legendLineHeight + 2 * margin)));
  PGuiElem bg = gui.darken();
  bg->setBounds(renderer.getSize());
  while (1) {
    callbacks.refreshScreen();
    bg->render(renderer);
    menu->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      GuiElem::propagateEvent(event, {menu.get()});
      if (exit)
        return;
    }
  }

}

optional<GameInfo::ItemInfo::Action> GuiBuilder::getItemChoice(const GameInfo::ItemInfo& itemInfo, Vec2 menuPos,
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
  PGuiElem stuff = drawListGui("", View::getListElem(options), View::NORMAL_MENU, &contentHeight, &index, &choice);
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
      GuiElem::propagateEvent(event, {stuff.get()});
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

vector<PGuiElem> GuiBuilder::drawSkillsList(const GameInfo::PlayerInfo& info) {
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

PGuiElem GuiBuilder::getSpellIcon(const GameInfo::PlayerInfo::Spell& spell, bool active) {
  vector<PGuiElem> ret = makeVec<PGuiElem>(
      getTooltip({spell.name, spell.help}),
      gui.spellIcon(spell.id));
  if (spell.timeout) {
    ret.push_back(gui.darken());
    ret.push_back(gui.centeredLabel(Renderer::HOR_VER, toString(*spell.timeout)));
  } else
  if (active)
    ret.push_back(gui.button(getButtonCallback({UserInputId::CAST_SPELL, spell.id})));
  return gui.stack(std::move(ret));
}

vector<PGuiElem> GuiBuilder::drawSpellsList(const GameInfo::PlayerInfo& info, bool active) {
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

vector<PGuiElem> GuiBuilder::drawEffectsList(const GameInfo::PlayerInfo& info) {
  vector<PGuiElem> lines;
  for (auto effect : info.effects)
    lines.push_back(gui.stack(
          getTooltip({effect.help}),
          gui.label(effect.name, effect.bad ? colors[ColorId::RED] : colors[ColorId::WHITE])));
  return lines;
}

PGuiElem GuiBuilder::drawPlayerInventory(GameInfo::PlayerInfo& info) {
  GuiFactory::ListBuilder list(gui, legendLineHeight);
  list.addElem(gui.label(info.getTitle(), colors[ColorId::WHITE]));
  list.addElem(gui.label("Level " + toString(info.level), colors[ColorId::WHITE]));
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
              callbacks.inputCallback({UserInputId::INVENTORY_ITEM,
                  InventoryItemInfo(item.ids, *choice)});}));
  }
  return gui.margins(
      gui.scrollable(list.buildVerticalList(), &inventoryScroll), -5, 0, 0, 0);
}

PGuiElem GuiBuilder::drawRightPlayerInfo(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
    gui.icon(gui.DIPLOMACY),
    gui.icon(gui.HELP));
  for (int i : All(buttons)) {
    if (int(minionTab) == i)
      buttons[i] = gui.border2(std::move(buttons[i]));
    else
      buttons[i] = gui.margins(std::move(buttons[i]), 6, 6, 6, 6);
    buttons[i] = gui.stack(std::move(buttons[i]),
        gui.button([this, i]() { minionTab = MinionTab(i); }));
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

typedef GameInfo::CreatureInfo CreatureInfo;

struct CreatureMapElem {
  ViewId viewId;
  int count;
  CreatureInfo any;
};

static map<string, CreatureMapElem> getCreatureMap(vector<CreatureInfo>& creatures) {
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

PGuiElem GuiBuilder::drawMinions(GameInfo::BandInfo& info) {
  map<string, CreatureMapElem> creatureMap = getCreatureMap(info.minions);
  map<string, CreatureMapElem> enemyMap = getCreatureMap(info.enemies);
  vector<PGuiElem> list;
  list.push_back(gui.label(info.monsterHeader, colors[ColorId::WHITE]));
  for (auto elem : creatureMap){
    vector<PGuiElem> line;
    vector<int> widths;
    line.push_back(gui.viewObject(elem.second.viewId, tilesOk));
    widths.push_back(40);
    Color col = elem.first == chosenCreature ? colors[ColorId::GREEN] : colors[ColorId::WHITE];
    line.push_back(gui.label(toString(elem.second.count) + "   " + elem.first, col));
    widths.push_back(200);
    function<void()> action;
    if (info.currentTeam || info.newTeam) {
      if (chosenCreature == elem.first)
        action = [this] () {
          chosenCreature = "";
        };
      else
        action = [this, elem] () {
          chosenCreature = elem.first;
          showTasks = false;
        };
    } else
      action = getButtonCallback({UserInputId::CREATURE_BUTTON, elem.second.any.uniqueId});
    list.push_back(gui.margins(gui.stack(gui.button(action),
            gui.horizontalList(std::move(line), widths)), 20, 0, 0, 0));
  }
  list.push_back(gui.label("Teams: ", colors[ColorId::WHITE]));
  const int elemWidth = 30;
  for (int i : All(info.teams)) {
    auto& team = info.teams[i];
    const int numPerLine = 8;
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        gui.stack(
          gui.button(getButtonCallback(info.currentTeam == i ? UserInput(UserInputId::CONFIRM_TEAM)
              : UserInput(UserInputId::EDIT_TEAM, team.id))),
          info.currentTeam == i 
              ? gui.viewObject(ViewId::TEAM_BUTTON_HIGHLIGHT, tilesOk)
              : gui.viewObject(ViewId::TEAM_BUTTON, tilesOk),
          gui.mouseHighlight2(gui.viewObject(ViewId::TEAM_BUTTON_HIGHLIGHT, tilesOk))));
    for (auto member : team.members) {
      currentLine.push_back(gui.stack(makeVec<PGuiElem>(
            gui.button(getButtonCallback({UserInputId::SET_TEAM_LEADER, TeamLeaderInfo(team.id, member)})),
            gui.viewObject(info.getMinion(member).viewId, tilesOk),
            gui.mouseHighlight2(gui.margins(
                gui.rectangle(colors[ColorId::TRANSPARENT], colors[ColorId::WHITE]), -3, -2, 3, 2)),
            gui.label(toString(info.getMinion(member).expLevel), 12))));
      if (currentLine.size() >= numPerLine)
        list.push_back(gui.horizontalList(std::move(currentLine), elemWidth));
    }
    if (!currentLine.empty())
      list.push_back(gui.horizontalList(std::move(currentLine), elemWidth));
    if (info.currentTeam == i)
      list.push_back(gui.horizontalList(makeVec<PGuiElem>(
              gui.stack(
                  gui.button(getButtonCallback({UserInputId::COMMAND_TEAM, team.id})),
                  gui.label("[command]")),
              gui.stack(
                  gui.button(getButtonCallback({UserInputId::CANCEL_TEAM, team.id})),
                  gui.label("[disband]")),
              gui.stack(
                  getHintCallback({"You can order an active team to move by clicking anywhere on known "
                      "territory."}),
                  gui.button(getButtonCallback({UserInputId::ACTIVATE_TEAM, team.id})),
                  gui.label("[activate]", colors[team.active ? ColorId::GREEN : ColorId::WHITE]))),
            {renderer.getTextLength("[command]") + 3, renderer.getTextLength("[disband]") + 3,
             renderer.getTextLength("[activate]") + 3}));
  }
  if (!info.newTeam)
    list.push_back(gui.stack(
        gui.button(getButtonCallback(UserInputId::CREATE_TEAM)),
        gui.label("[new team]", colors[ColorId::WHITE])));
  else
    list.push_back(gui.horizontalList(makeVec<PGuiElem>(
        gui.viewObject(ViewId::TEAM_BUTTON_HIGHLIGHT, tilesOk),
        gui.label("Click on minions to add.", colors[ColorId::LIGHT_BLUE])), elemWidth));
  if (info.currentTeam)
    list.push_back(gui.label("Click members to change leader.", colors[ColorId::LIGHT_BLUE]));
  list.push_back(gui.empty());
  if (info.payoutTimeRemaining > -1) {
    vector<PGuiElem> res;
    res.push_back(gui.label("Next payout [" + toString(info.payoutTimeRemaining) + "]:",
          colors[ColorId::WHITE]));
    res.push_back(gui.viewObject(info.numResource[0].viewId, tilesOk));
    res.push_back(gui.label(toString<int>(info.nextPayout), colors[ColorId::WHITE]));
    list.push_back(gui.horizontalList(std::move(res), {170, 30, 1}));
  }
  list.push_back(gui.horizontalList(makeVec<PGuiElem>(
          gui.stack(
            gui.label("Show tasks", colors[showTasks ? ColorId::GREEN : ColorId::WHITE]),
            gui.button([this] { showTasks = !showTasks; chosenCreature = ""; })),
          gui.stack(
            getHintCallback({"Morale affects minion's productivity and chances of fleeing from battle."}),
            gui.label("Show morale", colors[morale ? ColorId::GREEN : ColorId::WHITE]),
            gui.button([this] { morale = !morale; }))
      ), 120));
  list.push_back(gui.empty());
  if (!enemyMap.empty()) {
    list.push_back(gui.label("Enemies:", colors[ColorId::WHITE]));
    for (auto elem : enemyMap){
      vector<PGuiElem> line;
      line.push_back(gui.viewObject(elem.second.viewId, tilesOk));
      line.push_back(gui.label(toString(elem.second.count) + "   " + elem.first, colors[ColorId::WHITE]));
      list.push_back(gui.horizontalList(std::move(line), 20));
    }
  }
  if (!creatureMap.count(chosenCreature) || (!info.newTeam && !info.currentTeam))
    chosenCreature = "";
  return gui.scrollable(gui.verticalList(std::move(list), legendLineHeight), &minionsScroll, &scrollbarsHeld);
}

bool GuiBuilder::showMorale() const {
  return morale;
}

const int taskMapWindowWidth = 400;

void GuiBuilder::drawTasksOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
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

const int minionWindowWidth = 300;

void GuiBuilder::drawMinionsOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
  if (showTasks) {
    drawTasksOverlay(ret, info);
    return;
  }
  if (chosenCreature == "" || (!info.currentTeam && !info.newTeam))
    return;
  vector<PGuiElem> lines;
  vector<CreatureInfo> chosen;
  for (auto& c : info.minions)
    if (c.speciesName == chosenCreature)
      chosen.push_back(c);
  lines.push_back(gui.label("Click to add to team:", colors[ColorId::LIGHT_BLUE]));
  for (auto& c : chosen) {
    string text = "L: " + toString(c.expLevel) + "    " + info.tasks[c.uniqueId];
    if (c.speciesName != c.name)
      text = c.name + " " + text;
    vector<PGuiElem> line;
    line.push_back(gui.viewObject(c.viewId, tilesOk));
    line.push_back(gui.label(text, (info.currentTeam && contains(info.teams[*info.currentTeam].members, c.uniqueId))
          ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
    lines.push_back(gui.stack(
          gui.button(getButtonCallback(UserInput(UserInputId::ADD_TO_TEAM, c.uniqueId))),
          gui.horizontalList(std::move(line), 40)));
  }
  int margin = 20;
  ret.push_back({gui.miniWindow(gui.stack(
          gui.keyHandler([=] { chosenCreature = ""; }, {{Keyboard::Escape}}, true),
          gui.margins(gui.verticalList(std::move(lines), legendLineHeight), margin, margin, margin, margin))),
      Vec2(minionWindowWidth + 2 * margin, (chosen.size() + 2) * legendLineHeight + 2 * margin),
      OverlayInfo::TOP_RIGHT});
}

void GuiBuilder::drawBuildingsOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
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
          gui.margins(gui.verticalList(std::move(lines), legendLineHeight), margin, margin, margin, margin))),
      Vec2(minionWindowWidth + 2 * margin, height + 2 * margin),
      OverlayInfo::TOP_RIGHT});
}

void GuiBuilder::drawBandOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
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
    vector<PGuiElem> line;
    vector<int> lengths;
    for (auto& message : messages[i]) {
      string text = (line.empty() ? "" : " ") + message.getText();
      cutToFit(renderer, text, maxMessageLength - 2 * hMargin);
      line.push_back(gui.stack(
            gui.button(getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
            gui.label(text, getMessageColor(message))));
      lengths.push_back(renderer.getTextLength(text));
      if (message.isClickable()) {
        line.push_back(gui.labelUnicode(String(L'➚'), getMessageColor(message)));
        lengths.push_back(messageArrowLength);
      }
    }
    if (!messages[i].empty())
      lines.push_back(gui.horizontalList(std::move(line), lengths));
  }
  if (!lines.empty())
    ret.push_back({gui.translucentBackground(
        gui.margins(gui.verticalList(std::move(lines), lineHeight), hMargin, vMargin, hMargin, vMargin)),
        Vec2(maxMessageLength, lineHeight * messages.size() + 15), OverlayInfo::MESSAGES});
}

PGuiElem GuiBuilder::drawVillages(GameInfo::VillageInfo& info) {
  vector<PGuiElem> lines;
  for (auto elem : info.villages) {
    lines.push_back(gui.label(capitalFirst(elem.name), colors[ColorId::WHITE]));
    lines.push_back(gui.margins(gui.label("tribe: " + elem.tribeName, colors[ColorId::WHITE]), 40, 0, 0, 0));
    if (!elem.state.empty())
      lines.push_back(gui.margins(gui.label(elem.state, elem.state == "conquered" ? colors[ColorId::GREEN] : colors[ColorId::RED]),
            40, 0, 0, 0));
  }
  return gui.verticalList(std::move(lines), legendLineHeight);
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

Rectangle GuiBuilder::getMenuPosition(View::MenuType type) {
  int windowWidth = 800;
  int windowHeight = 400;
  int ySpacing;
  int yOffset = 0;
  switch (type) {
    case View::YES_NO_MENU:
      ySpacing = (renderer.getSize().y - 200) / 2;
      yOffset = - ySpacing + 100;
      break;
    case View::MAIN_MENU_NO_TILES:
      ySpacing = (renderer.getSize().y - windowHeight) / 2;
      break;
    case View::MAIN_MENU:
      windowWidth = 0.41 * renderer.getSize().y;
      ySpacing = renderer.getSize().y / 3;
      break;
    case View::GAME_CHOICE_MENU:
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


vector<PGuiElem> GuiBuilder::getMultiLine(const string& text, Color color, View::MenuType menuType, int maxWidth) {
  vector<PGuiElem> ret;
  for (const string& s : breakText(text, maxWidth)) {
    if (menuType != View::MenuType::MAIN_MENU)
      ret.push_back(gui.label(s, color));
    else
      ret.push_back(gui.mainMenuLabelBg(s, menuLabelVPadding));

  }
  return ret;
}

PGuiElem GuiBuilder::menuElemMargins(PGuiElem elem) {
  return gui.margins(std::move(elem), 10, 3, 10, 0);
}

PGuiElem GuiBuilder::getHighlight(View::MenuType type, const string& label, int height) {
  switch (type) {
    case View::MAIN_MENU: return menuElemMargins(gui.mainMenuLabel(label, menuLabelVPadding));
    default: return gui.highlight(height);
  }
}

PGuiElem GuiBuilder::drawListGui(const string& title, const vector<View::ListElem>& options,
    View::MenuType menuType, int* height, int* highlight, int* choice) {
  vector<PGuiElem> lines;
  vector<int> heights;
  if (!title.empty()) {
    lines.push_back(gui.margins(gui.label(capitalFirst(title), colors[ColorId::WHITE]), 30, 0, 0, 0));
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
  int leftMargin = 30;
  for (auto& elem : options) {
    columnWidth = max(columnWidth, renderer.getTextLength(elem.getText()) + 50);
    if (!elem.getSecondColumn().empty())
      secColumnWidth = max(secColumnWidth, 80 + renderer.getTextLength(elem.getSecondColumn()));
  }
  columnWidth = min(columnWidth, getMenuPosition(menuType).getW() - secColumnWidth - 140);
  if (menuType == View::MAIN_MENU)
    columnWidth = 1000000;
  for (int i : All(options)) {
    Color color;
    switch (options[i].getMod()) {
      case View::TITLE: color = gui.titleText; break;
      case View::INACTIVE: color = gui.inactiveText; break;
      case View::TEXT:
      case View::NORMAL: color = gui.text; break;
    }
    vector<PGuiElem> label1 = getMultiLine(options[i].getText(), color, menuType, columnWidth);
    if (options.size() == 1 && label1.size() > 1) { // hacky way of checking that we display a wall of text
      heights = vector<int>(label1.size(), listLineHeight);
      lines = std::move(label1);
      for (auto& line : lines)
        line = gui.margins(std::move(line), leftMargin, 0, 0, 0);
      break;
    }
    heights.push_back((label1.size() - 1) * listBrokenLineHeight + listLineHeight);
    PGuiElem line;
    if (menuType != View::MAIN_MENU)
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
    if (highlight && options[i].getMod() == View::NORMAL) {
      lines.back() = gui.stack(makeVec<PGuiElem>(
          gui.button([=]() { *choice = numActive; }),
          std::move(lines.back()),
          gui.mouseHighlight(getHighlight(menuType, options[i].getText(), listLineHeight), numActive, highlight)));
      ++numActive;
    }
    lines.back() = gui.margins(std::move(lines.back()), leftMargin, 0, 0, 0);
  }
  *height = accumulate(heights.begin(), heights.end(), 0);
  if (menuType != View::MAIN_MENU)
    return gui.verticalList(std::move(lines), heights);
  else
    return gui.verticalListFit(std::move(lines), 0.0);
}

static optional<GuiFactory::IconId> getMoraleIcon(double morale) {
  if (morale >= 0.5)
    return GuiFactory::MORALE_4;
  if (morale >= 0.01)
    return GuiFactory::MORALE_3;
  if (morale < -0.5)
    return GuiFactory::MORALE_1;
  if (morale < -0.01)
    return GuiFactory::MORALE_2;
  else
    return none;
}

PGuiElem GuiBuilder::drawMinionButtons(const vector<GameInfo::PlayerInfo>& minions,
    UniqueEntity<Creature>::Id& current) {
  vector<PGuiElem> list;
  CHECK(!minions.empty());
  list.push_back(gui.viewObject(minions[0].viewId, tilesOk));
  for (int i : All(minions)) {
    auto minionId = minions[i].creatureId;
    auto colorFun = [&current, minionId] { return colors[current == minionId ? ColorId::GREEN : ColorId::WHITE];};
    GuiFactory::ListBuilder line(gui);
    line.addElem(gui.label(minions[i].getFirstName(), colorFun),
        renderer.getTextLength(minions[i].getFirstName()) + 5);
    if (auto icon = getMoraleIcon(minions[i].morale))
      line.addElem(gui.topMargin(-2, gui.icon(*icon)), 20);
    line.addBackElem(gui.label("L:" + toString(minions[i].level), colorFun), 42);
    list.push_back(gui.stack(
        gui.button([&current, minionId] { current = minionId;}),
        line.buildHorizontalList()));
  }
  return gui.verticalList(std::move(list), legendLineHeight);
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
  }
}

static ColorId getTaskColor(GameInfo::PlayerInfo::MinionTaskInfo info) {
  if (info.inactive)
    return ColorId::GRAY;
  if (info.current)
    return ColorId::GREEN;
  else
    return ColorId::WHITE;
}

vector<PGuiElem> GuiBuilder::drawItemMenu(const vector<GameInfo::ItemInfo>& items, ItemMenuCallback callback,
    bool doneBut) {
  vector<PGuiElem> lines;
  for (int i : All(items))
    lines.push_back(getItemLine(items[i], [=] (Rectangle bounds) { callback(bounds, i);} ));
  if (doneBut)
    lines.push_back(gui.stack(
          gui.button2([=] { callback(Rectangle(), none); }, {Keyboard::Escape}),
          gui.centeredLabel(Renderer::HOR, "[done]", colors[ColorId::LIGHT_BLUE])));
  return lines;
}

PGuiElem GuiBuilder::drawActivityButton(const GameInfo::PlayerInfo& minion, MinionMenuCallback callback) {
  string curTask = "(none)";
  for (auto task : minion.minionTasks)
    if (task.current)
      curTask = getTaskText(task.task);
  return gui.stack(
      gui.horizontalList(makeVec<PGuiElem>(
          gui.label(curTask), gui.label("[change]", colors[ColorId::LIGHT_BLUE])),
        renderer.getTextLength(curTask) + 20),
      gui.button([=] (Rectangle bounds) {
          vector<PGuiElem> tasks;
          for (auto task : minion.minionTasks)
            tasks.push_back(gui.label(getTaskText(task.task), colors[getTaskColor(task)]));
          drawMiniMenu(std::move(tasks), [=] (int ind) {
              if (!minion.minionTasks[ind].inactive) {
                callback(View::MinionAction{minion.minionTasks[ind].task});
                  return true;
              } else
                return false;}, bounds.getBottomLeft(), 200);}));
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

vector<PGuiElem> GuiBuilder::drawEquipmentAndConsumables(const vector<GameInfo::ItemInfo>& items,
    MinionMenuCallback callback) {
  if (items.empty())
    return {};
  vector<PGuiElem> lines;
  vector<PGuiElem> itemElems = drawItemMenu(items,
      [=](Rectangle butBounds, optional<int> index) {
        const GameInfo::ItemInfo& item = items[*index];
        if (auto choice = getItemChoice(item, butBounds.getBottomLeft() + Vec2(50, 0), true))
          callback(View::MinionAction{View::MinionAction::ItemAction{item.ids, item.slot, *choice}});
      });
  lines.push_back(gui.label("Equipment", colors[ColorId::YELLOW]));
  for (int i : All(itemElems)) {
    lines.push_back(gui.leftMargin(3, std::move(itemElems[i])));
    if ((i == itemElems.size() || !items[i + 1].slot) && items[i].slot)
      lines.push_back(gui.label("Consumables", colors[ColorId::YELLOW]));
  }
  lines.push_back(gui.stack(
      gui.label("[add consumable]", colors[ColorId::LIGHT_BLUE]),
      gui.button([=] { callback(View::MinionAction{View::MinionAction::ItemAction{{}, none,
          GameInfo::ItemInfo::REPLACE}});})));
  return lines;
}

vector<PGuiElem> GuiBuilder::drawMinionActions(const GameInfo::PlayerInfo& minion, MinionMenuCallback callback) {
  vector<PGuiElem> line;
  for (auto action : minion.actions)
    switch (action) {
      case GameInfo::PlayerInfo::CONTROL:
        line.push_back(gui.stack(
            gui.label("[Control]", colors[ColorId::LIGHT_BLUE]),
            gui.button([=] { callback(View::MinionAction{View::MinionAction::ControlAction()}); })));
        break;
      case GameInfo::PlayerInfo::RENAME:
        line.push_back(gui.stack(
            gui.label("[Rename]", colors[ColorId::LIGHT_BLUE]),
            gui.button([=] { 
                if (auto name = getTextInput("Rename minion", minion.firstName, 10, "Press escape to cancel."))
                callback(View::MinionAction{View::MinionAction::RenameAction{*name}}); })));
        break;
      case GameInfo::PlayerInfo::BANISH:
        line.push_back(gui.stack(
            gui.label("[Banish]", colors[ColorId::LIGHT_BLUE]),
            gui.button([=] { callback(View::MinionAction{View::MinionAction::BanishAction()}); })));
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

PGuiElem GuiBuilder::drawMinionPage(const GameInfo::PlayerInfo& minion, MinionMenuCallback callback) {
  GuiFactory::ListBuilder list(gui, legendLineHeight);
  list.addElem(gui.label(minion.getTitle()));
  list.addElem(gui.horizontalList(drawMinionActions(minion, callback), 140));
  vector<PGuiElem> leftLines;
  leftLines.push_back(gui.label("Attributes", colors[ColorId::YELLOW]));
  for (auto& elem : drawAttributesOnPage(drawPlayerAttributes(minion.attributes)))
    leftLines.push_back(std::move(elem));
  for (auto& elem : drawEffectsList(minion))
    leftLines.push_back(std::move(elem));
  leftLines.push_back(gui.empty());
  leftLines.push_back(gui.label("Activity", colors[ColorId::YELLOW]));
  leftLines.push_back(drawActivityButton(minion, callback));
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
          drawEquipmentAndConsumables(minion.inventory, callback)), legendLineHeight)),
      topMargin, GuiFactory::TOP);
}

PGuiElem GuiBuilder::drawMinionMenu(const vector<GameInfo::PlayerInfo>& minions,
    UniqueEntity<Creature>::Id& current, MinionMenuCallback callback) {
  vector<PGuiElem> minionPages;
  for (int i : All(minions)) {
    auto minionId = minions[i].creatureId;
    minionPages.push_back(gui.conditional(gui.margins(drawMinionPage(minions[i], callback),
          10, 15, 10, 10), [minionId, &current] (GuiElem*) { return minionId == current; }));
  }
  int minionListWidth = 180;
  return gui.stack(
      gui.keyHandler([callback] { callback(none); }, {{Keyboard::Escape}}),
      gui.horizontalList(makeVec<PGuiElem>(
          gui.leftMargin(8, gui.topMargin(15, drawMinionButtons(minions, current))),
          gui.margins(gui.sprite(GuiFactory::TexId::VERT_BAR_MINI, GuiFactory::Alignment::LEFT),
            0, -15, 0, -15)), minionListWidth),
      gui.leftMargin(minionListWidth + 20, gui.stack(std::move(minionPages))));
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
      GuiElem::propagateEvent(event, {stuff.get()});
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


