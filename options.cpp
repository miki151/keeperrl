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
#include "scripted_ui_data.h"

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
  {OptionId::SINGLE_THREAD, 0},
  {OptionId::UNLOCK_ALL, 0},
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
  {OptionId::SINGLE_THREAD, "Use a single thread for loading operations"},
  {OptionId::UNLOCK_ALL, "Unlock all hidden gameplay features"},
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
  {OptionId::ENDLESS_ENEMIES, "Turn on recurrent enemy waves that attack your dungeon."},
  {OptionId::ENEMY_AGGRESSION, "The chance of your dungeon being attacked by enemies"},
  {OptionId::KEEPER_WARNING, "Display a pop up window whenever your Keeper is in danger"},
  {OptionId::KEEPER_WARNING_PAUSE, "Pause the game whenever your Keeper is in danger"},
  {OptionId::KEEPER_WARNING_TIMEOUT, "Number of turns before a new \"Keeper in danger\" warning is shown"},
  {OptionId::SINGLE_THREAD, "Please try this option if you're experiencing slow saving, loading, or map generation. "
        "Note: this will make the game unresponsive during the operation."},
  {OptionId::UNLOCK_ALL, "Unlocks all player characters and gameplay features that are normally unlocked by finding secrets in the game."},
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
      OptionId::KEEPER_WARNING,
      OptionId::KEEPER_WARNING_PAUSE,
      OptionId::KEEPER_WARNING_TIMEOUT,
      OptionId::SINGLE_THREAD,
      OptionId::UNLOCK_ALL,
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

Options::Options(const FilePath& path, KeybindingMap* k) : filename(path), keybindingMap(k) {
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

static optional<pair<int, int>> getIntRange(OptionId id) {
  switch (id) {
    case OptionId::MUSIC:
    case OptionId::SOUND:
      return make_pair(0, 100);
    case OptionId::AUTOSAVE2:
      return make_pair(0, 5000);
    case OptionId::KEEPER_WARNING_TIMEOUT:
      return make_pair(50, 500);
    default:
      return none;
  }
}

static bool continuousChange(OptionId id) {
  switch (id) {
    case OptionId::SOUND: return false;
   default: return true;
  }
}

static optional<int> getIntInterval(OptionId id) {
  switch (id) {
    case OptionId::AUTOSAVE2:
      return 500;
    case OptionId::KEEPER_WARNING_TIMEOUT:
      return 50;
    default:
      return none;
  }  
}

static bool isBoolean(OptionId id) {
  switch (id) {
    case OptionId::HINTS:
    case OptionId::ASCII:
    case OptionId::FULLSCREEN:
    case OptionId::VSYNC:
    case OptionId::KEEPER_WARNING:
    case OptionId::KEEPER_WARNING_PAUSE:
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
    case OptionId::SINGLE_THREAD:
    case OptionId::UNLOCK_ALL:
      return true;
    default:
      return false;
  }
}

string Options::getValueString(OptionId id) {
  Value value = getValue(id);
  switch (id) {
    case OptionId::HINTS:
    case OptionId::ASCII:
    case OptionId::FULLSCREEN:
    case OptionId::VSYNC:
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
    case OptionId::SINGLE_THREAD:
    case OptionId::UNLOCK_ALL:
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

void Options::setChoices(OptionId id, const vector<string>& v) {
  choices[id] = v;
}

bool Options::hasChoices(OptionId id) const {
  return !choices[id].empty();
}

optional<string> Options::getHint(OptionId id) {
  return getValueMaybe(hints, id);
}

void Options::handleBoolean(OptionId option, ScriptedUIDataElems::Record& data, bool& wasSet) {
  auto getCallback = [this, &wasSet](OptionId option, Value value) {
    return ScriptedUIDataElems::Callback {
      [option, value, &wasSet, this] {
        wasSet = true;
        setValue(option, value); return true; }
      };
  };
  auto value = getBoolValue(option);
  data.elems.insert({value ? "yes" : "no", ScriptedUIData{}});
  data.elems.insert({"callbackBool", getCallback(option, int(!value))});
}

void Options::handleIntInterval(OptionId option, ScriptedUIDataElems::Record& data, bool& wasSet) {
  auto value = getIntValue(option);
  auto interval = *getIntInterval(option);
  auto range = *getIntRange(option);
  data.elems.insert({"value", value > 0 ? toString(value) : "off"});
  data.elems.insert({"increase", ScriptedUIDataElems::Callback{
      [this, &wasSet, option, value, interval, range] {
        auto newVal = value + interval;
        if (newVal > range.second)
          newVal = range.first;
        this->setValue(option, newVal);
        wasSet = true;
        return true;
      }}});
  data.elems.insert({"decrease", ScriptedUIDataElems::Callback{
      [this, &wasSet, option, value, interval, range] {
        auto newVal = value - interval;
        if (newVal < range.first)
          newVal = range.second;
        this->setValue(option, newVal);
        wasSet = true;
        return true;
      }}});
}

void Options::handleSliding(OptionId option, ScriptedUIDataElems::Record& data, bool& wasSet) {
  auto range = *getIntRange(option);
  auto value = getIntValue(option);
  auto res  = ScriptedUIDataElems::SliderData {
    [&wasSet, option, range, this](double value) {
      this->setValue(option, int(range.first * (1 - value) + range.second * value));
      wasSet = true;
      return false;
    },
    double(value - range.first) / (range.second - range.first),
    continuousChange(option),
  };
  data.elems.insert({"sliderData", std::move(res)});
}

void Options::handle(View* view, OptionSet set, int lastIndex) {
  auto optionSet = optionSets.at(set);
  if (!view->zoomUIAvailable())
    optionSet.removeElementMaybe(OptionId::ZOOM_UI);
  ScriptedUIState state;
  while (1) {
    ScriptedUIDataElems::List options;
    bool wasSet = false;
    for (OptionId option : optionSet) {
      auto optionData = ScriptedUIDataElems::Record{{{"name", ScriptedUIDataElems::Label{names.at(option)}}}};
      if (hints.count(option))
        optionData.elems.insert({"tooltip", ScriptedUIDataElems::Label{hints.at(option)}});
      if (isBoolean(option))
        handleBoolean(option, optionData, wasSet);
      else if (getIntRange(option)) {
        if (getIntInterval(option))
          handleIntInterval(option, optionData, wasSet);
        else
          handleSliding(option, optionData, wasSet);
      }
      options.push_back(std::move(optionData));
    }
    view->scriptedUI("settings", options, state);
    if (!wasSet)
      return;
  }
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

KeybindingMap* Options::getKeybindingMap() {
  return keybindingMap;
}