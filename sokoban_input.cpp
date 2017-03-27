#include "stdafx.h"
#include "sokoban_input.h"


SokobanInput::SokobanInput(const FilePath& l, const FilePath& s) : levelsPath(l), statePath(s) {
  ifstream input(levelsPath.getPath());
  CHECK(input) << "Failed to load sokoban data from " << levelsPath;
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

static int getTableNum(const FilePath& path) {
  int ret = 0;
  ifstream in(path.getPath());
  if (in)
    in >> ret;
  return ret;
}

Table<char> SokobanInput::getNext() {
  ifstream input(levelsPath.getPath());
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
  ofstream(statePath.getPath()) << (curLevel + 1);
  return rest[curLevel % rest.size()];
}
