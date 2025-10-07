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

static TString getName(OptionId id) {
  switch (id) {
    case OptionId::HINTS: return TStringId("HINTS_OPTION_NAME");
    case OptionId::ASCII: return TStringId("ASCII_OPTION_NAME");
    case OptionId::MUSIC: return TStringId("MUSIC_OPTION_NAME");
    case OptionId::SOUND: return TStringId("SOUND_OPTION_NAME");
    case OptionId::KEEP_SAVEFILES: return TStringId("KEEP_SAVEFILES_OPTION_NAME");
    case OptionId::SHOW_MAP: return TStringId("SHOW_MAP_OPTION_NAME");
    case OptionId::FULLSCREEN: return TStringId("FULLSCREEN_OPTION_NAME");
    case OptionId::VSYNC: return TStringId("VSYNC_OPTION_NAME");
    case OptionId::FPS_LIMIT: return TStringId("FPS_LIMIT_OPTION_NAME");
    case OptionId::ZOOM_UI: return TStringId("ZOOM_UI_OPTION_NAME");
    case OptionId::DISABLE_MOUSE_WHEEL: return TStringId("DISABLE_MOUSE_WHEEL_OPTION_NAME");
    case OptionId::DISABLE_CURSOR: return TStringId("DISABLE_CURSOR_OPTION_NAME");
    case OptionId::ONLINE: return TStringId("ONLINE_OPTION_NAME");
    case OptionId::GAME_EVENTS: return TStringId("GAME_EVENTS_OPTION_NAME");
    case OptionId::AUTOSAVE2: return TStringId("AUTOSAVE2_OPTION_NAME");
    case OptionId::SUGGEST_TUTORIAL: return TStringId("SUGGEST_TUTORIAL_OPTION_NAME");
    case OptionId::CONTROLLER_HINT_MAIN_MENU: return TStringId("CONTROLLER_HINT_MAIN_MENU_OPTION_NAME");
    case OptionId::CONTROLLER_HINT_REAL_TIME: return TStringId("CONTROLLER_HINT_REAL_TIME_OPTION_NAME");
    case OptionId::CONTROLLER_HINT_TURN_BASED: return TStringId("CONTROLLER_HINT_TURN_BASED_OPTION_NAME");
    case OptionId::STARTING_RESOURCE: return TStringId("STARTING_RESOURCE_OPTION_NAME");
    case OptionId::START_WITH_NIGHT: return TStringId("START_WITH_NIGHT_OPTION_NAME");
    case OptionId::PLAYER_NAME: return TStringId("PLAYER_NAME_OPTION_NAME");
    case OptionId::SETTLEMENT_NAME: return TStringId("SETTLEMENT_NAME_OPTION_NAME");
    case OptionId::MAIN_VILLAINS: return TStringId("MAIN_VILLAINS_OPTION_NAME");
    case OptionId::RETIRED_VILLAINS: return TStringId("RETIRED_VILLAINS_OPTION_NAME");
    case OptionId::LESSER_VILLAINS: return TStringId("LESSER_VILLAINS_OPTION_NAME");
    case OptionId::MINOR_VILLAINS: return TStringId("MINOR_VILLAINS_OPTION_NAME");
    case OptionId::ALLIES: return TStringId("ALLIES_OPTION_NAME");
    case OptionId::CURRENT_MOD2: return TStringId("CURRENT_MOD2_OPTION_NAME");
    case OptionId::ENDLESS_ENEMIES: return TStringId("ENDLESS_ENEMIES_OPTION_NAME");
    case OptionId::ENEMY_AGGRESSION: return TStringId("ENEMY_AGGRESSION_OPTION_NAME");
    case OptionId::KEEPER_WARNING: return TStringId("KEEPER_WARNING_OPTION_NAME");
    case OptionId::KEEPER_WARNING_TIMEOUT: return TStringId("KEEPER_WARNING_TIMEOUT_OPTION_NAME");
    case OptionId::SINGLE_THREAD: return TStringId("SINGLE_THREAD_OPTION_NAME");
    case OptionId::UNLOCK_ALL: return TStringId("UNLOCK_ALL_OPTION_NAME");
    case OptionId::EXP_INCREASE: return TStringId("EXP_INCREASE_OPTION_NAME");
    case OptionId::DPI_AWARE: return TStringId("DPI_AWARE_OPTION_NAME");
    case OptionId::LANGUAGE: return TStringId("LANGUAGE_OPTION_NAME");
  }
};

static optional<TStringId> getHint(OptionId id) {
  switch (id) {
    case OptionId::HINTS: return TStringId("HINTS_OPTION_DESCRIPTION");
    case OptionId::ASCII: return TStringId("ASCII_OPTION_DESCRIPTION");
    case OptionId::KEEP_SAVEFILES: return TStringId("KEEP_SAVEFILES_OPTION_DESCRIPTION");
    case OptionId::FULLSCREEN: return TStringId("FULLSCREEN_OPTION_DESCRIPTION");
    case OptionId::VSYNC: return TStringId("VSYNC_OPTION_DESCRIPTION");
    case OptionId::FPS_LIMIT: return TStringId("FPS_LIMIT_OPTION_DESCRIPTION");
    case OptionId::ZOOM_UI: return TStringId("ZOOM_UI_OPTION_DESCRIPTION");
    case OptionId::ONLINE: return TStringId("ONLINE_OPTION_DESCRIPTION");
    case OptionId::GAME_EVENTS: return TStringId("GAME_EVENTS_OPTION_DESCRIPTION");
    case OptionId::AUTOSAVE2: return TStringId("AUTOSAVE2_OPTION_DESCRIPTION");
    case OptionId::ENDLESS_ENEMIES: return TStringId("ENDLESS_ENEMIES_OPTION_DESCRIPTION");
    case OptionId::ENEMY_AGGRESSION: return TStringId("ENEMY_AGGRESSION_OPTION_DESCRIPTION");
    case OptionId::KEEPER_WARNING: return TStringId("KEEPER_WARNING_OPTION_DESCRIPTION");
    case OptionId::KEEPER_WARNING_TIMEOUT: return TStringId("KEEPER_WARNING_TIMEOUT_OPTION_DESCRIPTION");
    case OptionId::SINGLE_THREAD: return TStringId("SINGLE_THREAD_OPTION_DESCRIPTION");
    case OptionId::UNLOCK_ALL: return TStringId("UNLOCK_ALL_OPTION_DESCRIPTION");
    case OptionId::EXP_INCREASE: return TStringId("EXP_INCREASE_OPTION_DESCRIPTION");
    case OptionId::DPI_AWARE: return TStringId("DPI_AWARE_OPTION_DESCRIPTION");
    default: return none;
  }
}

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

TString Options::getName(OptionId id) {
  return ::getName(id);
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
  if (hasChoices(id) && value.contains<string>()) {
    bool found = false;
    for (int i : All(choices[id]))
      if (value == choices[id][i]) {
        value = i;
        found = true;
        break;
      }
    CHECK(found) << "Option value not found: " << value;
  }
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
  if (!hasChoices(id)) {
    choices[id] = v;
    return;
  }
  auto currentChoice = getValueString(id);
  choices[id] = v;
  if (!v.contains(currentChoice))
    setValue(id, 0);
  else
    setValue(id, currentChoice);
}

bool Options::hasChoices(OptionId id) const {
  return !choices[id].empty();
}

optional<TString> Options::getHint(OptionId id) {
  if (auto res = ::getHint(id))
    return TString(*res);
  return none;
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

void Options::handleStringList(OptionId option, ScriptedUIDataElems::Record& data, View* view, bool& wasSet) {
  auto value = getIntValue(option);
  data.elems.insert({"value", TString(getValueString(option))});
  auto languages = choices[option].transform([](const string& s){ return TString(s); });
  data.elems.insert({"callbackBool",
    ScriptedUIDataElems::Callback {
      [option, &wasSet, languages, view, this] {
        wasSet = true;
        if (auto res = view->multiChoice(TStringId("CHOOSE_LANGUAGE"),  languages))
          this->setValue(option, *res);
        return true;
      }
    }
  });
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
      auto optionData = ScriptedUIDataElems::Record{{{"name", ScriptedUIDataElems::Label{getName(option)}}}};
      if (auto hint = getHint(option))
        optionData.elems.insert({"tooltip", ScriptedUIDataElems::Label{*hint}});
      if (isBoolean(option))
        handleBoolean(option, optionData, wasSet);
      else if (getIntRange(option)) {
        if (getIntInterval(option))
          handleIntInterval(option, optionData, wasSet);
        else
          handleSliding(option, optionData, wasSet);
      }
      else if (!choices[option].empty())
        handleStringList(option, optionData, view, wasSet);
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