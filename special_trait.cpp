#include "stdafx.h"
#include "special_trait.h"
#include "creature.h"
#include "creature_attributes.h"
#include "skill.h"

void applySpecialTrait(SpecialTrait trait, WCreature c) {
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
      [&] (SkillId skill) {
        if (Skill::get(skill)->isDiscrete())
          c->getAttributes().getSkills().insert(skill);
        else
          c->getAttributes().getSkills().increaseValue(skill, 0.4);
      }
  );
}
