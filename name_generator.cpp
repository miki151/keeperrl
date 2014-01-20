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

void NameGenerator::init(const string& firstNamesPath, const string& aztecNamesPath,
      const string& creatureNamesPath, const string& weaponNamesPath, const string& worldsPath,
      const string& townsPath, const string& dwarfsPath, const string& deitiesPath, const string& demonsPath,
      const string& dogsPath) {
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
  worldNames = NameGenerator(readLines(worldsPath));
  townNames = NameGenerator(readLines(townsPath));
  deityNames = NameGenerator(readLines(deitiesPath));
  dwarfNames = NameGenerator(readLines(dwarfsPath));
  demonNames = NameGenerator(readLines(demonsPath));
  dogNames = NameGenerator(readLines(dogsPath));
}


string NameGenerator::getNext() {
  CHECK(!names.empty()) << "Out of names!";
  string ret = names.front();
  names.pop();
  return ret;
}

  
NameGenerator::NameGenerator(vector<string> list) {
  random_shuffle(list.begin(), list.end(),[](int a) { return Random.getRandom(a);});
  for (string name : list)
    names.push(name);
}
