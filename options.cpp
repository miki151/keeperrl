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

string Options::filename;

const unordered_map<OptionId, int> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
  {OptionId::MUSIC, 1},
  {OptionId::KEEP_SAVEFILES, 0},
  {OptionId::SHOW_MAP, 0},
  {OptionId::STARTING_RESOURCE, 0},
  {OptionId::START_WITH_NIGHT, 0},
  {OptionId::EASY_KEEPER, 0},
  {OptionId::AGGRESSIVE_HEROES, 1},
  {OptionId::EASY_ADVENTURER, 1},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::MUSIC, "Music"},
  {OptionId::KEEP_SAVEFILES, "Keep save files"},
  {OptionId::SHOW_MAP, "Show map"},
  {OptionId::STARTING_RESOURCE, "Resource bonus"},
  {OptionId::START_WITH_NIGHT, "Start with night"},
  {OptionId::EASY_KEEPER, "Game difficulty"},
  {OptionId::AGGRESSIVE_HEROES, "Aggressive enemies"},
  {OptionId::EASY_ADVENTURER, "Game difficulty"},
};

const map<OptionSet, vector<OptionId>> optionSets {
  {OptionSet::GENERAL, {
      OptionId::HINTS,
      OptionId::ASCII,
      OptionId::MUSIC,
      OptionId::KEEP_SAVEFILES,
#ifndef RELEASE
      OptionId::SHOW_MAP,
#endif
  }},
  {OptionSet::KEEPER, {
      OptionId::AGGRESSIVE_HEROES,
#ifndef RELEASE
      OptionId::EASY_KEEPER,
      OptionId::START_WITH_NIGHT,
      OptionId::SHOW_MAP,
      OptionId::STARTING_RESOURCE,
#endif
  }},
  {OptionSet::ADVENTURER, {OptionId::EASY_ADVENTURER}},
};

map<OptionId, Options::Trigger> triggers;

void Options::addTrigger(OptionId id, Trigger trigger) {
  triggers[id] = trigger;
}

void Options::init(const string& path) {
  filename = path;
}

int Options::getValue(OptionId id) {
  auto values = readValues(filename);
  if (values.count(id))
    return values.at(id);
  else
    return defaults.at(id);
}

void Options::setValue(OptionId id, int value) {
  auto values = readValues(filename);
  values[id] = value;
  if (triggers.count(id))
    triggers.at(id)(value);
  writeValues(filename, values);
}

unordered_map<OptionId, vector<string>> valueNames {
  {OptionId::HINTS, { "off", "on" }},
  {OptionId::ASCII, { "off", "on" }},
  {OptionId::MUSIC, { "off", "on" }},
  {OptionId::KEEP_SAVEFILES, { "no", "yes" }},
  {OptionId::SHOW_MAP, { "no", "yes" }},
  {OptionId::STARTING_RESOURCE, { "no", "yes" }},
  {OptionId::START_WITH_NIGHT, { "no", "yes" }},
  {OptionId::EASY_KEEPER, { "hard", "easy" }},
  {OptionId::AGGRESSIVE_HEROES, { "no", "yes" }},
  {OptionId::EASY_ADVENTURER, { "hard", "easy" }},
};

bool Options::handleOrExit(View* view, OptionSet set, int lastIndex) {
  vector<View::ListElem> options;
  options.emplace_back("Change settings:", View::TITLE);
  for (OptionId option : optionSets.at(set))
    options.emplace_back(names.at(option) + "      " + valueNames.at(option)[getValue(option)]);
  options.emplace_back("Done");
  options.emplace_back("Cancel");
  if (lastIndex == -1)
    lastIndex = optionSets.at(set).size();
  auto index = view->chooseFromList("", options, lastIndex);
  if (!index || (*index) == optionSets.at(set).size() + 1)
    return false;
  else if (index && (*index) == optionSets.at(set).size())
    return true;
  OptionId option = optionSets.at(set)[*index];
  setValue(option, 1 - getValue(option));
  return handleOrExit(view, set, *index);
}

void Options::handle(View* view, OptionSet set, int lastIndex) {
  vector<View::ListElem> options;
  options.emplace_back("Change settings:", View::TITLE);
  for (OptionId option : optionSets.at(set))
    options.emplace_back(names.at(option) + "      " + valueNames.at(option)[getValue(option)]);
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex);
  if (!index || (*index) == optionSets.at(set).size())
    return;
  OptionId option = optionSets.at(set)[*index];
  setValue(option, 1 - getValue(option));
  handle(view, set, *index);
}

unordered_map<OptionId, int> Options::readValues(const string& path) {
  unordered_map<OptionId, int> ret;
  ifstream in(path);
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    vector<string> p = split(string(buf), {','});
    CHECK(p.size() == 2) << "Input error " << p;
    ret[OptionId(convertFromString<int>(p[0]))] = convertFromString<int>(p[1]);
  }
  return ret;
}

void Options::writeValues(const string& path, const unordered_map<OptionId, int>& values) {
  ofstream out(path);
  for (auto elem : values)
    out << int(elem.first) << "," << elem.second << std::endl;
}

