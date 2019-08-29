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
#include "name_generator_id.h"

SERIALIZE_DEF(NameGenerator, names)

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

NameGenerator::NameGenerator() {
  vector<string> input;
  for (int i : Range(1000)) {
    string ret;
    int parts = Random.choose({1, 2}, {3, 1});
    for (int k : Range(parts))
      ret += getWord() + " ";
    trim(ret);
    input.push_back(ret);
  }
  auto set = [&] (NameGeneratorId id, vector<string> input) {
    for (string name : Random.permutation(input))
      names[id].push_back(name);
  };
  set(NameGeneratorId("SCROLL"), input);
}


void NameGenerator::setNames(NameGeneratorId id, vector<string> v) {
  for (auto& name : Random.permutation(v))
    names[id].push_back(name);
}

string NameGenerator::getNext(NameGeneratorId id) {
  CHECK(!names[id].empty());
  string ret = names[id].front();
  names[id].pop_front();
  names[id].push_back(ret);
  return ret;
}

vector<string> NameGenerator::getAll(NameGeneratorId id) {
  return vector<string>(names[id].begin(), names[id].end());
}
