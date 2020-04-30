#include "creature_predicate.h"
#include "pretty_archive.h"
#include "creature.h"
#include "creature_attributes.h"
#include "equipment.h"
#include "item.h"
#include "body.h"
#include "game.h"

namespace Impl {
static bool applyToCreature(const CreaturePredicates::Enemy&, const Creature* victim, const Creature* attacker) {
  return !!attacker && victim->isEnemy(attacker);
}

static string getName(const CreaturePredicates::Enemy&) {
  return "enemies";
}

static string getNameNegated(const CreaturePredicates::Enemy&) {
  return "allies";
}

static bool applyToCreature(const CreaturePredicates::Automaton&, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getAutomatonSlots().first > 0;
}

static string getName(const CreaturePredicates::Automaton&) {
  return "automatons";
}

static string getNameNegated(const CreaturePredicates::Automaton&) {
  return "non-automatons";
}

static bool applyToCreature(const CreaturePredicates::Hidden&, const Creature* victim, const Creature* attacker) {
  return victim->isHidden() && !victim->knowsHiding(attacker);
}

static string getName(const CreaturePredicates::Hidden&) {
  return "hidden";
}

static bool apply(const CreaturePredicates::Name& e, Position pos, const Creature* attacker) {
  return e.pred->apply(pos, attacker);
}

static string getNameTopLevel(const CreaturePredicates::Name& e) {
  return e.name;
}

static string getName(const CreaturePredicates::Name& e) {
  return e.name;
}

static bool applyToCreature(const CreaturePredicates::HatedBy& p, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getHatedByEffect() == p.effect;
}

static string getName(const CreaturePredicates::HatedBy& p) {
  return LastingEffects::getHatedGroupName(p.effect);
}

static bool apply(const CreaturePredicates::Attacker& p, Position pos, const Creature* attacker) {
  return !!attacker && p.pred->apply(attacker->getPosition(), attacker);
}

static string getName(const CreaturePredicates::Attacker& p) {
  return p.pred->getNameInternal();
}

static bool apply(const CreaturePredicates::Not& p, Position pos, const Creature* attacker) {
  return !p.pred->apply(pos, attacker);
}

static string getName(const CreaturePredicates::Not& p) {
  return p.pred->getNameInternal(true);
}

static string getNameNegated(const CreaturePredicates::Not& p) {
  return p.pred->getNameInternal();
}

static bool applyToCreature(const CreaturePredicates::Ingredient& e, const Creature* victim, const Creature* attacker) {
  for (auto& item : victim->getEquipment().getItems(ItemIndex::RUNE))
    if (item->getIngredientType() == e.name)
      return true;
  return false;
}

static string getName(const CreaturePredicates::Ingredient& e) {
  return "holding " + e.name;
}

static bool applyToCreature(LastingEffect e, const Creature* victim, const Creature* attacker) {
  return victim->isAffected(e);
}

static string getName(LastingEffect e) {
  return "affected by " + LastingEffects::getName(e);
}

static bool applyToCreature(CreatureStatus s, const Creature* victim, const Creature* attacker) {
  return victim->getStatus().contains(s);
}

static string getName(CreatureStatus s) {
  return toLower(::getName(s)) + "s";
}

static bool apply(const CreaturePredicates::Flag& s, Position pos, const Creature* attacker) {
  return pos.getGame()->effectFlags.count(s.name);
}

static string getName(const CreaturePredicates::Flag& s) {
  return s.name;
}

static bool applyToCreature(BodyMaterial m, const Creature* victim, const Creature* attacker) {
  return victim->getBody().getMaterial() == m;
}

static string getName(BodyMaterial m) {
  return "made of "_s + getMaterialName(m);
}

static bool apply(const CreaturePredicates::And& p, Position pos, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (!pred.apply(pos, attacker))
      return false;
  return true;
}

static string getName(const CreaturePredicates::And& p) {
  return combine(p.pred.transform([] (const auto& pred) { return pred.getNameInternal(); }));
}

static bool apply(const CreaturePredicates::Or& p, Position pos, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (pred.apply(pos, attacker))
      return true;
  return false;
}

static string getName(const CreaturePredicates::Or& p) {
  return combine(p.pred.transform([] (const auto& pred) { return pred.getNameInternal(); }), " or "_s);
}

template <typename T>
static string getNameNegated(const T& p) {
  return "not " + Impl::getName(p);
}

template <typename T>
static string getNameTopLevel(const T& p) {
  return "against " + Impl::getName(p);
}

template <typename T, REQUIRE(applyToCreature(TVALUE(const T&), TVALUE(const Creature*), TVALUE(const Creature*)))>
static bool apply(const T& t, Position pos, const Creature* attacker) {
  if (auto c = pos.getCreature())
    return applyToCreature(t, c, attacker);
  return false;
}

}


bool CreaturePredicate::apply(Position pos, const Creature* attacker) const {
  return visit<bool>([&](const auto& p) { return Impl::apply(p, pos, attacker); });
}

string CreaturePredicate::getName() const {
  return visit<string>([&](const auto& p) { return Impl::getNameTopLevel(p); });
}

string CreaturePredicate::getNameInternal(bool negated) const {
  if (negated)
    return visit<string>([&](const auto& p) { return Impl::getNameNegated(p); });
  else
    return visit<string>([&](const auto& p) { return Impl::getName(p); });
}

#define VARIANT_TYPES_LIST CREATURE_PREDICATE_LIST
#define VARIANT_NAME CreaturePredicate
namespace CreaturePredicates {
#include "gen_variant_serialize.h"
template<>
#include "gen_variant_serialize_pretty.h"
template void serialize(InputArchive&, CreaturePredicate&);
template void serialize(OutputArchive&, CreaturePredicate&);
}
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME
