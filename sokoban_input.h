#pragma once

#include "util.h"

class SokobanInput {
  public:
  SokobanInput(const string& path);

  Table<char> getNext();

  private:
  string path;
};
