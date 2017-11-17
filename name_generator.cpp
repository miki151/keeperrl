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
#include "file_path.h"

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
  int syllables = Random.choose({1, 2, 3, 4}, {1, 4, 3, 1});
  string ret;
  for (int i : Range(syllables))
    ret += getSyllable();
  return ret;
}

vector<string> readLines(const FilePath& path) {
  vector<string> input;
  ifstream in(path.getPath());
  CHECK(in.is_open()) << "Unable to open " << path;
  char buf[100];
  while (in.getline(buf, 100))
    input.push_back(buf);
  return input;
}

void NameGenerator::init(const DirectoryPath& namesPath) {
  clearAll();
  vector<string> input;
  for (int i : Range(1000)) {
    string ret;
    int parts = Random.choose({1, 2}, {3, 1});
    for (int k : Range(parts))
      ret += getWord() + " ";
    trim(ret);
    input.push_back(ret);
  }
  set(NameGeneratorId::SCROLL, new NameGenerator(input));
  set(NameGeneratorId::FIRST_MALE, new NameGenerator(readLines(namesPath.file("first_male.txt"))));
  set(NameGeneratorId::FIRST_FEMALE, new NameGenerator(readLines(namesPath.file("first_female.txt"))));
  set(NameGeneratorId::AZTEC, new NameGenerator(readLines(namesPath.file("aztec_names.txt"))));
  set(NameGeneratorId::CREATURE, new NameGenerator(readLines(namesPath.file("creatures.txt"))));
  set(NameGeneratorId::WEAPON, new NameGenerator(readLines(namesPath.file("artifacts.txt"))));
  set(NameGeneratorId::WORLD, new NameGenerator(readLines(namesPath.file("world.txt"))));
  set(NameGeneratorId::TOWN, new NameGenerator(readLines(namesPath.file("town_names.txt"))));
  set(NameGeneratorId::DEITY, new NameGenerator(readLines(namesPath.file("gods.txt"))));
  set(NameGeneratorId::DWARF, new NameGenerator(readLines(namesPath.file("dwarfs.txt"))));
  set(NameGeneratorId::DEMON, new NameGenerator(readLines(namesPath.file("demons.txt"))));
  set(NameGeneratorId::DOG, new NameGenerator(readLines(namesPath.file("dogs.txt"))));
  set(NameGeneratorId::DRAGON, new NameGenerator(readLines(namesPath.file("dragons.txt"))));
  set(NameGeneratorId::CYCLOPS, new NameGenerator(readLines(namesPath.file("cyclops.txt"))));
  set(NameGeneratorId::ORC, new NameGenerator(readLines(namesPath.file("orc.txt"))));
  set(NameGeneratorId::VAMPIRE, new NameGenerator(readLines(namesPath.file("vampires.txt"))));
}


string NameGenerator::getNext() {
  CHECK(!names.empty());
  string ret = names.front();
  if (!oneName) {
    names.pop();
    names.push(ret);
  }
  return ret;
}

  
NameGenerator::NameGenerator(vector<string> list, bool oneN) : oneName(oneN) {
  for (string name : Random.permutation(list))
    names.push(name);
}
