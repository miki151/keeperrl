#include "stdafx.h"
#include "special_trait.h"
#include "creature.h"
#include "creature_attributes.h"
#include "skill.h"
#include "body.h"

void applySpecialTrait(SpecialTrait trait, Creature* c) {
  trait.visit(
      [&] (const ExtraTraining& t) {
        c->getAttributes().increaseMaxExpLevel(t.type, t.increase);
      },
      [&] (const AttrBonus& t) {
        c->getAttributes().increaseBaseAttr(t.attr, t.increase);
      },
      [&] (LastingEffect effect) {
        c->addPermanentEffect(effect);
      },
      [&] (ExtraBodyPart part) {
        c->getAttributes().add(part.part, part.count);
      },
      [&] (SkillId skill) {
        c->getAttributes().getSkills().increaseValue(skill, 0.4);
      },
      [&] (const OneOfTraits&) {
        FATAL << "Can't apply traits alternative";
      }
  );
}

SpecialTrait transformBeforeApplying(SpecialTrait trait) {
  return trait.visit(
      [&] (const auto&) {
        return trait;
      },
      [&] (const OneOfTraits& t) {
        return Random.choose(t.traits);
      }
  );
}
