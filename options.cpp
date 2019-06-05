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
  {OptionId::MUSIC, 100},
  {OptionId::SOUND, 100},
  {OptionId::KEEP_SAVEFILES, 0},
  {OptionId::SHOW_MAP, 0},
  {OptionId::FULLSCREEN, 0},
  {OptionId::FULLSCREEN_RESOLUTION, 0},
  {OptionId::VSYNC, 1},
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
  {OptionId::PLAYER_NAME, string("")},
  {OptionId::KEEPER_SEED, string("")},
  {OptionId::MAIN_VILLAINS, 4},
  {OptionId::RETIRED_VILLAINS, 1},
  {OptionId::LESSER_VILLAINS, 3},
  {OptionId::ALLIES, 2},
  {OptionId::INFLUENCE_SIZE, 3},
  {OptionId::GENERATE_MANA, 0},
  {OptionId::CURRENT_MOD, 0},
  {OptionId::ENDLESS_ENEMIES, 0},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::MUSIC, "Music volume"},
  {OptionId::SOUND, "SFX volume"},
  {OptionId::KEEP_SAVEFILES, "Keep save files"},
  {OptionId::SHOW_MAP, "Show map"},
  {OptionId::FULLSCREEN, "Fullscreen"},
  {OptionId::FULLSCREEN_RESOLUTION, "Fullscreen resolution"},
  {OptionId::VSYNC, "Vertical Sync"},
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
  {OptionId::PLAYER_NAME, "First name"},
  {OptionId::KEEPER_SEED, "Level generation seed"},
  {OptionId::MAIN_VILLAINS, "Main villains"},
  {OptionId::RETIRED_VILLAINS, "Retired villains"},
  {OptionId::LESSER_VILLAINS, "Lesser villains"},
  {OptionId::ALLIES, "Allies"},
  {OptionId::INFLUENCE_SIZE, "Min. tribes in influence zone"},
  {OptionId::GENERATE_MANA, "Generate mana in library"},
  {OptionId::CURRENT_MOD, "Current mod"},
  {OptionId::ENDLESS_ENEMIES, "Start endless enemy waves"},
};

const map<OptionId, string> hints {
  {OptionId::HINTS, "Display some extra helpful information during the game."},
  {OptionId::ASCII, "Switch to old school roguelike graphics."},
  {OptionId::KEEP_SAVEFILES, "Don't remove the save file when a game is loaded."},
  {OptionId::FULLSCREEN, "Switch between fullscreen and windowed mode."},
  {OptionId::FULLSCREEN_RESOLUTION, "Choose resolution for fullscreen mode."},
  {OptionId::VSYNC, "Limits frame rate to your monitor's refresh rate. Turning off may fix frame rate issues."},
  {OptionId::ZOOM_UI, "All UI and graphics are zoomed in 2x. "
      "Use you have a large resolution screen and things appear too small."},
  {OptionId::ONLINE, "Enable online features, like dungeon sharing and highscores."},
  {OptionId::GAME_EVENTS, "Enable sending anonymous statistics to the developer."},
  {OptionId::AUTOSAVE, "Autosave the game every " + toString(MainLoop::getAutosaveFreq()) + " turns. "
    "The save file will be used to recover in case of a crash."},
  {OptionId::WASD_SCROLLING, "Scroll the map using W-A-S-D keys. In this mode building shortcuts are accessed "
    "using alt + letter."},
  {OptionId::GENERATE_MANA, "Your minions will generate mana while working in the library."},
  {OptionId::ENDLESS_ENEMIES, "Turn on recurrent enemy waves that attack your dungeon."}
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
      OptionId::VSYNC,
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
      OptionId::PLAYER_NAME,
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
    case OptionId::PLAYER_NAME:
    case OptionId::KEEPER_SEED:
      return Options::STRING;
    case OptionId::GENERATE_MANA:
      return Options::BOOL;
    default:
      return Options::INT;
  }
}

vector<OptionId> Options::getOptions(OptionSet set) {
  return optionSets.at(set);
}

Options::Options(const FilePath& path) : filename(path) {
  readValues();
}

Options::Value Options::getValue(OptionId id) {
  if (overrides[id])
    return *overrides[id];
  readValues();
  return (*values)[id];
}

bool Options::getBoolValue(OptionId id) {
  return *getValue(id).getValueMaybe<int>();
}

string Options::getStringValue(OptionId id) {
  return getValueString(id);
}

int Options::getIntValue(OptionId id) {
  int v = *getValue(id).getValueMaybe<int>();
  if (limits[id]) {
    if (v > limits[id]->second)
      return limits[id]->second;
    if (v < limits[id]->first)
      return limits[id]->first;
  }
  return v;
}

void Options::setLimits(OptionId id, int minV, int maxV) {
  limits[id] = make_pair(minV, maxV);
}

optional<pair<int, int>> Options::getLimits(OptionId id) {
  if (!choices[id].empty())
    return make_pair(0, choices[id].size() - 1);
  return limits[id];
}

void Options::setValue(OptionId id, Value value) {
  readValues();
  (*values)[id] = value;
  if (triggers.count(id))
    triggers.at(id)(*value.getValueMaybe<int>());
  writeValues();
}

void Options::setDefaultString(OptionId id, const string& s) {
  defaultStrings[id] = s;
}

static string getOnOff(const Options::Value& value) {
  return *value.getValueMaybe<int>() ? "on" : "off";
}

static string getYesNo(const Options::Value& value) {
  return *value.getValueMaybe<int>() ? "yes" : "no";
}

string Options::getValueString(OptionId id) {
  Value value = getValue(id);
  switch (id) {
    case OptionId::HINTS:
    case OptionId::ASCII:
    case OptionId::FULLSCREEN:
    case OptionId::VSYNC:
    case OptionId::AUTOSAVE:
    case OptionId::WASD_SCROLLING:
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
    case OptionId::GENERATE_MANA:
    case OptionId::START_WITH_NIGHT:
      return getYesNo(value);
    case OptionId::PLAYER_NAME:
    case OptionId::KEEPER_SEED: {
      string val = *value.getValueMaybe<string>();
      if (val.empty())
        return defaultStrings[id];
      else
        return val;
    }
    case OptionId::ENDLESS_ENEMIES:
    case OptionId::CURRENT_MOD:
      return choices[id][(*value.getValueMaybe<int>() + choices[id].size()) % choices[id].size()];
    case OptionId::FULLSCREEN_RESOLUTION: {
      int val = *value.getValueMaybe<int>();
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
    case OptionId::SOUND:
    case OptionId::MUSIC:
      return toString(getIntValue(id)) + "%";
  }
}

optional<Options::Value> Options::readValue(OptionId id, const string& input) {
  switch (id) {
    case OptionId::PLAYER_NAME:
    case OptionId::KEEPER_SEED:
      return Options::Value(input);
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
    case OptionId::PLAYER_NAME:
      if (auto val = view->getText("Enter " + names.at(id), *value.getValueMaybe<string>(), 23,
            "Leave blank to use a random name."))
        setValue(id, *val);
      break;
    case OptionId::KEEPER_SEED:
      if (auto val = view->getText("Enter " + names.at(id), *value.getValueMaybe<string>(), 23,
            "Leave blank to use a random seed."))
        setValue(id, *val);
      break;
    case OptionId::MUSIC:
    case OptionId::SOUND:
      if (auto val = view->getNumber("Change " + getName(id), Range(0, 100), *value.getValueMaybe<int>(), 5))
        setValue(id, *val);
      break;
    case OptionId::FULLSCREEN_RESOLUTION:
      if (auto index = view->chooseFromList("Choose resolution.", ListElem::convert(choices[id])))
        setValue(id, *index);
      break;
    default:
      if (!choices[id].empty())
        setValue(id, *value.getValueMaybe<int>() + 1);
      else
        setValue(id, (int) !*value.getValueMaybe<int>());
      break;
  }
}

void Options::setChoices(OptionId id, const vector<string>& v) {
  choices[id] = v;
}

bool Options::hasChoices(OptionId id) const {
  return !choices[id].empty();
}

optional<string> Options::getHint(OptionId id) {
  return getValueMaybe(hints, id);
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
    ifstream in(filename.getPath());
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
      if (auto val = readValue(optionId, p[1])) {
        if ((optionId == OptionId::SOUND || optionId == OptionId::MUSIC) && *val == 1)
          *val = 100;
        (*values)[optionId] = *val;
      }
    }
  }
}

void Options::writeValues() {
  ofstream out(filename.getPath());
  for (OptionId id : ENUM_ALL(OptionId))
    out << EnumInfo<OptionId>::getString(id) << "," << (*values)[id] << std::endl;
}

