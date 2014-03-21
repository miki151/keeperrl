#include "stdafx.h"
#include "options.h"

string Options::filename;

const unordered_map<OptionId, int> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
  {OptionId::EASY_KEEPER, 1},
  {OptionId::AGGRESSIVE_HEROES, 1},
  {OptionId::EASY_ADVENTURER, 0},
};

const map<OptionId, string> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::EASY_KEEPER, "Game difficulty"},
  {OptionId::AGGRESSIVE_HEROES, "Aggressive enemies"},
  {OptionId::EASY_ADVENTURER, "Game difficulty"},
};

const map<OptionSet, vector<OptionId>> optionSets {
  {OptionSet::GENERAL, {OptionId::HINTS, OptionId::ASCII}},
  {OptionSet::KEEPER, {OptionId::EASY_KEEPER, OptionId::AGGRESSIVE_HEROES}},
  {OptionSet::ADVENTURER, {OptionId::EASY_ADVENTURER}},
};

void Options::init(const string& path) {
  filename = path;
}

int Options::getValue(OptionId id) {
  auto values = readValues(filename);
  if (values.count(id))
    return values.at(id);
  else
    return defaults.at(id);
}

void Options::setValue(OptionId id, int value) {
  auto values = readValues(filename);
  values[id] = value;
  writeValues(filename, values);
}

unordered_map<OptionId, vector<string>> valueNames {
  {OptionId::HINTS, { "off", "on" }},
  {OptionId::ASCII, { "off", "on" }},
  {OptionId::EASY_KEEPER, { "hard", "easy" }},
  {OptionId::AGGRESSIVE_HEROES, { "no", "yes" }},
  {OptionId::EASY_ADVENTURER, { "hard", "easy" }},
};

bool Options::handleOrExit(View* view, OptionSet set, int lastIndex) {
  vector<View::ListElem> options;
  options.emplace_back("Change settings:", View::TITLE);
  for (OptionId option : optionSets.at(set))
    options.emplace_back(names.at(option) + "      " + valueNames.at(option)[getValue(option)]);
  options.emplace_back("Done");
  options.emplace_back("Cancel");
  if (lastIndex == -1)
    lastIndex = optionSets.at(set).size();
  auto index = view->chooseFromList("", options, lastIndex);
  if (!index || (*index) == optionSets.at(set).size() + 1)
    return false;
  else if (index && (*index) == optionSets.at(set).size())
    return true;
  OptionId option = optionSets.at(set)[*index];
  setValue(option, 1 - getValue(option));
  return handleOrExit(view, set, *index);
}

void Options::handle(View* view, OptionSet set, int lastIndex) {
  vector<View::ListElem> options;
  options.emplace_back("Change settings:", View::TITLE);
  for (OptionId option : optionSets.at(set))
    options.emplace_back(names.at(option) + "      " + valueNames.at(option)[getValue(option)]);
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex);
  if (!index || (*index) == optionSets.at(set).size())
    return;
  OptionId option = optionSets.at(set)[*index];
  setValue(option, 1 - getValue(option));
  handle(view, set, *index);
}

unordered_map<OptionId, int> Options::readValues(const string& path) {
  unordered_map<OptionId, int> ret;
  ifstream in(path);
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    vector<string> p = split(string(buf), ',');
    CHECK(p.size() == 2) << "Input error " << p;
    ret[OptionId(convertFromString<int>(p[0]))] = convertFromString<int>(p[1]);
  }
  return ret;
}

void Options::writeValues(const string& path, const unordered_map<OptionId, int>& values) {
  ofstream out(path);
  for (auto elem : values)
    out << int(elem.first) << "," << elem.second << std::endl;
}

