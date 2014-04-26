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

#ifndef _NAME_GENERATOR
#define _NAME_GENERATOR

class NameGenerator {
  public:
  NameGenerator() = default;
  string getNext();

  static NameGenerator firstNames;
  static NameGenerator scrolls;
  static NameGenerator aztecNames;
  static NameGenerator creatureNames;
  static NameGenerator weaponNames;
  static NameGenerator worldNames;
  static NameGenerator townNames;
  static NameGenerator dwarfNames;
  static NameGenerator deityNames;
  static NameGenerator demonNames;
  static NameGenerator dogNames;
  static NameGenerator insults;

  static void init(
      const string& firstNamesPath,
      const string& aztecNamesPath,
      const string& specialCreaturesPath,
      const string& specialWeaponsPath,
      const string& worldsPath,
      const string& townsPath,
      const string& dwarfPath,
      const string& deitiesPath,
      const string& demonsPath,
      const string& dogsPath,
      const string& insultsPath
      );

  private:
  NameGenerator(vector<string> names, bool oneName = false);
  queue<string> names;
  bool oneName;
};

#endif
