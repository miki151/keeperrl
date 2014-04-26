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

#include "view.h"
#include "util.h"

enum class OptionId {
  HINTS,
  ASCII,
  MUSIC,

  EASY_KEEPER,
  AGGRESSIVE_HEROES,

  EASY_ADVENTURER,
};

enum class OptionSet {
  GENERAL,
  KEEPER,
  ADVENTURER,
};

ENUM_HASH(OptionId);

class Options {
  public:
  static void init(const string& path);
  static int getValue(OptionId);
  static void handle(View*, OptionSet, int lastIndex = 0);
  static bool handleOrExit(View*, OptionSet, int lastIndex = -1);
  typedef function<void(bool)> Trigger;
  static void addTrigger(OptionId, Trigger trigger);

  private:
  static void setValue(OptionId, int);
  static unordered_map<OptionId, int> readValues(const string& path);
  static void writeValues(const string& path, const unordered_map<OptionId, int>&);
  static string filename;
};


#endif
