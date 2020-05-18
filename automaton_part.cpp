#include "automaton_part.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item.h"
#include "sound.h"

SERIALIZE_DEF(AutomatonPart, effect, viewId, name, minionGroup, installedId, layer, automatonType, prefixes, prefixType)

bool AutomatonPart::isAvailable(const Creature* c, int numAssigned) const {
  return c->getSpareAutomatonSlots() > numAssigned &&
      c->getAttributes().getAutomatonSlots().second.contains(automatonType);
}

#include "pretty_archive.h"
template <>
void AutomatonPart::serialize(PrettyInputArchive& ar1, unsigned) {
  ar1(NAMED(automatonType), NAMED(layer), NAMED(installedId), NAMED(effect), OPTION(minionGroup), NAMED(prefixType));
}

