#pragma once

#include "util.h"
#include "file_path.h"

class SokobanInput {
  public:
  SokobanInput(const FilePath& levels, const FilePath& state);

  Table<char> getNext();

  private:
  FilePath levelsPath;
  FilePath statePath;
};
