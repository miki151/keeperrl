#include "stdafx.h"
#include "sokoban_input.h"


SokobanInput::SokobanInput(const string& p) : path(p) {
  ifstream input(path.c_str());
  CHECK(input) << "Failed to load sokoban data from " << path;
  CHECK(!path.empty());
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

static void printTable(ofstream& output, const Table<char>& table) {
  output << table.getBounds().width() << " " << table.getBounds().height() << endl;
  for (int y : table.getBounds().getYRange()) {
    for (int x : table.getBounds().getXRange())
      output << table[Vec2(x, y)];
    output << endl;
  }
}

Table<char> SokobanInput::getNext() {
  ifstream input(path.c_str());
  CHECK(input) << "Failed to load sokoban data from " << path;
  auto ret = readTable(input);
  CHECK(ret) << "Failed to read sokoban level from " << path;
  vector<Table<char>> rest;
  while(1) {
    if (auto next = readTable(input))
      rest.push_back(*next);
    else
      break;
  }
  input.close();
  ofstream output(path.c_str());
  for (auto& table : rest)
    printTable(output, table);
  printTable(output, *ret);
  return *ret;
}
