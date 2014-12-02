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
#include "window_renderer.h"

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

GuiBuilder::GuiBuilder(Renderer& r, InputCallback in, HintCallback hint, KeyboardCallback key)
    : renderer(r), inputCallback(in), hintCallback(hint), keyboardCallback(key) {
}

void GuiBuilder::reset() {
  chosenCreature = "";
  activeBuilding = 0;
  activeLibrary = -1;
}

const int legendLineHeight = 30;

PGuiElem GuiBuilder::getHintCallback(const string& s) {
  return GuiElem::mouseOverAction([this, s]() { hintCallback(s); });
}

function<void()> GuiBuilder::getButtonCallback(UserInput input) {
  return [this, input]() { inputCallback(input); };
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
  line.push_back(GuiElem::viewObject(button.object, tilesOk));
  vector<int> widths { 35 };
  if (!button.inactiveReason.empty())
    line.push_back(GuiElem::label(button.name + " " + button.count, colors[ColorId::GRAY], button.hotkey));
  else
    line.push_back(GuiElem::conditional(
          GuiElem::label(button.name + " " + button.count, colors[ColorId::GREEN], button.hotkey),
          GuiElem::label(button.name + " " + button.count, colors[ColorId::WHITE], button.hotkey),
          [=, &active] (GuiElem*) { return active == num; }));
  widths.push_back(100);
  if (button.cost) {
    string costText = toString(button.cost->second);
    line.push_back(GuiElem::label(costText, colors[ColorId::WHITE]));
    widths.push_back(renderer.getTextLength(costText) + 8);
    line.push_back(GuiElem::viewObject(button.cost->first, tilesOk));
    widths.push_back(25);
  }
  function<void()> buttonFun;
  if (button.inactiveReason == "" || button.inactiveReason == "inactive")
    buttonFun = [this, &active, num, tab] {
      active = num;
      setCollectiveTab(tab);
    };
  else {
    string s = button.inactiveReason;
    buttonFun = [this, s] {
//      presentText("", s);
    };
  }
  return GuiElem::stack(
      getHintCallback(capitalFirst(button.help)),
      GuiElem::button(buttonFun, button.hotkey),
      GuiElem::horizontalList(std::move(line), widths, 0, button.cost ? 2 : 0));
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
      //  setOptionsMenu(button1.groupName, groupedButtons, firstItem, active, tab);
        active = firstItem;
      };
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(button1.object, tilesOk));
      line.push_back(GuiElem::label(groupedButtons.front().groupName,
            active <= lastItem && active >= firstItem ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
      elems.push_back(GuiElem::stack(
            GuiElem::button(buttonFun),
            GuiElem::horizontalList(std::move(line), 35, 0)));
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
  elems.push_back(GuiElem::invisible(GuiElem::stack(std::move(invisible))));
  return elems;
}

PGuiElem GuiBuilder::drawBuildings(GameInfo::BandInfo& info) {
  return GuiElem::verticalList(drawButtons(info.buildings, activeBuilding, CollectiveTab::BUILDINGS),
      legendLineHeight, 0);
}

static PGuiElem getStandingGui(double standing) {
  sf::Color color = standing >= 0 ? sf::Color((1. - standing) * 255, 255, (1. - standing) * 255)
    : sf::Color(255, (1. + standing) * 255, (1. + standing) * 255);
  if (standing < -0.33)
    return GuiElem::label("bad", color);
  if (standing < 0.33)
    return GuiElem::label("neutral", color);
  else
    return GuiElem::label("good", color);
}

PGuiElem GuiBuilder::drawDeities(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines;
  for (int i : All(info.deities)) {
    lines.push_back(GuiElem::stack(
          GuiElem::button(getButtonCallback(UserInput(UserInputId::DEITIES, i))),
          GuiElem::label(capitalFirst(info.deities[i].name), colors[ColorId::WHITE])));
    lines.push_back(GuiElem::margins(GuiElem::horizontalList(makeVec<PGuiElem>(
              GuiElem::label("standing: ", colors[ColorId::WHITE]),
              getStandingGui(info.deities[i].standing)), 85, 0), 40, 0, 0, 0));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem GuiBuilder::drawTechnology(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines = drawButtons(info.libraryButtons, activeLibrary, CollectiveTab::TECHNOLOGY);
  for (int i : All(info.techButtons)) {
    vector<PGuiElem> line;
    line.push_back(GuiElem::viewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, ""), tilesOk));
    line.push_back(GuiElem::label(info.techButtons[i].name, colors[ColorId::WHITE], info.techButtons[i].hotkey));
    lines.push_back(GuiElem::stack(GuiElem::button(
          getButtonCallback(UserInput(UserInputId::TECHNOLOGY, i)), info.techButtons[i].hotkey),
          GuiElem::horizontalList(std::move(line), 35, 0)));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
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
    lines.push_back(GuiElem::label(line, colors[ColorId::LIGHT_BLUE]));
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
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

PGuiElem GuiBuilder::drawBottomBandInfo(GameInfo& gameInfo) {
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  GameInfo::SunlightInfo& sunlightInfo = gameInfo.sunlightInfo;
  vector<PGuiElem> topLine;
  int resourceSpace = 110;
  for (int i : All(info.numResource)) {
    vector<PGuiElem> res;
    res.push_back(GuiElem::viewObject(info.numResource[i].viewObject, tilesOk));
    res.push_back(GuiElem::label(toString<int>(info.numResource[i].count),
          info.numResource[i].count >= 0 ? colors[ColorId::WHITE] : colors[ColorId::RED]));
    topLine.push_back(GuiElem::stack(getHintCallback(info.numResource[i].name),
          GuiElem::horizontalList(std::move(res), 30, 0)));
  }
  vector<PGuiElem> bottomLine;
  bottomLine.push_back(getTurnInfoGui(gameInfo.time));
  bottomLine.push_back(getSunlightInfoGui(sunlightInfo));
  int numTop = topLine.size();
  int numBottom = bottomLine.size();
  return GuiElem::verticalList(makeVec<PGuiElem>(
      GuiElem::centerHoriz(GuiElem::horizontalList(std::move(topLine), resourceSpace, 0), numTop * resourceSpace),
      GuiElem::centerHoriz(GuiElem::horizontalList(std::move(bottomLine), 140, 0, 3), numBottom * 140)), 28, 0);
}

PGuiElem GuiBuilder::drawRightBandInfo(GameInfo::BandInfo& info, GameInfo::VillageInfo& villageInfo) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
      GuiElem::icon(GuiElem::BUILDING),
      GuiElem::icon(GuiElem::MINION),
      GuiElem::icon(GuiElem::LIBRARY),
      GuiElem::icon(GuiElem::DEITIES),
      GuiElem::icon(GuiElem::DIPLOMACY),
      GuiElem::icon(GuiElem::HELP));
  for (int i : All(buttons)) {
    if (int(collectiveTab) == i)
      buttons[i] = GuiElem::border2(std::move(buttons[i]));
    else
      buttons[i] = GuiElem::margins(std::move(buttons[i]), 6, 6, 6, 6);
    buttons[i] = GuiElem::stack(std::move(buttons[i]),
        GuiElem::button([this, i]() { setCollectiveTab(CollectiveTab(i)); }));
  }
  if (collectiveTab != CollectiveTab::MINIONS)
    chosenCreature = "";
  PGuiElem main;
  vector<PGuiElem> invisible;
  vector<pair<CollectiveTab, PGuiElem>> elems = makeVec<pair<CollectiveTab, PGuiElem>>(
      make_pair(CollectiveTab::MINIONS, drawMinions(info)),
      make_pair(CollectiveTab::BUILDINGS, drawBuildings(info)),
      make_pair(CollectiveTab::KEY_MAPPING, drawKeeperHelp()),
      make_pair(CollectiveTab::TECHNOLOGY, drawTechnology(info)),
      make_pair(CollectiveTab::WORKSHOP, drawDeities(info)),
      make_pair(CollectiveTab::VILLAGES, drawVillages(villageInfo)));
  for (auto& elem : elems)
    if (elem.first == collectiveTab)
      main = std::move(elem.second);
    else
      invisible.push_back(GuiElem::invisible(std::move(elem.second)));
  main = GuiElem::margins(std::move(main), 15, 15, 15, 5);
  PGuiElem butGui = GuiElem::margins(GuiElem::horizontalList(std::move(buttons), 50, 0), 0, 5, 0, 5);
  vector<PGuiElem> bottomLine;
  if (Clock::get().isPaused())
    bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { Clock::get().cont(); }),
          GuiElem::label("PAUSED", colors[ColorId::RED])));
  else
    bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { Clock::get().pause(); }),
          GuiElem::label("PAUSE", colors[ColorId::LIGHT_BLUE])));
  /*bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { switchZoom(); }),
        GuiElem::label("ZOOM", colors[ColorId::LIGHT_BLUE])));*/
  bottomLine.push_back(
      GuiElem::label("FPS " + toString(fpsCounter.getFps()), colors[ColorId::WHITE]));
  main = GuiElem::margin(GuiElem::margins(GuiElem::horizontalList(std::move(bottomLine), 90, 0), 30, 0, 0, 0),
      std::move(main), 48, GuiElem::BOTTOM);
  return GuiElem::stack(GuiElem::stack(std::move(invisible)),
      GuiElem::margin(std::move(butGui), std::move(main), 55, GuiElem::TOP));
}


PGuiElem GuiBuilder::getSunlightInfoGui(GameInfo::SunlightInfo& sunlightInfo) {
  vector<PGuiElem> line;
  Color color = sunlightInfo.description == "day" ? colors[ColorId::WHITE] : colors[ColorId::LIGHT_BLUE];
  line.push_back(GuiElem::label(sunlightInfo.description, color));
  line.push_back(GuiElem::label("[" + toString(sunlightInfo.timeRemaining) + "]", color));
  return GuiElem::stack(
    getHintCallback(sunlightInfo.description == "day"
      ? "Time remaining till nightfall." : "Time remaining till day."),
    GuiElem::horizontalList(std::move(line), renderer.getTextLength(sunlightInfo.description) + 5, 0));
}

PGuiElem GuiBuilder::getTurnInfoGui(int turn) {
  return GuiElem::stack(getHintCallback("Current turn"),
      GuiElem::label("T: " + toString(turn), colors[ColorId::WHITE]));
}

PGuiElem GuiBuilder::drawBottomPlayerInfo(GameInfo& gameInfo) {
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  GameInfo::SunlightInfo& sunlightInfo = gameInfo.sunlightInfo;
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives))
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
  vector<PGuiElem> topLine;
  vector<int> topWidths;
  string nameLine = capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title);
  topLine.push_back(GuiElem::label(nameLine, colors[ColorId::WHITE]));
  topWidths.push_back(renderer.getTextLength(nameLine) + 50);
  topLine.push_back(GuiElem::label(info.levelName, colors[ColorId::WHITE]));
  topWidths.push_back(200);
  vector<PGuiElem> bottomLine;
  vector<int> bottomWidths;
  bottomLine.push_back(getTurnInfoGui(gameInfo.time));;
  bottomWidths.push_back(100);
  bottomLine.push_back(getSunlightInfoGui(sunlightInfo));
  bottomWidths.push_back(100);
  int keySpacing = 50;
  return GuiElem::verticalList(makeVec<PGuiElem>(
        GuiElem::centerHoriz(GuiElem::horizontalList(std::move(topLine), topWidths, 0), 300),
        GuiElem::centerHoriz(GuiElem::horizontalList(std::move(bottomLine), bottomWidths, keySpacing), 200)), 28, 0);
}

void GuiBuilder::drawPlayerOverlay(vector<OverlayInfo>& ret, GameInfo::PlayerInfo& info) {
  if (info.lyingItems.empty())
    return;
  vector<PGuiElem> lines;
  const int maxElems = 6;
  const string title = "Click to pick up:";
  int numElems = min<int>(maxElems, info.lyingItems.size());
  Vec2 size = Vec2(renderer.getTextLength(title), (1 + numElems) * legendLineHeight);
  if (!info.lyingItems.empty()) {
    lines.push_back(GuiElem::label(title));
    for (int i : All(info.lyingItems)) {
      if (i == maxElems - 1) {
        lines.push_back(GuiElem::stack(
            GuiElem::label("[more]", colors[ColorId::LIGHT_BLUE]),
            GuiElem::button(getButtonCallback(UserInputId::PICK_UP))));
        break;
      } else {
        int viewObjectWidth = 30;
        size.x = max(size.x, viewObjectWidth + renderer.getTextLength(info.lyingItems[i].name));
        lines.push_back(GuiElem::stack(GuiElem::horizontalList(makeVec<PGuiElem>(
                GuiElem::viewObject(info.lyingItems[i].viewObject, true),
                GuiElem::label(info.lyingItems[i].name)), viewObjectWidth, 0),
            GuiElem::button(getButtonCallback({UserInputId::PICK_UP_ITEM, i}))));
      }
    }
  }
  ret.push_back({GuiElem::verticalList(std::move(lines), legendLineHeight, 0), size, OverlayInfo::LEFT});
}

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

PGuiElem GuiBuilder::drawPlayerHelp(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> lines;
  vector<KeyInfo> bottomKeys =  {
      { "I", "Inventory", {Keyboard::I}},
      { "E", "Equipment", {Keyboard::E}},
      { "Enter", "Interact or pick up", {Keyboard::Return}},
      { "D", "Drop item", {Keyboard::D}},
      { "A", "Apply item", {Keyboard::A}},
      { "T", "Throw item", {Keyboard::T}},
      { "S", "Cast spell", {Keyboard::S}},
      { "U", "Leave minion", {Keyboard::U}},
      { "C", "Chat with someone", {Keyboard::C}},
      { "H", "Hide", {Keyboard::H}},
      { "P", "Pay debt", {Keyboard::P}},
      { "M", "Message history", {Keyboard::M}},
      { "Space", "Wait", {Keyboard::Space}},
      { "F1", "More actions", {Keyboard::F1}},
  };
/*  if (info.possessed)
    bottomKeys = concat({{ "U", "Leave minion", {Keyboard::U}}}, bottomKeys);
  if (info.spellcaster)
    bottomKeys = concat({{ "S", "Cast spell", {Keyboard::S}}}, bottomKeys);*/
  for (int i : All(bottomKeys)) {
    string text = "[" + bottomKeys[i].keyDesc + "] " + bottomKeys[i].action;
    Event::KeyEvent key = bottomKeys[i].event;
    lines.push_back(GuiElem::stack(
          GuiElem::label(text, colors[ColorId::LIGHT_BLUE]),
          GuiElem::button([this, key]() { keyboardCallback(key);})));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem GuiBuilder::drawRightPlayerInfo(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
    GuiElem::icon(GuiElem::MINION),
    GuiElem::icon(GuiElem::HELP));
  for (int i : All(buttons)) {
    if (int(minionTab) == i)
      buttons[i] = GuiElem::border2(std::move(buttons[i]));
    else
      buttons[i] = GuiElem::margins(std::move(buttons[i]), 6, 6, 6, 6);
    buttons[i] = GuiElem::stack(std::move(buttons[i]),
        GuiElem::button([this, i]() { minionTab = MinionTab(i); }));
  }
  PGuiElem main;
  vector<pair<MinionTab, PGuiElem>> elems = makeVec<pair<MinionTab, PGuiElem>>(
    make_pair(MinionTab::STATS, drawPlayerStats(info)),
    make_pair(MinionTab::HELP, drawPlayerHelp(info)));
  for (auto& elem : elems)
    if (elem.first == minionTab)
      main = std::move(elem.second);
  main = GuiElem::margins(std::move(main), 15, 24, 15, 5);
  int numButtons = buttons.size();
  PGuiElem butGui = GuiElem::margins(
      GuiElem::centerHoriz(GuiElem::horizontalList(std::move(buttons), 50, 0), numButtons * 50), 0, 5, 0, 5);
  return GuiElem::margin(std::move(butGui), std::move(main), 55, GuiElem::TOP);
}

static Color getBonusColor(int bonus) {
  if (bonus < 0)
    return colors[ColorId::RED];
  if (bonus > 0)
    return colors[ColorId::GREEN];
  return colors[ColorId::WHITE];
}

PGuiElem GuiBuilder::drawPlayerStats(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> elems;
  if (info.weaponName != "") {
    elems.push_back(GuiElem::label("Wielding:", colors[ColorId::WHITE]));
    elems.push_back(GuiElem::label(info.weaponName, colors[ColorId::WHITE]));
  } else
    elems.push_back(GuiElem::label("barehanded", colors[ColorId::RED]));
  elems.push_back(GuiElem::empty());
  for (auto& elem : info.attributes) {
    elems.push_back(GuiElem::stack(getHintCallback(elem.help),
        GuiElem::horizontalList(makeVec<PGuiElem>(
          GuiElem::label(capitalFirst(elem.name + ":"), colors[ColorId::WHITE]),
          GuiElem::label(toString(elem.value), getBonusColor(elem.bonus))), 100, 1)));
  }
  elems.push_back(GuiElem::empty());
  elems.push_back(GuiElem::label("Skills:", colors[ColorId::WHITE]));
  for (auto& elem : info.skills) {
    elems.push_back(GuiElem::stack(getHintCallback(elem.help),
          GuiElem::label(capitalFirst(elem.name), colors[ColorId::WHITE])));
  }
  elems.push_back(GuiElem::empty());
  if (!info.team.empty()) {
    const int numPerLine = 6;
    vector<int> widths { 60 };
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        GuiElem::label("Team: ", colors[ColorId::WHITE]));
    for (auto& elem : info.team) {
      currentLine.push_back(GuiElem::stack(
 //             GuiElem::button(getButtonCallback({UserInputId::SET_TEAM_LEADER, TeamLeaderInfo(teamId, elem)})),
            GuiElem::viewObject(elem.viewObject, tilesOk),
            GuiElem::label(toString(elem.expLevel), 12)));
      widths.push_back(30);
      if (currentLine.size() >= numPerLine) {
        elems.push_back(GuiElem::horizontalList(std::move(currentLine), widths, 0));
        widths.clear();
      }
    }
    if (!currentLine.empty())
      elems.push_back(GuiElem::horizontalList(std::move(currentLine), widths, 0));
    elems.push_back(GuiElem::empty());
  }
  for (auto effect : info.effects)
    elems.push_back(GuiElem::label(effect.name, effect.bad ? colors[ColorId::RED] : colors[ColorId::GREEN]));
  return GuiElem::verticalList(std::move(elems), legendLineHeight, 0);
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
  list.push_back(GuiElem::label(info.monsterHeader, colors[ColorId::WHITE]));
  for (auto elem : creatureMap){
    vector<PGuiElem> line;
    vector<int> widths;
    line.push_back(GuiElem::viewObject(elem.second.object, tilesOk));
    widths.push_back(40);
    Color col = elem.first == chosenCreature ? colors[ColorId::GREEN] : colors[ColorId::WHITE];
    line.push_back(GuiElem::label(toString(elem.second.count) + "   " + elem.first, col));
    widths.push_back(200);
    function<void()> action;
    if (chosenCreature == elem.first)
      action = [this] () {
        chosenCreature = "";
      };
    else
      action = [this, elem] () {
        chosenCreature = elem.first;
      };
    list.push_back(GuiElem::margins(GuiElem::stack(GuiElem::button(action),
            GuiElem::horizontalList(std::move(line), widths, 0)), 20, 0, 0, 0));
  }
  list.push_back(GuiElem::label("Teams: ", colors[ColorId::WHITE]));
  const int elemWidth = 30;
  for (auto elem : info.teams) {
    TeamId teamId = elem.first;
    const auto team = elem.second;
    const int numPerLine = 8;
    vector<PGuiElem> currentLine = makeVec<PGuiElem>(
        GuiElem::stack(
          GuiElem::button(getButtonCallback({UserInputId::EDIT_TEAM, teamId})),
          GuiElem::label("X", colors[info.currentTeam == teamId ? ColorId::GREEN : ColorId::WHITE])));
    for (auto elem : team) {
      currentLine.push_back(GuiElem::stack(
            GuiElem::button(getButtonCallback({UserInputId::SET_TEAM_LEADER, TeamLeaderInfo(teamId, elem)})),
            GuiElem::viewObject(info.getMinion(elem).viewObject, tilesOk),
            GuiElem::label(toString(info.getMinion(elem).expLevel), 12)));
      if (currentLine.size() >= numPerLine)
        list.push_back(GuiElem::horizontalList(std::move(currentLine), elemWidth, 0));
    }
    if (!currentLine.empty())
      list.push_back(GuiElem::horizontalList(std::move(currentLine), elemWidth, 0));
    if (info.currentTeam == teamId)
      list.push_back(GuiElem::horizontalList(makeVec<PGuiElem>(
              GuiElem::stack(
                  GuiElem::button(getButtonCallback({UserInputId::COMMAND_TEAM, teamId})),
                  GuiElem::label("[command]")),
              GuiElem::stack(
                  GuiElem::button(getButtonCallback({UserInputId::CANCEL_TEAM, teamId})),
                  GuiElem::label("[disband]"))), 100, 0));
  }
  if (!info.newTeam)
    list.push_back(GuiElem::stack(
        GuiElem::button(getButtonCallback(UserInputId::CREATE_TEAM)),
        GuiElem::label("[new team]", colors[ColorId::WHITE])));
  else
    list.push_back(GuiElem::horizontalList(makeVec<PGuiElem>(
        GuiElem::label("X", colors[ColorId::GREEN]),
        GuiElem::label("Click on minions to add.", colors[ColorId::LIGHT_BLUE])), elemWidth, 0));
  list.push_back(GuiElem::empty());
  if (info.payoutTimeRemaining > -1) {
    vector<PGuiElem> res;
    res.push_back(GuiElem::label("Next payout [" + toString(info.payoutTimeRemaining) + "]:",
          colors[ColorId::WHITE]));
    res.push_back(GuiElem::viewObject(info.numResource[0].viewObject, tilesOk));
    res.push_back(GuiElem::label(toString<int>(info.nextPayout), colors[ColorId::WHITE]));
    list.push_back(GuiElem::horizontalList(std::move(res), {170, 30, 1}, 0));
    list.push_back(GuiElem::empty());
  }
  if (!enemyMap.empty()) {
    list.push_back(GuiElem::label("Enemies:", colors[ColorId::WHITE]));
    for (auto elem : enemyMap){
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(elem.second.object, tilesOk));
      line.push_back(GuiElem::label(toString(elem.second.count) + "   " + elem.first, colors[ColorId::WHITE]));
      list.push_back(GuiElem::horizontalList(std::move(line), 20, 0));
    }
  }
  if (!creatureMap.count(chosenCreature)) {
    chosenCreature = "";
  }
  return GuiElem::verticalList(std::move(list), legendLineHeight, 0);
}

const int minionWindowWidth = 300;

void GuiBuilder::drawMinionsOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
  if (chosenCreature == "")
    return;
  vector<PGuiElem> lines;
  vector<CreatureInfo> chosen;
  for (auto& c : info.minions)
    if (c.speciesName == chosenCreature)
      chosen.push_back(c);
  lines.push_back(GuiElem::label(info.currentTeam ? "Click to add to team:" : "Click to control:", colors[ColorId::LIGHT_BLUE]));
  for (auto& c : chosen) {
    string text = "L: " + toString(c.expLevel) + "    " + info.tasks[c.uniqueId];
    if (c.speciesName != c.name)
      text = c.name + " " + text;
    vector<PGuiElem> line;
    line.push_back(GuiElem::viewObject(c.viewObject, tilesOk));
    line.push_back(GuiElem::label(text, (info.currentTeam && contains(info.teams[*info.currentTeam],c.uniqueId))
          ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
    lines.push_back(GuiElem::stack(
          GuiElem::button(getButtonCallback(UserInput(UserInputId::CREATURE_BUTTON, c.uniqueId))),
          GuiElem::horizontalList(std::move(line), 40, 0)));
  }
  ret.push_back({GuiElem::verticalList(std::move(lines), legendLineHeight, 0),
      Vec2(minionWindowWidth, (chosen.size() + 2) * legendLineHeight),
      OverlayInfo::RIGHT});
}

void GuiBuilder::drawBuildingsOverlay(vector<OverlayInfo>& ret, GameInfo::BandInfo& info) {
  if (activeBuilding == -1 || info.buildings[activeBuilding].groupName.empty())
    return;
  string groupName = info.buildings[activeBuilding].groupName;
  vector<PGuiElem> lines;
  for (int i : All(info.buildings)) {
    auto& elem = info.buildings[i];
    if (elem.groupName == groupName)
      lines.push_back(getButtonLine(elem, i, activeBuilding, collectiveTab));
  }
  int height = lines.size() * legendLineHeight;
  ret.push_back({GuiElem::verticalList(std::move(lines), legendLineHeight, 0),
      Vec2(minionWindowWidth, height),
      OverlayInfo::LEFT});
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
      line.push_back(GuiElem::stack(
            GuiElem::button(getButtonCallback(UserInput(UserInputId::MESSAGE_INFO, message.getUniqueId()))),
            GuiElem::label(text, getMessageColor(message))));
      lengths.push_back(renderer.getTextLength(text));
      if (message.isClickable()) {
        line.push_back(GuiElem::labelUnicode(String(L'➚'), getMessageColor(message)));
        lengths.push_back(messageArrowLength);
      }
    }
    if (!messages[i].empty())
      lines.push_back(GuiElem::horizontalList(std::move(line), lengths, 0));
  }
  if (!lines.empty())
    ret.push_back({GuiElem::verticalList(std::move(lines), lineHeight, 0),
        Vec2(maxMessageLength, lineHeight * messages.size() + 15), OverlayInfo::MESSAGES});
}

PGuiElem GuiBuilder::drawVillages(GameInfo::VillageInfo& info) {
  vector<PGuiElem> lines;
  for (auto elem : info.villages) {
    lines.push_back(GuiElem::label(capitalFirst(elem.name), colors[ColorId::WHITE]));
    lines.push_back(GuiElem::margins(GuiElem::label("tribe: " + elem.tribeName, colors[ColorId::WHITE]), 40, 0, 0, 0));
    if (!elem.state.empty())
      lines.push_back(GuiElem::margins(GuiElem::label(elem.state, elem.state == "conquered" ? colors[ColorId::GREEN] : colors[ColorId::RED]),
            40, 0, 0, 0));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}
