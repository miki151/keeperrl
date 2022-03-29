#include "stdafx.h"
#include "special_trait.h"
#include "creature.h"
#include "creature_attributes.h"
#include "body.h"
#include "workshops.h"
#include "intrinsic_attack.h"
#include "content_factory.h"

optional<string> getExtraBodyPartPrefix(const ExtraBodyPart& part) {
  switch (part.part) {
    case BodyPart::HEAD:
      if (part.count == 1)
        return "two-headed"_s;
      else
        return "multi-headed"_s;
    case BodyPart::WING:
      return "winged"_s;
    default:
      return none;
  }
}

void applySpecialTrait(GlobalTime globalTime, SpecialTrait trait, Creature* c, const ContentFactory* factory) {
  trait.visit<void>(
      [&] (const ExtraTraining& t) {
        c->getAttributes().increaseMaxExpLevel(t.type, t.increase);
      },
      [&] (const AttrBonus& t) {
        c->getAttributes().increaseBaseAttr(t.attr, t.increase);
      },
      [&] (const SpecialAttr& t) {
        c->getAttributes().specialAttr[t.attr].push_back(make_pair(t.value, t.predicate));
      },
      [&] (const Lasting& effect) {
        if (effect.time)
          c->addEffect(effect.effect, *effect.time, globalTime, false);
        else
          c->addPermanentEffect(effect.effect);
      },
      [&] (ExtraBodyPart part) {
        c->getAttributes().add(part.part, part.count);
        if (auto prefix = getExtraBodyPartPrefix(part)) {
          c->getName().addBarePrefix(*prefix);
        }
      },
      [&] (const ExtraIntrinsicAttack& a) {
        auto attack = IntrinsicAttack(a.item);
        attack.isExtraAttack = true;
        attack.initializeItem(factory);
        c->getBody().addIntrinsicAttack(a.part, std::move(attack));
      },
      [&] (WorkshopType type) {
        c->getAttributes().setBaseAttr(factory->workshopInfo.at(type).attr, Workshops::getLegendarySkillThreshold());
      },
      [&] (CompanionInfo type) {
        c->getAttributes().companions.push_back(type);
      },
      [&] (const OneOfTraits&) {
        FATAL << "Can't apply traits alternative";
      }
  );
}

SpecialTrait transformBeforeApplying(SpecialTrait trait) {
  return trait.visit<SpecialTrait>(
      [&] (const auto&) {
        return trait;
      },
      [&] (const OneOfTraits& t) {
        return Random.choose(t.traits);
      }
  );
}
