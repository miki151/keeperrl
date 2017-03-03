#pragma once

#include "util.h"

class SokobanInput {
  public:
  SokobanInput(const string& levelsPath, const string& statePath);

  Table<char> getNext();

  private:
  string levelsPath;
  string statePath;
};
