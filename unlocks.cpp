#include "stdafx.h"
#include "unlocks.h"
#include "parse_game.h"

Unlocks::Unlocks(FilePath p) : path(p) {}
Unlocks Unlocks::allUnlocked() {
  return Unlocks();
}

Unlocks::Unlocks() {}

using UnlocksSet = set<string>;

static UnlocksSet read(FilePath path) {
  if (path.exists()) {
    CompressedInput input(path.getPath());
    UnlocksSet s;
    input.getArchive() >> s;
    return s;
  } else
    return UnlocksSet{};
}

static void write(FilePath path, const UnlocksSet& s) {
  CompressedOutput(path.getPath()).getArchive() << s;
}

bool Unlocks::isUnlocked(UnlockId id) const {
  return !!path ? read(*path).count(id) : true;
}

void Unlocks::unlock(UnlockId id) {
  if (path) {
    auto s = read(*path);
    s.insert(id);
    write(*path, s);
  }
}
