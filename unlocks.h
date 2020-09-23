#pragma once

#include "file_path.h"
#include "stdafx.h"
#include "util.h"

class Unlocks {
  public:
  Unlocks(FilePath);
  bool isUnlocked(UnlockId) const;
  void unlock(UnlockId);

  private:
  FilePath path;
};