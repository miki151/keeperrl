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

string getSyllable() {
  string vowels = "aeyuio";
  string consonants = "qwrtplkjhgfdszxcvbnm";
  string ret;
  if (Random.roll(3))
    ret += consonants[Random.get(consonants.size())];
  ret += vowels[Random.get(vowels.size())];
  if (Random.roll(3))
    ret += consonants[Random.get(consonants.size())];
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

void NameGenerator::init(const string& namesPath) {
  vector<string> input;
  for (int i : Range(1000)) {
    string ret;
    int parts = chooseRandom({1, 2}, {3, 1});
    for (int k : Range(parts))
      ret += getWord() + " ";
    trim(ret);
    input.push_back(ret);
  }
  set(NameGeneratorId::SCROLL, new NameGenerator(input));
  set(NameGeneratorId::FIRST, new NameGenerator(readLines(namesPath + "/first_names.txt")));
  set(NameGeneratorId::AZTEC, new NameGenerator(readLines(namesPath + "/aztec_names.txt")));
  set(NameGeneratorId::CREATURE, new NameGenerator(readLines(namesPath + "/creatures.txt")));
  set(NameGeneratorId::WEAPON, new NameGenerator(readLines(namesPath + "/artifacts.txt")));
  set(NameGeneratorId::WORLD, new NameGenerator(readLines(namesPath + "/world.txt"), true));
  set(NameGeneratorId::TOWN, new NameGenerator(readLines(namesPath + "/town_names.txt")));
  set(NameGeneratorId::DEITY, new NameGenerator(readLines(namesPath + "/gods.txt")));
  set(NameGeneratorId::DWARF, new NameGenerator(readLines(namesPath + "/dwarfs.txt")));
  set(NameGeneratorId::DEMON, new NameGenerator(readLines(namesPath + "/demons.txt")));
  set(NameGeneratorId::DOG, new NameGenerator(readLines(namesPath + "/dogs.txt")));
  set(NameGeneratorId::DRAGON, new NameGenerator(readLines(namesPath + "/dragons.txt")));
  set(NameGeneratorId::CYCLOPS, new NameGenerator(readLines(namesPath + "/cyclops.txt")));
}


string NameGenerator::getNext() {
  string ret = names.front();
  if (!oneName) {
    names.pop();
    names.push(ret);
  }
  return ret;
}

  
NameGenerator::NameGenerator(vector<string> list, bool oneN) : oneName(oneN) {
  for (string name : randomPermutation(list))
    names.push(name);
}
