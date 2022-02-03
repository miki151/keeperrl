#include "creature_predicate.h"
#include "pretty_archive.h"
#include "creature.h"
#include "creature_attributes.h"
#include "equipment.h"
#include "item.h"
#include "body.h"
#include "game.h"
#include "unlocks.h"
#include "collective.h"
#include "spell_map.h"
#include "level.h"
#include "sectors.h"
#include "movement_type.h"

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
  return victim->isAutomaton();
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

static bool applyToCreature(CreatureId id, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getCreatureId() == id;
}

static string getName(CreatureId s) {
  return toLower(s.data());
}

static bool apply(const CreaturePredicates::Flag& s, Position pos, const Creature* attacker) {
  return pos.getGame()->effectFlags.count(s.name);
}

static string getName(const CreaturePredicates::Flag& s) {
  return s.name;
}

static bool applyToCreature(const CreaturePredicates::CreatureFlag& s, const Creature* victim, const Creature*) {
  return victim->effectFlags.count(s.name);
}

static string getName(const CreaturePredicates::CreatureFlag& s) {
  return s.name;
}

static bool applyToCreature(const Gender& s, const Creature* victim, const Creature*) {
  return victim->getAttributes().getGender() == s;
}

static string getName(const Gender& s) {
  return get(s, "males", "females", "genderless");
}

static bool applyToCreature(const CreaturePredicates::Rider& s, const Creature* victim, const Creature*) {
  return victim->getSteed();
}

static string getName(const CreaturePredicates::Rider& s) {
  return "riders";
}

static bool applyToCreature(const CreaturePredicates::Kills& s, const Creature* victim, const Creature*) {
  return victim->getKills().size() >= s.cnt;
}

static string getName(const CreaturePredicates::Kills& s) {
  return "those with a minimum of " + toString(s.cnt) + " kills";
}

static bool apply(const CreaturePredicates::Unlocked& s, Position pos, const Creature* attacker) {
  return pos.getGame()->getUnlocks()->isUnlocked(s.id);
}

static string getName(const CreaturePredicates::Unlocked& s) {
  return s.id;
}

static bool apply(const CreaturePredicates::InTerritory& s, Position pos, const Creature* attacker) {
  auto col = pos.getCollective();
  return attacker && col && col->getCreatures().contains(attacker);
}

static string getNameTopLevel(const CreaturePredicates::InTerritory&) {
  return "when in territory";
}

static string getName(const CreaturePredicates::InTerritory&) {
  return "in territory";
}

static bool apply(FurnitureType type, Position pos, const Creature* attacker) {
  return !!pos.getFurniture(type);
}

static string getName(FurnitureType) {
  return "furniture";
}

static bool applyToCreature(BodyMaterial m, const Creature* victim, const Creature* attacker) {
  return victim->getBody().getMaterial() == m;
}

static string getName(BodyMaterial m) {
  return "made of "_s + getMaterialName(m);
}

static bool applyToCreature(CreaturePredicates::Spellcaster m, const Creature* victim, const Creature* attacker) {
  for (auto spell : victim->getSpellMap().getAvailable(victim))
    if (spell->getType() == SpellType::SPELL)
      return true;
  return false;
}

static string getName(CreaturePredicates::Spellcaster) {
  return "spellcasters";
}

static bool applyToCreature(CreaturePredicates::Humanoid, const Creature* victim, const Creature*) {
  return victim->getBody().isHumanoid();
}

static string getName(CreaturePredicates::Humanoid) {
  return "humanoids";
}

static string getNameNegated(CreaturePredicates::Humanoid) {
  return "non-humanoids";
}

static Collective* getCollective(const Creature* c) {
  auto game = NOTNULL(c->getGame());
  for (auto col : game->getCollectives())
    if (col->getCreatures().contains(c))
      return col;
  return nullptr;
}

static bool apply(CreaturePredicates::IsClosedOffPigsty, Position position, const Creature*) {
  return position.isClosedOff(MovementType(MovementTrait::WALK).setFarmAnimal());
}

static string getName(const CreaturePredicates::IsClosedOffPigsty) {
  return "inside an animal pen";
}

static bool apply(CreaturePredicates::CanCreatureEnter, Position position, const Creature*) {
  return position.canEnter({MovementTrait::WALK});
}

static string getName(const CreaturePredicates::CanCreatureEnter) {
  return "can enter position";
}

static bool apply(CreaturePredicates::Frequency f, Position position, const Creature*) {
  return position.getGame()->getGlobalTime().getVisibleInt() % f.value == 0;
}

static string getName(const CreaturePredicates::Frequency f) {
  return "with frequency " + toString(f.value);
}

static bool applyToCreature(CreaturePredicates::PopLimitReached, const Creature* victim, const Creature*) {
  if (auto col = getCollective(victim))
    return col->getMaxPopulation() <= col->getPopulationSize();
  return false;
}

static string getNameTopLevel(const CreaturePredicates::PopLimitReached) {
  return "when population limit is reached";
}

static string getName(const CreaturePredicates::PopLimitReached) {
  return "population limit is reached";
}

static bool applyToCreature(const CreaturePredicates::Health& p, const Creature* victim, const Creature* attacker) {
  auto health = victim->getBody().hasAnyHealth()
      ? victim->getBody().getHealth()
      : victim->getBody().getBodyPartHealth();
  return health >= p.from && health <= p.to;
}

static string getName(const CreaturePredicates::Health& p) {
  return "with health between "_s + toString(p.from) + " and " + toString(p.to);
}

static bool apply(CreaturePredicates::Night m, Position pos, const Creature*) {
  return pos.getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
}

static string getName(CreaturePredicates::Night m) {
  return "at night";
}

static string getNameNegated(CreaturePredicates::Night m) {
  return "during the day";
}

static string getName(const CreaturePredicates::Distance& p) {
  auto ret = "distance"_s;
  if (p.min)
    ret += " from " + toString(*p.min);
  if (p.max)
    ret += " up to " + toString(*p.max);
  return ret;
}

static bool apply(const CreaturePredicates::Distance& e, Position pos, const Creature* attacker) {
  auto dist = attacker->getPosition().dist8(pos).value_or(10000);
  return dist >= e.min.value_or(-1) && dist < e.max.value_or(10000);
}

static string getName(const CreaturePredicates::DistanceD& p) {
  auto ret = "distance"_s;
  if (p.min)
    ret += " from " + toString(*p.min);
  if (p.max)
    ret += " up to " + toString(*p.max);
  return ret;
}

static bool apply(const CreaturePredicates::DistanceD& e, Position pos, const Creature* attacker) {
  if (!attacker->getPosition().isSameLevel(pos))
    return false;
  auto dist = attacker->getPosition().getCoord().distD(pos.getCoord());
  return dist >= e.min.value_or(-1) && dist <= e.max.value_or(10000);
}

static string getName(const CreaturePredicates::AIAfraidOf&) {
  return "AI afraid of";
}

static bool applyToCreature(const CreaturePredicates::AIAfraidOf& e, const Creature* victim, const Creature* attacker) {
  return !!attacker && !attacker->shouldAIAttack(victim);
}

static bool apply(CreaturePredicates::Indoors m, Position pos, const Creature*) {
  return pos.isCovered();
}

static string getName(CreaturePredicates::Indoors m) {
  return "when indoors";
}

static string getNameNegated(CreaturePredicates::Indoors m) {
  return "when outdoors";
}

static bool apply(const CreaturePredicates::Translate& m, Position pos, const Creature* attacker) {
  return m.pred->apply(pos.plus(m.dir), attacker);
}

static string getName(const CreaturePredicates::Translate& m) {
  return m.pred->getNameInternal();
}

static bool apply(const CreaturePredicates::Area& m, Position pos, const Creature* attacker) {
  for (auto v : pos.getRectangle(Rectangle::centered(m.radius)))
    if (!m.pred->apply(v, attacker))
      return false;
  return true;
}

static string getName(const CreaturePredicates::Area& m) {
  return m.pred->getNameInternal();
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

bool CreaturePredicate::apply(Creature* c, const Creature* attacker) const {
  return apply(c->getPosition(), attacker);
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
