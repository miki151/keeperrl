#include "keeper_base_info.h"
#include "pretty_archive.h"

void KeeperBaseInfo::serialize(PrettyInputArchive& ar, unsigned int version) {
  if (ar.eatMaybe("insideMountain"))
    insideMountain = true;
  ar(size, layout);
}
