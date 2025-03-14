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

static TString getName(const CreaturePredicates::Enemy&, const ContentFactory*) {
  return TStringId("ENEMIES");
}

static TString getNameNegated(const CreaturePredicates::Enemy&, const ContentFactory*) {
  return TStringId("ALLIES");
}

static bool applyToCreature(const CreaturePredicates::SameTribe&, const Creature* victim, const Creature* attacker) {
  return !!attacker && victim->getTribeId() == attacker->getTribeId();
}

static TString getName(const CreaturePredicates::SameTribe&, const ContentFactory*) {
  return TStringId("OWN_TRIBE");
}

static bool applyToCreature(const CreaturePredicates::Automaton&, const Creature* victim, const Creature* attacker) {
  return victim->isAutomaton();
}

static TString getName(const CreaturePredicates::Automaton&, const ContentFactory*) {
  return TStringId("AUTOMATONS");
}

static TString getNameNegated(const CreaturePredicates::Automaton&, const ContentFactory*) {
  return TStringId("NON_AUTOMATONS");
}

static bool applyToCreature(const CreaturePredicates::Hidden&, const Creature* victim, const Creature* attacker) {
  return victim->isHidden() && !victim->knowsHiding(attacker);
}

static TString getName(const CreaturePredicates::Hidden&, const ContentFactory*) {
  return TStringId("HIDDEN");
}

static bool apply(const CreaturePredicates::Name& e, Position pos, const Creature* attacker) {
  return e.pred->apply(pos, attacker);
}

static TString getNameTopLevel(const CreaturePredicates::Name& e, const ContentFactory*) {
  return e.name;
}

static TString getName(const CreaturePredicates::Name& e, const ContentFactory*) {
  return e.name;
}

static bool applyToCreature(const CreaturePredicates::HatedBy& p, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getHatedByEffect() == p.effect;
}

static TString getName(const CreaturePredicates::HatedBy& p, const ContentFactory* f) {
  return *f->buffs.at(p.effect).hatedGroupName;
}

static bool apply(const CreaturePredicates::Attacker& p, Position pos, const Creature* attacker) {
  return !!attacker && p.pred->apply(attacker->getPosition(), attacker);
}

static TString getName(const CreaturePredicates::Attacker& p, const ContentFactory* f) {
  return p.pred->getNameInternal(f);
}

static bool apply(const CreaturePredicates::Not& p, Position pos, const Creature* attacker) {
  return !p.pred->apply(pos, attacker);
}

static TString getName(const CreaturePredicates::Not& p, const ContentFactory* f) {
  return p.pred->getNameInternal(f, true);
}

static TString getNameNegated(const CreaturePredicates::Not& p, const ContentFactory* f) {
  return p.pred->getNameInternal(f);
}

static bool applyToCreature(const CreaturePredicates::Ingredient& e, const Creature* victim, const Creature* attacker) {
  for (auto& item : victim->getEquipment().getItems(ItemIndex::RUNE))
    if (item->getIngredientType() == e.name)
      return true;
  return false;
}

static TString getName(const CreaturePredicates::Ingredient& e, const ContentFactory*) {
  return TSentence("CREATURES_HOLDING_INGREDIENT", TString(e.name));
}

static bool applyToCreature(const CreaturePredicates::EquipedIngredient& e, const Creature* victim, const Creature* attacker) {
  auto& equipment = victim->getEquipment();
  for (auto& item : equipment.getItems(ItemIndex::RUNE))
    if (equipment.isEquipped(item) && item->getIngredientType() == e.name)
      return true;
  return false;
}

static TString getName(const CreaturePredicates::EquipedIngredient& e, const ContentFactory*) {
  return TSentence("CREATURES_HOLDING_INGREDIENT", TString(e.name));
}


static bool apply(const CreaturePredicates::OnTheGround& e, Position pos, const Creature*) {
  for (auto& item : pos.getItems(ItemIndex::RUNE))
    if (item->getIngredientType() == e.name)
      return true;
  return false;
}

static TString getName(const CreaturePredicates::OnTheGround& e, const ContentFactory*) {
  return TSentence("INGREDIENT_ON_THE_GROUND", TString(e.name));
}

static bool applyToCreature(LastingOrBuff e, const Creature* victim, const Creature* attacker) {
  return isAffected(victim, e);
}

static TString getName(LastingOrBuff e, const ContentFactory* f) {
  return TSentence("CREATURES_AFFECTED_BY", ::getName(e, f));
}

static bool applyToCreature(CreatureStatus s, const Creature* victim, const Creature* attacker) {
  return victim->getStatus().contains(s);
}

static TString getName(CreatureStatus s, const ContentFactory*) {
  return getName(s);
}

static bool applyToCreature(CreatureId id, const Creature* victim, const Creature* attacker) {
  return victim->getAttributes().getCreatureId() == id;
}

static TString getName(CreatureId s, const ContentFactory*) {
  return TString(toLower(s.data()));
}

static bool apply(const CreaturePredicates::Flag& s, Position pos, const Creature* attacker) {
  return pos.getGame()->effectFlags.count(s.name);
}

static TString getName(const CreaturePredicates::Flag& s, const ContentFactory*) {
  return TString(s.name);
}

static bool applyToCreature(const CreaturePredicates::CreatureFlag& s, const Creature* victim, const Creature*) {
  return victim->effectFlags.count(s.name);
}

static TString getName(const CreaturePredicates::CreatureFlag& s, const ContentFactory*) {
  return TString(s.name);
}

static bool applyToCreature(const Gender& s, const Creature* victim, const Creature*) {
  return victim->getAttributes().getGender() == s;
}

static TString getName(const Gender& s, const ContentFactory*) {
  return get(s, TStringId("MALES"), TStringId("FEMALES"), TStringId("GENDERLESS"));
}

static bool applyToCreature(const CreaturePredicates::Rider& s, const Creature* victim, const Creature*) {
  return victim->getSteed();
}

static TString getName(const CreaturePredicates::Rider& s, const ContentFactory*) {
  return TStringId("RIDERS");
}

static bool applyToCreature(const CreaturePredicates::Kills& s, const Creature* victim, const Creature*) {
  return victim->getKills().size() >= s.cnt;
}

static TString getName(const CreaturePredicates::Kills& s, const ContentFactory*) {
  return TSentence("CREATURES_WITH_AT_LEAST_X_KILLS", TString(s.cnt));
}

static bool apply(const CreaturePredicates::Unlocked& s, Position pos, const Creature* attacker) {
  return pos.getGame()->getUnlocks()->isUnlocked(s.id);
}

static TString getName(const CreaturePredicates::Unlocked& s, const ContentFactory*) {
  return TString(s.id);
}

static bool apply(const CreaturePredicates::InTerritory& s, Position pos, const Creature* attacker) {
  auto col = pos.getCollective();
  return attacker && col && col->getCreatures().contains(attacker);
}

static TString getNameTopLevel(const CreaturePredicates::InTerritory&, const ContentFactory*) {
  return TStringId("WHEN_IN_TERRITORY");
}

static TString getName(const CreaturePredicates::InTerritory&, const ContentFactory*) {
  return TStringId("IN_TERRITORY");
}

static bool apply(FurnitureType type, Position pos, const Creature* attacker) {
  return !!pos.getFurniture(type);
}

static TString getName(FurnitureType, const ContentFactory*) {
  return TStringId("FURNITURE");
}

static bool apply(CreaturePredicates::ContainsGas type, Position pos, const Creature* attacker) {
  return pos.getGasAmount(type) > 0;
}

static TString getName(CreaturePredicates::ContainsGas type, const ContentFactory* f) {
  return TSentence("IN_GAS", f->tileGasTypes.at(type).name);
}

static bool applyToCreature(BodyMaterialId m, const Creature* victim, const Creature* attacker) {
  return victim->getBody().getMaterial() == m;
}

static TString getName(BodyMaterialId m, const ContentFactory* factory) {
  return TSentence("CREATURES_MADE_OF_MATERIAL", factory->bodyMaterials.at(m).name);
}

static bool applyToCreature(CreaturePredicates::Spellcaster m, const Creature* victim, const Creature* attacker) {
  for (auto spell : victim->getSpellMap().getAvailable(victim))
    if (spell->getType() == SpellType::SPELL)
      return true;
  return false;
}

static TString getName(CreaturePredicates::Spellcaster, const ContentFactory*) {
  return TStringId("SPELLCASTERS");
}

static bool applyToCreature(CreaturePredicates::Humanoid, const Creature* victim, const Creature*) {
  return victim->getBody().isHumanoid();
}

static TString getName(CreaturePredicates::Humanoid, const ContentFactory*) {
  return TStringId("HUMANOIDS");
}

static TString getNameNegated(CreaturePredicates::Humanoid, const ContentFactory*) {
  return TStringId("NON_HUMANOIDS");
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

static TString getName(const CreaturePredicates::IsClosedOffPigsty, const ContentFactory*) {
  return TStringId("INSIDE_AN_ANIMAL_PEN");
}

static bool apply(CreaturePredicates::CanCreatureEnter, Position position, const Creature*) {
  return position.canEnter({MovementTrait::WALK});
}

static TString getName(const CreaturePredicates::CanCreatureEnter, const ContentFactory*) {
  return TStringId("CAN_ENTER_POSITION");
}

static bool apply(CreaturePredicates::Frequency f, Position position, const Creature*) {
  return position.getGame()->getGlobalTime().getVisibleInt() % f.value == 0;
}

static TString getName(const CreaturePredicates::Frequency f, const ContentFactory*) {
  return TSentence("WITH_FREQUENCY", TString(f.value));
}

static bool applyToCreature(CreaturePredicates::PopLimitReached, const Creature* victim, const Creature*) {
  if (auto col = getCollective(victim))
    return col->getMaxPopulation() <= col->getPopulationSize();
  return false;
}

static TString getNameTopLevel(const CreaturePredicates::PopLimitReached, const ContentFactory*) {
  return TStringId("WHEN_POP_LIMIT_REACHED");
}

static TString getName(const CreaturePredicates::PopLimitReached, const ContentFactory*) {
  return TStringId("POP_LIMIT_REACHED");
}

static bool applyToCreature(const CreaturePredicates::Health& p, const Creature* victim, const Creature* attacker) {
  auto health = victim->getBody().hasAnyHealth(victim->getGame()->getContentFactory())
      ? victim->getBody().getHealth()
      : victim->getBody().getBodyPartHealth();
  return health >= p.from && health <= p.to;
}

static TString getName(const CreaturePredicates::Health& p, const ContentFactory*) {
  return TSentence("CREATURES_WITH_HEALTH_BETWEEN", TString(p.from), TString(p.to));
}

static bool apply(CreaturePredicates::Night m, Position pos, const Creature*) {
  return pos.getGame()->getSunlightInfo().getState() == SunlightState::NIGHT;
}

static TString getName(CreaturePredicates::Night m, const ContentFactory*) {
  return TStringId("AT_NIGHT");
}

static TString getNameNegated(CreaturePredicates::Night m, const ContentFactory*) {
  return TStringId("DURING_THE_DAY");
}

static TString getName(const CreaturePredicates::Distance& p, const ContentFactory*) {
  auto ret = "distance"_s;
  if (p.min)
    ret += " from " + toString(*p.min);
  if (p.max)
    ret += " up to " + toString(*p.max);
  return TString(ret);
}

static bool apply(const CreaturePredicates::Distance& e, Position pos, const Creature* attacker) {
  auto dist = attacker->getPosition().dist8(pos).value_or(10000);
  return dist >= e.min.value_or(-1) && dist < e.max.value_or(10000);
}

static TString getName(const CreaturePredicates::DistanceD& p, const ContentFactory*) {
  auto ret = "distance"_s;
  if (p.min)
    ret += " from " + toString(*p.min);
  if (p.max)
    ret += " up to " + toString(*p.max);
  return TString(ret);
}

static bool apply(const CreaturePredicates::DistanceD& e, Position pos, const Creature* attacker) {
  if (!attacker || !attacker->getPosition().isSameLevel(pos))
    return false;
  auto dist = attacker->getPosition().getCoord().distD(pos.getCoord());
  return dist >= e.min.value_or(-1) && dist <= e.max.value_or(10000);
}

static TString getName(const CreaturePredicates::AIAfraidOf&, const ContentFactory*) {
  return TStringId("AI_AFRAID_OF");
}

static bool applyToCreature(const CreaturePredicates::AIAfraidOf& e, const Creature* victim, const Creature* attacker) {
  return !!attacker && !attacker->shouldAIAttack(victim);
}

static bool apply(CreaturePredicates::Indoors m, Position pos, const Creature*) {
  return pos.isCovered();
}

static TString getName(CreaturePredicates::Indoors m, const ContentFactory*) {
  return TStringId("WHEN_INDOORS");
}

static TString getNameNegated(CreaturePredicates::Indoors m, const ContentFactory*) {
  return TStringId("WHEN_OUTDOORS");
}

static bool applyToCreature(const CreaturePredicates::AttributeAtLeast& a, const Creature* victim, const Creature*) {
  PROFILE;
  return victim->getAttr(a.attr) >= a.value;
}

static TString getName(const CreaturePredicates::AttributeAtLeast& a, const ContentFactory* f) {
  return TSentence("CREATURES_WITH_ATTR_AT_LEAST", TString(a.value), f->attrInfo.at(a.attr).name);
}

static bool applyToCreature(const CreaturePredicates::HasAnyHealth&, const Creature* victim, const Creature* attacker) {
  return victim->getBody().hasAnyHealth(victim->getGame()->getContentFactory());
}

static TString getName(const CreaturePredicates::HasAnyHealth&, const ContentFactory*) {
  return TStringId("WITH_HEALTH");
}

static bool applyToCreature(const CreaturePredicates::IsPlayer&, const Creature* victim, const Creature* attacker) {
  return victim->isPlayer();
}

static TString getName(const CreaturePredicates::IsPlayer&, const ContentFactory*) {
  return TStringId("THE_PLAYER");
}

static bool apply(const CreaturePredicates::TimeOfDay& t, Position pos, const Creature* attacker) {
  auto time = pos.getGame()->getSunlightInfo().getTimeSinceDawn();
  return time.getVisibleInt() < t.max && time.getVisibleInt() >= t.min;
}

static TString getName(const CreaturePredicates::TimeOfDay& m, const ContentFactory* f) {
  return TStringId("DURING_CERTAIN_TIME_OF_DAY");
}

static bool applyToCreature(const CreaturePredicates::MaxLevelBelow& p, const Creature* victim, const Creature* attacker) {
  return getValueMaybe(victim->getAttributes().getMaxExpLevel(), p.type).value_or(0) < p.value;
}

static TString getName(const CreaturePredicates::MaxLevelBelow& p, const ContentFactory*) {
  return TSentence("CREATURES_WITH_MAX_TRAINING_LEVEL_BELOW", TString(p.value));
}

static bool applyToCreature(const CreaturePredicates::ExperienceBelow& p, const Creature* victim, const Creature* attacker) {
  return victim->getCombatExperience(false, false) < p.value;
}

static TString getName(const CreaturePredicates::ExperienceBelow& p, const ContentFactory*) {
return TSentence("CREATURES_EXP_LEVEL_BELOW", TString(p.value));
}

static bool apply(const CreaturePredicates::Translate& m, Position pos, const Creature* attacker) {
  return m.pred->apply(pos.plus(m.dir), attacker);
}

static TString getName(const CreaturePredicates::Translate& m, const ContentFactory* f) {
  return m.pred->getNameInternal(f);
}

static bool apply(const CreaturePredicates::Area& m, Position pos, const Creature* attacker) {
  for (auto v : pos.getRectangle(Rectangle::centered(m.radius)))
    if (!m.pred->apply(v, attacker))
      return false;
  return true;
}

static TString getName(const CreaturePredicates::Area& m, const ContentFactory* f) {
  return m.pred->getNameInternal(f);
}

static bool apply(const CreaturePredicates::And& p, Position pos, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (!pred.apply(pos, attacker))
      return false;
  return true;
}

static TString getName(const CreaturePredicates::And& p, const ContentFactory* f) {
  return combineWithAnd(p.pred.transform([f] (const auto& pred) { return pred.getNameInternal(f); }));
}

static bool apply(const CreaturePredicates::Or& p, Position pos, const Creature* attacker) {
  for (auto& pred : p.pred)
    if (pred.apply(pos, attacker))
      return true;
  return false;
}

static TString getName(const CreaturePredicates::Or& p, const ContentFactory* f) {
  return combineWithOr(p.pred.transform([&] (const auto& pred) { return pred.getNameInternal(f); }));
}

template <typename T>
static TString getNameNegated(const T& p, const ContentFactory* f) {
  return TSentence("NOT", Impl::getName(p, f));
}

template <typename T>
static TString getNameTopLevel(const T& p, const ContentFactory* f) {
  return TSentence("AGAINST", Impl::getName(p, f));
}

template <typename T, REQUIRE(applyToCreature(TVALUE(const T&), TVALUE(const Creature*), TVALUE(const Creature*)))>
static bool apply(const T& t, Position pos, const Creature* attacker) {
  if (auto c = pos.getCreature())
    return applyToCreature(t, c, attacker);
  return false;
}

}

template <typename T, REQUIRE(Impl::applyToCreature(TVALUE(const T&), TVALUE(const Creature*), TVALUE(const Creature*)))>
static bool applyToCreature1(const T& t, const Creature* c, const Creature* attacker, int) {
  return Impl::applyToCreature(t, c, attacker);
}

template <typename T, REQUIRE(Impl::apply(TVALUE(const T&), TVALUE(Position), TVALUE(const Creature*)))>
static bool applyToCreature1(const T& t, const Creature* c, const Creature* attacker, double) {
  return Impl::apply(t, c->getPosition(), attacker);
}

bool CreaturePredicate::apply(Position pos, const Creature* attacker) const {
  return visit<bool>([&](const auto& p) { return Impl::apply(p, pos, attacker); });
}

bool CreaturePredicate::apply(const Creature* c, const Creature* attacker) const {
  return visit<bool>([&](const auto& p) { return applyToCreature1(p, c, attacker, 1); });
}

TString CreaturePredicate::getName(const ContentFactory* f) const {
  return visit<TString>([&](const auto& p) { return Impl::getNameTopLevel(p, f); });
}

 TString CreaturePredicate::getNameInternal(const ContentFactory* f, bool negated) const {
  if (negated)
    return visit<TString>([&](const auto& p) { return Impl::getNameNegated(p, f); });
  else
    return visit<TString>([&](const auto& p) { return Impl::getName(p, f); });
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
