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
#include "draw_line.h"
#include "game.h"
#include "game_event.h"
#include "view_id.h"
#include "move_info.h"

SERIALIZE_DEF(Spell, NAMED(id), NAMED(upgrade), NAMED(symbol), NAMED(effect), NAMED(cooldown), OPTION(castMessageType), NAMED(sound), OPTION(range), NAMED(fx), OPTION(endOnly), OPTION(targetSelf))
SERIALIZATION_CONSTRUCTOR_IMPL(Spell)

const string& Spell::getSymbol() const {
  return symbol;
}

const Effect& Spell::getEffect() const {
  return *effect;
}

int Spell::getCooldown() const {
  return cooldown;
}

optional<SoundId> Spell::getSound() const {
  return sound;
}

bool Spell::canTargetSelf() const {
  return targetSelf || range == 0;
}

string Spell::getDescription() const {
  return effect->getDescription();
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
    case CastMessageType::ABILITY:
      c->verb("use", "uses", "an ability");
      break;
  }
}

static optional<FXInfo> getProjectileFX(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}

static optional<FXInfo> getProjectileFX(const Effect& effect) {
  return effect.visit(
      [&](const auto&) -> optional<FXInfo> { return none; },
      [&](const Effect::Lasting& e) -> optional<FXInfo> { return getProjectileFX(e.lastingEffect); },
      [&](const Effect::Damage&) -> optional<FXInfo> { return {FXName::MAGIC_MISSILE}; },
      [&](const Effect::Blast&) -> optional<FXInfo> { return {FXName::AIR_BLAST}; }
  );
}

static optional<ViewId> getProjectile(LastingEffect effect) {
  switch (effect) {
    default:
      return none;
  }
}

static optional<ViewId> getProjectile(const Effect& effect) {
  return effect.visit(
      [&](const auto&) -> optional<ViewId> { return none; },
      [&](const Effect::Lasting& e) -> optional<ViewId> { return getProjectile(e.lastingEffect); },
      [&](const Effect::Damage&) -> optional<ViewId> { return ViewId("force_bolt"); },
      [&](const Effect::Fire&) -> optional<ViewId> { return ViewId("fireball"); },
      [&](const Effect::Blast&) -> optional<ViewId> { return ViewId("air_blast"); }
  );
}

void Spell::apply(Creature* c, Position target) const {
  if (target == c->getPosition()) {
    if (canTargetSelf())
      effect->apply(target, c);
    return;
  }
  auto thisFx = getProjectileFX(*effect);
  if (fx)
    thisFx = FXInfo{*fx};
  c->getGame()->addEvent(
      EventInfo::Projectile{std::move(thisFx), getProjectile(*effect), c->getPosition(), target, none});
  if (endOnly) {
    effect->apply(target, c);
    return;
  }
  vector<Position> trajectory;
  auto origin = c->getPosition().getCoord();
  for (auto& v : drawLine(origin, target.getCoord()))
    if (v != origin && v.dist8(origin) <= range) {
      trajectory.push_back(Position(v, target.getLevel()));
      if (trajectory.back().isDirEffectBlocked())
        break;
    }
  for (auto& pos : trajectory)
    effect->apply(pos, c);
}

int Spell::getRange() const {
  return range;
}

const string& Spell::getId() const {
  return id;
}

const optional<string>& Spell::getUpgrade() const {
  return upgrade;
}

MoveInfo Spell::getAIMove(const Creature* c) const {
  for (auto pos : c->getPosition().getRectangle(Rectangle::centered(range)))
    if (pos != c->getPosition() || targetSelf)
      if (effect->shouldAIApply(c, pos) == EffectAIIntent::WANTED)
        return c->castSpell(this, pos);
  return NoMove;
}


#include "pretty_archive.h"
template void Spell::serialize(PrettyInputArchive&, unsigned);
