#include "stdafx.h"
#include "sokoban_input.h"


SokobanInput::SokobanInput(const string& l, const string& s) : levelsPath(l), statePath(s) {
  ifstream input(levelsPath.c_str());
  CHECK(input) << "Failed to load sokoban data from " << levelsPath;
  CHECK(!levelsPath.empty());
}

static optional<Table<char>> readTable(ifstream& input) {
  Vec2 size;
  input >> size.x >> size.y;
  if (!input)
    return none;
  Table<char> ret(size);
  for (int y : ret.getBounds().getYRange()) {
    string s;
    input >> s;
    CHECK(s.size() == ret.getBounds().width());
    for (int x : All(s))
      ret[x][y] = s[x];
  }
  return ret;
}

static int getTableNum(const string& path) {
  int ret = 0;
  if (auto in = ifstream(path.c_str()))
    in >> ret;
  return ret;
}

Table<char> SokobanInput::getNext() {
  ifstream input(levelsPath.c_str());
  CHECK(input) << "Failed to load sokoban data from " << levelsPath;
  vector<Table<char>> rest;
  while(1) {
    if (auto next = readTable(input))
      rest.push_back(*next);
    else
      break;
  }
  CHECK(!rest.empty()) << "Failed to load sokoban data from " << levelsPath;
  input.close();
  int curLevel = getTableNum(statePath);
  ofstream(statePath) << (curLevel + 1);
  return rest[curLevel % rest.size()];
}
