#include "creature_predicate.h"
#include "pretty_archive.h"
#include "creature.h"
#include "creature_attributes.h"
#include "equipment.h"
#include "item.h"

namespace Impl {
static bool apply(const CreaturePredicates::Enemy&, const Creature* victim, const Creature* attacker) {
  return !!attacker && victim->isEnemy(attacker);
}

static string getName(const CreaturePredicates::Enemy&) {
  return "enemies";
}

static string getNameNegated(const CreaturePredicates::Enemy&) {
  return "allies";
}

static bool apply(const CreaturePredicates::Automaton&, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getAutomatonSlots().first > 0;
}

static string getName(const CreaturePredicates::Automaton&) {
  return "automatons";
}

static string getNameNegated(const CreaturePredicates::Automaton&) {
  return "non-automatons";
}

static bool apply(const CreaturePredicates::HatedBy& p, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getHatedByEffect() == p.effect;
}

static string getName(const CreaturePredicates::HatedBy& p) {
  return LastingEffects::getHatedGroupName(p.effect);
}

static bool apply(const CreaturePredicates::Attacker& p, const Creature* victim, const Creature* attacker) {
  return !!attacker && p.pred->apply(attacker, attacker);
}

static string getName(const CreaturePredicates::Attacker& p) {
  return p.pred->getName();
}

static bool apply(const CreaturePredicates::Not& p, const Creature* victim, const Creature* attacker) {
  return !p.pred->apply(victim, attacker);
}

static string getName(const CreaturePredicates::Not& p) {
  return p.pred->getName(true);
}

static string getNameNegated(const CreaturePredicates::Not& p) {
  return p.pred->getName();
}

static bool apply(const CreaturePredicates::Ingredient& e, const Creature* victim, const Creature* attacker) {
  for (auto& item : victim->getEquipment().getItems(ItemIndex::RUNE))
    if (item->getIngredientType() == e.name)
      return true;
  return false;
}

static string getName(const CreaturePredicates::Ingredient& e) {
  return "holding " + e.name;
}

static bool apply(LastingEffect e, const Creature* victim, const Creature* attacker) {
  return victim->isAffected(e);
}

static string getName(LastingEffect e) {
  return "affected by " + LastingEffects::getName(e);
}

static bool apply(CreatureStatus s, const Creature* victim, const Creature* attacker) {
  return victim->getStatus().contains(s);
}

static string getName(CreatureStatus s) {
  return toLower(::getName(s)) + "s";
}

static bool apply(const CreaturePredicates::And& p, const Creature* victim, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (!pred.apply(victim, attacker))
      return false;
  return true;
}

static string getName(const CreaturePredicates::And& p) {
  return combine(p.pred.transform([] (const auto& pred) { return pred.getName(); }));
}

static bool apply(const CreaturePredicates::Or& p, const Creature* victim, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (pred.apply(victim, attacker))
      return true;
  return false;
}

static string getName(const CreaturePredicates::Or& p) {
  return combine(p.pred.transform([] (const auto& pred) { return pred.getName(); }), " or "_s);
}

template <typename T>
static string getNameNegated(const T& p) {
  return "not " + Impl::getName(p);
}

}

bool CreaturePredicate::apply(const Creature* victim, const Creature* attacker) const {
  return visit<bool>([&](const auto& p) { return Impl::apply(p, victim, attacker); });
}

string CreaturePredicate::getName(bool negated) const {
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
