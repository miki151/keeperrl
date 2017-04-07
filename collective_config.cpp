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
#include "furniture_type.h"
#include "spawn_type.h"
#include "minion_task.h"
#include "furniture_usage.h"
#include "creature_attributes.h"
#include "collective.h"
#include "zones.h"
#include "construction_map.h"
#include "item_class.h"
#include "villain_type.h"
#include "furniture.h"
#include "immigrant_info.h"
#include "tutorial_highlight.h"

template <class Archive>
void CollectiveConfig::serialize(Archive& ar, const unsigned int version) {
  ar(immigrantInterval, maxPopulation, populationIncreases, immigrantInfo);
  ar(type, leaderAsFighter, spawnGhosts, ghostProb, guardianInfo);
}

SERIALIZABLE(CollectiveConfig);
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveConfig);

template <class Archive>
void AttractionInfo::serialize(Archive& ar, const unsigned int version) {
  ar(types, amountClaimed);
}

SERIALIZABLE(AttractionInfo);
SERIALIZATION_CONSTRUCTOR_IMPL(AttractionInfo);

template <class Archive>
void PopulationIncrease::serialize(Archive& ar, const unsigned int version) {
  ar(type, increasePerSquare, maxIncrease);
}

SERIALIZABLE(PopulationIncrease);

template <class Archive>
void GuardianInfo::serialize(Archive& ar, const unsigned int version) {
  ar(creature, probability, minEnemies, minVictims);
}

SERIALIZABLE(GuardianInfo);

void CollectiveConfig::addBedRequirementToImmigrants() {
  for (auto& info : immigrantInfo) {
    PCreature c = CreatureFactory::fromId(info.getId(0), TribeId::getKeeper());
    if (auto spawnType = c->getAttributes().getSpawnType()) {
      AttractionType bedType = getDormInfo()[*spawnType].bedType;
      bool hasBed = false;
      info.visitRequirements(makeDefaultVisitor(
          [&](const AttractionInfo& attraction) -> void {
            for (auto& type : attraction.types)
              if (type == bedType)
                hasBed = true;
          }
      ));
      if (!hasBed)
        info.addRequirement(AttractionInfo(1, bedType));
    }
  }
}

CollectiveConfig::CollectiveConfig(int interval, const vector<ImmigrantInfo>& im,
    CollectiveType t, int maxPop, vector<PopulationIncrease> popInc)
    : immigrantInterval(interval),
    maxPopulation(maxPop), populationIncreases(popInc), immigrantInfo(im), type(t) {
  if (type == KEEPER)
    addBedRequirementToImmigrants();
}

CollectiveConfig CollectiveConfig::keeper(int immigrantInterval, int maxPopulation,
    vector<PopulationIncrease> increases, const vector<ImmigrantInfo>& im) {
  return CollectiveConfig(immigrantInterval, im, KEEPER, maxPopulation, increases);
}

CollectiveConfig CollectiveConfig::withImmigrants(int interval, int maxPopulation, const vector<ImmigrantInfo>& im) {
  return CollectiveConfig(interval, im, VILLAGE, maxPopulation, {});
}

CollectiveConfig CollectiveConfig::noImmigrants() {
  return CollectiveConfig(0, {}, VILLAGE, 10000, {});
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

int CollectiveConfig::getImmigrantTimeout() const {
  return 500;
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

bool CollectiveConfig::hasImmigrantion(bool currentlyActiveModel) const {
  return type != KEEPER || currentlyActiveModel;
}

int CollectiveConfig::getImmigrantInterval() const {
  return immigrantInterval;
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

bool CollectiveConfig::bedsLimitImmigration() const {
  return type == KEEPER;
}

int CollectiveConfig::getMaxPopulation() const {
  return maxPopulation;
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

const EnumMap<SpawnType, DormInfo>& CollectiveConfig::getDormInfo() const {
  static EnumMap<SpawnType, DormInfo> dormInfo ([](SpawnType type) -> DormInfo {
      switch (type) {
        case SpawnType::HUMANOID: return {FurnitureType::BED, CollectiveWarning::BEDS};
        case SpawnType::UNDEAD: return {FurnitureType::GRAVE};
        case SpawnType::BEAST: return {FurnitureType::BEAST_CAGE};
        case SpawnType::DEMON: return {FurnitureType::DEMON_SHRINE};
      }
  });
  return dormInfo;
}

const vector<FurnitureType>& CollectiveConfig::getRoomsNeedingLight() const {
  static vector<FurnitureType> ret {
    FurnitureType::WORKSHOP,
    FurnitureType::FORGE,
    FurnitureType::LABORATORY,
    FurnitureType::JEWELER,
    FurnitureType::STEEL_FURNACE,
    FurnitureType::TRAINING_WOOD,
    FurnitureType::TRAINING_IRON,
    FurnitureType::TRAINING_STEEL,
    FurnitureType::BOOK_SHELF};
  return ret;
};

const vector<FloorInfo>& CollectiveConfig::getFloors() {
  static vector<FloorInfo> ret = {
    {FurnitureType::FLOOR_WOOD1, {CollectiveResourceId::WOOD, 2}, "Wooden", 0.02},
    {FurnitureType::FLOOR_WOOD2, {CollectiveResourceId::WOOD, 2}, "Wooden", 0.02},
    {FurnitureType::FLOOR_STONE1, {CollectiveResourceId::STONE, 2}, "Stone", 0.04},
    {FurnitureType::FLOOR_STONE2, {CollectiveResourceId::STONE, 2}, "Stone", 0.04},
    {FurnitureType::FLOOR_CARPET1, {CollectiveResourceId::GOLD, 2}, "Carpet", 0.06},
    {FurnitureType::FLOOR_CARPET2, {CollectiveResourceId::GOLD, 2}, "Carpet", 0.06},
  };
  return ret;
}

double CollectiveConfig::getEfficiencyBonus(FurnitureType type) {
  for (auto& info : getFloors())
    if (info.type == type)
      return info.efficiencyBonus;
  switch (type) {
    case FurnitureType::DUNGEON_WALL:
      return 0.04;
    default:
      return 0;
  }
}

bool CollectiveConfig::canBuildOutsideTerritory(FurnitureType type) {
  switch (type) {
    case FurnitureType::EYEBALL:
    case FurnitureType::KEEPER_BOARD:
    case FurnitureType::DUNGEON_WALL:
    case FurnitureType::BRIDGE: return true;
    default: return false;
  }
}

static StorageDestinationFun getFurnitureStorage(FurnitureType t) {
  return [t](WConstCollective col)->const set<Position>& { return col->getConstructions().getBuiltPositions(t); };
}

static StorageDestinationFun getZoneStorage(ZoneId zone) {
  return [zone](WConstCollective col)->const set<Position>& { return col->getZones().getPositions(zone); };
}

const ResourceInfo& CollectiveConfig::getResourceInfo(CollectiveResourceId id) {
  static EnumMap<CollectiveResourceId, ResourceInfo> resourceInfo([](CollectiveResourceId id)->ResourceInfo {
    switch (id) {
      case CollectiveResourceId::MANA:
        return { nullptr, none, ItemId::GOLD_PIECE, "mana", ViewId::MANA};
      case CollectiveResourceId::PRISONER_HEAD:
        return { nullptr, none, ItemId::GOLD_PIECE, "", ViewId::IMPALED_HEAD, true};
      case CollectiveResourceId::GOLD:
        return {getFurnitureStorage(FurnitureType::TREASURE_CHEST), ItemIndex::GOLD, ItemId::GOLD_PIECE, "gold", ViewId::GOLD};
      case CollectiveResourceId::WOOD:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::WOOD, ItemId::WOOD_PLANK, "wood", ViewId::WOOD_PLANK,
            false, TutorialHighlight::WOOD_RESOURCE};
      case CollectiveResourceId::IRON:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::IRON, ItemId::IRON_ORE, "iron", ViewId::IRON_ROCK};
      case CollectiveResourceId::STEEL:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::STEEL, ItemId::STEEL_INGOT, "steel", ViewId::STEEL_INGOT};
      case CollectiveResourceId::STONE:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::STONE, ItemId::ROCK, "granite", ViewId::ROCK};
      case CollectiveResourceId::CORPSE:
        return { getFurnitureStorage(FurnitureType::GRAVE), ItemIndex::REVIVABLE_CORPSE, ItemId::GOLD_PIECE, "corpses", ViewId::BODY_PART, true};
    }
  });
  return resourceInfo[id];
}

static CollectiveItemPredicate unMarkedItems() {
  return [](WConstCollective col, const WItem it) { return !col->isItemMarked(it); };
}


const vector<ItemFetchInfo>& CollectiveConfig::getFetchInfo() {
  static vector<ItemFetchInfo> ret {
      {ItemIndex::CORPSE, unMarkedItems(), getFurnitureStorage(FurnitureType::GRAVE), true, CollectiveWarning::GRAVES},
      {ItemIndex::GOLD, unMarkedItems(), getFurnitureStorage(FurnitureType::TREASURE_CHEST), false,
          CollectiveWarning::CHESTS},
      {ItemIndex::MINION_EQUIPMENT, [](WConstCollective col, const WItem it)
          { return it->getClass() != ItemClass::GOLD && !col->isItemMarked(it);},
          getZoneStorage(ZoneId::STORAGE_EQUIPMENT), false, CollectiveWarning::EQUIPMENT_STORAGE},
      {ItemIndex::WOOD, unMarkedItems(), getZoneStorage(ZoneId::STORAGE_RESOURCES), false,
          CollectiveWarning::RESOURCE_STORAGE},
      {ItemIndex::IRON, unMarkedItems(), getZoneStorage(ZoneId::STORAGE_RESOURCES), false,
          CollectiveWarning::RESOURCE_STORAGE},
      {ItemIndex::STEEL, unMarkedItems(), getZoneStorage(ZoneId::STORAGE_RESOURCES), false,
          CollectiveWarning::RESOURCE_STORAGE},
      {ItemIndex::STONE, unMarkedItems(), getZoneStorage(ZoneId::STORAGE_RESOURCES), false,
          CollectiveWarning::RESOURCE_STORAGE},
  };
  return ret;
}

MinionTaskInfo::MinionTaskInfo(Type t, const string& desc, optional<CollectiveWarning> w)
    : type(t), description(desc), warning(w) {
  CHECK(type != FURNITURE);
}

MinionTaskInfo::MinionTaskInfo() {}

MinionTaskInfo::MinionTaskInfo(FurnitureType type, const string& desc) : type(FURNITURE),
  furniturePredicate([type](WConstCreature, FurnitureType t) { return t == type;}),
    description(desc) {
}

MinionTaskInfo::MinionTaskInfo(UsagePredicate pred, const string& desc) : type(FURNITURE),
  furniturePredicate(pred), description(desc) {
}

MinionTaskInfo::MinionTaskInfo(UsagePredicate usagePred, ActivePredicate activePred, const string& desc) :
    type(FURNITURE), furniturePredicate(usagePred), activePredicate(activePred), description(desc) {
}

static EnumMap<WorkshopType, WorkshopInfo> workshops([](WorkshopType type)->WorkshopInfo {
  switch (type) {
    case WorkshopType::WORKSHOP: return {FurnitureType::WORKSHOP, "workshop", SkillId::WORKSHOP};
    case WorkshopType::FORGE: return {FurnitureType::FORGE, "forge", SkillId::FORGE};
    case WorkshopType::LABORATORY: return {FurnitureType::LABORATORY, "laboratory", SkillId::LABORATORY};
    case WorkshopType::JEWELER: return {FurnitureType::JEWELER, "jeweler", SkillId::JEWELER};
    case WorkshopType::STEEL_FURNACE: return {FurnitureType::STEEL_FURNACE, "steel furnace", SkillId::FURNACE};
  }});

optional<WorkshopType> CollectiveConfig::getWorkshopType(FurnitureType furniture) {
  static optional<EnumMap<FurnitureType, optional<WorkshopType>>> map;
  if (!map) {
    map.emplace();
    for (auto type : ENUM_ALL(WorkshopType))
      (*map)[workshops[type].furniture] = type;
  }
  return (*map)[furniture];
}

map<CollectiveResourceId, int> CollectiveConfig::getStartingResource() const {
  if (type == KEEPER)
    return {{CollectiveResourceId::MANA, 200}};
  else
    return map<CollectiveResourceId, int>{};
}

optional<int> CollectiveConfig::getTrainingMaxLevelIncrease(FurnitureType type) {
  switch (type) {
    case FurnitureType::TRAINING_WOOD:
      return 3;
    case FurnitureType::TRAINING_IRON:
      return 7;
    case FurnitureType::TRAINING_STEEL:
      return 12;
    default:
      return none;
  }
}

int CollectiveConfig::getManaForConquering(VillainType type) {
  switch (type) {
    case VillainType::MAIN: return 400;
    case VillainType::LESSER: return 200;
    default: return 0;
  }
}

CollectiveConfig::CollectiveConfig(const CollectiveConfig&) = default;

CollectiveConfig::~CollectiveConfig() {
}

const MinionTaskInfo& CollectiveConfig::getTaskInfo(MinionTask task) {
  static EnumMap<MinionTask, MinionTaskInfo> map([](MinionTask task) -> MinionTaskInfo {
    switch (task) {
      case MinionTask::TRAIN: return {[](WConstCreature c, FurnitureType t) {
            if (auto maxIncrease = CollectiveConfig::getTrainingMaxLevelIncrease(t))
              return !c || c->getAttributes().getExpIncrease(ExperienceType::TRAINING) < *maxIncrease;
            else
              return false;
          }, "training"};
      case MinionTask::SLEEP: return {FurnitureType::BED, "sleeping"};
      case MinionTask::EAT: return {MinionTaskInfo::EAT, "eating"};
      case MinionTask::GRAVE: return {FurnitureType::GRAVE, "sleeping"};
      case MinionTask::LAIR: return {FurnitureType::BEAST_CAGE, "sleeping"};
      case MinionTask::THRONE: return {FurnitureType::THRONE, "throne"};
      case MinionTask::STUDY: return {FurnitureType::BOOK_SHELF, "studying"};
      case MinionTask::PRISON: return {FurnitureType::PRISON, "prison"};
      case MinionTask::CROPS: return {FurnitureType::CROPS, "crops"};
      case MinionTask::RITUAL: return {FurnitureType::DEMON_SHRINE, "rituals"};
      case MinionTask::COPULATE: return {MinionTaskInfo::COPULATE, "copulation"};
      case MinionTask::EXPLORE: return {MinionTaskInfo::EXPLORE, "spying"};
      case MinionTask::SPIDER: return {MinionTaskInfo::SPIDER, "spider"};
      case MinionTask::EXPLORE_NOCTURNAL: return {MinionTaskInfo::EXPLORE, "spying"};
      case MinionTask::EXPLORE_CAVES: return {MinionTaskInfo::EXPLORE, "spying"};
      case MinionTask::BE_WHIPPED: return {FurnitureType::WHIPPING_POST, "being whipped"};
      case MinionTask::BE_TORTURED: return {FurnitureType::TORTURE_TABLE, "being tortured"};
      case MinionTask::CRAFT: return {[](WConstCreature c, FurnitureType t) {
            if (auto type = getWorkshopType(t))
              return !c || c->getAttributes().getSkills().getValue(getWorkshopInfo(*type).skill) > 0;
            else
              return false;
          },
          [](WConstCollective collective, FurnitureType t) {
            if (auto type = getWorkshopType(t))
              return !collective->getWorkshops().get(*type).isIdle();
            else
              return false;
          },
          "crafting"};
    }
  });
  return map[task];
}

const WorkshopInfo& CollectiveConfig::getWorkshopInfo(WorkshopType type) {
  return workshops[type];
}


unique_ptr<Workshops> CollectiveConfig::getWorkshops() const {
  return unique_ptr<Workshops>(new Workshops({
      {WorkshopType::WORKSHOP, {
          Workshops::Item::fromType(ItemId::FIRST_AID_KIT, 1, {CollectiveResourceId::WOOD, 4}),
          Workshops::Item::fromType(ItemId::LEATHER_ARMOR, 6, {CollectiveResourceId::WOOD, 20}),
          Workshops::Item::fromType(ItemId::LEATHER_HELM, 1, {CollectiveResourceId::WOOD, 6}),
          Workshops::Item::fromType(ItemId::LEATHER_BOOTS, 2, {CollectiveResourceId::WOOD, 10}),
          Workshops::Item::fromType(ItemId::LEATHER_GLOVES, 1, {CollectiveResourceId::WOOD, 2}),
          Workshops::Item::fromType(ItemId::CLUB, 3, {CollectiveResourceId::WOOD, 10}),
          Workshops::Item::fromType(ItemId::HEAVY_CLUB, 5, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TWO_H_WEAP),
          Workshops::Item::fromType(ItemId::BOW, 13, {CollectiveResourceId::WOOD, 20}).setTechId(TechId::ARCHERY),
          Workshops::Item::fromType(ItemId::ARROW, 5, {CollectiveResourceId::WOOD, 10})
                  .setBatchSize(20).setTechId(TechId::ARCHERY),
          Workshops::Item::fromType(ItemId::BOULDER_TRAP_ITEM, 20, {CollectiveResourceId::STONE, 50})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType({ItemId::TRAP_ITEM,
              TrapInfo({TrapType::POISON_GAS, EffectId::EMIT_POISON_GAS})}, 10, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType({ItemId::TRAP_ITEM,
              TrapInfo({TrapType::ALARM, EffectId::ALARM})}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType({ItemId::TRAP_ITEM, TrapInfo({TrapType::WEB,
                EffectType(EffectId::LASTING, LastingEffect::ENTANGLED)})}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType({ItemId::TRAP_ITEM,
              TrapInfo({TrapType::SURPRISE, EffectId::TELE_ENEMIES})}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType({ItemId::TRAP_ITEM, TrapInfo({TrapType::TERROR,
                EffectType(EffectId::LASTING, LastingEffect::PANIC)})}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
      }},
      {WorkshopType::FORGE, {
          Workshops::Item::fromType(ItemId::SWORD, 10, {CollectiveResourceId::IRON, 20}),
          Workshops::Item::fromType(ItemId::STEEL_SWORD, 20, {CollectiveResourceId::STEEL, 20})
                  .setTechId(TechId::STEEL_MAKING),
          Workshops::Item::fromType(ItemId::CHAIN_ARMOR, 30, {CollectiveResourceId::IRON, 40}),
          Workshops::Item::fromType(ItemId::STEEL_ARMOR, 60, {CollectiveResourceId::STEEL, 40})
                  .setTechId(TechId::STEEL_MAKING),
          Workshops::Item::fromType(ItemId::IRON_HELM, 8, {CollectiveResourceId::IRON, 16}),
          Workshops::Item::fromType(ItemId::IRON_BOOTS, 12, {CollectiveResourceId::IRON, 24}),
          Workshops::Item::fromType(ItemId::WAR_HAMMER, 16, {CollectiveResourceId::IRON, 40})
                  .setTechId(TechId::TWO_H_WEAP),
          Workshops::Item::fromType(ItemId::BATTLE_AXE, 22, {CollectiveResourceId::IRON, 50})
                  .setTechId(TechId::TWO_H_WEAP),
          Workshops::Item::fromType(ItemId::STEEL_BATTLE_AXE, 44, {CollectiveResourceId::STEEL, 50})
                  .setTechId(TechId::STEEL_MAKING),
      }},
      {WorkshopType::LABORATORY, {
          Workshops::Item::fromType({ItemId::POTION, EffectType{EffectId::LASTING, LastingEffect::SLOWED}}, 2,
              {CollectiveResourceId::MANA, 10}),
          Workshops::Item::fromType({ItemId::POTION, EffectType{EffectId::LASTING, LastingEffect::SLEEP}}, 2,
              {CollectiveResourceId::MANA, 10}),
          Workshops::Item::fromType({ItemId::POTION, EffectId::HEAL}, 4, {CollectiveResourceId::MANA, 30}),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::POISON_RESISTANT}}, 3, {CollectiveResourceId::MANA, 30}),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::POISON}}, 2, {CollectiveResourceId::MANA, 30}),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::SPEED}}, 4, {CollectiveResourceId::MANA, 30})
                  .setTechId(TechId::ALCHEMY_ADV),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::BLIND}}, 4, {CollectiveResourceId::MANA, 50})
                  .setTechId(TechId::ALCHEMY_ADV),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::FLYING}}, 6, {CollectiveResourceId::MANA, 80})
                  .setTechId(TechId::ALCHEMY_ADV),
          Workshops::Item::fromType({ItemId::POTION,
              EffectType{EffectId::LASTING, LastingEffect::INVISIBLE}}, 6, {CollectiveResourceId::MANA, 200})
                  .setTechId(TechId::ALCHEMY_ADV),
      }},
      {WorkshopType::JEWELER, {
          Workshops::Item::fromType({ItemId::RING, LastingEffect::POISON_RESISTANT}, 10,
              {CollectiveResourceId::GOLD, 20}),
          Workshops::Item::fromType({ItemId::RING, LastingEffect::FIRE_RESISTANT}, 10,
              {CollectiveResourceId::GOLD, 30}),
          Workshops::Item::fromType(ItemId::WARNING_AMULET, 10, {CollectiveResourceId::GOLD, 30}),
          Workshops::Item::fromType(ItemId::DEFENSE_AMULET, 10, {CollectiveResourceId::GOLD, 40}),
          Workshops::Item::fromType(ItemId::HEALING_AMULET, 10, {CollectiveResourceId::GOLD, 60}),
      }},
      {WorkshopType::STEEL_FURNACE, {
          Workshops::Item::fromType(ItemId::STEEL_INGOT, 50, {CollectiveResourceId::IRON, 30}).setBatchSize(10),
      }},
  }));
}
