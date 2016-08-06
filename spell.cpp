#include "stdafx.h"
#include "spell.h"
#include "effect.h"
#include "creature.h"
#include "player_message.h"
#include "creature_name.h"
#include "creature_factory.h"
#include "sound.h"
#include "lasting_effect.h"
#include "effect_type.h"

const string& Spell::getName() const {
  return name;
}

bool Spell::isDirected() const {
  return boost::get<DirEffectType>(&*effect);
}

bool Spell::hasEffect(EffectType t) const {
  return !isDirected() && boost::get<EffectType>(*effect) == t;
}

bool Spell::hasEffect(DirEffectType t) const {
  return isDirected() && boost::get<DirEffectType>(*effect) == t;
}

EffectType Spell::getEffectType() const {
  CHECK(!isDirected());
  return boost::get<EffectType>(*effect);
}

DirEffectType Spell::getDirEffectType() const{
  CHECK(isDirected());
  return boost::get<DirEffectType>(*effect);
}

int Spell::getDifficulty() const {
  return difficulty;
}

Spell::Spell(const string& n, EffectType e, int diff, SoundId s, CastMessageType msg)
    : name(n), effect(e), difficulty(diff), castMessageType(msg), sound(s) {
}

Spell::Spell(const string& n, DirEffectType e, int diff, SoundId s, CastMessageType msg)
    : name(n), effect(e), difficulty(diff), castMessageType(msg), sound(s) {
}

SoundId Spell::getSound() const {
  return sound;
}

string Spell::getDescription() const {
  if (isDirected())
    return Effect::getDescription(boost::get<DirEffectType>(*effect));
  else
    return Effect::getDescription(boost::get<EffectType>(*effect));
}

void Spell::addMessage(Creature* c) {
  switch (castMessageType) {
    case CastMessageType::STANDARD:
      c->playerMessage("You cast " + getName());
      c->monsterMessage(c->getName().the() + " casts a spell");
      break;
    case CastMessageType::AIR_BLAST:
      c->playerMessage("You cause an air blast!");
      c->monsterMessage(c->getName().the() + " causes an air blast!");
      break;
  }
}

void Spell::init() {
  set(SpellId::HEALING, new Spell("healing", EffectId::HEAL, 30, SoundId::SPELL_HEALING));
  set(SpellId::SUMMON_INSECTS, new Spell("summon insects", EffectType(EffectId::SUMMON, CreatureId::FLY), 30,
        SoundId::SPELL_SUMMON_INSECTS));
  set(SpellId::DECEPTION, new Spell("deception", EffectId::DECEPTION, 60, SoundId::SPELL_DECEPTION));
  set(SpellId::SPEED_SELF, new Spell("haste self", {EffectId::LASTING, LastingEffect::SPEED}, 60,
        SoundId::SPELL_SPEED_SELF));
  set(SpellId::STR_BONUS, new Spell("strength", {EffectId::LASTING, LastingEffect::STR_BONUS}, 90,
        SoundId::SPELL_STR_BONUS));
  set(SpellId::DEX_BONUS, new Spell("dexterity", {EffectId::LASTING, LastingEffect::DEX_BONUS}, 90,
        SoundId::SPELL_DEX_BONUS));
  set(SpellId::BLAST, new Spell("force bolt", DirEffectId::BLAST, 100, SoundId::SPELL_BLAST));
  set(SpellId::STUN_RAY, new Spell("stun ray",  DirEffectType(DirEffectId::CREATURE_EFFECT,
        EffectType(EffectId::LASTING, LastingEffect::STUNNED)) , 60, SoundId::SPELL_STUN_RAY));
  set(SpellId::MAGIC_SHIELD, new Spell("magic shield", {EffectId::LASTING, LastingEffect::MAGIC_SHIELD}, 100,
        SoundId::SPELL_MAGIC_SHIELD));
  set(SpellId::FIRE_SPHERE_PET, new Spell("fire sphere", EffectType(EffectId::SUMMON, CreatureId::FIRE_SPHERE), 20,
        SoundId::SPELL_FIRE_SPHERE_PET));
  set(SpellId::TELEPORT, new Spell("escape", EffectId::TELEPORT, 80, SoundId::SPELL_TELEPORT));
  set(SpellId::INVISIBILITY, new Spell("invisibility", {EffectId::LASTING, LastingEffect::INVISIBLE}, 150,
        SoundId::SPELL_INVISIBILITY));
  set(SpellId::WORD_OF_POWER, new Spell("word of power", EffectId::WORD_OF_POWER, 150,
        SoundId::SPELL_WORD_OF_POWER));
  set(SpellId::AIR_BLAST, new Spell("air blast", EffectId::AIR_BLAST, 150, SoundId::SPELL_AIR_BLAST,
        CastMessageType::AIR_BLAST));
  set(SpellId::SUMMON_SPIRIT, new Spell("summon spirits", EffectType(EffectId::SUMMON, CreatureId::SPIRIT), 150,
        SoundId::SPELL_SUMMON_SPIRIT));
  set(SpellId::PORTAL, new Spell("portal", EffectId::PORTAL, 150, SoundId::SPELL_PORTAL));
  set(SpellId::CURE_POISON, new Spell("cure poisoning", EffectId::CURE_POISON, 150, SoundId::SPELL_CURE_POISON));
  set(SpellId::METEOR_SHOWER, new Spell("meteor shower", EffectId::METEOR_SHOWER, 150,
        SoundId::SPELL_METEOR_SHOWER));
}
