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

#include "name_generator.h"
#include "util.h"

NameGenerator NameGenerator::scrolls;
NameGenerator NameGenerator::firstNames;
NameGenerator NameGenerator::aztecNames;
NameGenerator NameGenerator::creatureNames;
NameGenerator NameGenerator::weaponNames;
NameGenerator NameGenerator::worldNames;
NameGenerator NameGenerator::townNames;
NameGenerator NameGenerator::dwarfNames;
NameGenerator NameGenerator::deityNames;
NameGenerator NameGenerator::demonNames;
NameGenerator NameGenerator::dogNames;
NameGenerator NameGenerator::insults;

string getSyllable() {
  string vowels = "aeyuio";
  string consonants = "qwrtplkjhgfdszxcvbnm";
  string ret;
  if (Random.roll(3))
    ret += consonants[Random.getRandom(consonants.size())];
  ret += vowels[Random.getRandom(vowels.size())];
  if (Random.roll(3))
    ret += consonants[Random.getRandom(consonants.size())];
  return ret;
}

string getWord() {
  int syllables = chooseRandom({1, 2, 3, 4}, {1, 4, 3, 1});
  string ret;
  for (int i : Range(syllables))
    ret += getSyllable();
  return ret;
}

vector<string> readLines(const string& path) {
  vector<string> input;
  ifstream in(path);
  CHECK(in.is_open()) << "Unable to open " << path;
  char buf[100];
  while (in.getline(buf, 100))
    input.push_back(buf);
  return input;
}

vector<string> combined(const string& path) {
  vector<string> input[3];
  ifstream in(path);
  CHECK(in.is_open()) << "Unable to open " << path;
  int num;
  in >> num;
  char buf[100];
  for (int w : Range(3))
    for (int i : Range(num)) {
      in.getline(buf, 100);
      input[w].push_back(buf);
    }
  vector<string> ret;
  for (int i : Range(3000)) {
    ret.push_back("Thou " + chooseRandom(input[0]) + " " + chooseRandom(input[1]) + 
        " " + chooseRandom(input[2]) + "!");
    Debug() << ret.back();
  }
  return ret;
}

void NameGenerator::init(const string& firstNamesPath, const string& aztecNamesPath,
      const string& creatureNamesPath, const string& weaponNamesPath, const string& worldsPath,
      const string& townsPath, const string& dwarfsPath, const string& deitiesPath, const string& demonsPath,
      const string& dogsPath, const string& insultsPath) {
  vector<string> input;
  for (int i : Range(1000)) {
    string ret;
    int parts = chooseRandom({1, 2}, {3, 1});
    for (int k : Range(parts))
      ret += getWord() + " ";
    trim(ret);
    input.push_back(ret);
  }
  scrolls = NameGenerator(input);

  firstNames = NameGenerator(readLines(firstNamesPath));
  aztecNames = NameGenerator(readLines(aztecNamesPath));
  creatureNames = NameGenerator(readLines(creatureNamesPath));
  weaponNames = NameGenerator(readLines(weaponNamesPath));
  worldNames = NameGenerator(readLines(worldsPath), true);
  townNames = NameGenerator(readLines(townsPath));
  deityNames = NameGenerator(readLines(deitiesPath));
  dwarfNames = NameGenerator(readLines(dwarfsPath));
  demonNames = NameGenerator(readLines(demonsPath));
  insults = NameGenerator(combined(insultsPath));
}


string NameGenerator::getNext() {
  CHECK(!names.empty()) << "Out of names!";
  string ret = names.front();
  if (!oneName)
    names.pop();
  return ret;
}

  
NameGenerator::NameGenerator(vector<string> list, bool oneN) : oneName(oneN) {
  for (string name : randomPermutation(list))
    names.push(name);
}
