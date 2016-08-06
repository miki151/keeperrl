#include "stdafx.h"
#include "util.h"
#include "creature.h"
#include "collective_config.h"
#include "tribe.h"
#include "game.h"
#include "technology.h"
#include "collective_warning.h"
#include "item_type.h"
#include "resource_id.h"
#include "inventory.h"
#include "workshops.h"
#include "lasting_effect.h"
#include "item.h"
#include "square_type.h"
#include "view_id.h"

AttractionInfo::AttractionInfo(MinionAttraction a, double cl, double min, bool mand)
  : attraction(a), amountClaimed(cl), minAmount(min), mandatory(mand) {}

template <class Archive>
void CollectiveConfig::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(immigrantFrequency)
    & SVAR(payoutTime)
    & SVAR(payoutMultiplier)
    & SVAR(maxPopulation)
    & SVAR(populationIncreases)
    & SVAR(immigrantInfo)
    & SVAR(type)
    & SVAR(recruitingMinPopulation)
    & SVAR(leaderAsFighter)
    & SVAR(spawnGhosts)
    & SVAR(ghostProb)
    & SVAR(guardianInfo);
}

SERIALIZABLE(CollectiveConfig);
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveConfig);

template <class Archive>
void ImmigrantInfo::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(id)
    & SVAR(frequency)
    & SVAR(attractions)
    & SVAR(traits)
    & SVAR(spawnAtDorm)
    & SVAR(salary)
    & SVAR(techId)
    & SVAR(limit)
    & SVAR(groupSize)
    & SVAR(autoTeam)
    & SVAR(ignoreSpawnType);
}

SERIALIZABLE(ImmigrantInfo);

template <class Archive>
void AttractionInfo::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(attraction)
    & SVAR(amountClaimed)
    & SVAR(minAmount)
    & SVAR(mandatory);
}

SERIALIZABLE(AttractionInfo);
SERIALIZATION_CONSTRUCTOR_IMPL(AttractionInfo);

template <class Archive>
void PopulationIncrease::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(type)
    & SVAR(increasePerSquare)
    & SVAR(maxIncrease);
}

SERIALIZABLE(PopulationIncrease);

template <class Archive>
void GuardianInfo::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(creature)
    & SVAR(probability)
    & SVAR(minEnemies)
    & SVAR(minVictims);
}

SERIALIZABLE(GuardianInfo);

CollectiveConfig::CollectiveConfig(double freq, int payoutT, double payoutM, vector<ImmigrantInfo> im,
    CollectiveType t, int maxPop, vector<PopulationIncrease> popInc)
    : immigrantFrequency(freq), payoutTime(payoutT), payoutMultiplier(payoutM),
    maxPopulation(maxPop), populationIncreases(popInc), immigrantInfo(im), type(t) {}

CollectiveConfig CollectiveConfig::keeper(double freq, int payout, double payoutMult, int maxPopulation,
    vector<PopulationIncrease> increases, vector<ImmigrantInfo> im) {
  return CollectiveConfig(freq, payout, payoutMult, im, KEEPER, maxPopulation, increases);
}

CollectiveConfig CollectiveConfig::withImmigrants(double frequency, int maxPopulation, vector<ImmigrantInfo> im) {
  return CollectiveConfig(frequency, 0, 0, im, VILLAGE, maxPopulation, {});
}

CollectiveConfig CollectiveConfig::noImmigrants() {
  return CollectiveConfig(0, 0, 0, {}, VILLAGE, 10000, {});
}

CollectiveConfig& CollectiveConfig::setLeaderAsFighter() {
  leaderAsFighter = true;
  return *this;
}

CollectiveConfig& CollectiveConfig::setGhostSpawns(double prob, int num) {
  ghostProb = prob;
  spawnGhosts = num;
  return *this;
}

int CollectiveConfig::getNumGhostSpawns() const {
  return spawnGhosts;
}

double CollectiveConfig::getGhostProb() const {
  return ghostProb;
}

bool CollectiveConfig::isLeaderFighter() const {
  return leaderAsFighter;
}

bool CollectiveConfig::getManageEquipment() const {
  return type == KEEPER;
}

bool CollectiveConfig::getWorkerFollowLeader() const {
  return type == KEEPER;
}

bool CollectiveConfig::sleepOnlyAtNight() const {
  return type != KEEPER;
}

bool CollectiveConfig::activeImmigrantion(const Game* game) const {
  return type == KEEPER || game->isSingleModel();
}

double CollectiveConfig::getImmigrantFrequency() const {
  return immigrantFrequency;
}

int CollectiveConfig::getPayoutTime() const {
  return payoutTime;
}
 
double CollectiveConfig::getPayoutMultiplier() const {
  return payoutMultiplier;
}

bool CollectiveConfig::getStripSpawns() const {
  return type == KEEPER;
}

bool CollectiveConfig::getFetchItems() const {
  return type == KEEPER;
}

bool CollectiveConfig::getEnemyPositions() const {
  return type == KEEPER;
}

bool CollectiveConfig::getWarnings() const {
  return type == KEEPER;
}

bool CollectiveConfig::getConstructions() const {
  return type == KEEPER;
}

int CollectiveConfig::getMaxPopulation() const {
  return maxPopulation;
}

optional<int> CollectiveConfig::getRecruitingMinPopulation() const {
  return recruitingMinPopulation;
}

CollectiveConfig& CollectiveConfig::allowRecruiting(int minPop) {
  recruitingMinPopulation = minPop;
  return *this;
}

const vector<ImmigrantInfo>& CollectiveConfig::getImmigrantInfo() const {
  return immigrantInfo;
}

const vector<PopulationIncrease>& CollectiveConfig::getPopulationIncreases() const {
  return populationIncreases;
}

CollectiveConfig& CollectiveConfig::setGuardian(GuardianInfo info) {
  guardianInfo = info;
  return *this;
}

const optional<GuardianInfo>& CollectiveConfig::getGuardianInfo() const {
  return guardianInfo;
}

vector<BirthSpawn> CollectiveConfig::getBirthSpawns() const {
  if (type == KEEPER)
    return {{ CreatureId::GOBLIN, 1 },
      { CreatureId::ORC, 1 },
      { CreatureId::ORC_SHAMAN, 0.5 },
      { CreatureId::HARPY, 0.5 },
      { CreatureId::OGRE, 0.5 },
      { CreatureId::WEREWOLF, 0.5 },
      { CreatureId::SPECIAL_HM, 1.0, TechId::HUMANOID_MUT},
      { CreatureId::SPECIAL_BM, 1.0, TechId::BEAST_MUT },
    };
  else 
    return {};
}

optional<SquareType> CollectiveConfig::getSecondarySquare(SquareType type) {
  switch (type.getId()) {
    case SquareId::DORM: return SquareType(SquareId::BED);
    case SquareId::BEAST_LAIR: return SquareType(SquareId::BEAST_CAGE);
    case SquareId::CEMETERY: return SquareType(SquareId::GRAVE);
    default: return none;
  }
}

optional<SquareType> DormInfo::getBedType() const {
  return CollectiveConfig::getSecondarySquare(dormType);
}

const EnumMap<SpawnType, DormInfo>& CollectiveConfig::getDormInfo() const {
  static EnumMap<SpawnType, DormInfo> dormInfo {
    {SpawnType::HUMANOID, {SquareId::DORM, CollectiveWarning::BEDS}},
    {SpawnType::UNDEAD, {SquareId::CEMETERY}},
    {SpawnType::BEAST, {SquareId::BEAST_LAIR}},
    {SpawnType::DEMON, {SquareId::RITUAL_ROOM}},
  };
  return dormInfo;
}

const static unordered_set<SquareType> efficiencySquares {
  SquareId::TRAINING_ROOM,
  SquareId::TORTURE_TABLE,
  SquareId::WORKSHOP,
  SquareId::FORGE,
  SquareId::LABORATORY,
  SquareId::JEWELER,
  SquareId::LIBRARY,
};

unordered_set<SquareType> CollectiveConfig::getEfficiencySquares() const {
  return efficiencySquares;
}

vector<SquareType> CollectiveConfig::getRoomsNeedingLight() const {
  static vector<SquareType> ret {
    SquareId::WORKSHOP,
      SquareId::FORGE,
      SquareId::LABORATORY,
      SquareId::JEWELER,
      SquareId::TRAINING_ROOM,
      SquareId::LIBRARY};
  return ret;
};

int CollectiveConfig::getTaskDuration(const Creature* c, MinionTask task) const {
  switch (task) {
    case MinionTask::CONSUME:
    case MinionTask::COPULATE:
    case MinionTask::GRAVE:
    case MinionTask::LAIR:
    case MinionTask::EAT:
    case MinionTask::SLEEP: return 1;
    default: return 500 + 250 * c->getMorale();
  }
}

const static vector<SquareType> resourceStorage {SquareId::STOCKPILE, SquareId::STOCKPILE_RES};
const static vector<SquareType> equipmentStorage {SquareId::STOCKPILE, SquareId::STOCKPILE_EQUIP};

const vector<SquareType>& CollectiveConfig::getEquipmentStorage() {
  return equipmentStorage;
}

const vector<SquareType>& CollectiveConfig::getResourceStorage() {
  return resourceStorage;
}

const map<CollectiveResourceId, ResourceInfo>& CollectiveConfig::getResourceInfo() {
  static map<CollectiveResourceId, ResourceInfo> ret = {
    {CollectiveResourceId::MANA, { {}, none, ItemId::GOLD_PIECE, "mana", ViewId::MANA}},
    {CollectiveResourceId::PRISONER_HEAD, { {}, none, ItemId::GOLD_PIECE, "", ViewId::IMPALED_HEAD, true}},
    {CollectiveResourceId::GOLD,
        {{SquareId::TREASURE_CHEST}, ItemIndex::GOLD, ItemId::GOLD_PIECE, "gold", ViewId::GOLD}},
    {CollectiveResourceId::WOOD, { resourceStorage, ItemIndex::WOOD, ItemId::WOOD_PLANK, "wood", ViewId::WOOD_PLANK}},
    {CollectiveResourceId::IRON, { resourceStorage, ItemIndex::IRON, ItemId::IRON_ORE, "iron", ViewId::IRON_ROCK}},
    {CollectiveResourceId::STONE, { resourceStorage, ItemIndex::STONE, ItemId::ROCK, "granite", ViewId::ROCK}},
    {CollectiveResourceId::CORPSE,
      { {SquareId::CEMETERY}, ItemIndex::REVIVABLE_CORPSE, ItemId::GOLD_PIECE, "corpses", ViewId::BODY_PART, true}}
  };
  return ret;
}

MinionTaskInfo::MinionTaskInfo(vector<SquareType> s, const string& desc, optional<CollectiveWarning> w,
    double _cost, bool center) 
    : type(APPLY_SQUARE), squares(s), description(desc), warning(w), cost(_cost), centerOnly(center) {
}

MinionTaskInfo::MinionTaskInfo(Type t, const string& desc, optional<CollectiveWarning> w)
    : type(t), description(desc), warning(w) {
  CHECK(type != APPLY_SQUARE);
}


map<MinionTask, MinionTaskInfo> CollectiveConfig::getTaskInfo() const {
  map<MinionTask, MinionTaskInfo> ret {
    {MinionTask::TRAIN, {{SquareId::TRAINING_ROOM}, "training", CollectiveWarning::TRAINING, 1}},
    {MinionTask::WORKSHOP, {{SquareId::WORKSHOP}, "workshop", CollectiveWarning::WORKSHOP, 1}},
    {MinionTask::FORGE, {{SquareId::FORGE}, "forge", none, 1}},
    {MinionTask::LABORATORY, {{SquareId::LABORATORY}, "lab", none, 1}},
    {MinionTask::JEWELER, {{SquareId::JEWELER}, "jewellery", none, 1}},
    {MinionTask::SLEEP, {{SquareId::BED}, "sleeping", CollectiveWarning::BEDS}},
    {MinionTask::EAT, {MinionTaskInfo::EAT, "eating"}},
    {MinionTask::GRAVE, {{SquareId::GRAVE}, "sleeping", CollectiveWarning::GRAVES}},
    {MinionTask::LAIR, {{SquareId::BEAST_CAGE}, "sleeping"}},
    {MinionTask::LAIR, {{SquareId::BEAST_CAGE}, "sleeping"}},
    {MinionTask::THRONE, {{SquareId::THRONE}, "throne"}},
    {MinionTask::STUDY, {{SquareId::LIBRARY}, "studying", CollectiveWarning::LIBRARY, 1}},
    {MinionTask::PRISON, {{SquareId::PRISON}, "prison", CollectiveWarning::NO_PRISON}},
    {MinionTask::TORTURE, {{SquareId::TORTURE_TABLE}, "torture ordered",
                            CollectiveWarning::TORTURE_ROOM, 0, true}},
    {MinionTask::CROPS, {{SquareId::CROPS}, "crops"}},
    {MinionTask::RITUAL, {{SquareId::RITUAL_ROOM}, "rituals"}},
    {MinionTask::COPULATE, {MinionTaskInfo::COPULATE, "copulation"}},
    {MinionTask::CONSUME, {MinionTaskInfo::CONSUME, "consumption"}},
    {MinionTask::EXPLORE, {MinionTaskInfo::EXPLORE, "spying"}},
    {MinionTask::SPIDER, {MinionTaskInfo::SPIDER, "spider"}},
    {MinionTask::EXPLORE_NOCTURNAL, {MinionTaskInfo::EXPLORE, "spying"}},
    {MinionTask::EXPLORE_CAVES, {MinionTaskInfo::EXPLORE, "spying"}},
    {MinionTask::EXECUTE, {{SquareId::PRISON}, "execution ordered", CollectiveWarning::NO_PRISON}}};
  return ret;
};

static vector<WorkshopInfo> workshops {
  {SquareId::WORKSHOP, WorkshopType::WORKSHOP},
  {SquareId::FORGE, WorkshopType::FORGE},
  {SquareId::LABORATORY, WorkshopType::LABORATORY},
  {SquareId::JEWELER, WorkshopType::JEWELER},
};

const vector<WorkshopInfo>& CollectiveConfig::getWorkshopInfo() const {
  return workshops;
}


unique_ptr<Workshops> CollectiveConfig::getWorkshops() const {
  return unique_ptr<Workshops>(new Workshops({
      {WorkshopType::WORKSHOP, {
          Workshops::Item::fromType(ItemId::FIRST_AID_KIT, {CollectiveResourceId::WOOD, 20}, 1),
          Workshops::Item::fromType(ItemId::LEATHER_ARMOR, {CollectiveResourceId::WOOD, 100}, 6),
          Workshops::Item::fromType(ItemId::LEATHER_HELM, {CollectiveResourceId::WOOD, 30}, 1),
          Workshops::Item::fromType(ItemId::LEATHER_BOOTS, {CollectiveResourceId::WOOD, 50}, 2),
          Workshops::Item::fromType(ItemId::LEATHER_GLOVES, {CollectiveResourceId::WOOD, 10}, 1),
          Workshops::Item::fromType(ItemId::CLUB, {CollectiveResourceId::WOOD, 50}, 3),
          Workshops::Item::fromType(ItemId::HEAVY_CLUB, {CollectiveResourceId::WOOD, 100}, 5),
          Workshops::Item::fromType(ItemId::BOW, {CollectiveResourceId::WOOD, 100}, 13),
          Workshops::Item::fromType(ItemId::ARROW, {CollectiveResourceId::WOOD, 50}, 5, 20),
          Workshops::Item::fromType(ItemId::BOULDER_TRAP_ITEM, {CollectiveResourceId::STONE, 250}, 20),
          Workshops::Item::fromType({ItemId::TRAP_ITEM,
              TrapInfo({TrapType::POISON_GAS, EffectId::EMIT_POISON_GAS})}, {CollectiveResourceId::WOOD, 100}, 10),
          Workshops::Item::fromType({ItemId::TRAP_ITEM,
              TrapInfo({TrapType::ALARM, EffectId::ALARM})}, {CollectiveResourceId::WOOD, 100}, 8),
          Workshops::Item::fromType({ItemId::TRAP_ITEM, TrapInfo({TrapType::WEB,
                EffectType(EffectId::LASTING, LastingEffect::ENTANGLED)})}, {CollectiveResourceId::WOOD, 100}, 8),
          Workshops::Item::fromType({ItemId::TRAP_ITEM,
              TrapInfo({TrapType::SURPRISE, EffectId::TELE_ENEMIES})}, {CollectiveResourceId::WOOD, 100}, 8),
          Workshops::Item::fromType({ItemId::TRAP_ITEM, TrapInfo({TrapType::TERROR,
                EffectType(EffectId::LASTING, LastingEffect::PANIC)})}, {CollectiveResourceId::WOOD, 100}, 8),
      }},
      {WorkshopType::FORGE, {
          Workshops::Item::fromType(ItemId::SWORD, {CollectiveResourceId::IRON, 100}, 10),
          Workshops::Item::fromType(ItemId::SPECIAL_SWORD, {CollectiveResourceId::IRON, 1000}, 80),
          Workshops::Item::fromType(ItemId::CHAIN_ARMOR, {CollectiveResourceId::IRON, 200}, 30),
          Workshops::Item::fromType(ItemId::IRON_HELM, {CollectiveResourceId::IRON, 80}, 8),
          Workshops::Item::fromType(ItemId::IRON_BOOTS, {CollectiveResourceId::IRON, 120}, 12),
          Workshops::Item::fromType(ItemId::WAR_HAMMER, {CollectiveResourceId::IRON, 190}, 16), 
          Workshops::Item::fromType(ItemId::BATTLE_AXE, {CollectiveResourceId::IRON, 250}, 22),
          Workshops::Item::fromType(ItemId::SPECIAL_WAR_HAMMER, {CollectiveResourceId::IRON, 1900}, 120), 
          Workshops::Item::fromType(ItemId::SPECIAL_BATTLE_AXE, {CollectiveResourceId::IRON, 2000}, 180), 
      }},
      {WorkshopType::LABORATORY, {
          Workshops::Item::fromType({ItemId::POTION, EffectType{EffectId::LASTING, LastingEffect::SLOWED}},
              {CollectiveResourceId::MANA, 10}, 2),
          Workshops::Item::fromType({ItemId::POTION, EffectType{EffectId::LASTING, LastingEffect::SLEEP}},
              {CollectiveResourceId::MANA, 10}, 2),
          Workshops::Item::fromType({ItemId::POTION, EffectId::HEAL}, {CollectiveResourceId::MANA, 30}, 4),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::POISON_RESISTANT}}, {CollectiveResourceId::MANA, 30}, 3),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::POISON}}, {CollectiveResourceId::MANA, 30}, 2),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::SPEED}}, {CollectiveResourceId::MANA, 30}, 4),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::BLIND}}, {CollectiveResourceId::MANA, 50}, 4),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::FLYING}}, {CollectiveResourceId::MANA, 80}, 6),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::INVISIBLE}}, {CollectiveResourceId::MANA, 200}, 6),
      }},
      {WorkshopType::JEWELER, {
          Workshops::Item::fromType({ItemId::RING, LastingEffect::POISON_RESISTANT},
              {CollectiveResourceId::GOLD, 100}, 10),
          Workshops::Item::fromType({ItemId::RING, LastingEffect::FIRE_RESISTANT},
              {CollectiveResourceId::GOLD, 150}, 10),
          Workshops::Item::fromType(ItemId::WARNING_AMULET, {CollectiveResourceId::GOLD, 150}, 10),
          Workshops::Item::fromType(ItemId::DEFENSE_AMULET, {CollectiveResourceId::GOLD, 200}, 10),
          Workshops::Item::fromType(ItemId::HEALING_AMULET, {CollectiveResourceId::GOLD, 300}, 10),
      }},
  }));
}

