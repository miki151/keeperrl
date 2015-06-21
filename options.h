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
  KEEP_SAVEFILES,
  FULLSCREEN,
  ONLINE,
  AUTOSAVE,

  FAST_IMMIGRATION,
  ADVENTURER_NAME,

  KEEPER_NAME,
  SHOW_MAP,
  START_WITH_NIGHT,
  STARTING_RESOURCE
);

enum class OptionSet {
  GENERAL,
  KEEPER,
  ADVENTURER,
};

class View;

class Options {
  public:
  typedef variant<bool, string> Value;
  Options(const string& path, const string& overrides);
  bool getBoolValue(OptionId);
  string getStringValue(OptionId);
  void handle(View*, OptionSet, int lastIndex = 0);
  bool handleOrExit(View*, OptionSet, int lastIndex = -1);
  typedef function<void(bool)> Trigger;
  void addTrigger(OptionId, Trigger trigger);
  void setDefaultString(OptionId, const string&);

  private:
  void setValue(OptionId, Value);
  void changeValue(OptionId, const Options::Value&, View*);
  string getValueString(OptionId, Options::Value);
  Value getValue(OptionId);
  EnumMap<OptionId, Value> readValues();
  void writeValues(const EnumMap<OptionId, Value>&);
  string filename;
  EnumMap<OptionId, string> defaultStrings;
  EnumMap<OptionId, optional<Value>> overrides;
};


#endif
