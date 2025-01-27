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
#include "keybinding_map.h"
#include "content_factory.h"
#include "steam_input.h"

const EnumMap<OptionId, Options::Value> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
  {OptionId::MUSIC, 100},
  {OptionId::SOUND, 100},
  {OptionId::KEEP_SAVEFILES, 0},
  {OptionId::SHOW_MAP, 0},
  {OptionId::FULLSCREEN, 0},
  {OptionId::VSYNC, 1},
  {OptionId::FPS_LIMIT, 0},
  {OptionId::ZOOM_UI, 10},
  {OptionId::DISABLE_MOUSE_WHEEL, 0},
  {OptionId::DISABLE_CURSOR, 0},
  {OptionId::ONLINE, 1},
  {OptionId::GAME_EVENTS, 1},
  {OptionId::AUTOSAVE2, 1500},
  {OptionId::SUGGEST_TUTORIAL, 1},
  {OptionId::CONTROLLER_HINT_MAIN_MENU, 1},
  {OptionId::CONTROLLER_HINT_REAL_TIME, 1},
  {OptionId::CONTROLLER_HINT_TURN_BASED, 1},
  {OptionId::STARTING_RESOURCE, 0},
  {OptionId::PLAYER_NAME, string("")},
  {OptionId::SETTLEMENT_NAME, string("")},
  {OptionId::MAIN_VILLAINS, 12},
  {OptionId::RETIRED_VILLAINS, 4},
  {OptionId::LESSER_VILLAINS, 12},
  {OptionId::MINOR_VILLAINS, 16},
  {OptionId::ALLIES, 5},
  {OptionId::CURRENT_MOD2, vector<string>()},
  {OptionId::ENDLESS_ENEMIES, 2},
  {OptionId::ENEMY_AGGRESSION, 1},
  {OptionId::KEEPER_WARNING, 1},
  {OptionId::KEEPER_WARNING_TIMEOUT, 200},
  {OptionId::SINGLE_THREAD, 0},
  {OptionId::UNLOCK_ALL, 0},
  {OptionId::EXP_INCREASE, 1},
  {OptionId::DPI_AWARE, 0},
  {OptionId::LANGUAGE, 0}
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
  {OptionId::FPS_LIMIT, "Framerate limit"},
  {OptionId::ZOOM_UI, "Zoom in UI"},
  {OptionId::DISABLE_MOUSE_WHEEL, "Disable mouse wheel scrolling"},
  {OptionId::DISABLE_CURSOR, "Disable pretty mouse cursor"},
  {OptionId::ONLINE, "Online features"},
  {OptionId::GAME_EVENTS, "Anonymous statistics"},
  {OptionId::AUTOSAVE2, "Number of turns between autosaves"},
  {OptionId::SUGGEST_TUTORIAL, ""},
  {OptionId::CONTROLLER_HINT_MAIN_MENU, ""},
  {OptionId::CONTROLLER_HINT_REAL_TIME, ""},
  {OptionId::CONTROLLER_HINT_TURN_BASED, ""},
  {OptionId::STARTING_RESOURCE, "Resource bonus"},
  {OptionId::START_WITH_NIGHT, "Start with night"},
  {OptionId::PLAYER_NAME, "Name"},
  {OptionId::SETTLEMENT_NAME, "Settlement name"},
  {OptionId::MAIN_VILLAINS, "Main villains"},
  {OptionId::RETIRED_VILLAINS, "Retired villains"},
  {OptionId::LESSER_VILLAINS, "Lesser villains"},
  {OptionId::MINOR_VILLAINS, "Minor villains"},
  {OptionId::ALLIES, "Allies"},
  {OptionId::CURRENT_MOD2, "Current mod"},
  {OptionId::ENDLESS_ENEMIES, "Start endless enemy waves"},
  {OptionId::ENEMY_AGGRESSION, "Enemy aggression"},
  {OptionId::KEEPER_WARNING, "Keeper danger warning"},
  {OptionId::KEEPER_WARNING_TIMEOUT, "Keeper danger timeout"},
  {OptionId::SINGLE_THREAD, "Use a single thread for loading operations"},
  {OptionId::UNLOCK_ALL, "Unlock all hidden gameplay features"},
  {OptionId::EXP_INCREASE, "Enemy difficulty curve"},
  {OptionId::DPI_AWARE, "Override Windows DPI scaling"},
  {OptionId::LANGUAGE, "Language"},
};

const map<OptionId, string> hints {
  {OptionId::HINTS, "Display some extra helpful information during the game."},
  {OptionId::ASCII, "Switch to old school roguelike graphics."},
  {OptionId::KEEP_SAVEFILES, "Don't remove the save file when a game is loaded."},
  {OptionId::FULLSCREEN, "Switch between fullscreen and windowed mode."},
  {OptionId::VSYNC, "Limits frame rate to your monitor's refresh rate. Turning off may fix frame rate issues."},
  {OptionId::FPS_LIMIT, "Limits frame rate. Lower framerate keeps GPU cooler."},
  {OptionId::ZOOM_UI, "All UI and graphics are zoomed in. "
      "Use if you have a large resolution screen and things appear too small."},
  {OptionId::ONLINE, "Enable online features, like dungeon sharing and highscores."},
  {OptionId::GAME_EVENTS, "Enable sending anonymous statistics to the developer."},
  {OptionId::AUTOSAVE2, "Autosave the game every X number turns. "
    "The save file will be used to recover in case of a crash."},
  {OptionId::ENDLESS_ENEMIES, "Turn on recurrent enemy waves that attack your dungeon."},
  {OptionId::ENEMY_AGGRESSION, "The chance of your dungeon being attacked by enemies"},
  {OptionId::KEEPER_WARNING, "Display a pop up window whenever your Keeper is in danger"},
  {OptionId::KEEPER_WARNING_TIMEOUT, "Number of turns before a new \"Keeper in danger\" warning is shown"},
  {OptionId::SINGLE_THREAD, "Please try this option if you're experiencing slow saving, loading, or map generation. "
        "Note: this will make the game unresponsive during the operation."},
  {OptionId::UNLOCK_ALL, "Unlocks all player characters and gameplay features that are normally unlocked by finding secrets in the game."},
  {OptionId::EXP_INCREASE, "Defines the increase in experience for every lesser and main villain as you travel further away from your home site."},
  {OptionId::DPI_AWARE, "If you find the game blurry, this setting might help. Requires restarting the game. "},
};

const map<OptionSet, vector<OptionId>> optionSets {
  {OptionSet::GENERAL, {
      OptionId::LANGUAGE,
      OptionId::HINTS,
      OptionId::ASCII,
      OptionId::MUSIC,
#ifndef DISABLE_SFX
      OptionId::SOUND,
#endif
      OptionId::FULLSCREEN,
      OptionId::VSYNC,
      OptionId::FPS_LIMIT,
      OptionId::ZOOM_UI,
      OptionId::DISABLE_MOUSE_WHEEL,
      OptionId::DISABLE_CURSOR,
      OptionId::ONLINE,
      OptionId::GAME_EVENTS,
      OptionId::AUTOSAVE2,
      OptionId::KEEPER_WARNING,
      OptionId::KEEPER_WARNING_TIMEOUT,
      OptionId::SINGLE_THREAD,
      OptionId::UNLOCK_ALL,
#ifndef RELEASE
      OptionId::KEEP_SAVEFILES,
      OptionId::SHOW_MAP,
#endif
#ifdef WINDOWS
      OptionId::DPI_AWARE
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

Options::Options(const FilePath& path, KeybindingMap* k, MySteamInput* i) : filename(path), keybindingMap(k),
    steamInput(i) {
  readValues();
}

Options::Value Options::getValue(OptionId id) {
  switch (id) {
    case OptionId::CONTROLLER_HINT_MAIN_MENU:
    case OptionId::CONTROLLER_HINT_TURN_BASED:
    case OptionId::CONTROLLER_HINT_REAL_TIME:
      if (!steamInput || steamInput->controllers.empty())
        return 0;
      break;
    default:
      break;
  }
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
    case OptionId::ZOOM_UI:
      return make_pair(10, 20);
    case OptionId::FPS_LIMIT:
      return make_pair(0, 30);
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
    case OptionId::FPS_LIMIT:
      return 5;
    case OptionId::ZOOM_UI:
      return 2;
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
    case OptionId::KEEP_SAVEFILES:
    case OptionId::SHOW_MAP:
    case OptionId::SUGGEST_TUTORIAL:
    case OptionId::CONTROLLER_HINT_MAIN_MENU:
    case OptionId::CONTROLLER_HINT_REAL_TIME:
    case OptionId::CONTROLLER_HINT_TURN_BASED:
    case OptionId::STARTING_RESOURCE:
    case OptionId::ONLINE:
    case OptionId::GAME_EVENTS:
    case OptionId::DISABLE_MOUSE_WHEEL:
    case OptionId::DISABLE_CURSOR:
    case OptionId::START_WITH_NIGHT:
    case OptionId::SINGLE_THREAD:
    case OptionId::UNLOCK_ALL:
    case OptionId::DPI_AWARE:
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
    case OptionId::DPI_AWARE:
      return getOnOff(value);
    case OptionId::KEEP_SAVEFILES:
    case OptionId::SHOW_MAP:
    case OptionId::SUGGEST_TUTORIAL:
    case OptionId::CONTROLLER_HINT_MAIN_MENU:
    case OptionId::CONTROLLER_HINT_REAL_TIME:
    case OptionId::CONTROLLER_HINT_TURN_BASED:
    case OptionId::STARTING_RESOURCE:
    case OptionId::ONLINE:
    case OptionId::GAME_EVENTS:
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
    case OptionId::EXP_INCREASE:
    case OptionId::LANGUAGE:
    case OptionId::ENEMY_AGGRESSION:
      return choices[id][(*value.getValueMaybe<int>() + choices[id].size()) % choices[id].size()];
    case OptionId::FPS_LIMIT:
    case OptionId::MAIN_VILLAINS:
    case OptionId::LESSER_VILLAINS:
    case OptionId::MINOR_VILLAINS:
    case OptionId::RETIRED_VILLAINS:
    case OptionId::ALLIES:
      return toString(getIntValue(id));
    case OptionId::SOUND:
    case OptionId::MUSIC:
      return toString(getIntValue(id)) + "%";
    case OptionId::AUTOSAVE2:
    case OptionId::KEEPER_WARNING_TIMEOUT:
      return toString(getIntValue(id));
    case OptionId::ZOOM_UI:
      return toString(10 * getIntValue(id)) + "%";
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
  data.elems.insert({"value", TString(value > 0 ? getValueString(option) : "off"_s)});
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

void Options::handleStringList(OptionId option, ScriptedUIDataElems::Record& data, bool& wasSet) {
  auto value = getIntValue(option);
  data.elems.insert({"value", TString(getValueString(option))});
  data.elems.insert({"increase", ScriptedUIDataElems::Callback{
      [this, &wasSet, option, value] {
        this->setValue(option, (value + 1) % choices[option].size());
        wasSet = true;
        return true;
      }}});
  data.elems.insert({"decrease", ScriptedUIDataElems::Callback{
      [this, &wasSet, option, value] {
        this->setValue(option, (value + choices[option].size() - 1) % choices[option].size());
        wasSet = true;
        return true;
      }}});
}

void Options::handleSliding(OptionId option, ScriptedUIDataElems::Record& data, bool& wasSet) {
  auto range = *getIntRange(option);
  auto value = getIntValue(option);
  auto res = ScriptedUIDataElems::SliderData {
    [option, range, this](double value) {
      this->setValue(option, int(range.first * (1 - value) + range.second * value));
      return false;
    },
    double(value - range.first) / (range.second - range.first),
    continuousChange(option),
  };
  data.elems.insert({"sliderData", std::move(res)});
  auto keys = ScriptedUIDataElems::FocusKeysCallbacks {{
      {Keybinding("MENU_LEFT"), [&wasSet, option, range, this, value] {
        this->setValue(option, max(range.first, value - 10));
        wasSet = true;
        return true;
      }},
      {Keybinding("MENU_RIGHT"), [&wasSet, option, range, this, value] {
        this->setValue(option, min(range.second, value + 10));
        wasSet = true;
        return true;
      }}
  }};
  data.elems.insert({"sliderKeys", std::move(keys)});
}

static optional<SDL::SDL_Keysym> captureKey(View* view, const string& text) {
  ScriptedUIState state;
  optional<SDL::SDL_Keysym> ret;
  ScriptedUIDataElems::Record main;
  main.elems["callback"] = ScriptedUIDataElems::KeyCatcherCallback{
    [&](SDL::SDL_Keysym k) {
      ret = k;
      return true;
    }
  };
  main.elems["text"] = TString(text);
  view->scriptedUI("key_capture", main, state);
  return ret;
}

void Options::handle(View* view, const ContentFactory* factory, OptionSet set, int lastIndex) {
  bool keybindingsTab = false;
  auto optionSet = optionSets.at(set);
/*  if (!view->zoomUIAvailable())
    optionSet.removeElementMaybe(OptionId::ZOOM_UI);
*/  ScriptedUIState state;
  while (1) {
    state.sliderState.clear();
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
      else if (!choices[option].empty())
        handleStringList(option, optionData, wasSet);
      options.push_back(std::move(optionData));
    }
    ScriptedUIDataElems::List keybindings;
    for (auto& key : factory->keybindings) {
      auto r = ScriptedUIDataElems::Record{{
          {"label", TString(key.second.name)},
          {"key", ScriptedUIDataElems::Label{keybindingMap->getText(key.first).value_or(TString())}},
          {"clicked", ScriptedUIDataElems::Callback { [&wasSet, key, view, this] {
            auto captured = captureKey(view, key.second.name);
            if (captured && captured->sym != SDL::SDLK_ESCAPE)
               keybindingMap->set(key.first, *captured);
            wasSet = true;
            return true;
          }}}
      }};
      keybindings.push_back(std::move(r));
    }
    ScriptedUIDataElems::Record main;
    main.elems["set_options"] = ScriptedUIDataElems::Callback{
        [&] { keybindingsTab = false; wasSet = true; return true; }
    };
    main.elems["set_keys"] = ScriptedUIDataElems::Callback{ [&] {
      if (steamInput && !steamInput->controllers.empty()) {
        steamInput->showBindingScreen();
        return false;
      } else {
        keybindingsTab = true;
        wasSet = true;
        return true;
      }
    }};
    if (keybindingsTab) {
      main.elems["keybindings"] = std::move(keybindings);
      main.elems["reset_keys"] = ScriptedUIDataElems::Callback {[this, &wasSet, view] {
        if (view->yesOrNoPrompt(TStringId("RESTORE_KEYBINDINGS_CONFIRM")))
          keybindingMap->reset();
        wasSet = true;
        return true;
      }};
    } else
      main.elems["options"] = std::move(options);
    view->scriptedUI("settings", main, state);
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
        if (optionId == OptionId::ZOOM_UI) {
          if (*val == 0)
            *val = 10;
          if (*val == 1)
            *val = 20;
        }
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