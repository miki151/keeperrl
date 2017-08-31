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
#include "trap_type.h"
#include "spell_id.h"
#include "spell.h"
#include "creature_factory.h"
#include "resource_info.h"
#include "workshop_item.h"


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
      info.visitRequirements(makeVisitor(
          [&](const AttractionInfo& attraction) -> void {
            for (auto& type : attraction.types)
              if (type == bedType)
                hasBed = true;
          },
          [&](const auto&) {}
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

bool CollectiveConfig::getFollowLeaderIfNoTerritory() const {
  return type == KEEPER;
}

bool CollectiveConfig::hasVillainSleepingTask() const {
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
    FurnitureType::BOOKCASE_WOOD,
    FurnitureType::BOOKCASE_IRON,
    FurnitureType::BOOKCASE_GOLD,
  };
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
    case FurnitureType::TORCH_N:
    case FurnitureType::TORCH_E:
    case FurnitureType::TORCH_S:
    case FurnitureType::TORCH_W:
    case FurnitureType::TUTORIAL_ENTRANCE:
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
        return { nullptr, none, ItemType::GoldPiece{}, "mana", ViewId::MANA};
      case CollectiveResourceId::PRISONER_HEAD:
        return { nullptr, none, ItemType::GoldPiece{}, "", ViewId::IMPALED_HEAD, true};
      case CollectiveResourceId::GOLD:
        return {getFurnitureStorage(FurnitureType::TREASURE_CHEST), ItemIndex::GOLD, ItemType::GoldPiece{}, "gold", ViewId::GOLD};
      case CollectiveResourceId::WOOD:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::WOOD, ItemType::WoodPlank{}, "wood", ViewId::WOOD_PLANK,
            false, TutorialHighlight::WOOD_RESOURCE};
      case CollectiveResourceId::IRON:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::IRON, ItemType::IronOre{}, "iron", ViewId::IRON_ROCK};
      case CollectiveResourceId::STEEL:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::STEEL, ItemType::SteelIngot{}, "steel", ViewId::STEEL_INGOT};
      case CollectiveResourceId::STONE:
        return { getZoneStorage(ZoneId::STORAGE_RESOURCES), ItemIndex::STONE, ItemType::Rock{}, "granite", ViewId::ROCK};
      case CollectiveResourceId::CORPSE:
        return { getFurnitureStorage(FurnitureType::GRAVE), ItemIndex::REVIVABLE_CORPSE, ItemType::GoldPiece{}, "corpses", ViewId::BODY_PART, true};
    }
  });
  return resourceInfo[id];
}

static CollectiveItemPredicate unMarkedItems() {
  return [](WConstCollective col, WConstItem it) { return !col->isItemMarked(it); };
}


const vector<ItemFetchInfo>& CollectiveConfig::getFetchInfo() {
  static vector<ItemFetchInfo> ret {
      {ItemIndex::CORPSE, unMarkedItems(), getFurnitureStorage(FurnitureType::GRAVE), true, CollectiveWarning::GRAVES},
      {ItemIndex::GOLD, unMarkedItems(), getFurnitureStorage(FurnitureType::TREASURE_CHEST), false,
          CollectiveWarning::CHESTS},
      {ItemIndex::MINION_EQUIPMENT, [](WConstCollective col, WConstItem it)
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
  return map<CollectiveResourceId, int>{};
}

const vector<FurnitureType>& CollectiveConfig::getTrainingFurniture(ExperienceType type) {
  static EnumMap<ExperienceType, vector<FurnitureType>> ret(
      [](ExperienceType expType) {
        vector<FurnitureType> furniture;
        for (auto type : ENUM_ALL(FurnitureType))
          if (!!getTrainingMaxLevel(expType, type))
            furniture.push_back(type);
        return furniture;
      });
  return ret[type];
}

optional<int> CollectiveConfig::getTrainingMaxLevel(ExperienceType experienceType, FurnitureType type) {
  switch (experienceType) {
    case ExperienceType::MELEE:
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
      break;
    case ExperienceType::SPELL:
      switch (type) {
        case FurnitureType::BOOKCASE_WOOD:
          return 3;
        case FurnitureType::BOOKCASE_IRON:
          return 7;
        case FurnitureType::BOOKCASE_GOLD:
          return 12;
        default:
          return none;
      }
      break;
    default:
      return none;
  }
}

int CollectiveConfig::getManaForConquering(const optional<VillainType>& type) {
  if (type)
    switch (*type) {
      case VillainType::MAIN:
        return 200;
      case VillainType::LESSER:
        return 100;
      default:
        break;;
    }
  return 50;
}

CollectiveConfig::CollectiveConfig(const CollectiveConfig&) = default;

CollectiveConfig::~CollectiveConfig() {
}

static auto getTrainingPredicate(ExperienceType experienceType) {
  return [experienceType] (WConstCreature c, FurnitureType t) {
      if (auto maxIncrease = CollectiveConfig::getTrainingMaxLevel(experienceType, t))
        return !c || (c->getAttributes().getExpLevel(experienceType) < *maxIncrease &&
            !c->getAttributes().isTrainingMaxedOut(experienceType));
      else
        return false;
    };
}

const MinionTaskInfo& CollectiveConfig::getTaskInfo(MinionTask task) {
  static EnumMap<MinionTask, MinionTaskInfo> map([](MinionTask task) -> MinionTaskInfo {
    switch (task) {
      case MinionTask::WORKER: return {MinionTaskInfo::WORKER, "working"};
      case MinionTask::TRAIN: return {getTrainingPredicate(ExperienceType::MELEE), "training"};
      case MinionTask::SLEEP: return {FurnitureType::BED, "sleeping"};
      case MinionTask::EAT: return {MinionTaskInfo::EAT, "eating"};
      case MinionTask::GRAVE: return {FurnitureType::GRAVE, "sleeping"};
      case MinionTask::LAIR: return {FurnitureType::BEAST_CAGE, "sleeping"};
      case MinionTask::THRONE: return {FurnitureType::THRONE, "throne"};
      case MinionTask::STUDY: return {getTrainingPredicate(ExperienceType::SPELL), "studying"};
      case MinionTask::PRISON: return {FurnitureType::PRISON, "prison"};
      case MinionTask::CROPS: return {FurnitureType::CROPS, "crops"};
      case MinionTask::RITUAL: return {FurnitureType::DEMON_SHRINE, "rituals"};
      case MinionTask::ARCHERY: return {MinionTaskInfo::ARCHERY, "archery range"};
      case MinionTask::COPULATE: return {MinionTaskInfo::COPULATE, "copulation"};
      case MinionTask::EXPLORE: return {MinionTaskInfo::EXPLORE, "spying"};
      case MinionTask::SPIDER: return {MinionTaskInfo::SPIDER, "spider"};
      case MinionTask::EXPLORE_NOCTURNAL: return {MinionTaskInfo::EXPLORE, "spying"};
      case MinionTask::EXPLORE_CAVES: return {MinionTaskInfo::EXPLORE, "spying"};
      case MinionTask::BE_WHIPPED: return {FurnitureType::WHIPPING_POST, "being whipped"};
      case MinionTask::BE_TORTURED: return {FurnitureType::TORTURE_TABLE, "being tortured"};
      case MinionTask::BE_EXECUTED: return {FurnitureType::GALLOWS, "being executed"};
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
          Workshops::Item::fromType(ItemType::LeatherArmor{}, 6, {CollectiveResourceId::WOOD, 20}),
          Workshops::Item::fromType(ItemType::LeatherHelm{}, 1, {CollectiveResourceId::WOOD, 6}),
          Workshops::Item::fromType(ItemType::LeatherBoots{}, 2, {CollectiveResourceId::WOOD, 10}),
          Workshops::Item::fromType(ItemType::LeatherGloves{}, 1, {CollectiveResourceId::WOOD, 2}),
          Workshops::Item::fromType(ItemType::Club{}, 3, {CollectiveResourceId::WOOD, 10})
              .setTutorialHighlight(TutorialHighlight::SCHEDULE_CLUB),
          Workshops::Item::fromType(ItemType::HeavyClub{}, 5, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TWO_H_WEAP),
          Workshops::Item::fromType(ItemType::Bow{}, 13, {CollectiveResourceId::WOOD, 20}).setTechId(TechId::ARCHERY),
          Workshops::Item::fromType(ItemType::WoodenStaff{}, 13, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::MAGICAL_WEAPONS),
          Workshops::Item::fromType(ItemType::TrapItem{TrapType::BOULDER}, 20, {CollectiveResourceId::STONE, 50})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType(ItemType::TrapItem{TrapType::POISON_GAS}, 10, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType(ItemType::TrapItem{TrapType::ALARM}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType(ItemType::TrapItem{TrapType::WEB}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType(ItemType::TrapItem{TrapType::SURPRISE}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
          Workshops::Item::fromType(ItemType::TrapItem{TrapType::TERROR}, 8, {CollectiveResourceId::WOOD, 20})
                  .setTechId(TechId::TRAPS),
      }},
      {WorkshopType::FORGE, {
          Workshops::Item::fromType(ItemType::Sword{}, 10, {CollectiveResourceId::IRON, 20}),
          Workshops::Item::fromType(ItemType::SteelSword{}, 20, {CollectiveResourceId::STEEL, 20})
                  .setTechId(TechId::STEEL_MAKING),
          Workshops::Item::fromType(ItemType::ChainArmor{}, 30, {CollectiveResourceId::IRON, 40}),
          Workshops::Item::fromType(ItemType::SteelArmor{}, 60, {CollectiveResourceId::STEEL, 40})
                  .setTechId(TechId::STEEL_MAKING),
          Workshops::Item::fromType(ItemType::IronHelm{}, 8, {CollectiveResourceId::IRON, 16}),
          Workshops::Item::fromType(ItemType::IronBoots{}, 12, {CollectiveResourceId::IRON, 24}),
          Workshops::Item::fromType(ItemType::WarHammer{}, 16, {CollectiveResourceId::IRON, 40})
                  .setTechId(TechId::TWO_H_WEAP),
          Workshops::Item::fromType(ItemType::BattleAxe{}, 22, {CollectiveResourceId::IRON, 50})
                  .setTechId(TechId::TWO_H_WEAP),
          Workshops::Item::fromType(ItemType::SteelBattleAxe{}, 44, {CollectiveResourceId::STEEL, 50})
                  .setTechId(TechId::STEEL_MAKING),
         Workshops::Item::fromType(ItemType::IronStaff{}, 20, {CollectiveResourceId::IRON, 40})
                 .setTechId(TechId::MAGICAL_WEAPONS),
      }},
      {WorkshopType::LABORATORY, {
          Workshops::Item::fromType(ItemType::Potion{Effect::Lasting{LastingEffect::SLOWED}}, 2,
              {CollectiveResourceId::GOLD, 2}),
          Workshops::Item::fromType(ItemType::Potion{Effect::Lasting{LastingEffect::SLEEP}}, 2,
              {CollectiveResourceId::GOLD, 2}),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::POISON_RESISTANT}}, 4, {CollectiveResourceId::GOLD, 6}),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::SPEED}}, 4, {CollectiveResourceId::GOLD, 6}),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::TELEPATHY}}, 4, {CollectiveResourceId::GOLD, 6}),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::REGENERATION}}, 4, {CollectiveResourceId::GOLD, 8}),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::POISON}}, 4, {CollectiveResourceId::GOLD, 8}),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::FLYING}}, 4, {CollectiveResourceId::GOLD, 8}),
          Workshops::Item::fromType(ItemType::Potion{Effect::Heal{}}, 4, {CollectiveResourceId::GOLD, 10})
             .setTechId(TechId::ALCHEMY_ADV),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::BLIND}}, 4, {CollectiveResourceId::GOLD, 15})
                  .setTechId(TechId::ALCHEMY_ADV),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::MELEE_RESISTANCE}}, 6, {CollectiveResourceId::GOLD, 20})
                  .setTechId(TechId::ALCHEMY_ADV),
          Workshops::Item::fromType(ItemType::Potion{
              Effect::Lasting{LastingEffect::INVISIBLE}}, 6, {CollectiveResourceId::GOLD, 20})
                  .setTechId(TechId::ALCHEMY_ADV),
          //Alchemical conversion to and from gold
          Workshops::Item::fromType(ItemType::GoldPiece{}, 5, {CollectiveResourceId::IRON, 30})
              .setTechId(TechId::ALCHEMY_CONV).setBatchSize(10),
          Workshops::Item::fromType(ItemType::WoodPlank{}, 5, {CollectiveResourceId::GOLD, 10})
              .setTechId(TechId::ALCHEMY_CONV).setBatchSize(10),
          Workshops::Item::fromType(ItemType::IronOre{}, 5, {CollectiveResourceId::GOLD, 10})
              .setTechId(TechId::ALCHEMY_CONV).setBatchSize(10),
         Workshops::Item::fromType(ItemType::Rock{}, 5, {CollectiveResourceId::GOLD, 10})
             .setTechId(TechId::ALCHEMY_CONV).setBatchSize(10),
      }},
      {WorkshopType::JEWELER, {
          Workshops::Item::fromType(ItemType::Ring{LastingEffect::POISON_RESISTANT}, 10,
              {CollectiveResourceId::GOLD, 20}),
          Workshops::Item::fromType(ItemType::Ring{LastingEffect::FIRE_RESISTANT}, 10,
              {CollectiveResourceId::GOLD, 30}),
          Workshops::Item::fromType(ItemType::Ring{LastingEffect::MAGIC_RESISTANCE}, 10,
              {CollectiveResourceId::GOLD, 30}),
          Workshops::Item::fromType(ItemType::Amulet{LastingEffect::NIGHT_VISION}, 10, {CollectiveResourceId::GOLD, 20}),
          Workshops::Item::fromType(ItemType::Amulet{LastingEffect::ELF_VISION}, 10, {CollectiveResourceId::GOLD, 20}),
          Workshops::Item::fromType(ItemType::Amulet{LastingEffect::WARNING}, 10, {CollectiveResourceId::GOLD, 30}),
          Workshops::Item::fromType(ItemType::DefenseAmulet{}, 10, {CollectiveResourceId::GOLD, 40}),
          Workshops::Item::fromType(ItemType::Amulet{LastingEffect::REGENERATION}, 10, {CollectiveResourceId::GOLD, 60}),
      }},
      {WorkshopType::STEEL_FURNACE, {
          Workshops::Item::fromType(ItemType::SteelIngot{}, 50, {CollectiveResourceId::IRON, 30}).setBatchSize(10),
      }},
  }));
}

vector<Technology*> CollectiveConfig::getInitialTech() const {
  if (type == KEEPER)
    return {
      Technology::get(TechId::GEOLOGY1),
      Technology::get(TechId::SPELLS),
    };
  else
    return {};
}
