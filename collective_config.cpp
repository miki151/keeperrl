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
#include "minion_activity.h"
#include "furniture_usage.h"
#include "creature_attributes.h"
#include "collective.h"
#include "zones.h"
#include "construction_map.h"
#include "item_class.h"
#include "villain_type.h"
#include "furniture.h"
#include "tutorial_highlight.h"
#include "spell.h"
#include "creature_factory.h"
#include "resource_info.h"
#include "workshop_item.h"
#include "body.h"
#include "view_id.h"
#include "view_object.h"
#include "territory.h"
#include "furniture_factory.h"
#include "storage_id.h"
#include "immigrant_info.h"
#include "conquer_condition.h"
#include "content_factory.h"
#include "content_factory.h"
#include "bed_type.h"

template <class Archive>
void CollectiveConfig::serialize(Archive& ar, const unsigned int version) {
  ar(immigrantInterval, maxPopulation, conquerCondition);
  ar(type, leaderAsFighter, spawnGhosts, ghostProb, guardianInfo);
}

SERIALIZABLE(CollectiveConfig);
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveConfig);

template <class Archive>
void GuardianInfo::serialize(Archive& ar, const unsigned int version) {
  ar(creature, probability, minEnemies, minVictims);
}

SERIALIZABLE(GuardianInfo);

static optional<BedType> getBedType(const Creature* c) {
  if (!c->getBody().needsToSleep())
    return none;
  if (c->getStatus().contains(CreatureStatus::PRISONER))
    return BedType::PRISON;
  if (c->getBody().isUndead())
    return BedType::COFFIN;
  if (c->getBody().isHumanoid())
    return BedType::BED;
  else
    return BedType::CAGE;
}

void CollectiveConfig::addBedRequirementToImmigrants(vector<ImmigrantInfo>& immigrantInfo, ContentFactory* factory) {
  for (auto& info : immigrantInfo) {
    PCreature c = factory->creatures.fromId(info.getId(0), TribeId::getDarkKeeper());
    if (info.getInitialRecruitment() == 0)
      if (auto bedType = getBedType(c.get())) {
        bool hasBed = false;
        info.visitRequirements(makeVisitor(
            [&](const AttractionInfo& attraction) -> void {
              for (auto& type : attraction.types)
                if (auto fType = type.getValueMaybe<FurnitureType>())
                  if (factory->furniture.getData(*fType).getBedType() == *bedType)
                    hasBed = true;
            },
            [&](const auto&) {}
        ));
        if (!hasBed) {
          info.addRequirement(AttractionInfo(1, factory->furniture.getBedFurniture(*bedType)[0]));
        }
      }
  }
}

CollectiveConfig::CollectiveConfig(TimeInterval interval, CollectiveType t, int maxPop)
    : immigrantInterval(interval), maxPopulation(maxPop), type(t),
      conquerCondition(ConquerCondition::KILL_FIGHTERS_AND_LEADER) {
}

CollectiveConfig CollectiveConfig::keeper(TimeInterval immigrantInterval, int maxPopulation) {
  return CollectiveConfig(immigrantInterval, KEEPER, maxPopulation);
}

CollectiveConfig CollectiveConfig::withImmigrants(TimeInterval interval, int maxPopulation) {
  return CollectiveConfig(interval, VILLAGE, maxPopulation);
}

CollectiveConfig CollectiveConfig::noImmigrants() {
  return CollectiveConfig(TimeInterval {}, VILLAGE, 10000);
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

TimeInterval CollectiveConfig::getImmigrantTimeout() const {
  return 500_visible;
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

bool CollectiveConfig::stayInTerritory() const {
  return type != KEEPER;
}

bool CollectiveConfig::hasVillainSleepingTask() const {
  return type != KEEPER;
}

bool CollectiveConfig::allowHealingTaskOutsideTerritory() const {
  return type == KEEPER;
}

bool CollectiveConfig::hasImmigrantion(bool currentlyActiveModel) const {
  return type != KEEPER || currentlyActiveModel;
}

TimeInterval CollectiveConfig::getImmigrantInterval() const {
  return immigrantInterval;
}

bool CollectiveConfig::getStripSpawns() const {
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

CollectiveConfig& CollectiveConfig::setGuardian(GuardianInfo info) {
  guardianInfo = info;
  return *this;
}

const optional<GuardianInfo>& CollectiveConfig::getGuardianInfo() const {
  return guardianInfo;
}

bool CollectiveConfig::isConquered(const Collective* collective) const {
  switch (conquerCondition) {
    case ConquerCondition::KILL_FIGHTERS_AND_LEADER:
      return collective->getCreatures(MinionTrait::FIGHTER).empty() && !collective->getLeader();
    case ConquerCondition::DESTROY_BUILDINGS:
      for (auto& elem : collective->getConstructions().getAllFurniture())
        if (auto f = elem.first.getFurniture(elem.second))
          if (f->isWall())
            return false;
      return true;
  }
}

CollectiveConfig& CollectiveConfig::setConquerCondition(ConquerCondition c) {
  conquerCondition = c;
  return *this;
}

const ResourceInfo& CollectiveConfig::getResourceInfo(CollectiveResourceId id) {
  static EnumMap<CollectiveResourceId, ResourceInfo> resourceInfo([](CollectiveResourceId id)->ResourceInfo {
    switch (id) {
      case CollectiveResourceId::PRISONER_HEAD:
        return { none, none, ItemType::GoldPiece{}, "", ViewId("impaled_head"), true, none};
      case CollectiveResourceId::DEMON_PIETY:
        return { none, none, ItemType::GoldPiece{}, "", ViewId("impaled_head"), false, none};
      case CollectiveResourceId::GOLD:
        return {StorageId::GOLD, ItemIndex::GOLD, ItemType::GoldPiece{}, "gold", ViewId("gold"), false, none};
      case CollectiveResourceId::WOOD:
        return { StorageId::RESOURCE, ItemIndex::WOOD, ItemType::WoodPlank{}, "wood", ViewId("wood_plank"),
            false, TutorialHighlight::WOOD_RESOURCE};
      case CollectiveResourceId::IRON:
        return { StorageId::RESOURCE, ItemIndex::IRON, ItemType::IronOre{}, "iron", ViewId("iron_rock"), false, none};
      case CollectiveResourceId::ADA:
        return { StorageId::RESOURCE, ItemIndex::ADA, ItemType::AdaOre{}, "adamantium", ViewId("ada_ore"), false, none};
      case CollectiveResourceId::STONE:
        return { StorageId::RESOURCE, ItemIndex::STONE, ItemType::Rock{}, "granite", ViewId("rock"), false, none};
      case CollectiveResourceId::CORPSE:
        return { StorageId::CORPSES, ItemIndex::REVIVABLE_CORPSE, ItemType::GoldPiece{}, "corpses", ViewId("body_part"), true, none};
    }
  });
  return resourceInfo[id];
}

static CollectiveItemPredicate unMarkedItems() {
  return [](WConstCollective col, const Item* it) { return !col->isItemMarked(it); };
}


const vector<ItemFetchInfo>& CollectiveConfig::getFetchInfo() const {
  if (type == KEEPER) {
    static vector<ItemFetchInfo> ret {
        {ItemIndex::CORPSE, unMarkedItems(), StorageId::CORPSES, CollectiveWarning::GRAVES},
        {ItemIndex::GOLD, unMarkedItems(), StorageId::GOLD, CollectiveWarning::CHESTS},
        {ItemIndex::MINION_EQUIPMENT, [](WConstCollective col, const Item* it)
            { return it->getClass() != ItemClass::GOLD && !col->isItemMarked(it);},
            StorageId::EQUIPMENT, CollectiveWarning::EQUIPMENT_STORAGE},
        {ItemIndex::WOOD, unMarkedItems(), StorageId::RESOURCE,
            CollectiveWarning::RESOURCE_STORAGE},
        {ItemIndex::IRON, unMarkedItems(), StorageId::RESOURCE,
            CollectiveWarning::RESOURCE_STORAGE},
        {ItemIndex::ADA, unMarkedItems(), StorageId::RESOURCE,
            CollectiveWarning::RESOURCE_STORAGE},
        {ItemIndex::STONE, unMarkedItems(), StorageId::RESOURCE,
            CollectiveWarning::RESOURCE_STORAGE},
        /*{ItemIndex::TRAP, unMarkedItems(), [](WConstCollective col) -> const PositionSet& {
                return col->getTerritory().getAllAsSet(); },
            CollectiveWarning::RESOURCE_STORAGE},*/
    };
    return ret;
  } else {
    static vector<ItemFetchInfo> empty;
    return empty;
  }
}

MinionActivityInfo::MinionActivityInfo(Type t, const string& desc) : type(t), description(desc) {
  CHECK(type != FURNITURE);
}

MinionActivityInfo::MinionActivityInfo() {}

MinionActivityInfo::MinionActivityInfo(FurnitureType type, const string& desc)
    : type(FURNITURE), furniturePredicate(
        [type](const ContentFactory*, WConstCollective, const Creature*, FurnitureType t) { return t == type;}),
      description(desc) {
}

MinionActivityInfo::MinionActivityInfo(UsagePredicate pred, const string& desc)
    : type(FURNITURE), furniturePredicate(pred), description(desc) {
}

optional<WorkshopType> CollectiveConfig::getWorkshopType(FurnitureType furniture) {
  for (auto type : ENUM_ALL(WorkshopType))
    if (getWorkshopInfo(type).furniture == furniture)
      return type;
  return none;
  /*static optional<EnumMap<FurnitureType, optional<WorkshopType>>> map;
  if (!map) {
    map.emplace();
    for (auto type : ENUM_ALL(WorkshopType))
      (*map)[workshops[type].furniture] = type;
  }
  return (*map)[furniture];*/
}

map<CollectiveResourceId, int> CollectiveConfig::getStartingResource() const {
  return map<CollectiveResourceId, int>{};
}

CollectiveConfig::CollectiveConfig(const CollectiveConfig&) = default;

CollectiveConfig::~CollectiveConfig() {
}

static auto getTrainingPredicate(ExperienceType experienceType) {
  return [experienceType] (const ContentFactory* contentFactory, WConstCollective, const Creature* c, FurnitureType t) {
      if (auto maxIncrease = contentFactory->furniture.getData(t).getMaxTraining(experienceType))
        return !c || (c->getAttributes().getExpLevel(experienceType) < maxIncrease &&
            !c->getAttributes().isTrainingMaxedOut(experienceType));
      else
        return false;
    };
}

const MinionActivityInfo& CollectiveConfig::getActivityInfo(MinionActivity task) {
  static EnumMap<MinionActivity, MinionActivityInfo> map([](MinionActivity task) -> MinionActivityInfo {
    switch (task) {
      case MinionActivity::IDLE: return {MinionActivityInfo::IDLE, "idle"};
      case MinionActivity::CONSTRUCTION: return {MinionActivityInfo::WORKER, "construction"};
      case MinionActivity::HAULING: return {MinionActivityInfo::WORKER, "hauling"};
      case MinionActivity::WORKING: return {MinionActivityInfo::WORKER, "labour"};
      case MinionActivity::DIGGING: return {MinionActivityInfo::WORKER, "digging"};
      case MinionActivity::TRAIN: return {getTrainingPredicate(ExperienceType::MELEE), "training"};
      case MinionActivity::SLEEP: return {[](const ContentFactory* f, WConstCollective, const Creature* c, FurnitureType t) {
            if (!c)
              return !!f->furniture.getData(t).getBedType();
            return getBedType(c) == f->furniture.getData(t).getBedType();
          }, "sleeping"};
      case MinionActivity::EAT: return {MinionActivityInfo::EAT, "eating"};
      case MinionActivity::THRONE: return {FurnitureType("THRONE"), "throne"};
      case MinionActivity::STUDY: return {getTrainingPredicate(ExperienceType::SPELL), "studying"};
      case MinionActivity::CROPS: return {FurnitureType("CROPS"), "crops"};
      case MinionActivity::RITUAL: return {FurnitureType("DEMON_SHRINE"), "rituals"};
      case MinionActivity::ARCHERY: return {MinionActivityInfo::ARCHERY, "archery range"};
      case MinionActivity::COPULATE: return {MinionActivityInfo::COPULATE, "copulation"};
      case MinionActivity::EXPLORE: return {MinionActivityInfo::EXPLORE, "spying"};
      case MinionActivity::SPIDER: return {MinionActivityInfo::SPIDER, "spider"};
      case MinionActivity::EXPLORE_NOCTURNAL: return {MinionActivityInfo::EXPLORE, "spying"};
      case MinionActivity::EXPLORE_CAVES: return {MinionActivityInfo::EXPLORE, "spying"};
      case MinionActivity::BE_WHIPPED: return {FurnitureType("WHIPPING_POST"), "being whipped"};
      case MinionActivity::BE_TORTURED: return {FurnitureType("TORTURE_TABLE"), "being tortured"};
      case MinionActivity::BE_EXECUTED: return {FurnitureType("GALLOWS"), "being executed"};
      case MinionActivity::CRAFT: return {[](const ContentFactory*, WConstCollective col, const Creature* c, FurnitureType t) {
            if (auto type = getWorkshopType(t))
              return !c || !col || (c->getAttributes().getSkills().getValue(getWorkshopInfo(*type).skill) > 0 &&
                  !col->getWorkshops().get(*type).isIdle(
                      col, c->getAttributes().getSkills().getValue(getWorkshopInfo(*type).skill), c->getMorale()));
            else
              return false;
          },
          "crafting"};
    }
  });
  return map[task];
}

const WorkshopInfo& CollectiveConfig::getWorkshopInfo(WorkshopType type) {
  static EnumMap<WorkshopType, WorkshopInfo> workshops([](WorkshopType type)->WorkshopInfo {
    switch (type) {
      case WorkshopType::WORKSHOP: return {FurnitureType("WORKSHOP"), "workshop", SkillId::WORKSHOP};
      case WorkshopType::FORGE: return {FurnitureType("FORGE"), "forge", SkillId::FORGE};
      case WorkshopType::LABORATORY: return {FurnitureType("LABORATORY"), "laboratory", SkillId::LABORATORY};
      case WorkshopType::JEWELER: return {FurnitureType("JEWELLER"), "jeweler", SkillId::JEWELER};
    }});
  return workshops[type];
}
