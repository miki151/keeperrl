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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "util.h"

RICH_ENUM(OptionId,
  HINTS,
  ASCII,
  MUSIC,
  SOUND,
  KEEP_SAVEFILES,
  FULLSCREEN,
  FULLSCREEN_RESOLUTION,
  ONLINE,
  AUTOSAVE,
  WASD_SCROLLING,
  ZOOM_UI,
  DISABLE_MOUSE_WHEEL,

  FAST_IMMIGRATION,
  ADVENTURER_NAME,

  KEEPER_NAME,
  KEEPER_SEED,
  SHOW_MAP,
  START_WITH_NIGHT,
  STARTING_RESOURCE,

  MAIN_VILLAINS,
  RETIRED_VILLAINS,
  LESSER_VILLAINS,
  ALLIES,
  INFLUENCE_SIZE
);

enum class OptionSet {
  GENERAL,
  KEEPER,
  ADVENTURER,
  CAMPAIGN,
};

class View;

class Options {
  public:
  typedef variant<int, string> Value;
  Options(const string& path, const string& overrides);
  bool getBoolValue(OptionId);
  string getStringValue(OptionId);
  const string& getName(OptionId);
  enum Type { INT, BOOL, STRING };
  Type getType(OptionId);
  string getValueString(OptionId);
  void setValue(OptionId, Value);
  int getChoiceValue(OptionId);
  int getIntValue(OptionId);
  void setLimits(OptionId, int min, int max);
  optional<pair<int, int>> getLimits(OptionId);
  vector<OptionId> getOptions(OptionSet);
  void handle(View*, OptionSet, int lastIndex = 0);
  bool handleOrExit(View*, OptionSet, int lastIndex = -1);
  typedef function<void(int)> Trigger;
  void addTrigger(OptionId, Trigger trigger);
  void setDefaultString(OptionId, const string&);
  void setChoices(OptionId, const vector<string>&);

  private:
  optional<Value> readValue(OptionId, const string&);
  void changeValue(OptionId, const Options::Value&, View*);
  Value getValue(OptionId);
  void readValues();
  optional<EnumMap<OptionId, Value>> values;
  void writeValues();
  string filename;
  EnumMap<OptionId, string> defaultStrings;
  EnumMap<OptionId, optional<Value>> overrides;
  EnumMap<OptionId, vector<string>> choices;
  EnumMap<OptionId, optional<pair<int, int>>> limits;
};


#endif
