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
#include "fx_name.h"
#include "keybinding.h"

template <class Archive>
void Spell::serializeImpl(Archive& ar, const unsigned int) {
  ar(NAMED(upgrade), NAMED(symbol), NAMED(effect), NAMED(cooldown), OPTION(message), NAMED(sound), OPTION(range), NAMED(fx), OPTION(endOnly), OPTION(targetSelf), OPTION(blockedByWall), NAMED(projectileViewId), NAMED(maxHits), NAMED(keybinding), OPTION(type));
}

template <class Archive>
void Spell::serialize(Archive& ar, const unsigned int v) {
  ar(id);
  serializeImpl(ar, v);
}

SERIALIZABLE(Spell)
SERIALIZATION_CONSTRUCTOR_IMPL(Spell)
STRUCT_IMPL(Spell)

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

vector<string> Spell::getDescription(const ContentFactory* f) const {
  vector<string> description = {effect->getDescription(f), "Cooldown: " + toString(getCooldown())};
  if (getRange() > 0)
    description.push_back("Range: " + toString(getRange()));
  return description;
}

void Spell::addMessage(Creature* c) const {
  if (!message.first.empty())
    c->verb(message.first, message.second);
}

void Spell::apply(Creature* c, Position target) const {
  if (target == c->getPosition()) {
    if (canTargetSelf())
      effect->apply(target, c);
    return;
  }
  auto thisFx = effect->getProjectileFX();
  if (fx)
    thisFx = *fx;
  auto thisProjectile = effect->getProjectile();
  if (projectileViewId) {
    thisProjectile = projectileViewId;
    thisFx = none;
  }
  if (endOnly) {
    c->getGame()->addEvent(
        EventInfo::Projectile{std::move(thisFx), thisProjectile, c->getPosition(), target, none});
    effect->apply(target, c);
    return;
  }
  vector<Position> trajectory;
  auto origin = c->getPosition().getCoord();
  for (auto& v : drawLine(origin, target.getCoord()))
    if (v != origin && v.dist8(origin) <= range) {
      trajectory.push_back(Position(v, target.getLevel()));
      if (isBlockedBy(trajectory.back()))
        break;
    }
  int remainingHits = maxHits.value_or(10000);
  auto lastPos = trajectory.front();
  for (auto& pos : trajectory) {
    lastPos = pos;
    if (effect->apply(pos, c))
      --remainingHits;
    if (remainingHits <= 0)
      break;
  }
  c->getGame()->addEvent(
      EventInfo::Projectile{std::move(thisFx), thisProjectile, c->getPosition(), lastPos, none});
}

int Spell::getRange() const {
  return range;
}

bool Spell::isEndOnly() const {
  return endOnly;
}

void Spell::setSpellId(SpellId i) {
  id = i;
}

SpellId Spell::getId() const {
  return id;
}

string Spell::getName(const ContentFactory* f) const {
  return effect->getName(f);
}

optional<SpellId> Spell::getUpgrade() const {
  return upgrade;
}

bool Spell::isFriendlyFire(const Creature* c, Position to) const {
  PROFILE;
  return checkTrajectory(c, to) >= 0;
}

int Spell::checkTrajectory(const Creature* c, Position to) const {
  PROFILE;
  if (endOnly || to == c->getPosition())
    return effect->shouldAIApply(c, to);
  int ret = 0;
  Position from = c->getPosition();
  for (auto& v : drawLine(from, to))
    if (v != from) {
      if (isBlockedBy(v))
        return -1;
      auto value = effect->shouldAIApply(c, v);
      if (value < 0)
        return value;
      ret += value;
    }
  return ret;
}

void Spell::getAIMove(const Creature* c, MoveInfo& ret) const {
  PROFILE;
  auto tryMove = [&ret] (int value, CreatureAction action) {
    if (value > ret.getValue())
      ret = MoveInfo(value, std::move(action));
  };
  if (c->isReady(this))
    for (auto pos : c->getPosition().getRectangle(Rectangle::centered(range))) {
      auto value = checkTrajectory(c, pos);
      if (value > 0 && ((pos == c->getPosition() && canTargetSelf()) || (c->canSee(pos) && pos != c->getPosition())))
        tryMove(value, c->castSpell(this, pos));
    }
}

bool Spell::isBlockedBy(Position pos) const {
  return blockedByWall && pos.isDirEffectBlocked();
}

optional<Keybinding> Spell::getKeybinding() const {
  return keybinding;
}

SpellType Spell::getType() const {
  return type;
}

#include "pretty_archive.h"
template <>
void Spell::serialize(PrettyInputArchive& ar1, unsigned v) {
  serializeImpl(ar1, v);
}

