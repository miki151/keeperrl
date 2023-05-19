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
#include "content_factory.h"
#include "tile_gas_info.h"

namespace Impl {
static bool applyToCreature(const CreaturePredicates::Enemy&, const Creature* victim, const Creature* attacker) {
  return !!attacker && victim->isEnemy(attacker);
}

static string getName(const CreaturePredicates::Enemy&, const ContentFactory*) {
  return "enemies";
}

static string getNameNegated(const CreaturePredicates::Enemy&, const ContentFactory*) {
  return "allies";
}

static bool applyToCreature(const CreaturePredicates::SameTribe&, const Creature* victim, const Creature* attacker) {
  return !!attacker && victim->getTribeId() == attacker->getTribeId();
}

static string getName(const CreaturePredicates::SameTribe&, const ContentFactory*) {
  return "own tribe";
}

static bool applyToCreature(const CreaturePredicates::Automaton&, const Creature* victim, const Creature* attacker) {
  return victim->isAutomaton();
}

static string getName(const CreaturePredicates::Automaton&, const ContentFactory*) {
  return "automatons";
}

static string getNameNegated(const CreaturePredicates::Automaton&, const ContentFactory*) {
  return "non-automatons";
}

static bool applyToCreature(const CreaturePredicates::Hidden&, const Creature* victim, const Creature* attacker) {
  return victim->isHidden() && !victim->knowsHiding(attacker);
}

static string getName(const CreaturePredicates::Hidden&, const ContentFactory*) {
  return "hidden";
}

static bool apply(const CreaturePredicates::Name& e, Position pos, const Creature* attacker) {
  return e.pred->apply(pos, attacker);
}

static string getNameTopLevel(const CreaturePredicates::Name& e, const ContentFactory*) {
  return e.name;
}

static string getName(const CreaturePredicates::Name& e, const ContentFactory*) {
  return e.name;
}

static bool applyToCreature(const CreaturePredicates::HatedBy& p, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getHatedByEffect() == p.effect;
}

static string getName(const CreaturePredicates::HatedBy& p, const ContentFactory* f) {
  return *f->buffs.at(p.effect).hatedGroupName;
}

static bool apply(const CreaturePredicates::Attacker& p, Position pos, const Creature* attacker) {
  return !!attacker && p.pred->apply(attacker->getPosition(), attacker);
}

static string getName(const CreaturePredicates::Attacker& p, const ContentFactory* f) {
  return p.pred->getNameInternal(f);
}

static bool apply(const CreaturePredicates::Not& p, Position pos, const Creature* attacker) {
  return !p.pred->apply(pos, attacker);
}

static string getName(const CreaturePredicates::Not& p, const ContentFactory* f) {
  return p.pred->getNameInternal(f, true);
}

static string getNameNegated(const CreaturePredicates::Not& p, const ContentFactory* f) {
  return p.pred->getNameInternal(f);
}

static bool applyToCreature(const CreaturePredicates::Ingredient& e, const Creature* victim, const Creature* attacker) {
  for (auto& item : victim->getEquipment().getItems(ItemIndex::RUNE))
    if (item->getIngredientType() == e.name)
      return true;
  return false;
}

static string getName(const CreaturePredicates::Ingredient& e, const ContentFactory*) {
  return "holding " + e.name;
}

static bool apply(const CreaturePredicates::OnTheGround& e, Position pos, const Creature*) {
  for (auto& item : pos.getItems(ItemIndex::RUNE))
    if (item->getIngredientType() == e.name)
      return true;
  return false;
}

static string getName(const CreaturePredicates::OnTheGround& e, const ContentFactory*) {
  return "on the ground " + e.name;
}

static bool applyToCreature(LastingOrBuff e, const Creature* victim, const Creature* attacker) {
  return isAffected(victim, e);
}

static string getName(LastingOrBuff e, const ContentFactory* f) {
  return "affected by " + ::getName(e, f);
}

static bool applyToCreature(CreatureStatus s, const Creature* victim, const Creature* attacker) {
  return victim->getStatus().contains(s);
}

static string getName(CreatureStatus s, const ContentFactory*) {
  return toLower(::getName(s)) + "s";
}

static bool applyToCreature(CreatureId id, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getCreatureId() == id;
}

static string getName(CreatureId s, const ContentFactory*) {
  return toLower(s.data());
}

static bool apply(const CreaturePredicates::Flag& s, Position pos, const Creature* attacker) {
  return pos.getGame()->effectFlags.count(s.name);
}

static string getName(const CreaturePredicates::Flag& s, const ContentFactory*) {
  return s.name;
}

static bool applyToCreature(const CreaturePredicates::CreatureFlag& s, const Creature* victim, const Creature*) {
  return victim->effectFlags.count(s.name);
}

static string getName(const CreaturePredicates::CreatureFlag& s, const ContentFactory*) {
  return s.name;
}

static bool applyToCreature(const Gender& s, const Creature* victim, const Creature*) {
  return victim->getAttributes().getGender() == s;
}

static string getName(const Gender& s, const ContentFactory*) {
  return get(s, "males", "females", "genderless");
}

static bool applyToCreature(const CreaturePredicates::Rider& s, const Creature* victim, const Creature*) {
  return victim->getSteed();
}

static string getName(const CreaturePredicates::Rider& s, const ContentFactory*) {
  return "riders";
}

static bool applyToCreature(const CreaturePredicates::Kills& s, const Creature* victim, const Creature*) {
  return victim->getKills().size() >= s.cnt;
}

static string getName(const CreaturePredicates::Kills& s, const ContentFactory*) {
  return "those with a minimum of " + toString(s.cnt) + " kills";
}

static bool apply(const CreaturePredicates::Unlocked& s, Position pos, const Creature* attacker) {
  return pos.getGame()->getUnlocks()->isUnlocked(s.id);
}

static string getName(const CreaturePredicates::Unlocked& s, const ContentFactory*) {
  return s.id;
}

static bool apply(const CreaturePredicates::InTerritory& s, Position pos, const Creature* attacker) {
  auto col = pos.getCollective();
  return attacker && col && col->getCreatures().contains(attacker);
}

static string getNameTopLevel(const CreaturePredicates::InTerritory&, const ContentFactory*) {
  return "when in territory";
}

static string getName(const CreaturePredicates::InTerritory&, const ContentFactory*) {
  return "in territory";
}

static bool apply(FurnitureType type, Position pos, const Creature* attacker) {
  return !!pos.getFurniture(type);
}

static string getName(FurnitureType, const ContentFactory*) {
  return "furniture";
}

static bool apply(CreaturePredicates::ContainsGas type, Position pos, const Creature* attacker) {
  return pos.getGasAmount(type) > 0;
}

static string getName(CreaturePredicates::ContainsGas type, const ContentFactory* f) {
  return "in " + f->tileGasTypes.at(type).name;
}

static bool applyToCreature(BodyMaterialId m, const Creature* victim, const Creature* attacker) {
  return victim->getBody().getMaterial() == m;
}

static string getName(BodyMaterialId m, const ContentFactory* factory) {
  return "made of "_s + factory->bodyMaterials.at(m).name;
}

static bool applyToCreature(CreaturePredicates::Spellcaster m, const Creature* victim, const Creature* attacker) {
  for (auto spell : victim->getSpellMap().getAvailable(victim))
    if (spell->getType() == SpellType::SPELL)
      return true;
  return false;
}

static string getName(CreaturePredicates::Spellcaster, const ContentFactory*) {
  return "spellcasters";
}

static bool applyToCreature(CreaturePredicates::Humanoid, const Creature* victim, const Creature*) {
  return victim->getBody().isHumanoid();
}

static string getName(CreaturePredicates::Humanoid, const ContentFactory*) {
  return "humanoids";
}

static string getNameNegated(CreaturePredicates::Humanoid, const ContentFactory*) {
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

static string getName(const CreaturePredicates::IsClosedOffPigsty, const ContentFactory*) {
  return "inside an animal pen";
}

static bool apply(CreaturePredicates::CanCreatureEnter, Position position, const Creature*) {
  return position.canEnter({MovementTrait::WALK});
}

static string getName(const CreaturePredicates::CanCreatureEnter, const ContentFactory*) {
  return "can enter position";
}

static bool apply(CreaturePredicates::Frequency f, Position position, const Creature*) {
  return position.getGame()->getGlobalTime().getVisibleInt() % f.value == 0;
}

static string getName(const CreaturePredicates::Frequency f, const ContentFactory*) {
  return "with frequency " + toString(f.value);
}

static bool applyToCreature(CreaturePredicates::PopLimitReached, const Creature* victim, const Creature*) {
  if (auto col = getCollective(victim))
    return col->getMaxPopulation() <= col->getPopulationSize();
  return false;
}

static string getNameTopLevel(const CreaturePredicates::PopLimitReached, const ContentFactory*) {
  return "when population limit is reached";
}

static string getName(const CreaturePredicates::PopLimitReached, const ContentFactory*) {
  return "population limit is reached";
}

static bool applyToCreature(const CreaturePredicates::Health& p, const Creature* victim, const Creature* attacker) {
  auto health = victim->getBody().hasAnyHealth(victim->getGame()->getContentFactory())
      ? victim->getBody().getHealth()
      : victim->getBody().getBodyPartHealth();
  return health >= p.from && health <= p.to;
}

static string getName(const CreaturePredicates::Health& p, const ContentFactory*) {
  return "with health between "_s + toString(p.from) + " and " + toString(p.to);
}

static bool apply(CreaturePredicates::Night m, Position pos, const Creature*) {
  return pos.getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
}

static string getName(CreaturePredicates::Night m, const ContentFactory*) {
  return "at night";
}

static string getNameNegated(CreaturePredicates::Night m, const ContentFactory*) {
  return "during the day";
}

static string getName(const CreaturePredicates::Distance& p, const ContentFactory*) {
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

static string getName(const CreaturePredicates::DistanceD& p, const ContentFactory*) {
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

static string getName(const CreaturePredicates::AIAfraidOf&, const ContentFactory*) {
  return "AI afraid of";
}

static bool applyToCreature(const CreaturePredicates::AIAfraidOf& e, const Creature* victim, const Creature* attacker) {
  return !!attacker && !attacker->shouldAIAttack(victim);
}

static bool apply(CreaturePredicates::Indoors m, Position pos, const Creature*) {
  return pos.isCovered();
}

static string getName(CreaturePredicates::Indoors m, const ContentFactory*) {
  return "when indoors";
}

static string getNameNegated(CreaturePredicates::Indoors m, const ContentFactory*) {
  return "when outdoors";
}

static bool applyToCreature(const CreaturePredicates::AttributeAtLeast& a, const Creature* victim, const Creature*) {
  PROFILE;
  return victim->getAttr(a.attr) >= a.value;
}

static string getName(const CreaturePredicates::AttributeAtLeast& a, const ContentFactory* f) {
  return "at least " + toString(a.value) + " " + f->attrInfo.at(a.attr).name;
}

static bool apply(const CreaturePredicates::Translate& m, Position pos, const Creature* attacker) {
  return m.pred->apply(pos.plus(m.dir), attacker);
}

static string getName(const CreaturePredicates::Translate& m, const ContentFactory* f) {
  return m.pred->getNameInternal(f);
}

static bool apply(const CreaturePredicates::Area& m, Position pos, const Creature* attacker) {
  for (auto v : pos.getRectangle(Rectangle::centered(m.radius)))
    if (!m.pred->apply(v, attacker))
      return false;
  return true;
}

static string getName(const CreaturePredicates::Area& m, const ContentFactory* f) {
  return m.pred->getNameInternal(f);
}

static bool apply(const CreaturePredicates::And& p, Position pos, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (!pred.apply(pos, attacker))
      return false;
  return true;
}

static string getName(const CreaturePredicates::And& p, const ContentFactory* f) {
  return combine(p.pred.transform([f] (const auto& pred) { return pred.getNameInternal(f); }));
}

static bool apply(const CreaturePredicates::Or& p, Position pos, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (pred.apply(pos, attacker))
      return true;
  return false;
}

static string getName(const CreaturePredicates::Or& p, const ContentFactory* f) {
  return combine(p.pred.transform([&] (const auto& pred) { return pred.getNameInternal(f); }), " or "_s);
}

template <typename T>
static string getNameNegated(const T& p, const ContentFactory* f) {
  return "not " + Impl::getName(p, f);
}

template <typename T>
static string getNameTopLevel(const T& p, const ContentFactory* f) {
  return "against " + Impl::getName(p, f);
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

string CreaturePredicate::getName(const ContentFactory* f) const {
  return visit<string>([&](const auto& p) { return Impl::getNameTopLevel(p, f); });
}

string CreaturePredicate::getNameInternal(const ContentFactory* f, bool negated) const {
  if (negated)
    return visit<string>([&](const auto& p) { return Impl::getNameNegated(p, f); });
  else
    return visit<string>([&](const auto& p) { return Impl::getName(p, f); });
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
