#include "stdafx.h"
#include "content_factory.h"
#include "name_generator.h"
#include "game_config.h"
#include "creature_inventory.h"
#include "creature_attributes.h"
#include "furniture.h"
#include "player_role.h"
#include "tribe_alignment.h"
#include "item.h"
#include "key_verifier.h"
#include "spell_school_id.h"
#include "name_generator_id.h"
#include "build_info.h"
#include "immigrant_info.h"
#include "external_enemies.h"
#include "external_enemies.h"
#include "enemy_info.h"
#include "item_attributes.h"
#include "resource_counts.h"
#include "z_level_info.h"
#include "equipment.h"

template <class Archive>
void ContentFactory::serialize(Archive& ar, const unsigned int) {
  ar(creatures, furniture, resources, zLevels, tilePaths, enemies, externalEnemies, itemFactory, workshopGroups, immigrantsData, buildInfo, villains, gameIntros, playerCreatures, technology, items, buildingInfo);
  creatures.setContentFactory(this);
}

SERIALIZABLE(ContentFactory)

static bool isZLevel(const vector<ZLevelInfo>& levels, int depth) {
  for (auto& l : levels)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      return true;
  return false;
}

bool areResourceCounts(const vector<ResourceDistribution>& resources, int depth) {
  for (auto& l : resources)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      return true;
  return false;
}

optional<string> ContentFactory::readCreatureFactory(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<CreatureId>, CreatureAttributes> attributes;
  if (auto res = config->readObject(attributes, GameConfigId::CREATURE_ATTRIBUTES, keyVerifier))
    return *res;
  for (auto& attr : attributes)
    for (auto skill : ENUM_ALL(SkillId)) {
      auto value = attr.second.getSkills().getValue(skill);
      if (value < 0 || value > 1)
        return "Skill value for "_s + attr.first.data() + " must be between 0 and one, inclusive.";
    }
  map<PrimaryId<SpellSchoolId>, SpellSchool> spellSchools;
  vector<Spell> spells;
  if (auto res = config->readObject(spells, GameConfigId::SPELLS, keyVerifier))
    return *res;
  if (auto res = config->readObject(spellSchools, GameConfigId::SPELL_SCHOOLS, keyVerifier))
    return *res;
  map<PrimaryId<NameGeneratorId>, vector<string>> firstNames;
  if (auto res = config->readObject(firstNames, GameConfigId::NAMES, keyVerifier))
    return *res;
  NameGenerator nameGenerator;
  for (auto& elem : firstNames)
    nameGenerator.setNames(elem.first, elem.second);
  keyVerifier->addKey<CreatureId>("KRAKEN");
  for (auto& elem : CreatureFactory::getSpecialParams())
    keyVerifier->addKey<CreatureId>(elem.first.data());
  creatures = CreatureFactory(std::move(nameGenerator), convertKeys(std::move(attributes)),
      convertKeys(std::move(spellSchools)), std::move(spells));
  creatures.setContentFactory(this);
  return none;
}

optional<string> ContentFactory::readFurnitureFactory(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<FurnitureType>, Furniture> elems;
  if (auto res = config->readObject(elems, GameConfigId::FURNITURE, keyVerifier))
    return *res;
  map<FurnitureType, OwnerPointer<Furniture>> furnitureDefs;
  map<PrimaryId<FurnitureListId>, FurnitureList> furnitureLists;
  for (auto& elem : elems) {
    elem.second.setType(elem.first);
    furnitureDefs.insert(make_pair(elem.first, makeOwner<Furniture>(elem.second)));
  }
  if (auto res = config->readObject(furnitureLists, GameConfigId::FURNITURE_LISTS, keyVerifier))
    return *res;
  furniture = FurnitureFactory(std::move(furnitureDefs), convertKeys(std::move(furnitureLists)));
  return none;
}

static optional<string> checkGroupCounts(const map<string, vector<ImmigrantInfo>>& immigrants) {
  for (auto& group : immigrants)
    for (auto& elem : group.second)
      if (elem.getGroupSize().isEmpty())
        return "Bad immigrant group size: " + toString(elem.getGroupSize()) +
            ". Lower bound is inclusive, upper bound exclusive.";
  return none;
}

optional<string> ContentFactory::readVillainsTuple(const GameConfig* gameConfig, KeyVerifier* keyVerifier) {
  if (auto error = gameConfig->readObject(villains, GameConfigId::CAMPAIGN_VILLAINS, keyVerifier))
    return "Error reading campaign villains definition"_s + *error;
  auto has = [](vector<Campaign::VillainInfo> v, VillainType type) {
    return std::any_of(v.begin(), v.end(), [type](const auto& elem){ return elem.type == type; });
  };
  for (auto villainType : {VillainType::ALLY, VillainType::MAIN, VillainType::LESSER})
    for (auto role : ENUM_ALL(PlayerRole))
      for (auto alignment : ENUM_ALL(TribeAlignment)) {
        int index = [&] {
          switch (role) {
            case PlayerRole::KEEPER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return 0;
                case TribeAlignment::LAWFUL:
                  return 1;
              }
            case PlayerRole::ADVENTURER:
              switch (alignment) {
                case TribeAlignment::EVIL:
                  return 2;
                case TribeAlignment::LAWFUL:
                  return 3;
              }
          }
        }();
        if (!has(villains[index], villainType))
          return "Empty " + EnumInfo<VillainType>::getString(villainType) + " villain list for alignment: "
                    + EnumInfo<TribeAlignment>::getString(alignment);
      }
  return none;
}

optional<string> ContentFactory::readPlayerCreatures(const GameConfig* config, KeyVerifier* keyVerifier) {
  if (auto error = config->readObject(playerCreatures, GameConfigId::PLAYER_CREATURES, keyVerifier))
    return "Error reading player creature definitions"_s + *error;
  if (playerCreatures.first.empty() || playerCreatures.second.empty())
    return "Keeper and adventurer lists must each contain at least 1 entry."_s;
  for (auto& keeperInfo : playerCreatures.first) {
    bool hotkeys[128] = {0};
    vector<BuildInfo> buildInfoTmp;
    set<string> allDataGroups;
    for (auto& group : buildInfo) {
      allDataGroups.insert(group.first);
      if (keeperInfo.buildingGroups.contains(group.first))
        buildInfoTmp.append(group.second);
    }
    for (auto& tech : keeperInfo.initialTech)
      if (!keeperInfo.technology.contains(tech))
        return "Technology not found: "_s + tech.data();
    for (auto& info : buildInfoTmp)
      for (auto& requirement : info.requirements)
        if (auto tech = requirement.getReferenceMaybe<TechId>())
          if (!keeperInfo.technology.contains(*tech))
            return "Technology prerequisite \""_s + tech->data() + "\" of build item \"" + info.name + "\" is not available";
    WorkshopArray merged;
    set<string> allWorkshopGroups;
    for (auto& group : workshopGroups) {
      allWorkshopGroups.insert(group.first);
      if (keeperInfo.workshopGroups.contains(group.first))
        for (int i : Range(EnumInfo<WorkshopType>::size))
          merged[i].append(group.second[i]);
    }
    for (auto& elem : merged)
      for (auto& item : elem)
        if (item.tech && !technology.techs.count(*item.tech))
          return "Technology prerequisite \""_s + item.tech->data() + "\" of workshop item \"" + item.item.get(this)->getName()
              + "\" is not available";
    for (auto elem : keeperInfo.immigrantGroups)
      if (!immigrantsData.count(elem))
        return "Undefined immigrant group: \"" + elem + "\"";
    for (auto& group : keeperInfo.buildingGroups)
      if (!allDataGroups.count(group))
        return "Building menu group \"" + group + "\" not found";
    for (auto& info : buildInfoTmp) {
      if (info.hotkey != '\0') {
        if (hotkeys[int(info.hotkey)])
          return "Hotkey \'" + string(1, info.hotkey) + "\' is used more than once in building menu";
        hotkeys[int(info.hotkey)] = true;
      }
    }
  }
  return none;
}

optional<string> ContentFactory::readItems(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<CustomItemId>, ItemAttributes> itemsTmp;
  if (auto res = config->readObject(itemsTmp, GameConfigId::ITEMS, keyVerifier))
    return *res;
  for (auto& elem : itemsTmp)
    if (elem.second.equipmentSlot == EquipmentSlot::RANGED_WEAPON && !elem.second.rangedWeapon)
      return "Item "_s + elem.first.data() + " has RANGED_WEAPON slot, but no rangedWeapon entry.";
  items = convertKeys(itemsTmp);
  return none;
}

optional<string> ContentFactory::readBuildingInfo(const GameConfig* config, KeyVerifier* keyVerifier) {
  map<PrimaryId<BuildingId>, BuildingInfo> buildingsTmp;
  if (auto res = config->readObject(buildingsTmp, GameConfigId::BUILDING_INFO, keyVerifier))
    return *res;
  buildingInfo = convertKeys(buildingsTmp);
  return none;
}

optional<string> ContentFactory::readData(const GameConfig* config) {
  KeyVerifier keyVerifier;
  if (auto error = config->readObject(technology, GameConfigId::TECHNOLOGY, &keyVerifier))
    return *error;
  if (auto error = config->readObject(workshopGroups, GameConfigId::WORKSHOPS_MENU, &keyVerifier))
    return *error;
  if (auto error = config->readObject(immigrantsData, GameConfigId::IMMIGRATION, &keyVerifier))
    return *error;
  if (auto error = checkGroupCounts(immigrantsData))
    return *error;
  if (auto error = config->readObject(buildInfo, GameConfigId::BUILD_MENU, &keyVerifier))
    return *error;
  if (auto error = readVillainsTuple(config, &keyVerifier))
    return *error;
  if (auto error = config->readObject(gameIntros, GameConfigId::GAME_INTRO_TEXT, &keyVerifier))
    return *error;
  if (auto error = readPlayerCreatures(config, &keyVerifier))
    return *error;
  if (auto res = config->readObject(zLevels, GameConfigId::Z_LEVELS, &keyVerifier))
    return *res;
  if (auto res = config->readObject(resources, GameConfigId::RESOURCE_COUNTS, &keyVerifier))
    return *res;
  if (auto res = readItems(config, &keyVerifier))
    return *res;
  if (auto res = readBuildingInfo(config, &keyVerifier))
    return *res;
  map<PrimaryId<EnemyId>, EnemyInfo> enemiesTmp;
  if (auto res = config->readObject(enemiesTmp, GameConfigId::ENEMIES, &keyVerifier))
    return *res;
  enemies = convertKeys(enemiesTmp);
  for (auto& enemy : enemies)
    if (auto res = getReferenceMaybe(buildingInfo, enemy.second.settlement.buildingId))
      enemy.second.settlement.buildingInfo = *res;
  if (auto res = config->readObject(externalEnemies, GameConfigId::EXTERNAL_ENEMIES, &keyVerifier))
    return *res;
  if (auto res = readCreatureFactory(config, &keyVerifier))
    return *res;
  if (auto res = readFurnitureFactory(config, &keyVerifier))
    return *res;
  map<PrimaryId<ItemListId>, ItemList> itemLists;
  if (auto res = config->readObject(itemLists, GameConfigId::ITEM_LISTS, &keyVerifier))
    return *res;
  itemFactory = ItemFactory(convertKeys(std::move(itemLists)));
  vector<TileInfo> tileDefs;
  for (auto id : {"bridge", "tutorial_entrance", "accept_immigrant", "reject_immigrant", "fog_of_war_corner"})
    keyVerifier.addKey<ViewId>(id);
  if (auto res = config->readObject(tileDefs, GameConfigId::TILES, &keyVerifier))
    return *res;
  tilePaths = TilePaths(std::move(tileDefs), config->getModName());
  for (int alignment = 0; alignment < 2; ++alignment) {
    vector<ZLevelInfo> levels = concat<ZLevelInfo>({zLevels[0], zLevels[1 + alignment]});
    for (int depth = 0; depth < 1000; ++depth) {
      if (!isZLevel(levels, depth))
        return "No z-level found for depth " + toString(depth) + ". Please fix z-level config.";
      if (!areResourceCounts(resources, depth))
        return "No resource distribution found for depth " + toString(depth) + ". Please fix resources config.";
    }
  }
  auto errors = keyVerifier.verify();
  if (!errors.empty())
    return errors.front();
  return none;
}

void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
  furniture.merge(std::move(f.furniture));
  tilePaths.merge(std::move(f.tilePaths));
  mergeMap(std::move(f.items), items);
}

CreatureFactory& ContentFactory::getCreatures() {
  creatures.setContentFactory(this);
  return creatures;
}

const CreatureFactory& ContentFactory::getCreatures() const {
  creatures.setContentFactory(this);
  return creatures;
}

ContentFactory::ContentFactory() {}
ContentFactory::~ContentFactory() {}
ContentFactory::ContentFactory(ContentFactory&&) = default;
