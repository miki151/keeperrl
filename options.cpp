#include "stdafx.h"
#include "options.h"

string Options::filename;

const unordered_map<OptionId, int> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
  {OptionId::EASY_GAME, 1},
};

const vector<pair<OptionId, string>> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
  {OptionId::EASY_GAME, "Game difficulty"},
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
  {OptionId::EASY_GAME, { "hard", "easy" }}};

unordered_set<OptionId> disabledInGame { OptionId::EASY_GAME };

void Options::handle(View* view, bool inGame, int lastIndex) {
  vector<View::ListElem> options;
  options.emplace_back("Set options:", View::TITLE);
  for (auto elem : names) {
    options.emplace_back(elem.second + "      " + valueNames[elem.first][getValue(elem.first)],
        inGame && disabledInGame.count(elem.first) ? View::INACTIVE : View::NORMAL);
  }
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex);
  if (!index || (*index) == names.size())
    return;
  setValue(names[*index].first, 1 - getValue(names[*index].first));
  handle(view, inGame, *index);
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

