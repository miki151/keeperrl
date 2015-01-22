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

const EnumMap<OptionId, Options::Value> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
  {OptionId::MUSIC, 1},
  {OptionId::KEEP_SAVEFILES, 0},
  {OptionId::SHOW_MAP, 0},
  {OptionId::SMOOTH_MOVEMENT, 1},
  {OptionId::FULLSCREEN, 0},
  {OptionId::FAST_IMMIGRATION, 0},
  {OptionId::STARTING_RESOURCE, 0},
  {OptionId::START_WITH_NIGHT, 0},
  {OptionId::KEEPER_NAME, string("")},
  {OptionId::ADVENTURER_NAME, string("")},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::MUSIC, "Music"},
  {OptionId::KEEP_SAVEFILES, "Keep save files"},
  {OptionId::SHOW_MAP, "Show map"},
  {OptionId::SMOOTH_MOVEMENT, "Smooth movement"},
  {OptionId::FULLSCREEN, "Fullscreen"},
  {OptionId::FAST_IMMIGRATION, "Fast immigration"},
  {OptionId::STARTING_RESOURCE, "Resource bonus"},
  {OptionId::START_WITH_NIGHT, "Start with night"},
  {OptionId::KEEPER_NAME, "Keeper's name"},
  {OptionId::ADVENTURER_NAME, "Adventurer's name"},
};

const map<OptionSet, vector<OptionId>> optionSets {
  {OptionSet::GENERAL, {
      OptionId::HINTS,
      OptionId::ASCII,
      OptionId::MUSIC,
      OptionId::KEEP_SAVEFILES,
      OptionId::SMOOTH_MOVEMENT,
      OptionId::FULLSCREEN,
#ifndef RELEASE
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
  }},
  {OptionSet::ADVENTURER, {
      OptionId::ADVENTURER_NAME,
  }},
};

map<OptionId, Options::Trigger> triggers;

void Options::addTrigger(OptionId id, Trigger trigger) {
  triggers[id] = trigger;
}

Options::Options(const string& path) : filename(path) {
}

Options::Value Options::getValue(OptionId id) {
  return readValues()[id];
}

bool Options::getBoolValue(OptionId id) {
  return boost::get<bool>(getValue(id));
}

string Options::getStringValue(OptionId id) {
  return getValueString(id, getValue(id));
}

void Options::setValue(OptionId id, Value value) {
  auto values = readValues();
  values[id] = value;
  if (triggers.count(id))
    triggers.at(id)(boost::get<bool>(value));
  writeValues(values);
}

void Options::setDefaultString(OptionId id, const string& s) {
  defaultStrings[id] = s;
}

static string getOnOff(const Options::Value& value) {
  return boost::get<bool>(value) ? "on" : "off";
}

static string getYesNo(const Options::Value& value) {
  return boost::get<bool>(value) ? "yes" : "no";
}

string Options::getValueString(OptionId id, Options::Value value) {
  switch (id) {
    case OptionId::HINTS:
    case OptionId::ASCII:
    case OptionId::SMOOTH_MOVEMENT:
    case OptionId::FULLSCREEN:
    case OptionId::MUSIC: return getOnOff(value);
    case OptionId::KEEP_SAVEFILES:
    case OptionId::SHOW_MAP:
    case OptionId::FAST_IMMIGRATION:
    case OptionId::STARTING_RESOURCE:
    case OptionId::START_WITH_NIGHT: return getYesNo(value);
    case OptionId::ADVENTURER_NAME:
    case OptionId::KEEPER_NAME: {
        string val = boost::get<string>(value);
        if (val.empty())
          return defaultStrings[id];
        else
          return val;
        }
  }
}

static optional<Options::Value> readValue(OptionId id, const string& input) {
  switch (id) {
    case OptionId::ADVENTURER_NAME:
    case OptionId::KEEPER_NAME: return Options::Value(input);
    default:
        if (auto ret = fromStringSafe<int>(input))
          return Options::Value(*ret);
        else
          return none;
  }
}

static View::MenuType getMenuType(OptionSet set) {
  switch (set) {
    default: return View::NORMAL_MENU;
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
    default:
        setValue(id, !boost::get<bool>(value));
  }
}

bool Options::handleOrExit(View* view, OptionSet set, int lastIndex) {
  if (!optionSets.count(set))
    return true;
  vector<View::ListElem> options;
  options.emplace_back("Change settings:", View::TITLE);
  for (OptionId option : optionSets.at(set))
    options.emplace_back(names.at(option), getValueString(option, getValue(option)));
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
  vector<View::ListElem> options;
  options.emplace_back("Change settings:", View::TITLE);
  for (OptionId option : optionSets.at(set))
    options.emplace_back(names.at(option), getValueString(option, getValue(option)));
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex, getMenuType(set));
  if (!index || (*index) == optionSets.at(set).size())
    return;
  OptionId option = optionSets.at(set)[*index];
  changeValue(option, getValue(option), view);
  handle(view, set, *index);
}

EnumMap<OptionId, Options::Value> Options::readValues() {
  EnumMap<OptionId, Value> ret = defaults;
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
      ret[optionId] = *val;
  }
  return ret;
}

void Options::writeValues(const EnumMap<OptionId, Value>& values) {
  ofstream out(filename);
  for (OptionId id : ENUM_ALL(OptionId))
    out << EnumInfo<OptionId>::getString(id) << "," << values[id] << std::endl;
}

