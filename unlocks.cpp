#include "stdafx.h"
#include "unlocks.h"
#include "parse_game.h"
#include "options.h"

Unlocks::Unlocks(Options* options, FilePath p) : options(options), path(p) {}
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
  return !options || options->getBoolValue(OptionId::UNLOCK_ALL) || read(*path).count(id);
}

void Unlocks::unlock(UnlockId id) {
  if (path) {
    auto s = read(*path);
    s.insert(id);
    write(*path, s);
  }
}

void Unlocks::achieve(AchievementId id) {
  unlock("ACH_"_s + id.data());
}

bool Unlocks::isAchieved(AchievementId id) const {
  return isUnlocked("ACH_"_s + id.data());
}