#pragma once

#include "file_path.h"
#include "stdafx.h"
#include "util.h"

class Unlocks {
  public:
  Unlocks(FilePath);
  static Unlocks allUnlocked();
  bool isUnlocked(UnlockId) const;
  void unlock(UnlockId);

  private:
  Unlocks();
  optional<FilePath> path;
};