#include "stdafx.h"
#include "spell.h"
#include "effect.h"
#include "creature.h"
#include "player_message.h"
#include "creature_name.h"
#include "creature_factory.h"
#include "sound.h"
#include "lasting_effect.h"
#include "effect.h"
#include "furniture_type.h"
#include "attr_type.h"
#include "attack_type.h"
#include "fx_name.h"

const string& Spell::getName() const {
  return name;
}

bool Spell::isDirected() const {
  return effect->contains<DirEffectType>();
}

bool Spell::hasEffect(Effect t) const {
  return effect->getReferenceMaybe<Effect>() == t;
}

bool Spell::hasEffect(DirEffectType t) const {
  return effect->getReferenceMaybe<DirEffectType>() == t;
}

Effect Spell::getEffect() const {
  return *effect->getReferenceMaybe<Effect>();
}

DirEffectType Spell::getDirEffectType() const{
  return *effect->getReferenceMaybe<DirEffectType>();
}

int Spell::getDifficulty() const {
  return difficulty;
}

Spell::Spell(const string& n, Effect e, int diff, SoundId s, optional<FXInfo> fx, optional<FXInfo> splashFx,
             CastMessageType msg)
    : name(n), effect(e), difficulty(diff), castMessageType(msg), sound(s), fx(fx), splashFx(splashFx) {
}

Spell::Spell(const string& n, DirEffectType e, int diff, SoundId s, optional<FXInfo> fx, optional<FXInfo> splashFx,
             CastMessageType msg)
    : name(n), effect(e), difficulty(diff), castMessageType(msg), sound(s), fx(fx), splashFx(splashFx) {
}

SoundId Spell::getSound() const {
  return sound;
}

string Spell::getDescription() const {
  return effect->visit(
      [](const Effect& e) { return e.getDescription(); },
      [](const DirEffectType& e) { return ::getDescription(e); }
  );
}

void Spell::addMessage(WCreature c) {
  switch (castMessageType) {
    case CastMessageType::STANDARD:
      c->secondPerson("You cast " + getName());
      c->thirdPerson(c->getName().the() + " casts a spell");
      break;
    case CastMessageType::AIR_BLAST:
      c->secondPerson("You create an air blast!");
      c->thirdPerson(c->getName().the() + " creates an air blast!");
      break;
  }
}

void Spell::init() {
  set(SpellId::HEAL_SELF, new Spell("heal self", Effect::Heal{}, 30, SoundId::SPELL_HEALING,
        FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_GREEN)));
  set(SpellId::SUMMON_INSECTS, new Spell("summon insects", Effect::Summon{CreatureId::FLY}, 30,
        SoundId::SPELL_SUMMON_INSECTS));
  set(SpellId::DECEPTION, new Spell("deception", Effect::Deception{}, 60, SoundId::SPELL_DECEPTION));
  set(SpellId::SPEED_SELF, new Spell("haste self", Effect::Lasting{LastingEffect::SPEED}, 60,
        SoundId::SPELL_SPEED_SELF, FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_BLUE)));
  set(SpellId::DAM_BONUS, new Spell("damage", Effect::Lasting{LastingEffect::DAM_BONUS}, 90,
        SoundId::SPELL_STR_BONUS, FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_RED)));
  set(SpellId::DEF_BONUS, new Spell("defense", Effect::Lasting{LastingEffect::DEF_BONUS}, 90,
        SoundId::SPELL_DEX_BONUS, FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_BROWN)));
  set(SpellId::HEAL_OTHER, new Spell("heal other",  DirEffectType(1, DirEffectId::CREATURE_EFFECT,
      Effect::Heal{}) , 6, SoundId::SPELL_HEALING));
  set(SpellId::FIRE_SPHERE_PET, new Spell("fire sphere", Effect::Summon{CreatureId::FIRE_SPHERE}, 20,
        SoundId::SPELL_FIRE_SPHERE_PET));
  set(SpellId::TELEPORT, new Spell("escape", Effect::Teleport{}, 80, SoundId::SPELL_TELEPORT));
  set(SpellId::INVISIBILITY, new Spell("invisibility", Effect::Lasting{LastingEffect::INVISIBLE}, 150,
        SoundId::SPELL_INVISIBILITY, FXInfo(FXName::CIRCULAR_SPELL, Color::WHITE)));
  set(SpellId::BLAST,
      new Spell("blast", DirEffectType(4, DirEffectId::BLAST), 100, SoundId::SPELL_BLAST, {FXName::AIR_BLAST}));
  set(SpellId::MAGIC_MISSILE, new Spell("magic missile", DirEffectType(4, DirEffectId::CREATURE_EFFECT,
      Effect::Damage{AttrType::SPELL_DAMAGE, AttackType::SPELL}), 3, SoundId::SPELL_BLAST, {FXName::MAGIC_MISSILE}, {FXName::MAGIC_MISSILE_SPLASH}));
  set(SpellId::FIREBALL, new Spell("fireball", DirEffectType(4, DirEffectId::FIREBALL), 3, SoundId::SPELL_BLAST,
              {FXName::FIREBALL}));
  set(SpellId::CIRCULAR_BLAST, new Spell("circular blast", Effect::CircularBlast{}, 150, SoundId::SPELL_AIR_BLAST,
              {FXName::CIRCULAR_BLAST}, none, CastMessageType::AIR_BLAST));
  set(SpellId::SUMMON_SPIRIT, new Spell("summon spirits", Effect::Summon{CreatureId::SPIRIT}, 150,
        SoundId::SPELL_SUMMON_SPIRIT));
  set(SpellId::CURE_POISON, new Spell("cure poisoning", Effect::CurePoison{}, 150, SoundId::SPELL_CURE_POISON,
              FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_GREEN)));
  set(SpellId::METEOR_SHOWER, new Spell("meteor shower", Effect::PlaceFurniture{FurnitureType::METEOR_SHOWER}, 150,
        SoundId::SPELL_METEOR_SHOWER));
  set(SpellId::PORTAL, new Spell("portal", Effect::PlaceFurniture{FurnitureType::PORTAL}, 150, SoundId::SPELL_PORTAL));
  set(SpellId::SUMMON_ELEMENT, new Spell("summon element", Effect::SummonElement{}, 150, SoundId::SPELL_SUMMON_SPIRIT));
}

optional<int> Spell::getLearningExpLevel() const {
  switch (getId()) {
    case SpellId::HEAL_SELF: return 1;
    case SpellId::SUMMON_INSECTS: return 2;
    case SpellId::HEAL_OTHER: return 3;
    case SpellId::MAGIC_MISSILE: return 4;
    case SpellId::DECEPTION: return 4;
    case SpellId::TELEPORT: return 5;
    case SpellId::SPEED_SELF: return 5;
    //case SpellId::STUN_RAY: return 6;
    case SpellId::CURE_POISON: return 6;
    case SpellId::BLAST: return 6;
    case SpellId::CIRCULAR_BLAST: return 7;
    case SpellId::FIREBALL: return 7;
    case SpellId::DEF_BONUS: return 8;
    case SpellId::SUMMON_ELEMENT: return 8;
    case SpellId::DAM_BONUS: return 9;
    case SpellId::FIRE_SPHERE_PET: return 10;
    case SpellId::METEOR_SHOWER: return 11;
    case SpellId::INVISIBILITY: return 12;
    default:
      return none;
  }
}

optional<FXInfo> Spell::getFX() const {
  return fx;
};

optional<FXInfo> Spell::getSplashFX() const {
  return splashFx;
};
