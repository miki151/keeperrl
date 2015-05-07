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
  line.push_back(gui.viewObject(button.object, tilesOk));
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
      gui.horizontalList(std::move(line), widths, 0, button.cost ? 2 : 0));
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
      line.push_back(gui.viewObject(button1.object, tilesOk));
      line.push_back(gui.label(groupedButtons.front().groupName,
            active <= lastItem && active >= firstItem ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
      elems.push_back(gui.stack(
            gui.button(buttonFun),
            gui.horizontalList(std::move(line), 35, 0)));
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
      legendLineHeight, 0), &buildingsScroll, &scrollbarsHeld);
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
              getStandingGui(info.deities[i].standing)), 85, 0), 40, 0, 0, 0));
  }
  return gui.verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem GuiBuilder::drawTechnology(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines = drawButtons(info.libraryButtons, activeLibrary, CollectiveTab::TECHNOLOGY);
  for (int i : All(info.techButtons)) {
    vector<PGuiElem> line;
    line.push_back(gui.viewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, ""), tilesOk));
    line.push_back(gui.label(info.techButtons[i].name, colors[ColorId::WHITE], info.techButtons[i].hotkey));
    lines.push_back(gui.stack(gui.button(
          getButtonCallback(UserInput(UserInputId::TECHNOLOGY, i)), info.techButtons[i].hotkey),
          gui.horizontalList(std::move(line), 35, 0)));
  }
  return gui.verticalList(std::move(lines), legendLineHeight, 0);
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
  return gui.verticalList(std::move(lines), legendLineHeight, 0);
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
    res.push_back(gui.viewObject(info.numResource[i].viewObject, tilesOk));
    res.push_back(gui.label(toString<int>(info.numResource[i].count),
          info.numResource[i].count >= 0 ? colors[ColorId::WHITE] : colors[ColorId::RED]));
    topLine.push_back(gui.stack(getHintCallback({info.numResource[i].name}),
          gui.horizontalList(std::move(res), 30, 0)));
  }
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(getTurnInfoGui(gameInfo.time));
  bottomLine.push_back(getSunlightInfoGui(sunlightInfo));
  bottomLine.push_back(gui.label("population: " + toString(gameInfo.bandInfo.minionCount) + " / " + toString(gameInfo.bandInfo.minionLimit)));
  int numTop = topLine.size();
  int numBottom = bottomLine.size();
  return gui.verticalList(makeVec<PGuiElem>(
      gui.centerHoriz(gui.horizontalList(std::move(topLine), resourceSpace, 0), numTop * resourceSpace),
      gui.centerHoriz(gui.horizontalList(std::move(bottomLine), 140, 0, 3), numBottom * 140)), 28, 0);
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
      gui.centerHoriz(gui.horizontalList(std::move(buttons), 50, 0), numButtons * 50), 0, 5, 0, 5);
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(gui.stack(
      gui.horizontalList(makeVec<PGuiElem>(
          gui.label("speed:"),
          gui.label(getCurrentGameSpeedName(),
              colors[clock->isPaused() ? ColorId::RED : ColorId::WHITE])), 60, 0),
      gui.button([&] { gameSpeedDialogOpen = !gameSpeedDialogOpen; })));
  bottomLine.push_back(
      gui.label("FPS " + toString(fpsCounter.getFps()) + " / " + toString(upsCounter.getFps()),
      colors[ColorId::WHITE]));
  PGuiElem bottomElem = gui.horizontalList(std::move(bottomLine), 160, 0);
  bottomElem = gui.verticalList(makeVec<PGuiElem>(
      gui.label(renderer.setViewCount > 1000 ? ("Warning: " + toString(renderer.setViewCount)) : "",
          colors[ColorId::RED]),
      std::move(bottomElem)), 30, 0);
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
            gui.label("[space]")), keyMargin, 0),
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
             gui.label("'" + string(1, getHotkeyChar(speed)) + "' ", color)), keyMargin, 0),
          gui.button([=] { gameSpeed = speed; gameSpeedDialogOpen = false; clock->cont();},
            getHotkey(speed))));
  }
  reverse(lines.begin(), lines.end());
  Vec2 size(150, legendLineHeight * lines.size() - 10);
  PGuiElem dialog = gui.verticalList(std::move(lines), legendLineHeight, 0);
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
    gui.horizontalList(std::move(line), renderer.getTextLength(sunlightInfo.description) + 5, 0));
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

PGuiElem GuiBuilder::drawBottomPlayerInfo(GameInfo& gameInfo) {
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  vector<PGuiElem> topLine;
  for (int i : All(info.attributes)) {
    auto& elem = info.attributes[i];
    topLine.push_back(gui.stack(getTooltip({elem.name, elem.help}),
        gui.horizontalList(makeVec<PGuiElem>(
          gui.icon(getAttrIcon(elem.id)),
          gui.margins(gui.label(toString(elem.value), getBonusColor(elem.bonus)), 0, 2, 0, 0)), 30, 1)));
  }
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(getTurnInfoGui(gameInfo.time));
  bottomLine.push_back(getSunlightInfoGui(gameInfo.sunlightInfo));
  int numTop = topLine.size();
  int numBottom = bottomLine.size();
  return gui.verticalList(makeVec<PGuiElem>(
      gui.centerHoriz(gui.horizontalList(std::move(topLine), resourceSpace, 0), numTop * resourceSpace),
      gui.centerHoriz(gui.horizontalList(std::move(bottomLine), 140, 0, 3), numBottom * 140)), 28, 0);
}

static int viewObjectWidth = 27;
vector<string> GuiBuilder::getItemHint(const GameInfo::PlayerInfo::ItemInfo& item) {
  vector<string> out { capitalFirst(item.fullName)};
  if (!item.description.empty())
    out.push_back(item.description);
  if (item.equiped)
    out.push_back("Equiped.");
  return out;
}

PGuiElem GuiBuilder::getItemLine(const GameInfo::PlayerInfo::ItemInfo& item, function<void(Rectangle)> onClick) {
  vector<PGuiElem> line;
  vector<int> widths;
  line.push_back(gui.viewObject(item.viewObject, tilesOk));
  widths.push_back(viewObjectWidth);
  Color color = colors[item.equiped ? ColorId::GREEN : ColorId::WHITE];
  if (item.number > 1) {
    line.push_back(gui.label(toString(item.number), color));
    widths.push_back(renderer.getTextLength(toString(item.number) + " "));
  }
  line.push_back(gui.label(item.name, color));
  widths.push_back(100);
  return gui.margins(gui.stack(gui.horizontalList(std::move(line), widths, 0),
      getTooltip(getItemHint(item)),
      gui.button(onClick)), -4, 0, 0, 0);
}

PGuiElem GuiBuilder::getTooltip(const vector<string>& text) {
  return gui.conditional(gui.tooltip(text), [this] (GuiElem*) { return !disableTooltip;});
}

void GuiBuilder::drawPlayerOverlay(vector<OverlayInfo>& ret, GameInfo::PlayerInfo& info) {
  if (info.lyingItems.empty())
    return;
  vector<PGuiElem> lines;
  const int maxElems = 6;
  const string title = "Click to pick up:";
  int numElems = min<int>(maxElems, info.lyingItems.size());
  Vec2 size = Vec2(60 + renderer.getTextLength(title), (1 + numElems) * legendLineHeight);
  if (!info.lyingItems.empty()) {
    for (int i : All(info.lyingItems)) {
      size.x = max(size.x, 40 + viewObjectWidth + renderer.getTextLength(info.lyingItems[i].name));
      lines.push_back(getItemLine(info.lyingItems[i],
          [this, i](Rectangle) { callbacks.inputCallback({UserInputId::PICK_UP_ITEM, i}); }));
    }
  }
  ret.push_back({gui.margin(
        gui.label(title),
        gui.scrollable(
          gui.verticalList(std::move(lines), legendLineHeight, 0), &lyingItemsScroll),
        legendLineHeight, GuiFactory::TOP),
      size, OverlayInfo::TOP_RIGHT});
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
  return gui.verticalList(std::move(lines), legendLineHeight, 0);
}

static string getActionText(GameInfo::PlayerInfo::ItemInfo::Action a) {
  switch (a) {
    case GameInfo::PlayerInfo::ItemInfo::DROP: return "drop";
    case GameInfo::PlayerInfo::ItemInfo::EQUIP: return "equip";
    case GameInfo::PlayerInfo::ItemInfo::THROW: return "throw";
    case GameInfo::PlayerInfo::ItemInfo::UNEQUIP: return "remove";
    case GameInfo::PlayerInfo::ItemInfo::APPLY: return "apply";
  }
}

const int listLineHeight = 30;
const int listBrokenLineHeight = 24;

void GuiBuilder::handleItemChoice(const GameInfo::PlayerInfo::ItemInfo& itemInfo, Vec2 menuPos) {
  renderer.flushEvents(Event::KeyPressed);
  int contentHeight;
  int choice = -1;
  int index = 0;
  disableTooltip = true;
  DestructorFunction dFun([this] { disableTooltip = false; });
  vector<string> options = transform2<string>(itemInfo.actions, bindFunction(getActionText));
  if (options.empty())
    return;
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
      if (choice > -1) {
        if (index > -1 && index < itemInfo.actions.size())
          callbacks.inputCallback({UserInputId::INVENTORY_ITEM, InventoryItemInfo(itemInfo.ids,
                itemInfo.actions[index])});
        return;
      }
      if (choice == -100)
        return;
      if (event.type == Event::MouseButtonPressed && 
          !Vec2(event.mouseButton.x, event.mouseButton.x).inRectangle(stuff->getBounds()))
        return;
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
                callbacks.inputCallback({UserInputId::INVENTORY_ITEM, InventoryItemInfo(itemInfo.ids,
                      itemInfo.actions[index])});
                return;
              }
          case Keyboard::Escape: return;
          default: break;
        }
    }
  }
}

string GuiBuilder::getPlayerTitle(GameInfo::PlayerInfo& info) {
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives)) {
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
    break; // more than one won't fit the line
  }
  return title;
}

PGuiElem GuiBuilder::drawPlayerInventory(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> lines;
  string title = getPlayerTitle(info);
  string nameLine = capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title);
  lines.push_back(gui.label(nameLine, colors[ColorId::WHITE]));
  lines.push_back(gui.label("Level " + toString(info.level), colors[ColorId::WHITE]));
  if (!info.effects.empty()) {
    for (auto effect : info.effects)
      lines.push_back(gui.stack(
            getTooltip({effect.help}),
            gui.label(effect.name, effect.bad ? colors[ColorId::RED] : colors[ColorId::WHITE])));
  }
  lines.push_back(gui.empty());
  if (!info.team.empty()) {
    const int numPerLine = 6;
    vector<int> widths { 60 };
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        gui.label("Team: ", colors[ColorId::WHITE]));
    for (auto& elem : info.team) {
      currentLine.push_back(gui.stack(
            gui.viewObject(elem.viewObject, tilesOk),
            gui.label(toString(elem.expLevel), 12)));
      widths.push_back(30);
      if (currentLine.size() >= numPerLine) {
        lines.push_back(gui.horizontalList(std::move(currentLine), widths, 0));
        widths.clear();
      }
    }
    if (!currentLine.empty())
      lines.push_back(gui.horizontalList(std::move(currentLine), widths, 0));
    lines.push_back(gui.empty());
  }
  if (!info.skills.empty()) {
    lines.push_back(gui.label("Skills", colors[ColorId::YELLOW]));
    for (auto& elem : info.skills)
      lines.push_back(gui.stack(getTooltip({elem.help}),
            gui.label(capitalFirst(elem.name), colors[ColorId::WHITE])));
    lines.push_back(gui.empty());
  }
  if (!info.spells.empty()) {
    lines.push_back(gui.label("Spells", colors[ColorId::YELLOW]));
    for (int i : All(info.spells))
      lines.push_back(gui.stack(getTooltip({info.spells[i].help}),
            gui.button(getButtonCallback({UserInputId::CAST_SPELL, i})),
            gui.label(capitalFirst(info.spells[i].name),
                colors[info.spells[i].available ? ColorId::WHITE : ColorId::GRAY])));
    lines.push_back(gui.empty());
  }
  if (!info.inventory.empty()) {
    lines.push_back(gui.label("Inventory", colors[ColorId::YELLOW]));
    for (auto& section : info.inventory) {
      //   lines.push_back(gui.label(section.title, colors[ColorId::YELLOW]));
      for (auto& item : section.items)
        lines.push_back(getItemLine(item, [=](Rectangle butBounds) {
              handleItemChoice(item, butBounds.getBottomLeft() + Vec2(50, 0)); }));
    }
  }
  return gui.margins(
      gui.scrollable(gui.verticalList(std::move(lines), legendLineHeight, 0), &inventoryScroll), -5, 0, 0, 0);
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
      gui.centerHoriz(gui.horizontalList(std::move(buttons), 50, 0), numButtons * 50), 0, 5, 0, 5);
  return gui.margin(std::move(butGui), std::move(main), 55, gui.TOP);
}

PGuiElem GuiBuilder::drawPlayerStats(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> elems;
  elems.push_back(gui.empty());
  elems.push_back(gui.empty());
  elems.push_back(gui.label("Skills:", colors[ColorId::WHITE]));
  for (auto& elem : info.skills) {
    elems.push_back(gui.stack(getTooltip({elem.help}),
          gui.label(capitalFirst(elem.name), colors[ColorId::WHITE])));
  }
  elems.push_back(gui.empty());
  if (!info.team.empty()) {
    const int numPerLine = 6;
    vector<int> widths { 60 };
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        gui.label("Team: ", colors[ColorId::WHITE]));
    for (auto& elem : info.team) {
      currentLine.push_back(gui.stack(
            gui.viewObject(elem.viewObject, tilesOk),
            gui.label(toString(elem.expLevel), 12)));
      widths.push_back(30);
      if (currentLine.size() >= numPerLine) {
        elems.push_back(gui.horizontalList(std::move(currentLine), widths, 0));
        widths.clear();
      }
    }
    if (!currentLine.empty())
      elems.push_back(gui.horizontalList(std::move(currentLine), widths, 0));
    elems.push_back(gui.empty());
  }
  for (auto effect : info.effects)
    elems.push_back(gui.label(effect.name, effect.bad ? colors[ColorId::RED] : colors[ColorId::GREEN]));
  return gui.scrollable(gui.verticalList(std::move(elems), legendLineHeight, 0), &playerStatsScroll);
}

typedef GameInfo::CreatureInfo CreatureInfo;

struct CreatureMapElem {
  ViewObject object;
  int count;
  CreatureInfo any;
};

static map<string, CreatureMapElem> getCreatureMap(vector<CreatureInfo>& creatures) {
  map<string, CreatureMapElem> creatureMap;
  for (int i : All(creatures)) {
    auto elem = creatures[i];
    if (!creatureMap.count(elem.speciesName)) {
      creatureMap.insert(make_pair(elem.speciesName, CreatureMapElem({elem.viewObject, 1, elem})));
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
    line.push_back(gui.viewObject(elem.second.object, tilesOk));
    widths.push_back(40);
    Color col = elem.first == chosenCreature ? colors[ColorId::GREEN] : colors[ColorId::WHITE];
    line.push_back(gui.label(toString(elem.second.count) + "   " + elem.first, col));
    widths.push_back(200);
    function<void()> action;
    if (chosenCreature == elem.first)
      action = [this] () {
        chosenCreature = "";
      };
    else
      action = [this, elem] () {
        chosenCreature = elem.first;
        showTasks = false;
      };
    list.push_back(gui.margins(gui.stack(gui.button(action),
            gui.horizontalList(std::move(line), widths, 0)), 20, 0, 0, 0));
  }
  list.push_back(gui.label("Teams: ", colors[ColorId::WHITE]));
  const int elemWidth = 30;
  for (auto elem : info.teams) {
    TeamId teamId = elem.first;
    const auto team = elem.second;
    const int numPerLine = 8;
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        gui.stack(
          gui.button(getButtonCallback({UserInputId::EDIT_TEAM, teamId})),
          info.currentTeam == teamId 
              ? gui.viewObject(ViewId::TEAM_BUTTON_HIGHLIGHT, tilesOk)
              : gui.viewObject(ViewId::TEAM_BUTTON, tilesOk),
          gui.mouseHighlight2(gui.viewObject(ViewId::TEAM_BUTTON_HIGHLIGHT, tilesOk))));
    for (auto elem : team) {
      currentLine.push_back(gui.stack(makeVec<PGuiElem>(
            gui.button(getButtonCallback({UserInputId::SET_TEAM_LEADER, TeamLeaderInfo(teamId, elem)})),
            gui.viewObject(info.getMinion(elem).viewObject, tilesOk),
            gui.mouseHighlight2(gui.margins(
                gui.rectangle(colors[ColorId::TRANSPARENT], colors[ColorId::WHITE]), -3, -2, 3, 2)),
            gui.label(toString(info.getMinion(elem).expLevel), 12))));
      if (currentLine.size() >= numPerLine)
        list.push_back(gui.horizontalList(std::move(currentLine), elemWidth, 0));
    }
    if (!currentLine.empty())
      list.push_back(gui.horizontalList(std::move(currentLine), elemWidth, 0));
    if (info.currentTeam == teamId)
      list.push_back(gui.horizontalList(makeVec<PGuiElem>(
              gui.stack(
                  gui.button(getButtonCallback({UserInputId::COMMAND_TEAM, teamId})),
                  gui.label("[command]")),
              gui.stack(
                  gui.button(getButtonCallback({UserInputId::CANCEL_TEAM, teamId})),
                  gui.label("[disband]"))), 100, 0));
  }
  if (!info.newTeam)
    list.push_back(gui.stack(
        gui.button(getButtonCallback(UserInputId::CREATE_TEAM)),
        gui.label("[new team]", colors[ColorId::WHITE])));
  else
    list.push_back(gui.horizontalList(makeVec<PGuiElem>(
        gui.viewObject(ViewId::TEAM_BUTTON_HIGHLIGHT, tilesOk),
        gui.label("Click on minions to add.", colors[ColorId::LIGHT_BLUE])), elemWidth, 0));
  list.push_back(gui.empty());
  if (info.payoutTimeRemaining > -1) {
    vector<PGuiElem> res;
    res.push_back(gui.label("Next payout [" + toString(info.payoutTimeRemaining) + "]:",
          colors[ColorId::WHITE]));
    res.push_back(gui.viewObject(info.numResource[0].viewObject, tilesOk));
    res.push_back(gui.label(toString<int>(info.nextPayout), colors[ColorId::WHITE]));
    list.push_back(gui.horizontalList(std::move(res), {170, 30, 1}, 0));
  }
  list.push_back(gui.horizontalList(makeVec<PGuiElem>(
          gui.stack(
            gui.label("Show tasks", colors[showTasks ? ColorId::GREEN : ColorId::WHITE]),
            gui.button([this] { showTasks = !showTasks; chosenCreature = ""; })),
          gui.stack(
            getHintCallback({"Morale affects minion's productivity and chances of fleeing from battle."}),
            gui.label("Show morale", colors[morale ? ColorId::GREEN : ColorId::WHITE]),
            gui.button([this] { morale = !morale; }))
      ), 120, 0));
  list.push_back(gui.empty());
  if (!enemyMap.empty()) {
    list.push_back(gui.label("Enemies:", colors[ColorId::WHITE]));
    for (auto elem : enemyMap){
      vector<PGuiElem> line;
      line.push_back(gui.viewObject(elem.second.object, tilesOk));
      line.push_back(gui.label(toString(elem.second.count) + "   " + elem.first, colors[ColorId::WHITE]));
      list.push_back(gui.horizontalList(std::move(line), 20, 0));
    }
  }
  if (!creatureMap.count(chosenCreature)) {
    chosenCreature = "";
  }
  return gui.scrollable(gui.verticalList(std::move(list), legendLineHeight, 0), &minionsScroll, &scrollbarsHeld);
}

bool GuiBuilder::showMorale() const {
  return morale;
}

const int taskMapWindowWidth = 350;

void GuiBuilder::drawTasksOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
  if (info.taskMap.empty())
    return;
  vector<PGuiElem> lines;
  vector<PGuiElem> freeLines;
  for (auto& elem : info.taskMap)
    if (elem.creature)
      lines.push_back(gui.horizontalList(makeVec<PGuiElem>(
            gui.viewObject(info.getMinion(*elem.creature).viewObject, tilesOk),
            gui.label(elem.name, colors[elem.priority ? ColorId::GREEN : ColorId::WHITE])), 35, 0));
    else
      freeLines.push_back(gui.horizontalList(makeVec<PGuiElem>(
            gui.empty(),
            gui.label(elem.name, colors[elem.priority ? ColorId::GREEN : ColorId::WHITE])), 35, 0));
  int lineHeight = 25;
  append(lines, std::move(freeLines));
  ret.push_back({gui.verticalList(std::move(lines), lineHeight, 0),
      Vec2(taskMapWindowWidth, (info.taskMap.size() + 1) * lineHeight),
      OverlayInfo::TOP_RIGHT});
}

const int minionWindowWidth = 300;

void GuiBuilder::drawMinionsOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
  if (showTasks) {
    drawTasksOverlay(ret, info);
    return;
  }
  if (chosenCreature == "")
    return;
  vector<PGuiElem> lines;
  vector<CreatureInfo> chosen;
  for (auto& c : info.minions)
    if (c.speciesName == chosenCreature)
      chosen.push_back(c);
  lines.push_back(gui.label(info.currentTeam ? "Click to add to team:" : "Click to control:",
        colors[ColorId::LIGHT_BLUE]));
  for (auto& c : chosen) {
    string text = "L: " + toString(c.expLevel) + "    " + info.tasks[c.uniqueId];
    if (c.speciesName != c.name)
      text = c.name + " " + text;
    vector<PGuiElem> line;
    line.push_back(gui.viewObject(c.viewObject, tilesOk));
    line.push_back(gui.label(text, (info.currentTeam && contains(info.teams[*info.currentTeam],c.uniqueId))
          ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
    lines.push_back(gui.stack(
          gui.button(getButtonCallback(UserInput(UserInputId::CREATURE_BUTTON, c.uniqueId))),
          gui.horizontalList(std::move(line), 40, 0)));
  }
  ret.push_back({gui.verticalList(std::move(lines), legendLineHeight, 0),
      Vec2(minionWindowWidth, (chosen.size() + 2) * legendLineHeight),
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
        gui.centeredLabel("[close]", colors[ColorId::LIGHT_BLUE]),
        gui.button([=] { hideBuildingOverlay = true;})));
  int height = lines.size() * legendLineHeight - 8;
  ret.push_back({gui.verticalList(std::move(lines), legendLineHeight, 0),
      Vec2(minionWindowWidth, height),
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
  vector<vector<PlayerMessage>> messages = fitMessages(renderer, messageBuffer, maxMessageLength,
      getNumMessageLines());
  int lineHeight = 20;
  vector<PGuiElem> lines;
  for (int i : All(messages)) {
    vector<PGuiElem> line;
    vector<int> lengths;
    for (auto& message : messages[i]) {
      string text = (line.empty() ? "" : " ") + message.getText();
      cutToFit(renderer, text, maxMessageLength);
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
      lines.push_back(gui.horizontalList(std::move(line), lengths, 0));
  }
  if (!lines.empty())
    ret.push_back({gui.verticalList(std::move(lines), lineHeight, 0),
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
  return gui.verticalList(std::move(lines), legendLineHeight, 0);
}

const double menuLabelVPadding = 0.15;

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
      line = gui.verticalList(std::move(label1), listBrokenLineHeight, 0);
    else
      line = std::move(getOnlyElement(label1));
    if (!options[i].getTip().empty())
      line = gui.stack(std::move(line),
          gui.tooltip({options[i].getTip()}));
    if (!options[i].getSecondColumn().empty())
      line = gui.horizontalList(makeVec<PGuiElem>(std::move(line),
            gui.label(options[i].getSecondColumn())), columnWidth + 80, 0);
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
    return gui.verticalList(std::move(lines), heights, 0);
  else
    return gui.verticalListFit(std::move(lines), 0.0);
}

