#include "stdafx.h"
#include "creature_inventory.h"


SERIALIZE_DEF(CreatureInventoryElem, type, countMin, countMax, chance, prefixChance, alternative)

#include "pretty_archive.h"

template <>
void CreatureInventoryElem::serialize(PrettyInputArchive& ar1, const unsigned int) {
  ar1(NAMED(type), OPTION(countMin), OPTION(countMax), OPTION(chance), OPTION(prefixChance), NAMED(alternative));
  ar1(endInput());
  if (countMin > countMax)
    ar1.error("countMin > countMax (" + toString(countMin) + ", " + toString(countMax) + ")");
}
