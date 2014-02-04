#include "stdafx.h"
#include "options.h"

string Options::filename;

const unordered_map<OptionId, int> defaults {
  {OptionId::HINTS, 1},
  {OptionId::ASCII, 0},
};

const vector<pair<OptionId, string>> names {
  {OptionId::HINTS, "In-game hints"},
  {OptionId::ASCII, "Unicode graphics"},
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

vector<string> offOn { "off", "on" };

void Options::handle(View* view, int lastIndex) {
  vector<View::ListElem> options;
  for (auto elem : names) {
    options.push_back(elem.second + "      " + offOn[getValue(elem.first)]);
  }
  options.emplace_back("Done");
  auto index = view->chooseFromList("", options, lastIndex);
  if (!index || (*index) == names.size())
    return;
  setValue(names[*index].first, 1 - getValue(names[*index].first));
  handle(view, *index);
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

