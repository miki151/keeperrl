#pragma once

#include "file_path.h"
#include "stdafx.h"
#include "util.h"

class Options;

class Unlocks {
  public:
  Unlocks(Options*, FilePath);
  static Unlocks allUnlocked();
  bool isUnlocked(UnlockId) const;
  void unlock(UnlockId);

  private:
  Unlocks();
  Options* options = nullptr;
  optional<FilePath> path;
};