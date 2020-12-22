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

#pragma once

#include "util.h"
#include "file_path.h"

RICH_ENUM(OptionId,
  HINTS,
  ASCII,
  MUSIC,
  SOUND,
  KEEP_SAVEFILES,
  FULLSCREEN,
  VSYNC,
  ONLINE,
  GAME_EVENTS,
  AUTOSAVE2,
  WASD_SCROLLING,
  ZOOM_UI,
  DISABLE_MOUSE_WHEEL,
  DISABLE_CURSOR,
  KEEPER_WARNING,
  KEEPER_WARNING_PAUSE,
  KEEPER_WARNING_TIMEOUT,

  SUGGEST_TUTORIAL,

  PLAYER_NAME,
  SETTLEMENT_NAME,
  SHOW_MAP,
  START_WITH_NIGHT,
  STARTING_RESOURCE,

  MAIN_VILLAINS,
  RETIRED_VILLAINS,
  LESSER_VILLAINS,
  ALLIES,
  CURRENT_MOD2,
  ENDLESS_ENEMIES,
  ENEMY_AGGRESSION,
  SINGLE_THREAD
);

enum class OptionSet {
  GENERAL,
  KEEPER,
  ADVENTURER,
};

namespace ScriptedUIDataElems {
  struct Record;
}

class View;

class Options {
  public:
  typedef variant<int, string, vector<string>> Value;
  Options(const FilePath& path);
  bool getBoolValue(OptionId);
  string getStringValue(OptionId);
  vector<string> getVectorStringValue(OptionId);
  bool hasVectorStringValue(OptionId, const string&);
  void addVectorStringValue(OptionId, const string&);
  void removeVectorStringValue(OptionId, const string&);
  const string& getName(OptionId);
  string getValueString(OptionId);
  void setValue(OptionId, Value);
  int getIntValue(OptionId);
  void setLimits(OptionId, Range);
  optional<Range> getLimits(OptionId);
  vector<OptionId> getOptions(OptionSet);
  void handle(View*, OptionSet, int lastIndex = 0);
  typedef function<void(int)> Trigger;
  void addTrigger(OptionId, Trigger trigger);
  void setChoices(OptionId, const vector<string>&);
  bool hasChoices(OptionId) const;
  optional<string> getHint(OptionId);

  private:
  optional<Value> readValue(OptionId, const vector<string>&);
  void changeValue(OptionId, const Options::Value&, View*);
  void handleSliding(OptionId, ScriptedUIDataElems::Record&, bool&);
  void handleIntInterval(OptionId, ScriptedUIDataElems::Record&, bool&);
  void handleBoolean(OptionId, ScriptedUIDataElems::Record&, bool&);
  Value getValue(OptionId);
  void readValues();
  optional<EnumMap<OptionId, Value>> values;
  void writeValues();
  FilePath filename;
  EnumMap<OptionId, optional<Value>> overrides;
  EnumMap<OptionId, vector<string>> choices;
  EnumMap<OptionId, optional<Range>> limits;
};


