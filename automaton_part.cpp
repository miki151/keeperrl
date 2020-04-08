#include "automaton_part.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item.h"
#include "sound.h"

SERIALIZE_DEF(AutomatonPart, effect, viewId, name, usesSlot)

bool AutomatonPart::isAvailable(const Creature* c, int numAssigned) const {
  return c->automatonParts.size() + numAssigned < c->getAttributes().getAutomatonSlots();
}

void AutomatonPart::apply(Creature* c) const {
  effect.apply(c->getPosition());
  if (usesSlot)
    c->automatonParts.push_back(*this);
  c->addSound(SoundId::TRAP_ARMING);
}

#include "pretty_archive.h"
template <>
void AutomatonPart::serialize(PrettyInputArchive& ar1, unsigned) {
  ar1(NAMED(effect), OPTION(usesSlot));
}

