#include "automaton_part.h"
#include "automaton_slot.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item.h"

SERIALIZE_DEF(AutomatonPart, slot, effect, viewId, name)

bool AutomatonPart::isAvailable(const Creature* c, int numAssigned) const {
  if (c->automatonParts.size() + numAssigned >= c->getAttributes().getAutomatonSlots())
    return false;
  int numInSlot = 0;
  for (auto& part : c->automatonParts)
    if (part.slot == slot)
      ++numInSlot;
  return numInSlot < effect.size();
}

void AutomatonPart::apply(Creature* c) const {
  int numInSlot = 0;
  for (auto& part : c->automatonParts)
    if (part.slot == slot)
      ++numInSlot;
  effect[numInSlot].apply(c->getPosition());
  c->automatonParts.push_back(*this);
}

#include "pretty_archive.h"
template <>
void AutomatonPart::serialize(PrettyInputArchive& ar1, unsigned) {
  ar1(slot, effect);
}

