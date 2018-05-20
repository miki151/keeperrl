#include "stdafx.h"
#include "special_trait.h"
#include "creature.h"
#include "creature_attributes.h"
#include "skill.h"

void applySpecialTrait(SpecialTrait trait, WCreature c) {
  trait.visit(
      [&] (const ExtraTraining& t) {
        c->getAttributes().increaseMaxExpLevel(t.type, t.increase);
        c->getAttributes().getName().setAdjective(getSpecialTraitAdjective(t.type));
        },
      [&] (LastingEffect effect) {
        c->addPermanentEffect(effect);
        c->getAttributes().getName().setAdjective(getSpecialTraitAdjective(effect));
      },
      [&] (SkillId skill) {
        if (Skill::get(skill)->isDiscrete())
          c->getAttributes().getSkills().insert(skill);
        else
          c->getAttributes().getSkills().increaseValue(skill, 0.4);
          c->getAttributes().getName().setJobDescription(getSpecialTraitJobName(skill));
          
      }
  );
}

string getSpecialTraitJobName(SkillId skill) {
  switch (skill) {
    case SkillId::AMBUSH:
      return "(assassin)";
      break;
    case SkillId::STEALING:
    case SkillId::DISARM_TRAPS:
      return "(thief)";
      break;
    case SkillId::SWIMMING:
      return "(swimmer)";
      break;
    case SkillId::CONSTRUCTION:
      return "(builder)";
      break;
    case SkillId::DIGGING:
    case SkillId::NAVIGATION_DIGGING:
      return "(miner)";
      break;
    case SkillId::SORCERY:
      return "(wizard)";
      break;
    case SkillId::CROPS:
      return "(farmer)";
      break;
    case SkillId::SPIDER:
      return "(webber)";
      break;
    case SkillId::STEALTH:
      return "(spy)";
      break;
    case SkillId::WORKSHOP:
      return "(crafter)";
      break;
    case SkillId::FORGE:
      return "(blacksmith)";
      break;
    case SkillId::LABORATORY:
      return "(brewer)";
      break;
    case SkillId::JEWELER:
      return "(jeweler)";
      break;
    case SkillId::EXPLORE:
    case SkillId::EXPLORE_NOCTURNAL:
    case SkillId::EXPLORE_CAVES:
      return "(explorer)";
      break;
    case SkillId::FURNACE:
    case SkillId::CONSUMPTION:
    case SkillId::COPULATION:
      return "(unusal)";
      break;
    }
}

string getSpecialTraitAdjective(ExperienceType type) {
  switch (type) {
    case ExperienceType::MELEE:
      return "muscular";
      break;
    case ExperienceType::SPELL:
      return "intelligent";
      break;
    case ExperienceType::ARCHERY:
      return "dexterous";
      break;
  }
}

string getSpecialTraitAdjective(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::SLEEP:
    case LastingEffect::BLEEDING:
    case LastingEffect::ENTANGLED:
    case LastingEffect::TIED_UP:
    case LastingEffect::STUNNED:
    case LastingEffect::PREGNANT:
    case LastingEffect::SUNLIGHT_VULNERABLE:
    case LastingEffect::SUMMONED:
    case LastingEffect::COLLAPSED:
    case LastingEffect::POISON:      
      return "damaged";
      break;
    case LastingEffect::PANIC:
      return "nervous";
      break;
    case LastingEffect::RAGE:
      return "angry";
      break;
    case LastingEffect::SLOWED:
      return "slow";
      break;
    case LastingEffect::SPEED:
      return "possessed";
      break;
    case LastingEffect::DAM_BONUS:
     return "mean";
      break;
    case LastingEffect::DEF_BONUS:
     return "huge";
     break;
    case LastingEffect::HALLU:
     return "crazy";
     break;
    case LastingEffect::BLIND:
      return "blind";
      break;
    case LastingEffect::INVISIBLE:
      return "ghostly";
      break;
    case LastingEffect::POISON_RESISTANT:
      return "thick-skinned";
      break;
    case LastingEffect::FIRE_RESISTANT:
      return "unburnable";
      break;
    case LastingEffect::FLYING:
      return "winged";
      break;
    case LastingEffect::INSANITY:
      return "insane";
      break;
    case LastingEffect::PEACEFULNESS:
      return "soft-headed";
      break;
    case LastingEffect::LIGHT_SOURCE:
      return "shining";
      break;
    case LastingEffect::DARKNESS_SOURCE:
      return "evil";
      break;
    case LastingEffect::SLEEP_RESISTANT:
      return "unsleeping";
      break;
    case LastingEffect::MAGIC_RESISTANCE:
      return "spell-resistant";
      break;
    case LastingEffect::MELEE_RESISTANCE:
      return "dodging";
      break;
    case LastingEffect::RANGED_RESISTANCE:
      return "arrow-proof";
      break;
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
      return "vulnerable";
      break;
    case LastingEffect::ELF_VISION:
      return "eagle-eyed";
      break;
    case LastingEffect::NIGHT_VISION:
      return "night-eyed";
      break;
    case LastingEffect::REGENERATION:
      return "energetic";
      break;
    case LastingEffect::WARNING:
      return "intuitive";
      break;
    case LastingEffect::TELEPATHY:
      return "telepathic";
      break;
    case LastingEffect::SATIATED:
      return "ever-full";
      break;
    case LastingEffect::RESTED:
      return "ever-resting";
      break;
  }
}
