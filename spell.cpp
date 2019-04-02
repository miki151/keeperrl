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

SERIALIZE_DEF(Spell, NAMED(symbol), NAMED(effect), NAMED(cooldown), OPTION(castMessageType), NAMED(sound))
SERIALIZATION_CONSTRUCTOR_IMPL(Spell)

const string& Spell::getSymbol() const {
  return symbol;
}

bool Spell::isDirected() const {
  return effect->contains<DirEffectType>();
}

bool Spell::hasEffect(const Effect& t) const {
  return effect->getReferenceMaybe<Effect>() == t;
}

bool Spell::hasEffect(const DirEffectType& t) const {
  return effect->getReferenceMaybe<DirEffectType>() == t;
}

const Effect& Spell::getEffect() const {
  return *effect->getReferenceMaybe<Effect>();
}

const DirEffectType& Spell::getDirEffectType() const{
  return *effect->getReferenceMaybe<DirEffectType>();
}

const Spell::SpellVariant& Spell::getVariant() const {
  return *effect;
}

int Spell::getCooldown() const {
  return cooldown;
}

optional<SoundId> Spell::getSound() const {
  return sound;
}

string Spell::getDescription() const {
  return effect->visit(
      [](const Effect& e) { return e.getDescription(); },
      [](const DirEffectType& e) { return ::getDescription(e); }
  );
}

void Spell::addMessage(Creature* c) const {
  switch (castMessageType) {
    case CastMessageType::STANDARD:
      c->verb("cast", "casts", "a spell");
      break;
    case CastMessageType::AIR_BLAST:
      c->verb("create", "creates", "an air blast!");
      break;
    case CastMessageType::BREATHE_FIRE:
      c->verb("breathe", "breathes", "fire!");
      break;
  }
}

#include "pretty_archive.h"
template void Spell::serialize(PrettyInputArchive&, unsigned);
