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
  {OptionId::VSYNC, 1},
  {OptionId::ZOOM_UI, 0},
  {OptionId::DISABLE_MOUSE_WHEEL, 0},
  {OptionId::DISABLE_CURSOR, 0},
  {OptionId::ONLINE, 1},
  {OptionId::GAME_EVENTS, 1},
  {OptionId::AUTOSAVE2, 1500},
  {OptionId::WASD_SCROLLING, 0},
  {OptionId::SUGGEST_TUTORIAL, 1},
  {OptionId::STARTING_RESOURCE, 0},
  {OptionId::START_WITH_NIGHT, 0},
  {OptionId::PLAYER_NAME, string("")},
  {OptionId::SETTLEMENT_NAME, string("")},
  {OptionId::MAIN_VILLAINS, 4},
  {OptionId::RETIRED_VILLAINS, 1},
  {OptionId::LESSER_VILLAINS, 3},
  {OptionId::ALLIES, 2},
  {OptionId::CURRENT_MOD2, vector<string>()},
  {OptionId::ENDLESS_ENEMIES, 2},
  {OptionId::ENEMY_AGGRESSION, 1},
  {OptionId::KEEPER_WARNING, 1},
  {OptionId::KEEPER_WARNING_PAUSE, 1},
  {OptionId::KEEPER_WARNING_TIMEOUT, 200},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::MUSIC, "Music volume"},
  {OptionId::SOUND, "SFX volume"},
  {OptionId::KEEP_SAVEFILES, "Keep save files"},
  {OptionId::SHOW_MAP, "Show map"},
  {OptionId::FULLSCREEN, "Fullscreen"},
  {OptionId::VSYNC, "Vertical Sync"},
  {OptionId::ZOOM_UI, "Zoom in UI"},
  {OptionId::DISABLE_MOUSE_WHEEL, "Disable mouse wheel scrolling"},
  {OptionId::DISABLE_CURSOR, "Disable pretty mouse cursor"},
  {OptionId::ONLINE, "Online features"},
  {OptionId::GAME_EVENTS, "Anonymous statistics"},
  {OptionId::AUTOSAVE2, "Number of turns between autosaves"},
  {OptionId::WASD_SCROLLING, "WASD scrolling"},
  {OptionId::SUGGEST_TUTORIAL, ""},
  {OptionId::STARTING_RESOURCE, "Resource bonus"},
  {OptionId::START_WITH_NIGHT, "Start with night"},
  {OptionId::PLAYER_NAME, "Name"},
  {OptionId::SETTLEMENT_NAME, "Settlement name"},
  {OptionId::MAIN_VILLAINS, "Main villains"},
  {OptionId::RETIRED_VILLAINS, "Retired villains"},
  {OptionId::LESSER_VILLAINS, "Lesser villains"},
  {OptionId::ALLIES, "Allies"},
  {OptionId::CURRENT_MOD2, "Current mod"},
  {OptionId::ENDLESS_ENEMIES, "Start endless enemy waves"},
  {OptionId::ENEMY_AGGRESSION, "Enemy aggression"},
  {OptionId::KEEPER_WARNING, "Keeper danger warning"},
  {OptionId::KEEPER_WARNING_PAUSE, "Keeper danger auto-pause"},
  {OptionId::KEEPER_WARNING_TIMEOUT, "Keeper danger timeout"},
};

const map<OptionId, string> hints {
  {OptionId::HINTS, "Display some extra helpful information during the game."},
  {OptionId::ASCII, "Switch to old school roguelike graphics."},
  {OptionId::KEEP_SAVEFILES, "Don't remove the save file when a game is loaded."},
  {OptionId::FULLSCREEN, "Switch between fullscreen and windowed mode."},
  {OptionId::VSYNC, "Limits frame rate to your monitor's refresh rate. Turning off may fix frame rate issues."},
  {OptionId::ZOOM_UI, "All UI and graphics are zoomed in 2x. "
      "Use you have a large resolution screen and things appear too small."},
  {OptionId::ONLINE, "Enable online features, like dungeon sharing and highscores."},
  {OptionId::GAME_EVENTS, "Enable sending anonymous statistics to the developer."},
  {OptionId::AUTOSAVE2, "Autosave the game every X number turns. "
    "The save file will be used to recover in case of a crash."},
  {OptionId::WASD_SCROLLING, "Scroll the map using W-A-S-D keys. In this mode building shortcuts are accessed "
    "using alt + letter."},
  {OptionId::ENDLESS_ENEMIES, "Turn on recurrent enemy waves that attack your dungeon."},
  {OptionId::ENEMY_AGGRESSION, "The chance of your dungeon being attacked by enemies"},
  {OptionId::KEEPER_WARNING, "Display a pop up window whenever your Keeper is in danger"},
  {OptionId::KEEPER_WARNING_PAUSE, "Pause the game whenever your Keeper is in danger"},
  {OptionId::KEEPER_WARNING_TIMEOUT, "Timeout before a new Keeper danger warning is shown"},
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
      OptionId::ZOOM_UI,
      OptionId::DISABLE_MOUSE_WHEEL,
      OptionId::DISABLE_CURSOR,
      OptionId::ONLINE,
      OptionId::GAME_EVENTS,
      OptionId::AUTOSAVE2,
      OptionId::WASD_SCROLLING,
      OptionId::KEEPER_WARNING,
      OptionId::KEEPER_WARNING_PAUSE,
      OptionId::KEEPER_WARNING_TIMEOUT,
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
#endif
      OptionId::PLAYER_NAME,
      OptionId::SETTLEMENT_NAME,
  }},
};

static map<OptionId, Options::Trigger> triggers;

void Options::addTrigger(OptionId id, Trigger trigger) {
  triggers[id] = trigger;
}

const string& Options::getName(OptionId id) {
  return names.at(id);
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

vector<string> Options::getVectorStringValue(OptionId id) {
  return *getValue(id).getValueMaybe<vector<string>>();
}

bool Options::hasVectorStringValue(OptionId id, const string& value) {
  return getValue(id).getValueMaybe<vector<string>>()->contains(value);
}

void Options::addVectorStringValue(OptionId id, const string& value) {
  if (!hasVectorStringValue(id, value)) {
    auto v = * getValue(id).getValueMaybe<vector<string>>();
    v.push_back(value);
    setValue(id, v);
  }
}

void Options::removeVectorStringValue(OptionId id, const string& value) {
  auto v = *getValue(id).getValueMaybe<vector<string>>();
  v.removeElementMaybe(value);
  setValue(id, v);
}

int Options::getIntValue(OptionId id) {
  int v = *getValue(id).getValueMaybe<int>();
  if (auto& limit = limits[id])
    v = max(limit->getStart(), min(limit->getEnd() - 1, v));
  return v;
}

void Options::setLimits(OptionId id, Range r) {
  limits[id] = r;
}

optional<Range> Options::getLimits(OptionId id) {
  if (!choices[id].empty())
    return Range(0, choices[id].size());
  return limits[id];
}

void Options::setValue(OptionId id, Value value) {
  readValues();
  (*values)[id] = value;
  if (triggers.count(id))
    triggers.at(id)(*value.getValueMaybe<int>());
  writeValues();
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
    case OptionId::WASD_SCROLLING:
    case OptionId::KEEPER_WARNING:
    case OptionId::KEEPER_WARNING_PAUSE:
      return getOnOff(value);
    case OptionId::KEEP_SAVEFILES:
    case OptionId::SHOW_MAP:
    case OptionId::SUGGEST_TUTORIAL:
    case OptionId::STARTING_RESOURCE:
    case OptionId::ONLINE:
    case OptionId::GAME_EVENTS:
    case OptionId::ZOOM_UI:
    case OptionId::DISABLE_MOUSE_WHEEL:
    case OptionId::DISABLE_CURSOR:
    case OptionId::START_WITH_NIGHT:
      return getYesNo(value);
    case OptionId::SETTLEMENT_NAME:
    case OptionId::PLAYER_NAME:
      return *value.getValueMaybe<string>();
    case OptionId::CURRENT_MOD2:
      return combine(*value.getValueMaybe<vector<string>>(), true);
    case OptionId::ENDLESS_ENEMIES:
    case OptionId::ENEMY_AGGRESSION:
      return choices[id][(*value.getValueMaybe<int>() + choices[id].size()) % choices[id].size()];
    case OptionId::MAIN_VILLAINS:
    case OptionId::LESSER_VILLAINS:
    case OptionId::RETIRED_VILLAINS:
    case OptionId::ALLIES:
      return toString(getIntValue(id));
    case OptionId::SOUND:
    case OptionId::MUSIC:
      return toString(getIntValue(id)) + "%";
    case OptionId::AUTOSAVE2:
    case OptionId::KEEPER_WARNING_TIMEOUT:
      return toString(getIntValue(id));
  }
}

optional<Options::Value> Options::readValue(OptionId id, const vector<string>& input) {
  switch (id) {
    case OptionId::SETTLEMENT_NAME:
    case OptionId::PLAYER_NAME:
      return Options::Value(input[0]);
    case OptionId::CURRENT_MOD2:
      if (input.size() == 1 && input[0].empty())
        return Options::Value(vector<string>());
      return Options::Value(input);
    default:
      if (auto ret = fromStringSafe<int>(input[0]))
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
    case OptionId::KEEPER_WARNING_TIMEOUT:
      if (auto val = view->getNumber("Change " + lowercase(getName(id)), Range(0, 500), *value.getValueMaybe<int>(), 50))
        setValue(id, *val);
      break;
    case OptionId::AUTOSAVE2:
      if (auto val = view->getNumber("Change " + lowercase(getName(id)), Range(0, 5000), *value.getValueMaybe<int>(), 500))
        setValue(id, *val);
      break;
    case OptionId::MUSIC:
    case OptionId::SOUND:
      if (auto val = view->getNumber("Change " + getName(id), Range(0, 100), *value.getValueMaybe<int>(), 5))
        setValue(id, *val);
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
  auto optionSet = optionSets.at(set);
  if (!view->zoomUIAvailable())
    optionSet.removeElementMaybe(OptionId::ZOOM_UI);
  for (OptionId option : optionSet) {
    options.push_back(ListElem(names.at(option),
      getValueString(option)));
    if (hints.count(option))
      options.back().setTip(hints.at(option));
  }
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex, getMenuType(set));
  if (!index || (*index) == optionSet.size())
    return;
  OptionId option = optionSet[*index];
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
      OptionId optionId;
      if (auto id = EnumInfo<OptionId>::fromStringSafe(p[0]))
        optionId = *id;
      else
        continue;
      if (auto val = readValue(optionId, p.getSuffix(p.size() - 1))) {
        if ((optionId == OptionId::SOUND || optionId == OptionId::MUSIC) && *val == 1)
          *val = 100;
        (*values)[optionId] = *val;
      }
    }
  }
}

static void outputConfigString(ostream& os, const Options::Value& value) {
  value.visit(
      [&](const vector<string>& v) {
        for (int i : All(v)) {
          os << v[i];
          if (i < v.size() - 1)
            os << ",";
        }
      },
      [&](const auto& x) {
        os << x;
      }
  );
}

void Options::writeValues() {
  ofstream out(filename.getPath());
  for (OptionId id : ENUM_ALL(OptionId)) {
    out << EnumInfo<OptionId>::getString(id) << ",";
    outputConfigString(out, (*values)[id]);
    out << std::endl;
  }
}

