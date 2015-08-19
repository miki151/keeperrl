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
#include "options.h"
#include "main_loop.h"
#include "view.h"

const EnumMap<OptionId, Options::Value> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
  {OptionId::MUSIC, 1},
  {OptionId::KEEP_SAVEFILES, 0},
  {OptionId::SHOW_MAP, 0},
  {OptionId::FULLSCREEN, 0},
  {OptionId::FULLSCREEN_RESOLUTION, 0},
  {OptionId::ONLINE, 1},
  {OptionId::AUTOSAVE, 1},
  {OptionId::WASD_SCROLLING, 0},
  {OptionId::FAST_IMMIGRATION, 0},
  {OptionId::STARTING_RESOURCE, 0},
  {OptionId::START_WITH_NIGHT, 0},
  {OptionId::KEEPER_NAME, string("")},
  {OptionId::KEEPER_SEED, string("")},
  {OptionId::ADVENTURER_NAME, string("")},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::MUSIC, "Music"},
  {OptionId::KEEP_SAVEFILES, "Keep save files"},
  {OptionId::SHOW_MAP, "Show map"},
  {OptionId::FULLSCREEN, "Fullscreen"},
  {OptionId::FULLSCREEN_RESOLUTION, "Fullscreen resolution"},
  {OptionId::ONLINE, "Online exchange of dungeons and highscores"},
  {OptionId::AUTOSAVE, "Autosave"},
  {OptionId::WASD_SCROLLING, "WASD scrolling"},
  {OptionId::FAST_IMMIGRATION, "Fast immigration"},
  {OptionId::STARTING_RESOURCE, "Resource bonus"},
  {OptionId::START_WITH_NIGHT, "Start with night"},
  {OptionId::KEEPER_NAME, "Keeper's name"},
  {OptionId::KEEPER_SEED, "Level generation seed"},
  {OptionId::ADVENTURER_NAME, "Adventurer's name"},
};

const map<OptionId, string> hints {
  {OptionId::HINTS, "Display some extra helpful information during the game."},
  {OptionId::ASCII, "Switch to old school roguelike graphics."},
  {OptionId::MUSIC, ""},
  {OptionId::KEEP_SAVEFILES, "Don't remove the save file when a game is loaded."},
  {OptionId::SHOW_MAP, ""},
  {OptionId::FULLSCREEN, "Switch between fullscreen and windowed mode."},
  {OptionId::FULLSCREEN_RESOLUTION, "Choose resolution for fullscreen mode."},
  {OptionId::ONLINE, "Upload your highscores and retired dungeons to keeperrl.com."},
  {OptionId::AUTOSAVE, "Autosave the game every " + toString(MainLoop::getAutosaveFreq()) + " turns. "
    "The save file will be used to recover in case of a crash."},
  {OptionId::WASD_SCROLLING, "Scroll the map using W-A-S-D keys. In this mode building shortcuts are accessed "
    "using alt + letter."},
  {OptionId::FAST_IMMIGRATION, ""},
  {OptionId::STARTING_RESOURCE, ""},
  {OptionId::START_WITH_NIGHT, ""},
  {OptionId::KEEPER_NAME, ""},
  {OptionId::KEEPER_SEED, ""},
  {OptionId::ADVENTURER_NAME, ""},
};

const map<OptionSet, vector<OptionId>> optionSets {
  {OptionSet::GENERAL, {
      OptionId::HINTS,
      OptionId::ASCII,
      OptionId::MUSIC,
      OptionId::FULLSCREEN,
      OptionId::FULLSCREEN_RESOLUTION,
      OptionId::ONLINE,
      OptionId::AUTOSAVE,
      OptionId::WASD_SCROLLING,
#ifndef RELEASE
      OptionId::KEEP_SAVEFILES,
      OptionId::SHOW_MAP,
#endif
  }},
  {OptionSet::KEEPER, {
#ifndef RELEASE
      OptionId::START_WITH_NIGHT,
      OptionId::SHOW_MAP,
      OptionId::STARTING_RESOURCE,
      OptionId::FAST_IMMIGRATION,
#endif
      OptionId::KEEPER_NAME,
      OptionId::KEEPER_SEED,
  }},
  {OptionSet::ADVENTURER, {
      OptionId::ADVENTURER_NAME,
  }},
};

map<OptionId, Options::Trigger> triggers;

void Options::addTrigger(OptionId id, Trigger trigger) {
  triggers[id] = trigger;
}

static EnumMap<OptionId, optional<Options::Value>> parseOverrides(const string& s) {
  EnumMap<OptionId, optional<Options::Value>> ret;
  for (string op : split(s, {','})) {
    auto parts = split(op, {'='});
    CHECK(parts.size() == 2);
    OptionId id = EnumInfo<OptionId>::fromString(parts[0]);
    if (parts[1] == "n")
      ret[id] = false;
    else if (parts[1] == "y")
      ret[id] = true;
    else
      FAIL << "Bad override " << parts;
  }
  return ret;
}

Options::Options(const string& path, const string& _overrides)
    : filename(path), overrides(parseOverrides(_overrides)) {
}

Options::Value Options::getValue(OptionId id) {
  if (overrides[id])
    return *overrides[id];
  return readValues()[id];
}

bool Options::getBoolValue(OptionId id) {
  return boost::get<int>(getValue(id));
}

string Options::getStringValue(OptionId id) {
  return getValueString(id, getValue(id));
}

int Options::getChoiceValue(OptionId id) {
  return boost::get<int>(getValue(id));
}

void Options::setValue(OptionId id, Value value) {
  readValues();
  (*values)[id] = value;
  if (triggers.count(id))
    triggers.at(id)(boost::get<int>(value));
  writeValues();
}

void Options::setDefaultString(OptionId id, const string& s) {
  defaultStrings[id] = s;
}

static string getOnOff(const Options::Value& value) {
  return boost::get<int>(value) ? "on" : "off";
}

static string getYesNo(const Options::Value& value) {
  return boost::get<int>(value) ? "yes" : "no";
}

string Options::getValueString(OptionId id, Options::Value value) {
  switch (id) {
    case OptionId::HINTS:
    case OptionId::ASCII:
    case OptionId::FULLSCREEN:
    case OptionId::AUTOSAVE:
    case OptionId::WASD_SCROLLING:
    case OptionId::MUSIC: return getOnOff(value);
    case OptionId::KEEP_SAVEFILES:
    case OptionId::SHOW_MAP:
    case OptionId::FAST_IMMIGRATION:
    case OptionId::STARTING_RESOURCE:
    case OptionId::ONLINE:
    case OptionId::START_WITH_NIGHT: return getYesNo(value);
    case OptionId::ADVENTURER_NAME:
    case OptionId::KEEPER_SEED:
    case OptionId::KEEPER_NAME: {
        string val = boost::get<string>(value);
        if (val.empty())
          return defaultStrings[id];
        else
          return val;
        }
    case OptionId::FULLSCREEN_RESOLUTION: return choices[id][boost::get<int>(value)];
  }
}

optional<Options::Value> Options::readValue(OptionId id, const string& input) {
  switch (id) {
    case OptionId::ADVENTURER_NAME:
    case OptionId::KEEPER_SEED:
    case OptionId::KEEPER_NAME: return Options::Value(input);
    default:
        if (auto ret = fromStringSafe<int>(input))
          return Options::Value(*ret);
        else
          return none;
  }
}

static MenuType getMenuType(OptionSet set) {
  switch (set) {
    default: return MenuType::NORMAL;
  }
}

void Options::changeValue(OptionId id, const Options::Value& value, View* view) {
  switch (id) {
    case OptionId::KEEPER_NAME:
    case OptionId::ADVENTURER_NAME:
        if (auto val = view->getText("Enter " + names.at(id), boost::get<string>(value), 23,
              "Leave blank to use a random name."))
          setValue(id, *val);
        break;
    case OptionId::KEEPER_SEED:
        if (auto val = view->getText("Enter " + names.at(id), boost::get<string>(value), 23,
              "Leave blank to use a random seed."))
          setValue(id, *val);
        break;
    case OptionId::FULLSCREEN_RESOLUTION:
        if (auto index = view->chooseFromList("Choose resolution.", ListElem::convert(choices[id])))
          setValue(id, *index);
        break;
    default:
        setValue(id, !boost::get<int>(value));
  }
}

void Options::setChoices(OptionId id, const vector<string>& v) {
  choices[id] = v;
}

bool Options::handleOrExit(View* view, OptionSet set, int lastIndex) {
  if (!optionSets.count(set))
    return true;
  vector<ListElem> options;
  options.emplace_back("Change settings:", ListElem::TITLE);
  for (OptionId option : optionSets.at(set))
    options.push_back(ListElem(names.at(option),
          getValueString(option, getValue(option))).setTip(hints.at(option)));
  options.emplace_back("Done");
  if (lastIndex == -1)
    lastIndex = optionSets.at(set).size();
  auto index = view->chooseFromList("", options, lastIndex, getMenuType(set));
  if (!index)
    return false;
  else if (index && (*index) == optionSets.at(set).size())
    return true;
  OptionId option = optionSets.at(set)[*index];
  changeValue(option, getValue(option), view);
  return handleOrExit(view, set, *index);
}

void Options::handle(View* view, OptionSet set, int lastIndex) {
  vector<ListElem> options;
  options.emplace_back("Change settings:", ListElem::TITLE);
  for (OptionId option : optionSets.at(set))
    options.push_back(ListElem(names.at(option),
          getValueString(option, getValue(option))).setTip(hints.at(option)));
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex, getMenuType(set));
  if (!index || (*index) == optionSets.at(set).size())
    return;
  OptionId option = optionSets.at(set)[*index];
  changeValue(option, getValue(option), view);
  handle(view, set, *index);
}

const EnumMap<OptionId, Options::Value>& Options::readValues() {
  if (!values) {
    values = defaults;
    ifstream in(filename);
    while (1) {
      char buf[100];
      in.getline(buf, 100);
      if (!in)
        break;
      vector<string> p = split(string(buf), {','});
      if (p.empty())
        continue;
      if (p.size() == 1)
        p.push_back("");
      OptionId optionId;
      if (auto id = EnumInfo<OptionId>::fromStringSafe(p[0]))
        optionId = *id;
      else
        continue;
      if (auto val = readValue(optionId, p[1]))
        (*values)[optionId] = *val;
    }
  }
  return *values;
}

void Options::writeValues() {
  ofstream out(filename);
  for (OptionId id : ENUM_ALL(OptionId))
    out << EnumInfo<OptionId>::getString(id) << "," << (*values)[id] << std::endl;
}

