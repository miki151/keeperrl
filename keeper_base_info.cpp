#include "keeper_base_info.h"
#include "pretty_archive.h"
#include "clock.h"

void KeeperBaseInfo::serialize(PrettyInputArchive& ar1, unsigned int version) {
  if (ar1.eatMaybe("insideMountain"))
    insideMountain = true;
  if (ar1.eatMaybe("christmasSpecial"))
    addTerritory = false;
  if (ar1.eatMaybe("dontAddTerritory"))
    addTerritory = false;
  ar1(size, layout);
}

bool KeeperBaseInfo::isActive() {
  return !christmasSpecial || Clock::isChristmas();
}