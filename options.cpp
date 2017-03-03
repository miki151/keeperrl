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
  {OptionId::SOUND, 1},
  {OptionId::KEEP_SAVEFILES, 0},
  {OptionId::SHOW_MAP, 0},
  {OptionId::FULLSCREEN, 0},
  {OptionId::FULLSCREEN_RESOLUTION, 0},
  {OptionId::ZOOM_UI, 0},
  {OptionId::DISABLE_MOUSE_WHEEL, 0},
  {OptionId::DISABLE_CURSOR, 0},
  {OptionId::ONLINE, 1},
  {OptionId::GAME_EVENTS, 1},
  {OptionId::AUTOSAVE, 1},
  {OptionId::WASD_SCROLLING, 0},
  {OptionId::FAST_IMMIGRATION, 0},
  {OptionId::STARTING_RESOURCE, 0},
  {OptionId::START_WITH_NIGHT, 0},
  {OptionId::KEEPER_NAME, string("")},
  {OptionId::KEEPER_TYPE, 0},
  {OptionId::KEEPER_SEED, string("")},
  {OptionId::ADVENTURER_NAME, string("")},
  {OptionId::ADVENTURER_TYPE, 0},
  {OptionId::MAIN_VILLAINS, 4},
  {OptionId::RETIRED_VILLAINS, 1},
  {OptionId::LESSER_VILLAINS, 3},
  {OptionId::ALLIES, 2},
  {OptionId::INFLUENCE_SIZE, 3},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::MUSIC, "Music"},
  {OptionId::SOUND, "Sound effects"},
  {OptionId::KEEP_SAVEFILES, "Keep save files"},
  {OptionId::SHOW_MAP, "Show map"},
  {OptionId::FULLSCREEN, "Fullscreen"},
  {OptionId::FULLSCREEN_RESOLUTION, "Fullscreen resolution"},
  {OptionId::ZOOM_UI, "Zoom in UI"},
  {OptionId::DISABLE_MOUSE_WHEEL, "Disable mouse wheel scrolling"},
  {OptionId::DISABLE_CURSOR, "Disable pretty mouse cursor"},
  {OptionId::ONLINE, "Online features"},
  {OptionId::GAME_EVENTS, "Anonymous statistics"},
  {OptionId::AUTOSAVE, "Autosave"},
  {OptionId::WASD_SCROLLING, "WASD scrolling"},
  {OptionId::FAST_IMMIGRATION, "Fast immigration"},
  {OptionId::STARTING_RESOURCE, "Resource bonus"},
  {OptionId::START_WITH_NIGHT, "Start with night"},
  {OptionId::KEEPER_NAME, "Keeper's name"},
  {OptionId::KEEPER_TYPE, "Keeper's gender"},
  {OptionId::KEEPER_SEED, "Level generation seed"},
  {OptionId::ADVENTURER_NAME, "Adventurer's name"},
  {OptionId::ADVENTURER_TYPE, "Adventurer's gender"},
  {OptionId::MAIN_VILLAINS, "Main villains"},
  {OptionId::RETIRED_VILLAINS, "Retired villains"},
  {OptionId::LESSER_VILLAINS, "Lesser villains"},
  {OptionId::ALLIES, "Allies"},
  {OptionId::INFLUENCE_SIZE, "Min. tribes in influence zone"},
};

const map<OptionId, string> hints {
  {OptionId::HINTS, "Display some extra helpful information during the game."},
  {OptionId::ASCII, "Switch to old school roguelike graphics."},
  {OptionId::KEEP_SAVEFILES, "Don't remove the save file when a game is loaded."},
  {OptionId::FULLSCREEN, "Switch between fullscreen and windowed mode."},
  {OptionId::FULLSCREEN_RESOLUTION, "Choose resolution for fullscreen mode."},
  {OptionId::ZOOM_UI, "All UI and graphics are zoomed in 2x. "
      "Use you have a large resolution screen and things appear too small."},
  {OptionId::ONLINE, "Enable online features, like dungeon sharing and highscores."},
  {OptionId::GAME_EVENTS, "Enable sending anonymous statistics to the developer."},
  {OptionId::AUTOSAVE, "Autosave the game every " + toString(MainLoop::getAutosaveFreq()) + " turns. "
    "The save file will be used to recover in case of a crash."},
  {OptionId::WASD_SCROLLING, "Scroll the map using W-A-S-D keys. In this mode building shortcuts are accessed "
    "using alt + letter."},
};

const map<OptionSet, vector<OptionId>> optionSets {
  {OptionSet::GENERAL, {
      OptionId::HINTS,
      OptionId::ASCII,
      OptionId::MUSIC,
#ifndef DISABLE_SFX
      OptionId::SOUND,
#endif
      OptionId::FULLSCREEN,
  //    OptionId::FULLSCREEN_RESOLUTION,
      OptionId::ZOOM_UI,
      OptionId::DISABLE_MOUSE_WHEEL,
      OptionId::DISABLE_CURSOR,
      OptionId::ONLINE,
      OptionId::GAME_EVENTS,
      OptionId::AUTOSAVE,
      OptionId::WASD_SCROLLING,
#ifndef RELEASE
      OptionId::KEEP_SAVEFILES,
      OptionId::SHOW_MAP,
      OptionId::FAST_IMMIGRATION,
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
};

static map<OptionId, Options::Trigger> triggers;

void Options::addTrigger(OptionId id, Trigger trigger) {
  triggers[id] = trigger;
}

const string& Options::getName(OptionId id) {
  return names.at(id);
}

Options::Type Options::getType(OptionId id) {
  switch (id) {
    case OptionId::ADVENTURER_NAME:
    case OptionId::KEEPER_SEED:
    case OptionId::KEEPER_NAME:
      return Options::STRING;
    case OptionId::ADVENTURER_TYPE:
    case OptionId::KEEPER_TYPE:
      return Options::PLAYER_TYPE;
    default:
      return Options::INT;
  }
}

vector<OptionId> Options::getOptions(OptionSet set) {
  return optionSets.at(set);
}

Options::Options(const string& path) : filename(path) {
  readValues();
}

Options::Value Options::getValue(OptionId id) {
  if (overrides[id])
    return *overrides[id];
  readValues();
  return (*values)[id];
}

bool Options::getBoolValue(OptionId id) {
  return boost::get<int>(getValue(id));
}

string Options::getStringValue(OptionId id) {
  return getValueString(id);
}

int Options::getChoiceValue(OptionId id) {
  return boost::get<int>(getValue(id));
}

int Options::getIntValue(OptionId id) {
  int v = boost::get<int>(getValue(id));
  if (limits[id]) {
    if (v > limits[id]->second)
      return limits[id]->second;
    if (v < limits[id]->first)
      return limits[id]->first;
  }
  return v;
}

CreatureId Options::getCreatureId(OptionId id) {
  return choicesCreatureId[id].at(boost::get<int>(getValue(id)) % choicesCreatureId[id].size());
}

void Options::setNextCreatureId(OptionId id) {
  setValue(id, boost::get<int>(getValue(id)) + 1);
}

void Options::setLimits(OptionId id, int minV, int maxV) {
  limits[id] = make_pair(minV, maxV);
}

optional<pair<int, int>> Options::getLimits(OptionId id) {
  return limits[id];
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

string Options::getValueString(OptionId id) {
  Value value = getValue(id);
  switch (id) {
    case OptionId::HINTS:
    case OptionId::ASCII:
    case OptionId::FULLSCREEN:
    case OptionId::AUTOSAVE:
    case OptionId::WASD_SCROLLING:
    case OptionId::SOUND:
    case OptionId::MUSIC:
      return getOnOff(value);
    case OptionId::KEEP_SAVEFILES:
    case OptionId::SHOW_MAP:
    case OptionId::FAST_IMMIGRATION:
    case OptionId::STARTING_RESOURCE:
    case OptionId::ONLINE:
    case OptionId::GAME_EVENTS:
    case OptionId::ZOOM_UI:
    case OptionId::DISABLE_MOUSE_WHEEL:
    case OptionId::DISABLE_CURSOR:
    case OptionId::START_WITH_NIGHT:
      return getYesNo(value);
    case OptionId::ADVENTURER_NAME:
    case OptionId::KEEPER_SEED:
    case OptionId::KEEPER_NAME: {
      string val = boost::get<string>(value);
      if (val.empty())
        return defaultStrings[id];
      else
        return val;
      }
    case OptionId::FULLSCREEN_RESOLUTION: {
      int val = boost::get<int>(value);
      if (val >= 0 && val < choices[id].size())
        return choices[id][val];
      else
        return "";
    }
    case OptionId::MAIN_VILLAINS:
    case OptionId::LESSER_VILLAINS:
    case OptionId::RETIRED_VILLAINS:
    case OptionId::INFLUENCE_SIZE:
    case OptionId::ALLIES:
      return toString(getIntValue(id));
    case OptionId::KEEPER_TYPE:
    case OptionId::ADVENTURER_TYPE:
      return toString((int)getCreatureId(id));
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

void Options::setChoices(OptionId id, const vector<CreatureId>& v) {
  choicesCreatureId[id] = v;
}

bool Options::handleOrExit(View* view, OptionSet set, int lastIndex) {
  if (!optionSets.count(set))
    return true;
  vector<ListElem> options;
  options.emplace_back("Change settings:", ListElem::TITLE);
  for (OptionId option : optionSets.at(set)) {
    options.push_back(ListElem(names.at(option),
        getValueString(option)));
    if (hints.count(option))
      options.back().setTip(hints.at(option));
  }
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
  for (OptionId option : optionSets.at(set)) {
    options.push_back(ListElem(names.at(option),
      getValueString(option)));
    if (hints.count(option))
      options.back().setTip(hints.at(option));
  }
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex, getMenuType(set));
  if (!index || (*index) == optionSets.at(set).size())
    return;
  OptionId option = optionSets.at(set)[*index];
  changeValue(option, getValue(option), view);
  handle(view, set, *index);
}

void Options::readValues() {
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
}

void Options::writeValues() {
  ofstream out(filename);
  for (OptionId id : ENUM_ALL(OptionId))
    out << EnumInfo<OptionId>::getString(id) << "," << (*values)[id] << std::endl;
}

