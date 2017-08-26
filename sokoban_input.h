#pragma once

#include "util.h"
#include "file_path.h"

class SokobanInput {
  public:
  SokobanInput(const FilePath& levels, const FilePath& state);

  Table<char> getNext();
  static optional<Table<char> > readTable(ifstream&);

  private:
  FilePath levelsPath;
  FilePath statePath;
};
