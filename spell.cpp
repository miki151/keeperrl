#include "stdafx.h"
#include "spell.h"
#include "effect.h"

const string& Spell::getName() const {
  return name;
}

bool Spell::isDirected() const {
  return boost::get<DirEffectType>(&effect);
}

bool Spell::hasEffect(EffectType t) const {
  return !isDirected() && boost::get<EffectType>(effect) == t;
}

bool Spell::hasEffect(DirEffectType t) const {
  return isDirected() && boost::get<DirEffectType>(effect) == t;
}

EffectType Spell::getEffectType() const {
  CHECK(!isDirected());
  return boost::get<EffectType>(effect);
}

DirEffectType Spell::getDirEffectType() const{
  CHECK(isDirected());
  return boost::get<DirEffectType>(effect);
}

int Spell::getDifficulty() const {
  return difficulty;
}

Spell::Spell(const string& n, EffectType e, int diff) : name(n), effect(e), difficulty(diff) {
}

Spell::Spell(const string& n, DirEffectType e, int diff) : name(n), effect(e), difficulty(diff) {
}

string Spell::getDescription() const {
  if (isDirected())
    return Effect::getDescription(boost::get<DirEffectType>(effect));
  else
    return Effect::getDescription(boost::get<EffectType>(effect));
}

void Spell::init() {
  set(SpellId::HEALING, new Spell("healing", EffectId::HEAL, 30));
  set(SpellId::SUMMON_INSECTS, new Spell("summon insects", EffectId::SUMMON_INSECTS, 30));
  set(SpellId::DECEPTION, new Spell("deception", EffectId::DECEPTION, 60));
  set(SpellId::SPEED_SELF, new Spell("haste self", {EffectId::LASTING, LastingEffect::SPEED}, 60));
  set(SpellId::STR_BONUS, new Spell("strength", {EffectId::LASTING, LastingEffect::STR_BONUS}, 90));
  set(SpellId::DEX_BONUS, new Spell("dexterity", {EffectId::LASTING, LastingEffect::DEX_BONUS}, 90));
  set(SpellId::BLAST, new Spell("force bolt", DirEffectId::BLAST, 100));
  set(SpellId::STUN_RAY, new Spell("stun ray", 
        DirEffectType(DirEffectId::CREATURE_EFFECT, EffectType(EffectId::LASTING, LastingEffect::STUNNED)) , 60));
  set(SpellId::MAGIC_SHIELD, new Spell("magic shield", {EffectId::LASTING, LastingEffect::MAGIC_SHIELD}, 100));
  set(SpellId::FIRE_SPHERE_PET, new Spell("fire sphere", EffectId::FIRE_SPHERE_PET, 20));
  set(SpellId::TELEPORT, new Spell("escape", EffectId::TELEPORT, 80));
  set(SpellId::INVISIBILITY, new Spell("invisibility", {EffectId::LASTING, LastingEffect::INVISIBLE}, 150));
  set(SpellId::WORD_OF_POWER, new Spell("word of power", EffectId::WORD_OF_POWER, 150));
  set(SpellId::SUMMON_SPIRIT, new Spell("summon spirits", EffectId::SUMMON_SPIRIT, 150));
  set(SpellId::PORTAL, new Spell("portal", EffectId::PORTAL, 150));
  set(SpellId::CURE_POISON, new Spell("cure poisoning", EffectId::CURE_POISON, 150));
  set(SpellId::METEOR_SHOWER, new Spell("meteor shower", EffectId::METEOR_SHOWER, 150));
}
