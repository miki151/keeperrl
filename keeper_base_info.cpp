#include "keeper_base_info.h"
#include "pretty_archive.h"

void KeeperBaseInfo::serialize(PrettyInputArchive& ar1, unsigned int version) {
  if (ar1.eatMaybe("insideMountain"))
    insideMountain = true;
  ar1(size, layout);
}
